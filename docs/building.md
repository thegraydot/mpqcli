# Building

## Requirements

- cmake 3.25 or later
- C++ 17 compiler
- StormLib (provided as Git Submodule)
- CLI11 (provided as Git Submodule)
- hash-library (provided as Git Submodule)

## Linux

```bash
$ git clone --recursive https://github.com/thegraydot/mpqcli.git
$ cd mpqcli
$ cmake -B build/release -DCMAKE_BUILD_TYPE=Release
$ cmake --build build/release
```

The `mpqcli` binary will be available in: `./build/release/bin/mpqcli`

## Windows

```bash
$ git clone --recursive https://github.com/thegraydot/mpqcli.git
$ cd mpqcli
$ cmake -B build/release
$ cmake --build build/release --config Release
```

The `mpqcli.exe` binary will be available in: `.\build\release\bin\mpqcli.exe`

## Dependencies

### StormLib

This project requires the [StormLib](https://github.com/ladislav-zezula/StormLib) library. Many thanks to [Ladislav Zezula](https://github.com/ladislav-zezula) for authoring such a good library and releasing the code under an open-source licence. The StormLib library has a number of requirements. However, the build method specifies using the libraries bundled with StormLib.

### CLI11

This project also uses the [CLI11](https://github.com/CLIUtils/CLI11) command line parser for C++11 and beyond. It provides simple and easy-to-use CLI arguments.

## Tests

The tests are written with the [Catch2](https://github.com/catchorg/Catch2) framework. They come in two layers, each built as its own binary in `build/dev/bin`:

- **Unit tests** (`test/unit/`, built as `mpqcli_unit_tests`) call the core library directly, with one test file per source file in `src/`.
- **Functional tests** (`test/functional/`, built as `mpqcli_functional_tests`) run the built `mpqcli` binary as a user would, using [subprocess.h](https://github.com/sheredom/subprocess.h). There is one test file per subcommand, checking its exit code and output.

Catch2 and subprocess.h are submodules like the other dependencies, and the tests are built by default. Build, then run each layer:

```bash
$ cmake -B build/dev
$ cmake --build build/dev
$ ctest --test-dir build/dev -L unit
$ ctest --test-dir build/dev -L functional
```

On Windows, add `-C Release` (or whichever configuration you built) to each `ctest` command. To build without the tests, configure with `-DMPQCLI_BUILD_TESTING=OFF`.

With the Makefile, `make test` builds and runs the unit tests and `make test_functional` the functional tests; `make test_all` runs both.

### Real archives

A few functional tests read two genuine World of Warcraft patch archives, the only archives in the suite with a real Blizzard signature. They are not committed to the repository, and those tests are skipped until the archives are present. Fetch them with:

```bash
$ make fetch_integration_data
```

This needs [filenget](https://github.com/thomaslaurenson/filenget) on your `PATH`, and checks each download against the checksums in `test/integration_data.sha256`. The archives go in `test/data/integration/`. To keep them somewhere else, set `MPQCLI_INTEGRATION_DATA` to that directory, either in the environment or as a CMake cache variable.
