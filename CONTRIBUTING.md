# Contributing

Contributions are welcome. Please read the guidelines below before opening a pull request.

## Before You Start

If you are unsure whether a feature fits the project, or whether an existing tool could already be combined with `mpqcli` to achieve the same result, open an issue first. This avoids wasted effort and keeps the project focused.

**mpqcli follows the Unix philosophy.** The tool is designed to do one thing well and to compose with other tools via pipes and redirection. If you find yourself wanting to add functionality that could be handled by a separate tool - for example, sorting the output of `list` - the right answer is usually to pipe the output to that tool rather than adding it here.

## Prerequisites and Setup

Clone the repository and initialise submodules:

```
git clone https://github.com/thegraydot/mpqcli.git
cd mpqcli
git submodule update --init --recursive
```

Install the clang lint tools:

```
make install_clang_tools
```

## Makefile Reference

Run `make help` to list all available targets. Common ones:

| Target                     | Description                                                        |
|----------------------------|--------------------------------------------------------------------|
| `make install_clang_tools` | Install clang-format and clang-tidy via apt                        |
| `make build_linux`         | Build for Linux using cmake                                        |
| `make build_windows`       | Build for Windows using cmake                                      |
| `make build_clean`         | Remove the cmake build directory                                   |
| `make test_create_venv`    | Create Python venv and install test dependencies (first-time only) |
| `make test_mpqcli`         | Run the pytest test suite                                          |
| `make lint`                | Run all C++ linters (clang-format + clang-tidy)                    |
| `make fmt_check`           | Check formatting only (dry run)                                    |
| `make fmt`                 | Auto-fix formatting in-place                                       |
| `make lint_cpp`            | Run clang-tidy static analysis                                     |
| `make clean`               | Remove all build and test artifacts                                |

## Requirements for a Pull Request

### 1. Builds on your platform

Make sure the project builds cleanly on your development machine before opening a PR:

```
make build_linux   # Linux
make build_windows # Windows
```

A PR automatically triggers the CI build workflow, which compiles and tests across all supported Linux targets (AMD64 and ARM64, glibc and musl). You are not expected to reproduce all of those locally.

### 2. Tests pass

Run the test suite before submitting:

```
make test_create_venv  # first-time setup only
make test_mpqcli
```

All tests must pass without errors.

### 3. New features should include tests

If your change adds or modifies user-facing functionality - such as a new subcommand flag or a change in output format - please include a corresponding test in the `test/` directory. The existing test files (`test_list.py`, `test_add.py`, etc.) are good references for the test style and fixtures used.

### 4. Linting must pass

All C++ code is formatted with clang-format and analysed with clang-tidy. Run the full suite before submitting:

```
make lint
```

If there are formatting violations, auto-fix them with:

```
make fmt
```

Then re-run `make lint` to confirm everything passes.

### 5. Match the existing code style

C++ formatting is enforced by `.clang-format` (Google style base). Static analysis is enforced by `.clang-tidy`. Both configs live in the repo root. Python tests should follow the style of the existing test files.

#### Suppression policy

Suppressions are occasionally necessary for third-party code or intentional patterns. When suppressing a clang-tidy warning:

- Use `// NOLINT(check-name)` with the specific check name - bare `// NOLINT` is not acceptable
- Every suppression must have a comment explaining why it is justified

```cpp
// NOLINT(bugprone-easily-swappable-parameters): parameters validated by CLI11
```

#### Disabling clang-format locally

Use `// clang-format off` / `// clang-format on` only when the default formatting genuinely hurts readability (e.g. column-aligned tables). Add a brief comment explaining the intent:

```cpp
// clang-format off: preserve column-aligned flag-to-char mappings for readability
if (flags & MPQ_FILE_IMPLODE)   result += 'i';
if (flags & MPQ_FILE_COMPRESS)  result += 'c';
// clang-format on
```

## Known Design Constraints

### StormLib locale state is global and not thread-safe

`SFileSetLocale` sets a process-wide locale variable (`g_lcFileLocale`) inside StormLib. All locale-sensitive operations in `mpq.cpp` - file open, add, remove, read, extract, and list - call `SFileSetLocale` immediately before the relevant StormLib call. There is no locale-explicit alternative in StormLib's public API (`SFileOpenFileEx`, `SFileAddFileEx`, etc. all read `g_lcFileLocale` internally).

This means:

- The `SFileSetLocale` + StormLib-call sequence is **not atomic** and would be unsafe under concurrency
- mpqcli is intentionally **single-threaded**; do not introduce threads or async I/O without auditing every locale-sensitive call site in `mpq.cpp`

If you add a new StormLib call that is locale-sensitive, follow the existing pattern: call `SFileSetLocale` immediately before it, with no intervening calls between the two.

## Workflow Summary

1. Fork the repository and create a branch for your change
2. Run `git submodule update --init --recursive` after cloning
3. Run `make install_clang_tools` to install lint dependencies
4. Make your changes and verify they build: `make build_linux`
5. Run `make lint` and fix any issues
6. Run `make test_mpqcli` and confirm all tests pass
7. Open a pull request with a clear description of what was changed and why
