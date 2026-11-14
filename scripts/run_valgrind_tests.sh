#!/bin/bash

# Runs the pytest suite with every mpqcli invocation wrapped in Valgrind, then
# reports the memory errors and leaks it found. Used both locally and by the
# Valgrind GitHub Actions workflow.
#
# Findings only warn: the script exits 0 when Valgrind reports problems or tests
# fail under it, and non-zero only when the run itself could not be carried out.
# Test failures are gated by the regular test workflow, not here.

set -euo pipefail

SCRIPT_DIR="$(dirname "$(readlink -fm "$0")")"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# The tests look for the binary here (see test/conftest.py)
BIN="$PROJECT_DIR/build/dev/bin/mpqcli"
REAL_BIN="$BIN.real"
VENV_DIR="$PROJECT_DIR/.venv"
VALGRIND_LOG_DIR="$PROJECT_DIR/valgrind_logs"

LEAK_PATTERN='(definitely|indirectly) lost: [1-9][0-9,]* bytes'
ERROR_PATTERN='ERROR SUMMARY: [1-9][0-9]* errors'

USE_DOCKER=false
PYTEST_ARGS=()

usage() {
    echo "Usage: $0 [OPTIONS] [-- PYTEST_ARGS...]"
    echo ""
    echo "Run the pytest suite with Valgrind memory leak detection"
    echo ""
    echo "Options:"
    echo "  -d, --docker    Run in a Docker container (see Dockerfile.valgrind)"
    echo "  -h, --help      Show this help message"
    echo ""
    echo "Arguments after -- are passed to pytest, e.g. -- -k test_add"
    echo ""
    echo "The binary is built as Debug in build/dev, and temporarily replaced by a"
    echo "wrapper that runs it under Valgrind. Logs are written to valgrind_logs/."
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--docker)
            USE_DOCKER=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            PYTEST_ARGS=("$@")
            break
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            exit 1
            ;;
    esac
done

if [ "$USE_DOCKER" = true ]; then
    echo -e "${GREEN}Running tests with Valgrind in Docker...${NC}"

    # The container user gets the host user's UID/GID, so that it can write the
    # logs into the mounted directory and they end up owned by the host user
    echo -e "${YELLOW}Building Docker image...${NC}"
    docker build \
        --build-arg USER_ID="$(id -u)" \
        --build-arg GROUP_ID="$(id -g)" \
        -t mpqcli-valgrind \
        -f "$PROJECT_DIR/Dockerfile.valgrind" \
        "$PROJECT_DIR"

    mkdir -p "$VALGRIND_LOG_DIR"
    exec docker run --rm \
        -v "$VALGRIND_LOG_DIR:/mpqcli/valgrind_logs" \
        mpqcli-valgrind \
        scripts/run_valgrind_tests.sh -- "${PYTEST_ARGS[@]}"
fi

if ! command -v valgrind &> /dev/null; then
    echo -e "${RED}Error: Valgrind is not installed${NC}"
    echo "Install it with: sudo apt-get install valgrind"
    exit 1
fi

cd "$PROJECT_DIR"

# A run killed before it could clean up leaves the wrapper in place of the
# binary. Put the real one back first: wrapping the wrapper would lose it, and
# the build would take the newer wrapper for an up-to-date binary.
if [ -f "$REAL_BIN" ]; then
    echo -e "${YELLOW}Restoring binary left behind by an interrupted run...${NC}"
    mv "$REAL_BIN" "$BIN"
fi

# Always build, so that the tests never run a stale binary. Debug keeps the
# symbols and line numbers Valgrind needs for readable stack traces.
echo -e "${YELLOW}Building mpqcli (Debug)...${NC}"
make build BUILD_TYPE=Debug

if [ ! -d "$VENV_DIR" ]; then
    echo -e "${YELLOW}Setting up Python test environment...${NC}"
    make test_create_venv
fi

# Logs from an earlier run would otherwise be counted again. The directory is
# emptied rather than removed, as in Docker it is a mount point.
mkdir -p "$VALGRIND_LOG_DIR"
find "$VALGRIND_LOG_DIR" -mindepth 1 -delete

# Replace the binary in place with a Valgrind wrapper, and restore it on exit
mv "$BIN" "$REAL_BIN"
trap 'mv "$REAL_BIN" "$BIN"' EXIT
trap 'exit 130' INT TERM
cat > "$BIN" << EOF
#!/bin/bash
exec valgrind --leak-check=full \\
     --show-leak-kinds=all \\
     --track-origins=yes \\
     --log-file="$VALGRIND_LOG_DIR/valgrind_%p.log" \\
     "$REAL_BIN" "\$@"
EOF
chmod +x "$BIN"

echo -e "${YELLOW}Running pytest with Valgrind wrapper...${NC}"
pytest_status=0
"$VENV_DIR/bin/python" -m pytest test -v --tb=short "${PYTEST_ARGS[@]}" || pytest_status=$?

# pytest exits 1 when tests failed. Anything else above 0 means it could not
# run the suite properly (interrupted, usage error, no tests collected).
if [ "$pytest_status" -gt 1 ]; then
    echo -e "${RED}pytest did not complete (exit code $pytest_status)${NC}"
    exit "$pytest_status"
fi

echo ""
echo -e "${YELLOW}Analyzing results...${NC}"

shopt -s nullglob
logs=("$VALGRIND_LOG_DIR"/*.log)
shopt -u nullglob

total_logs=${#logs[@]}
error_logs=()
leak_logs=()
if [ "$total_logs" -gt 0 ]; then
    mapfile -t error_logs < <(grep -lE "$ERROR_PATTERN" "${logs[@]}" || true)
    mapfile -t leak_logs < <(grep -lE "$LEAK_PATTERN" "${logs[@]}" || true)
fi

if [ "$total_logs" -eq 0 ]; then
    echo -e "${RED}No Valgrind logs were written; the wrapper did not run${NC}"
    exit 1
fi

echo "Logs analyzed:    $total_logs"
echo "Logs with errors: ${#error_logs[@]}"
echo "Logs with leaks:  ${#leak_logs[@]}"
echo "Tests failed:     $([ "$pytest_status" -eq 1 ] && echo yes || echo no)"

clean=true
if [ "${#error_logs[@]}" -gt 0 ] || [ "${#leak_logs[@]}" -gt 0 ] || [ "$pytest_status" -ne 0 ]; then
    clean=false
fi

if [ "$clean" = true ]; then
    echo -e "${GREEN}No memory errors or leaks detected${NC}"
else
    echo -e "${YELLOW}Warning: potential memory issues detected. See the logs in $VALGRIND_LOG_DIR${NC}"
    for log in "${error_logs[@]}"; do
        grep -H "ERROR SUMMARY" "$log"
    done
    for log in "${leak_logs[@]}"; do
        grep -HE "$LEAK_PATTERN" "$log"
    done
fi

# Report to GitHub Actions, when running there
if [ -n "${GITHUB_OUTPUT:-}" ]; then
    echo "status=$([ "$clean" = true ] && echo success || echo warning)" >> "$GITHUB_OUTPUT"
fi
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    {
        echo "## Valgrind Memory Leak Analysis"
        echo ""
        if [ "$clean" = true ]; then
            echo "✅ **No memory errors or leaks detected.**"
        else
            echo "⚠️ **Potential memory issues detected**"
        fi
        echo ""
        echo "- Logs analyzed:    $total_logs"
        echo "- Logs with errors: ${#error_logs[@]}"
        echo "- Logs with leaks:  ${#leak_logs[@]}"
        if [ "$pytest_status" -ne 0 ]; then
            echo "- Some tests failed under Valgrind"
        fi
        if [ "${#error_logs[@]}" -gt 0 ] || [ "${#leak_logs[@]}" -gt 0 ]; then
            echo ""
            echo "#### Logs with findings"
            echo ""
            echo '```'
            printf '%s\n' "${error_logs[@]}" "${leak_logs[@]}" | sort -u | xargs -r -n1 basename
            echo '```'
        fi
        if [ "$clean" = false ]; then
            echo ""
            echo "Please review the Valgrind logs for details."
        fi
    } >> "$GITHUB_STEP_SUMMARY"
fi
