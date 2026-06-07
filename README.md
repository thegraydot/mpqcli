# mpqcli

![Build Status](https://img.shields.io/github/actions/workflow/status/thegraydot/mpqcli/tag.yml?style=flat) ![Test Status](https://img.shields.io/github/actions/workflow/status/thegraydot/mpqcli/tag.yml?style=flat&label=test)

![Release Version](https://img.shields.io/github/v/release/thegraydot/mpqcli?style=flat)

![Release downloads](https://img.shields.io/github/downloads/thegraydot/mpqcli/total?label=release_downloads) ![Package downloads](https://img.shields.io/badge/package_downloads-894-green)

A command-line tool to create, add, remove, list, extract, read, and verify MPQ archives using the [StormLib library](https://github.com/ladislav-zezula/StormLib).

> ⚠️ **Warning:** This project is under active development and will change functionality between released versions until version 1.0.0.

## Overview

**This is a command-line tool, designed for automation and built with the Unix philosophy in mind.** It is designed to work seamlessly with other command-line tools, supporting piping, redirection, and integration into shell scripts and workflows. For example:

- Run one command to create an MPQ archive from a directory of files or a single file
- Run one command to list all files in an MPQ archive
- Pipe the output to `grep` or other tools to search, filter, or process files
- Redirect output to files or other commands for further automation

If you require an MPQ tool with a graphical interface (GUI) and explicit support for more MPQ archive versions, I would recommend using [Ladik's MPQ Editor](http://www.zezula.net/en/mpq/download.html).

## Releases

Pre-built binaries are available for Linux and Windows on [GitHub releases](https://github.com/thegraydot/mpqcli/releases).

Docker images are available on [GitHub packages](https://github.com/thegraydot/mpqcli/pkgs/container/mpqcli).

## Quickstart

```bash
curl -fsSL https://raw.githubusercontent.com/thegraydot/mpqcli/main/scripts/install.sh | bash
...
mpqcli list <some-mpq-archive>
```

Microsoft Windows:

```powershell
irm https://raw.githubusercontent.com/thegraydot/mpqcli/main/scripts/install.ps1 | iex
...
mpqcli.exe list <some-mpq-archive>
```

## Documentation

Please see [mpqcli.thegraydot.com](https://mpqcli.thegraydot.com) for full documentation.

## Contributing

Please see the [CONTRIBUTING.md](CONTRIBUTING.md) document.
