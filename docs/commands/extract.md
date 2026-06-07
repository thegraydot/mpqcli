# extract

Extract one or all files from a target MPQ archive.

## Extract all files from an MPQ archive

The output will be saved in a folder with the same name as the target MPQ file, without the extension.

```bash
$ mpqcli extract wow-patch.mpq
[*] Extracted: BM_COKETENT01.BLP
[*] Extracted: Blizzard_CraftUI.xml
[*] Extracted: CreatureSoundData.dbc
...
[*] Extracted: Blizzard_CraftUI.lua
[*] Extracted: 30ee7bd3959906e358eff01332cf045e.blp
[*] Extracted: realmlist.wtf
```

## Extract all files to a target directory

Extract files to a specific target directory, which will be created if it doesn't already exist. In this example, `patch-1.10` is the user-specified output directory.

```bash
$ mpqcli extract -o patch-1.10 wow-patch.mpq
```

## Extract all files keeping the folder structure

Use the `-k` or `--keep` flag to preserve the directory structure from the archive when extracting. Without this flag, all files are placed flat in the output directory, with only the filename retained.

```bash
$ mpqcli extract -k wow-patch.mpq
[*] Extracted: Interface/FrameXML/BlizzardFrameXML.lua
[*] Extracted: Interface/FrameXML/BlizzardFrameXML.xml
```

## Extract all files with an external listfile

Older MPQ archives do not contain (complete) file paths of their content. By providing an external listfile that lists the content of the MPQ archive, the extracted files will have the correct names and paths. Listfiles can be downloaded on [Ladislav Zezula's site](http://www.zezula.net/en/mpq/download.html).

```bash
$ mpqcli extract -l path/to/listfile War2Dat.mpq
```

## Extract one specific file

Extract a single file using the `-f` option. If the target file in the MPQ archive is nested (in a directory) you need to include the full path. Similar to the examples above, you can use the `-o` argument to specify the output directory.

```bash
$ mpqcli extract -f "Documentation\Layout\Greeting.html" \
    "World of Warcraft_1.12.1.5875/Data/base.MPQ"
```

## Extract one specific file with locale

Use the `--locale` argument to specify the locale of the file to extract. If there is no file with the requested name and locale, the default locale will be used instead.

```bash
$ mpqcli extract -f "rez\gluBNRes.res" Patch_rt.mpq --locale deDE
```
