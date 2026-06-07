# Introduction

A command-line tool to create, add, remove, list, extract, read, verify, and compact MPQ archives using the [StormLib library](https://github.com/ladislav-zezula/StormLib).

> ⚠️ **Warning:** This project is under active development and will change functionality between released versions until version 1.0.0.

## Overview

**This is a command-line tool, designed for automation and built with the Unix philosophy in mind.** It is designed to work seamlessly with other command-line tools, supporting piping, redirection, and integration into shell scripts and workflows. For example:

- Run one command to create an MPQ archive from a directory of files or a single file
- Run one command to list all files in an MPQ archive
- Pipe the output to `grep` or other tools to search, filter, or process files
- Redirect output to files or other commands for further automation

If you require an MPQ tool with a graphical interface (GUI) and explicit support for more MPQ archive versions - I would recommend using [Ladik's MPQ Editor](http://www.zezula.net/en/mpq/download.html).
