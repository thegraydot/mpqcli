# Changelog

## 0.10.1 - 2026-07-03

### Added

- Command completion for common shells (bash, zsh, fish and powershell)

### Fixed

- Memory leak in read subcommand
- Inconsistencies in documentation

### Updated

- Code style to conform to Google CPP style with slight variations
- StormLib dependency for a variety of fixes

### Thanks

- Thanks to @sjoblomj for the contributions in this release

## 0.10.0 - 2026-06-07

### Added

- Compact subcommand to compress MPQ archive after creation/editing
- Support passing multiple files to the `add` and `remove` subcommands
- Support reading file paths from stdin in the `add` and `remove` subcommands
- Support adding entire directories with the `add` subcommand
- Add an update flag to the `add` subcommand to skip files whose archived size matches the on-disk size

### Fixed

- `extract` command now reports an error when the output directory cannot be created
- Path traversal guard in `extract` uses fully resolved paths, closing a potential bypass
- Crash when reading strong signatures from malformed or truncated archives
- Docker glibc image updated to ubuntu:24.04
- Adding files is now ordered and operating system agnostic
- `add` subcommand now returns non-zero exit codes on failure
- Error messages now include StormLib error codes
- Duplicate error messages removed
- Makefile build targets corrected in `CONTRIBUTING.md`

### Changed

- `--filename-in-archive` and `--directory-in-archive` replaced by a unified `--path` flag
- The path flag on add now acts as an archive path prefix when a directory is given

### Thanks

- Thanks to @sjoblomj for the contributions in this release

## 0.9.9 - 2026-04-05

### Fixed

- Potential path traversal attack in extract subcommand
- Empty MPQ archive name when directory supplied with trailing slash
- Bug where MPQ internal files were added with create command
- Signature being added automatically when not requested
- Potential DWORD overflow in `AddFiles`

## 0.9.8 - 2026-03-22

### Added

- The `create` subcommand now supports a single file
- The `add` and `create` subcommands have additional arguments for specifiying the file location in the MPQ archive (`name-in-archive` and `dir-in-archive`)
- The `add` subcommand can now overwrite existing files if requested (`overwrite`)

### Fixed

- Several memory leaks

### Thanks

- Thanks to @sjoblomj for adding more features and fixes in this release

## 0.9.7 - 2026-02-23

### Added

- Game profile support for `create` and `add` subcommands via `-g/--game` option
- Game profiles for Diablo, StarCraft, Warcraft, WoW, and more
- Game setting override options for `create` subcommand
- Compression override options for `add` subcommand
- Support for unfamiliar/unknown locales via raw hex LCID
- Additional built-in locales: `ptBR`, `esMX`, `ptPT`

### Fixed

- Subcommands now properly verify locale before operating on files
- Error messages now include locale context in `remove`, `extract`, and `read`
- MPQ archives are now properly closed across all subcommands
- Non-zero exit codes returned on failure in `extract` and `remove`
- `list` subcommand now shows long output when properties are requested

### Changed

- Improved locale printing and display throughout

### Thanks

- A big thank you to @sjoblomj for adding so many features in this release

## 0.9.6 - 2025-12-23

### Added

- Proper support for locales in MPQ files

### Fixed

- Fixed bug where MPQ signature only printed on success

## 0.9.5 - 2025-10-23

### Changed

- Long (detailed) list format to output ISO 8601 timestamps

### Fixed

- Message prefix format consistency

## 0.9.4 - 2025-10-12

### Added

- Filename added to program output
- Added simple wrapper around verify command

### Fixed

- Show help menu when no arguments provided

## 0.9.3 - 2025-10-03

### Added

- Support for `add` subcommand to have path inside MPQ archive

### Fixed

- Bug in `add` subcommand when exceeding max file count

## 0.9.2 - 2025-08-21

### Fixed

- Bug in `create` subcommand resulting in incorrect max file count

## 0.9.1 - 2025-08-19

### Fixed

- Bug in Dockerfiles resulting in missing commit hash from version

## 0.9.0 - 2025-08-19

### Added

- Linux binaries for amd64 and arm64
- Linux binaries built with musl and glibc

### Updated

- Install script for Linux to provided better binary download options

### Changed

- Linux binary release naming convention

## 0.8.1 - 2025-08-15

### Added

- Added print raw bytes for `read` and `verify` subcommands

### Fixed

- Tests for `read` and `verify` subcommands

### Removed

- Ability to print hex or plaintext in `read` and `verify` subcommands
- Removed messages when printing signature value

## 0.8.0 - 2025-08-11

### Added

- Added `add`/`remove` subcommands working
- Added tests for `add`/`remove` subcommands
- Added `max-files` property to `info` subcommand

### Changed

- Change for default maximum file count when creating MPQ archives

## 0.7.0 - 2025-08-09

### Added

- Added detailed (long) listing

### Fixed

- Updated create subcommand to always include attributes
- Updated Info function to support archive/file property fetching

## 0.6.2 - 2025-07-11

### Added

- Added tests for `extract` subcommand
- Added dependabot configuration for Python and Docker

### Fixed

- Fixed deprecated Windows Action running to Server 2022

## 0.6.1 - 2025-06-27

### Fixed

- Updated readme and help menu to match application functionality

## 0.6.0 - 2025-06-02

### Added

- Added the `read` subcommand
- Added tests for `read` subcommand
- Added new helper functions to print ASCII or hexidecimal output

### Fixed

- Fixed line endings in Windows pytest cases

## 0.5.0 - 2025-05-05

### Added

- Added the `verify` subcommand
- Included argument to print digital signature
- Added ability to sign archive when creating MPQ
- Added tests for `verify` subcommand

## 0.4.4 - 2025-05-05

### Fixed

- Refactored `create` subcommand tests

## 0.4.3 - 2025-05-01

### Added

- Refactored `info` subcommand
- Added test for `info` subcommand

### Fixed

- Updated tests to be multi-platform (Windows)

### Removed

- Removed exe patch split functionality

## 0.4.2 - 2025-04-22

### Added

- Added Dockerfile
- Added Docker build and release workflows
- Added `add` and `remove` subcommand placeholders
- Added `about` subcommand

## 0.4.1 - 2025-04-22

### Added

- Added Python testing environment
- Added tests for `list` subcommand
- Added test workflow

## 0.4.0 - 2025-04-12

### Added

- Added Windows binary build to releases

## 0.3.2 - 2025-03-29

### Added

- External listfile support for `list` and `extract` by @sjoblomj
- Added functions for add and remove files

### Fixed

- Fixed path extraction on Linux to create nested folder structure

## 0.3.1 - 2025-03-03

### Fixed

- Fixed path extraction on Linux to create nested folder structure

## 0.3.0 - 2025-02-24

### Added

- Added `create` subcommand
- Added download information to readme

### Fixed

- Improved Linux binary compatibility

## 0.2.3 - 2025-02-01

### Fixed

- Improved Linux binary compatibility

## 0.2.2 - 2025-02-01

### Added

- Added the `extract`, `verify` and `list` subcommands
- Added `release.yml` workflow for Linux binary
