# Commands

Many of the command examples use the MPQ archive file, named `wow-patch.mpq`, from a original (Vanilla) World of Warcraft patch file, named `WoW-1.10.0-to-1.10.1-enGB-patch.zip`. If you want to replicate these examples, you can [download the `wow-patch.mpq` file](https://archive.org/download/World_of_Warcraft_Client_and_Installation_Archive/Patches/1.x/WoW-1.10.0-to-1.10.1-enGB-patch.zip/wow-patch.mpq) from the Internet Archive.


The `mpqcli` program has the following subcommands:

| Command | Description |
|---|---|
| [`version`](./commands/version.md) | Print the tool version number |
| [`about`](./commands/about.md) | Print information about the tool |
| [`completion`](./commands/completion.md) | Print shell completion script to stdout |
| [`info`](./commands/info.md) | Print information about MPQ archive properties |
| [`create`](./commands/create.md) | Create an MPQ archive from files and directories |
| [`add`](./commands/add.md) | Add a file to an existing MPQ archive |
| [`remove`](./commands/remove.md) | Remove a file from an existing MPQ archive |
| [`rename`](./commands/rename.md) | Rename a file in an existing MPQ archive |
| [`list`](./commands/list.md) | List files in an MPQ archive |
| [`extract`](./commands/extract.md) | Extract files from an MPQ archive |
| [`read`](./commands/read.md) | Read a specific file to stdout |
| [`verify`](./commands/verify.md) | Verify an MPQ archive signature |
| [`compact`](./commands/compact.md) | Compact (defragment) an MPQ archive |

## Exit codes

Every subcommand exits with one of these codes, so a script can tell the outcomes apart. [Find the archives that fail verification](./advanced.md#find-the-archives-that-fail-verification) shows one in use.

| Code | Meaning |
|---|---|
| `0` | Success. A file `add` skipped because it is already in the archive still counts as success. |
| `1` | The command failed: for example, the archive could not be opened, a file is not in it, or `verify` found no valid signature. The reason is on stderr. |
| `2` | An unexpected internal error. Please report it. |
| `100` to `114` | The command line could not be parsed. The most common are `104` (a value of the wrong type), `105` (a value that is not allowed), `106` (a missing argument) and `109` (an extra argument). |
| `130` | Interrupted with Ctrl+C. On Linux mpqcli ends by the signal itself, which a shell reports as `130`; on Windows it exits with `130`. |
