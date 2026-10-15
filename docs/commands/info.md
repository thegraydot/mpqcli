# info

Print information about MPQ archive properties.

## Print information about an MPQ archive

The `info` subcommand prints a list of useful information (property keys and values) of an MPQ archive.

```bash
$ mpqcli info wow-patch.mpq
Archive size: 1798918
File count: 65
Format version: 1
Header offset: 0
Header size: 32
Max files: 128
Signature type: Weak
```

## Print one specific MPQ archive property

The `info` subcommand supports the following properties:

- `archive-size`
- `file-count`
- `format-version`
- `header-offset`
- `header-size`
- `max-files`
- `signature-type`

You can use the `-q` or `--property` argument with the `info` subcommand to print just the value of a specific property. This can be useful for automation, for example, to determine the signature type of a directory of MPQ archives.

```bash
$ mpqcli info --property file-count wow-patch.mpq
65
```
