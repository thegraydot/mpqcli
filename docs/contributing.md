# Contributing

Contributions are welcome. Please read the guidelines below before opening a pull request.

## Before you start

If you are unsure whether a feature fits the project, or whether an existing tool could already be combined with `mpqcli` to achieve the same result, open an issue first. This avoids wasted effort and keeps the project focused.

**mpqcli follows the Unix philosophy.** The tool is designed to do one thing well and to compose with other tools via pipes and redirection. If you find yourself wanting to add functionality that could be handled by a separate tool - for example, sorting the output of `list` - the right answer is usually to pipe the output to that tool rather than adding it here.

## Prerequisites and setup

Clone the repository and initialise submodules:

```sh
git clone https://github.com/thegraydot/mpqcli.git
cd mpqcli
git submodule update --init --recursive
```

Install the clang lint tools, pinned to the major version the checks are formatted against:

```sh
sudo apt-get install -y clang-18 clang-format-18 clang-tidy-18
```

## Makefile reference

Run `make help` to list all available targets. Common ones:

| Target | Description |
|---|---|
| `make configure` | Configure the cmake build (uses the default compiler) |
| `make build` | Build the project into build/dev |
| `make test_create_venv` | Create Python venv and install test dependencies (first-time only) |
| `make test_mpqcli` | Run the pytest test suite |
| `make check_all` | Run every static check (clang-format, clang-tidy, completion scripts) |
| `make check_format` | Check formatting only (dry run) |
| `make format` | Auto-fix formatting in-place |
| `make configure_lint` | Configure build/lint with clang++ for clang-tidy |
| `make check_lint` | Run clang-tidy static analysis |
| `make check_embed` | Syntax-check the embedded completion scripts |
| `make clean` | Remove all build, test and docs artefacts |

## Requirements for a pull request

### 1. Builds on your platform

Make sure the project builds cleanly on your development machine before opening a PR:

```sh
make build
```

On Windows without a POSIX shell, use the cmake commands from the building page instead.

A PR automatically triggers the CI build workflow, which compiles and tests across all supported Linux targets (AMD64 and ARM64). You are not expected to reproduce all of those locally.

### 2. Tests pass

Run the test suite before submitting:

```sh
make test_create_venv  # first-time setup only
make test_mpqcli
```

All tests must pass without errors.

### 3. New features should include tests

If your change adds or modifies user-facing functionality - such as a new subcommand flag or a change in output format - please include a corresponding test in the `test/` directory. The existing test files (`test_list.py`, `test_add.py`, etc.) are good references for the test style and fixtures used.

### 4. Linting must pass

All C++ code is formatted with clang-format and analysed with clang-tidy, and the completion scripts are parsed by their shells. `make check_all` runs all three checks. It configures a clang build tree of its own for clang-tidy, so it needs no prior `make configure`:

```sh
make check_all
```

If there are formatting violations, auto-fix them with:

```sh
make format
```

Then re-run `make check_all` to confirm everything passes.

### 5. Match the existing code style

C++ formatting is enforced by `.clang-format` (LLVM style base). Static analysis is enforced by `.clang-tidy`. Both configs live in the repo root. Python tests should follow the style of the existing test files.

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
// Preserve column-aligned flag-to-char mappings for readability
// clang-format off
if (flags & MPQ_FILE_IMPLODE)   result += 'i';
if (flags & MPQ_FILE_COMPRESS)  result += 'c';
// clang-format on
```

## Known design constraints

### StormLib locale state is global and not thread-safe

`SFileSetLocale` sets a process-wide locale variable (`g_lcFileLocale`) inside StormLib. All locale-sensitive operations in `src/mpq/` - file open, add, remove, read, extract, and list - call `SFileSetLocale` immediately before the relevant StormLib call. There is no locale-explicit alternative in StormLib's public API (`SFileOpenFileEx`, `SFileAddFileEx`, etc. all read `g_lcFileLocale` internally).

This means:

- The `SFileSetLocale` + StormLib-call sequence is **not atomic** and would be unsafe under concurrency
- mpqcli is intentionally **single-threaded**; do not introduce threads or async I/O without auditing every locale-sensitive call site in `src/mpq/`

If you add a new StormLib call that is locale-sensitive, follow the existing pattern: call `SFileSetLocale` immediately before it, with no intervening calls between the two.

## Workflow summary

1. Fork the repository and create a branch for your change
2. Run `git submodule update --init --recursive` after cloning
3. Install the clang tools as shown above
4. Make your changes and verify they build: `make build`
5. Run `make check_all`, fixing any issues
6. Run `make test_mpqcli` and confirm all tests pass
7. Open a pull request with a clear description of what was changed and why
