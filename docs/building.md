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

This project implements End-to-end (E2E) testing, sometimes referred to as system testing or integration testing. This methodology is used because it verifies application functionality by simulating actual usage by an end user. Testing includes creating a variety of MPQ archives, as well as dynamically downloading some small (~1-5MB) MPQ archives from the Internet Archive. The Python programming language coupled with the [pytest framework](https://github.com/pytest-dev/pytest) is used to implement testing, mainly due to ease of implementation.

To configure the testing environment you will need Python installed, as well as the required `pytest` package. On a Debian-based Linux system, the following will configure the environment:

```bash
$ sudo apt install python3-venv python3-pip
$ python3 -m venv .venv
$ source .venv/bin/activate
$ pip3 install -r test/requirements.txt
```

The tests run the binary in `build/dev`, so build it there and then run the tests using:

```bash
$ cmake -B build/dev
$ cmake --build build/dev
$ python3 -m pytest test -s
```

`make test` does the same in one step.
