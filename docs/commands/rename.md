# rename

Rename a file in an existing MPQ archive without extracting and re-adding it.
The archive is the first positional argument, followed by the current and new archive paths.
This operation does not cause MPQ fragmentation, and thus it is not necessary to
[`compact`](compact.md) the archive.

```bash
$ mpqcli rename wow-patch.mpq old-name.txt new-name.txt
[~] Renaming file: old-name.txt -> new-name.txt
```

Use `--locale` to rename only the copy stored under a specific locale. Without `--locale`,
the file stored under the default locale is renamed.

```bash
$ mpqcli rename wow-patch.mpq old-name.txt new-name.txt --locale esES
[~] Renaming file for locale esES: old-name.txt -> new-name.txt
```
