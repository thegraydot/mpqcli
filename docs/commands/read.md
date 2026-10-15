# read

Read a specific file to stdout.

## Read a specific file from an MPQ archive

Read the `patch.cmd` file from an MPQ archive and print the file contents to stdout. Even though the subcommand always outputs bytes, plaintext files will be human-readable.

```bash
$ mpqcli read wow-patch.mpq patch.cmd
* set the product patch name
PatchVersion This patch upgrades World of Warcraft from version 1.10.0.5195 to version 1.10.1.5230.
* make sure that we don't patch version 1.10.1.5230 or greater
FileVersionEqualTo    "$(InstallPath)\WoW.exe"    1.10.0.5195   0xffff.0xffff.0xffff.0xffff
* ProductVersionLessThan "$(InstallPath)\WoW.exe"    0.0.0.256  0.0.0.0xffff
Language enGB
SetLauncher "$(InstallPath)\WoW.exe"
* SetUninstall $(WinDir)\WoWUnin.dat
Self "World of Warcraft"
WoWPatchIndex 2
PatchSize 1992294400
```

The tool will always print output in bytes and is designed to be "piped" or redirected. For example, redirect the binary `WoW.exe` file from a WoW patch to a local file:

```bash
$ mpqcli read wow-patch.mpq "WoW.exe" > WoW.exe
```

Another example, piping the bytes to the `xxd` tool.

```bash
$ mpqcli read wow-patch.mpq "WoW.exe" | xxd
00000000: 1800 0404 de2a a5da 3240 4500 3250 4500  .....*..2@E.2PE.
00000010: 69ac 2703 0859 c601 0a7a 4500 8942 5344  i.'..Y...zE..BSD
00000020: 4946 4634 30a0 2905 8203 f244 0482 3250  IFF40.)....D..2P
00000030: 4504 8080 0280 9002 8098 0281 d801 0585  E...............
```

## Read one specific file with locale

Use the `-e` or `--locale` argument to specify the locale of the file to read. If there is no file with the requested name and locale, the default locale will be used instead.

```bash
$ mpqcli read Patch_rt.mpq "rez\stat_txt.tbl" --locale ptPT
```
