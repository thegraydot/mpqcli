# Commands

Many of the command examples use the MPQ archive file, named `wow-patch.mpq`, from a original (Vanilla) World of Warcraft patch file, named `WoW-1.10.0-to-1.10.1-enGB-patch.zip`. If you want to replicate these examples, you can [download the `wow-patch.mpq` file](https://archive.org/download/World_of_Warcraft_Client_and_Installation_Archive/Patches/1.x/WoW-1.10.0-to-1.10.1-enGB-patch.zip/wow-patch.mpq) from the Internet Archive.


The `mpqcli` program has the following subcommands:

| Command | Description |
|---|---|
| [`version`](./commands/version.md) | Print the tool version number |
| [`about`](./commands/about.md) | Print information about the tool |
| [`completion`](./commands/completion.md) | Print shell completion script to stdout |
| [`info`](./commands/info.md) | Print information about MPQ archive properties |
| [`create`](./commands/create.md) | Create an MPQ archive from a target directory or a single file |
| [`add`](./commands/add.md) | Add a file to an existing MPQ archive |
| [`remove`](./commands/remove.md) | Remove a file from an existing MPQ archive |
| [`list`](./commands/list.md) | List files in a target MPQ archive |
| [`extract`](./commands/extract.md) | Extract one or all files from a target MPQ archive |
| [`read`](./commands/read.md) | Read a specific file to stdout |
| [`verify`](./commands/verify.md) | Verify a target MPQ archive signature |
| [`compact`](./commands/compact.md) | Compact (defragment) an MPQ archive |
