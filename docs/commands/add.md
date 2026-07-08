# add

Add one or more files to an existing MPQ archive.

The archive is always the first positional argument, followed by one or more files or directories.
All inputs are processed in a single open/close cycle, which is significantly faster than
calling `add` once per file.

## Add a single file

```bash
$ echo "For The Horde" > fth.txt
$ mpqcli add wow-patch.mpq fth.txt
[+] Adding file: fth.txt
```

## Add multiple files at once

Pass more than one file path after the archive. The archive is opened once for all files.

```bash
$ mpqcli add wow-patch.mpq fth.txt fta.txt fts.txt
[+] Adding file: fth.txt
[+] Adding file: fta.txt
[+] Adding file: fts.txt
```

## Add files from stdin

Pass `-` as the file argument to read paths from standard input. This works with `find`,
`ls`, or any other tool that produces file paths.

```bash
$ find . -name "*.blp" | mpqcli add wow-patch.mpq -
[+] Adding file: textures\Creature\Bear\Bear.blp
[+] Adding file: textures\Creature\Wolf\Wolf.blp
...
```

## Add a directory

Pass a directory path to recursively add all files within it. The directory structure is
preserved relative to the directory root.

```bash
$ mpqcli add wow-patch.mpq textures/
[+] Adding file: Creature\Bear\Bear.blp
[+] Adding file: Creature\Wolf\Wolf.blp
```

Use `--path` to add a prefix to every archived path:

```bash
$ mpqcli add wow-patch.mpq textures/ --path textures
[+] Adding file: textures\Creature\Bear\Bear.blp
[+] Adding file: textures\Creature\Wolf\Wolf.blp
```

## Skip unchanged files with --update

When adding a directory, the `--update` flag skips files that have not changed since they
were last added to the archive. This is useful for incremental updates where only changed
files need to be re-added.

The skip decision is made in two steps:

1. **File size** must match. If the sizes differ the file is always re-added.
2. If the sizes match, the archive's `(attributes)` file is consulted for a stronger
   check. The most reliable attribute available wins:
   - **MD5** – if the archive stores MD5 checksums, the MD5 of the local file is computed
     and compared. A match skips the file; a mismatch re-adds it.
   - **CRC32** – used when MD5 is absent. Same logic.
   - **Timestamp** – used when neither MD5 nor CRC32 is present. The file's
     last-modification time is compared at one-second resolution.
   - **No attributes** – if the archive has no `(attributes)` file, the file is always
     re-added even when sizes match, because no reliable content check is possible.

```bash
$ mpqcli add wow-patch.mpq textures/ --update --overwrite
[~] Skipping unchanged file: Creature\Bear\Bear.blp
[+] Adding file: Creature\Wolf\Wolf.blp
[*] For textures: 1 files added, 1 files skipped, 0 files failed.
```

## Control where files are stored

For single files, one can specify both directory and filename in one step using `-p` or `--path`:

```bash
$ mpqcli add wow-patch.mpq fts.txt --path "texts\swarm.txt"
[+] Adding file: texts\swarm.txt
```

For directories, one can specify the base directory using `-p` or `--path`:

```bash
$ mpqcli add wow-patch.mpq textures/ --path textures
[+] Adding file: textures\Creature\Bear\Bear.blp
[+] Adding file: textures\Creature\Wolf\Wolf.blp
[*] For textures: 2 files added, 0 files skipped, 0 files failed.
```

## Overwrite existing files

Without `--overwrite`, any file that already exists in the archive is skipped:

```bash
$ mpqcli add wow-patch.mpq allegiance.txt
[!] File already exists in MPQ archive: allegiance.txt - Skipping...
```

Set `-w` or `--overwrite` to replace it:

```bash
$ mpqcli add wow-patch.mpq allegiance.txt --overwrite
[+] File already exists in MPQ archive: allegiance.txt - Overwriting...
[+] Adding file: allegiance.txt
```

## Add with a locale

Use `--locale` to store the file under a specific locale. Files added without `--locale`
use the default locale.

```bash
$ mpqcli add wow-patch.mpq allianz.txt --locale deDE
[+] Adding file for locale deDE: allianz.txt
```

## Add with game-specific compression

Use `-g` or `--game` to apply the compression and encryption rules for a specific game.

```bash
$ mpqcli add archive.mpq khwhat1.wav --game warcraft2
[+] Adding file: khwhat1.wav
```
