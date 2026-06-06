# compact

Compacts an MPQ archive. It performs a complete archive rebuild, effectively defragmenting the MPQ archive, removing all gaps that have been created by adding, replacing, renaming or deleting files within the archive. To succeed, the function requires all files in MPQ archive to be accessible and their filenames to be known.

This may take several minutes to complete for large archives.

```bash
$ mpqcli compact wow-patch.mpq
[*] Compacting archive. This may take some time...
```


## Use an external listfile

The compact command requires all files inside the archive to be known. Older MPQ archives do not contain (complete) file paths of their content. By using the `-l` or `--listfile` argument, one can provide an external listfile that lists the content of the MPQ archive. Listfiles can be downloaded on [Ladislav Zezula's site](http://www.zezula.net/en/mpq/download.html).

```bash
$ mpqcli compact -l /path/to/listfile DIABDAT.MPQ
[*] Compacting archive. This may take some time...
```
