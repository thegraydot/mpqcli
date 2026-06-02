# remove

Remove one or more files from an existing MPQ archive.

The archive is always the first positional argument, followed by one or more archive paths
to remove. All removals happen in a single open/close cycle.

## Remove a single file

```bash
$ mpqcli remove wow-patch.mpq fth.txt
[-] Removing file: fth.txt
```

## Remove multiple files at once

Pass more than one archive path after the archive argument:

```bash
$ mpqcli remove wow-patch.mpq fth.txt fta.txt fts.txt
[-] Removing file: fth.txt
[-] Removing file: fta.txt
[-] Removing file: fts.txt
```

## Remove files from stdin

Pass `-` to read archive paths from standard input:

```bash
$ echo -e "fth.txt\nfta.txt" | mpqcli remove wow-patch.mpq -
[-] Removing file: fth.txt
[-] Removing file: fta.txt
```

## Remove a file with a given locale

Use `--locale` to remove only the copy stored under a specific locale. Without `--locale`,
the file stored under the default locale is removed.

```bash
$ mpqcli remove wow-patch.mpq alianza.txt --locale esES
[-] Removing file for locale esES: alianza.txt
```
