# create

Create an MPQ archive from files and directories.

## Create an MPQ archive from a directory

Name the new archive first, then the directory to put in it. Automatically adds `(listfile)` to the archive, and will skip this file if it exists in the directory.

```bash
$ mpqcli create archive.mpq <directory>
```

The default mode of operation for the `create` subcommand is to take everything from the directory (and below) and recursively add it to the archive. The directory's contents go at the root of the archive, and the structure below it is retained. Windows-style backslash path separators are used (`\`), as per the observed behaviour in most MPQ archives.

The archive must not already exist; `create` never overwrites a file. If an input is missing or any file cannot be added, no archive is created.

## Create an MPQ archive from several files and directories

Name as many files and directories as needed after the archive. Each file is stored under its own name, and each directory's contents go at the root of the archive. If two inputs hold a file with the same name, the first one is kept.

```bash
$ mpqcli create archive.mpq readme.txt textures/ sounds/
```

Pass `-` in place of the files to read their paths from stdin, one per line:

```bash
$ find art -name '*.blp' | mpqcli create art.mpq -
```

## Create an MPQ archive for a specific game

Target a specific game version by using the `-g` or `--game` argument. This will automatically set the correct archive format version and settings, although they can be overridden.

```bash
$ mpqcli create -g starcraft archive.mpq <directory>
$ mpqcli create --game wow-wotlk \
    --sector-size 16384 \
    --format-version 3 archive.mpq <directory>
```

## Create an MPQ archive from a single file

Create an MPQ file from a single file.

```bash
$ mpqcli create --game diablo2 archive.mpq <file>
```

## Control where files are stored

For a single file, one can specify both directory and filename in one step using `-p` or `--path`:

```bash
$ mpqcli create archive.mpq fts.txt --path "texts\swarm.txt"
[+] Adding file: texts\swarm.txt
```

For directories, or several inputs, `-p` or `--path` is a prefix that every file is stored under:

```bash
$ mpqcli create archive.mpq textures/ --path textures
[+] Adding file: textures\Creature\Bear\Bear.blp
[+] Adding file: textures\Creature\Wolf\Wolf.blp
```

## Create and sign an MPQ archive

Use the `-s` or `--sign` argument to cryptographically sign an MPQ archive with the Blizzard weak signature.

```bash
$ mpqcli create --format-version 1 --sign archive.mpq <directory>
```

## Create an MPQ archive with a given locale

Use the `--locale` argument to specify the locale that all added files will have in the archive. Note that subsequent added files will have the default locale unless the `--locale` argument is specified again.

```bash
$ mpqcli create archive.mpq <directory> --locale koKR
```
