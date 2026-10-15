# list

List files in a target MPQ archive.

## List all files in an MPQ archive

Pretty simple, list files in an MPQ archive. Useful to "pipe" to other tools, such as `grep` (see the [Advanced Examples](../advanced.md) page).

```bash
$ mpqcli list wow-patch.mpq
BM_COKETENT01.BLP
Blizzard_CraftUI.xml
CreatureSoundData.dbc
...
Blizzard_CraftUI.lua
30ee7bd3959906e358eff01332cf045e.blp
realmlist.wtf
```

## List all files with detailed output

Similar to the `ls` command with the `-l` and `-a` options, additional detailed information can be included with the `list` subcommand. The `-a` option includes printing "special" files used in MPQ archives including: `(listfile)`, `(attributes)` and `(signature)`.

```bash
$ mpqcli list -d -a wow-patch.mpq
   88604 enUS 2006-03-29 02:02:37  BM_COKETENT01.BLP
     243 enUS 2006-04-04 21:28:14  Blizzard_CraftUI.xml
     388 enUS 2006-03-29 19:32:46  CreatureSoundData.dbc
     ...
     184 enUS 2006-04-04 21:28:14  Blizzard_CraftUI.lua
   44900 enUS 2006-03-29 02:01:02  30ee7bd3959906e358eff01332cf045e.blp
      68 enUS 2006-04-07 00:58:44  realmlist.wtf
```

## List specific properties

The `list` subcommand supports listing the following properties:

- `hash-index` - Index in the hash table where the file entry is.
- `name-hash1` - The first hash of the file name.
- `name-hash2` - The second hash of the file name.
- `name-hash3` - 64-bit Jenkins hash of the file name, used for searching in the HET table.
- `locale` - Locale info of the file.
- `file-index` - Index in the file table of the file.
- `byte-offset` - Offset of the file in the MPQ, relative to the MPQ header.
- `file-time` - Timestamp of the file.
- `file-size` - Uncompressed file size of the file, in bytes.
- `compressed-size` - Compressed file size of the file, in bytes.
- `encryption-key` - Encryption key for the file.
- `encryption-key-raw` - Encryption key for the file.
- `flags` - File flags for the file within MPQ:
  * `i`: File is Imploded (By PKWARE Data Compression Library).
  * `c`: File is Compressed (By any of multiple methods).
  * `e`: File is Encrypted.
  * `2`: File is Encrypted with key v2.
  * `p`: File is a Patch file.
  * `u`: File is stored as a single Unit, rather than split into sectors.
  * `d`: File is a Deletion marker. Used in MPQ patches, indicating that the file no longer exists.
  * `r`: File has Sector CRC checksums for each sector. This is ignored if the file is not compressed or imploded.
  * `s`: Present on STANDARD.SNP\(signature).
  * `x`: File exists; this is reset if the file is deleted.
  * `m`: Mask for a file being compressed.
  * `n`: Use default flags for internal files.
  * `f`: Fix key; This is obsolete.

You can use the `--property` argument with the `list` subcommand to print the given properties. Many properties can be given, and they will be printed in the order given.

```bash
$ mpqcli list -d -a Patch_rt.mpq --property hash-index --property locale --property flags
...
10893 enUS     ixmn  glue\ScorePv\pMain.pcx
11173 enUS     ixmn  rez\gluAll.tbl
11174 deDE     ixmn  rez\gluAll.tbl
11175 esES     ixmn  rez\gluAll.tbl
11176 frFR     ixmn  rez\gluAll.tbl
11177 itIT     ixmn  rez\gluAll.tbl
11178 ptBR     ixmn  rez\gluAll.tbl
11726 enUS     cxmn  (attributes)
11884 enUS     ixmn  glue\ScoreTd\pMain.pcx
...
```

## List all files with an external listfile

Older MPQ archives do not contain (complete) file paths of their content. By using the `-l` or `--listfile` argument, one can provide an external listfile that lists the content of the MPQ archive, so that the listed files will have the correct paths. Listfiles can be downloaded on [Ladislav Zezula's site](http://www.zezula.net/en/mpq/download.html).

```bash
$ mpqcli list -l /path/to/listfile StarDat.mpq
```
