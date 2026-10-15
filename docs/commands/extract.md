# extract

Extract every file, or only the named files, from an MPQ archive.

## Extract all files from an MPQ archive

The output will be saved in a folder with the same name as the MPQ archive, without the extension. Files keep the folder structure they have in the archive.

```bash
$ mpqcli extract wow-patch.mpq
[*] Extracted: BM_COKETENT01.BLP
[*] Extracted: Blizzard_CraftUI.xml
[*] Extracted: CreatureSoundData.dbc
...
[*] Extracted: Interface/FrameXML/BlizzardFrameXML.lua
[*] Extracted: 30ee7bd3959906e358eff01332cf045e.blp
[*] Extracted: realmlist.wtf
```

## Extract all files to an output directory

Extract files to a specific output directory, which will be created if it doesn't already exist. In this example, `patch-1.10` is the user-specified output directory.

```bash
$ mpqcli extract -o patch-1.10 wow-patch.mpq
```

## Extract all files without the folder structure

Use the `--flat` flag to write every file straight into the output directory, keeping only the filename. If two files would end up with the same name, nothing is extracted.

```bash
$ mpqcli extract --flat wow-patch.mpq
[*] Extracted: BlizzardFrameXML.lua
[*] Extracted: BlizzardFrameXML.xml
```

## Extract all files with an external listfile

Older MPQ archives do not contain (complete) file paths of their content. By providing an external listfile that lists the content of the MPQ archive, the extracted files will have the correct names and paths. Listfiles can be downloaded on [Ladislav Zezula's site](http://www.zezula.net/en/mpq/download.html).

```bash
$ mpqcli extract -l path/to/listfile War2Dat.mpq
```

The listfile only applies when extracting every file, so it cannot be combined with named files.

## Extract specific files

Name one or more files after the archive to extract only those. A file nested in a directory needs its full path in the archive, and is extracted with its folders; add `--flat` to drop them. Similar to the examples above, you can use the `-o` argument to specify the output directory.

```bash
$ mpqcli extract "World of Warcraft_1.12.1.5875/Data/base.MPQ" \
    "Documentation\Layout\Greeting.html"
[*] Extracted: Documentation/Layout/Greeting.html
```

## Extract files named on stdin

Pass `-` in place of the file names to read them from stdin, one per line. If no names arrive, nothing is extracted. See the [advanced examples](../advanced.md) for searching an archive and extracting the matches.

```bash
$ mpqcli list wow-patch.mpq | grep -i '\.exe$' | mpqcli extract wow-patch.mpq -
```

## Extract one specific file with locale

Use the `--locale` argument to specify the locale of the file to extract. If there is no file with the requested name and locale, the default locale will be used instead.

```bash
$ mpqcli extract Patch_rt.mpq "rez\gluBNRes.res" --locale deDE
```
