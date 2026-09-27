# Advanced examples

## Search and extract files on Linux

The `mpqcli` tool has no native search feature - instead, it is designed to be integrated with other, external operating system tools. For example, `mpqcli list` can be "piped" to `grep` in Linux or `Select-String` in Windows PowerShell to perform searching.

The following command lists all files in an MPQ archive, and each filename is filtered using `grep` - selecting files ending in `.exe` (note: `.` is a regex wildcard matching any character; use `\.exe` for a strictly literal match) - which is then passed back to `mpqcli extract`. The result: search and extract all `exe` files.

```bash
$ mpqcli list wow-patch.mpq | grep -i \.exe | xargs -I@ mpqcli extract -f "@" wow-patch.mpq
[*] Extracted: Launcher.exe
[*] Extracted: BackgroundDownloader.exe
[*] Extracted: WoW.exe
[*] Extracted: BNUpdate.exe
[*] Extracted: Repair.exe
[*] Extracted: WowError.exe
```

Note that directories are specified with backslashes, which need to be escaped (due to how grep and xargs handles them). The following example extracts all `dat` files in the `arr` directory from the `StarDat.mpq` archive and handles escaping via `sed`.

```bash
$ mpqcli list -l scbw.txt StarDat.mpq | grep -i "arr\\\.*dat$" | sort | sed 's|\\|\\\\|g' | \
    xargs -I@ mpqcli extract -f "@" -k StarDat.mpq
[*] Extracted: arr/flingy.dat
[*] Extracted: arr/images.dat
[*] Extracted: arr/mapdata.dat
[*] Extracted: arr/orders.dat
[*] Extracted: arr/portdata.dat
[*] Extracted: arr/sfxdata.dat
[*] Extracted: arr/sprites.dat
[*] Extracted: arr/techdata.dat
[*] Extracted: arr/units.dat
[*] Extracted: arr/upgrades.dat
[*] Extracted: arr/weapons.dat
```

## Search and extract files on Windows

The following command lists all files in an MPQ archive, and each filename is filtered using `Select-String` - selecting files ending in `.exe` (note: `.` is a regex wildcard matching any character; use `\.exe` for a strictly literal match) - which is then passed back to `mpqcli extract`. The result: search and extract all `exe` files.

```powershell
PS> mpqcli.exe list wow-patch.mpq | Select-String -Pattern \.exe | `
    ForEach-Object { mpqcli.exe extract -f $_ wow-patch.mpq }
[*] Extracted: Launcher.exe
[*] Extracted: BackgroundDownloader.exe
[*] Extracted: WoW.exe
[*] Extracted: BNUpdate.exe
[*] Extracted: Repair.exe
[*] Extracted: WowError.exe
```
