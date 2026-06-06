#Requires -Version 5.1
<#
.SYNOPSIS
    PowerShell argument completer for the mpqcli tool.

.DESCRIPTION
    Provides tab/menu completion for mpqcli subcommands, options, and known
    value sets (game profiles, locales, info/list properties). File-path
    arguments fall back to PowerShell's native filesystem completion.

.NOTES
    Source it from your $PROFILE:
        . /path/to/mpqcli-completion.ps1

    Or copy it into a module / profile script that loads at startup.
    Works for any executable named 'mpqcli' or 'mpqcli.exe'.
#>

Register-ArgumentCompleter -Native -CommandName 'mpqcli', 'mpqcli.exe' -ScriptBlock {
    param($wordToComplete, $commandAst, $cursorPosition)

    # Static value sets (derived from mpqcli source)

    $subcommands = [ordered]@{
        'version'    = 'Prints program version'
        'about'      = 'Prints program information'
        'info'       = 'Prints info about an MPQ archive'
        'create'     = 'Create an MPQ archive from target file or directory'
        'add'        = 'Add files to an existing MPQ archive'
        'remove'     = 'Remove files from an existing MPQ archive'
        'list'       = 'List files from the MPQ archive'
        'extract'    = 'Extract files from the MPQ archive'
        'read'       = 'Read a file from an MPQ archive'
        'verify'     = 'Verify the MPQ archive'
        'completion' = 'Generate shell completion script'
    }

    $gameProfiles = @(
        'generic',
        'diablo1', 'diablo', 'd1',
        'lordsofmagic', 'lomse',
        'starcraft', 'starcraft1', 'sc1',
        'warcraft2', 'wc2', 'war2',
        'diablo2', 'd2',
        'warcraft3', 'wc3', 'war3',
        'warcraft3-map', 'wc3-map', 'war3-map',
        'wow1', 'wow-vanilla',
        'wow2', 'wow-tbc',
        'wow3', 'wow-wotlk',
        'wow4', 'wow-cataclysm',
        'wow5', 'wow-mop',
        'starcraft2', 'sc2',
        'diablo3', 'd3'
    )

    $locales = @(
        'default',
        'enUS', 'enGB', 'zhTW', 'zhCN', 'csCZ', 'deDE', 'esES', 'esMX',
        'frFR', 'itIT', 'jaJP', 'koKR', 'nlNL', 'plPL', 'ptBR', 'ptPT', 'ruRU'
    )

    $infoProperties = @(
        'format-version', 'header-offset', 'header-size', 'archive-size',
        'file-count', 'max-files', 'signature-type'
    )

    $listProperties = @(
        'hash-index', 'name-hash1', 'name-hash2', 'name-hash3', 'locale',
        'file-index', 'byte-offset', 'file-time', 'file-size', 'compressed-size',
        'flags', 'encryption-key', 'encryption-key-raw'
    )

    # Options available per subcommand. Each value is a hashtable mapping the
    # option token to a short help string.
    $optionSpec = @{
        'info' = @{
            '-p'         = 'Print only a specific property value'
            '--property' = 'Print only a specific property value'
        }
        'create' = @{
            '-p'              = 'Archive path for a single file, or prefix for a directory'
            '--path'          = 'Archive path for a single file, or prefix for a directory'
            '-o'              = 'Output MPQ archive'
            '--output'        = 'Output MPQ archive'
            '-s'              = 'Sign the MPQ archive'
            '--sign'          = 'Sign the MPQ archive'
            '--locale'        = 'Locale to use for added files'
            '-g'              = 'Game profile for MPQ creation'
            '--game'          = 'Game profile for MPQ creation'
            '--version'       = 'Override the MPQ archive version (1-4)'
            '--stream-flags'  = 'Override stream flags'
            '--sector-size'   = 'Override sector size'
            '--raw-chunk-size'= 'Override raw chunk size for MPQ v4'
            '--file-flags1'   = 'Override file flags for (listfile)'
            '--file-flags2'   = 'Override file flags for (attributes)'
            '--file-flags3'   = 'Override file flags for (signature)'
            '--attr-flags'    = 'Override attribute flags (CRC32, FILETIME, MD5)'
            '--flags'             = 'Override MPQ file flags for added files'
            '--compression'       = 'Override compression for first sector'
            '--compression-next'  = 'Override compression for subsequent sectors'
        }
        'add' = @{
            '-p'          = 'Archive path for a single file, or prefix for a directory'
            '--path'      = 'Archive path for a single file, or prefix for a directory'
            '-w'          = 'Overwrite file if it already is in MPQ archive'
            '--overwrite' = 'Overwrite file if it already is in MPQ archive'
            '-u'          = 'Skip files whose archived size matches on-disk size'
            '--update'    = 'Skip files whose archived size matches on-disk size'
            '--locale'    = 'Locale to use for added file'
            '-g'          = 'Game profile for compression rules'
            '--game'      = 'Game profile for compression rules'
            '--flags'             = 'Override MPQ file flags'
            '--compression'       = 'Override compression for first sector'
            '--compression-next'  = 'Override compression for subsequent sectors'
        }
        'remove' = @{
            '--locale' = 'Locale of file to remove'
        }
        'list' = @{
            '-l'         = 'File listing content of an MPQ archive'
            '--listfile' = 'File listing content of an MPQ archive'
            '-d'         = 'File listing with additional columns'
            '--detailed' = 'File listing with additional columns'
            '-a'         = 'File listing including hidden files'
            '--all'      = 'File listing including hidden files'
            '-p'         = 'Print only specific property values'
            '--property' = 'Print only specific property values'
        }
        'extract' = @{
            '-o'         = 'Output directory'
            '--output'   = 'Output directory'
            '-f'         = 'Target file to extract'
            '--file'     = 'Target file to extract'
            '-k'         = 'Keep folder structure'
            '--keep'     = 'Keep folder structure'
            '-l'         = 'File listing content of an MPQ archive'
            '--listfile' = 'File listing content of an MPQ archive'
            '--locale'   = 'Preferred locale for extracted file'
        }
        'read' = @{
            '--locale' = 'Preferred locale for read file'
        }
        'verify' = @{
            '-p'      = 'Print the digital signature (in hex)'
            '--print' = 'Print the digital signature (in hex)'
        }
        'version'    = @{}
        'about'      = @{}
        'completion' = @{}
    }

    # Options whose *argument* should complete from a static value set.
    $valueOptions = @{
        '--locale'   = $locales
        '-g'         = $gameProfiles
        '--game'     = $gameProfiles
    }
    # Property options depend on the subcommand (info vs list), handled below.

    # Parse the current command line

    # Tokenized elements of the command line, excluding the executable itself.
    $elements = @($commandAst.CommandElements | Select-Object -Skip 1 |
        ForEach-Object { $_.ToString() })

    # Identify the active subcommand (first element that is a known subcommand).
    $subcommand = $null
    foreach ($el in $elements) {
        if ($subcommands.Contains($el)) { $subcommand = $el; break }
    }

    # The token immediately preceding the cursor (the one we may be an arg to).
    $prevToken = $null
    if ($elements.Count -ge 1) {
        if ([string]::IsNullOrEmpty($wordToComplete)) {
            $prevToken = $elements[-1]
        }
        elseif ($elements.Count -ge 2) {
            $prevToken = $elements[-2]
        }
    }

    $emit = {
        param($items, $type)
        foreach ($i in $items) {
            if ($i.Value -like "$wordToComplete*") {
                [System.Management.Automation.CompletionResult]::new(
                    $i.Value, $i.Value,
                    [System.Management.Automation.CompletionResultType]::$type,
                    $i.Tip)
            }
        }
    }

    # 1) No subcommand yet -> complete subcommands
    if (-not $subcommand) {
        $items = foreach ($k in $subcommands.Keys) {
            [pscustomobject]@{ Value = $k; Tip = $subcommands[$k] }
        }
        return & $emit $items 'Command'
    }

    # 2) completion subcommand -> complete shell names
    if ($subcommand -eq 'completion') {
        if (-not ($wordToComplete -like '-*')) {
            $shells = [ordered]@{
                'bash'       = 'Generate bash completion script'
                'zsh'        = 'Generate zsh completion script'
                'powershell' = 'Generate PowerShell completion script'
                'fish'       = 'Generate fish completion script'
            }
            $items = foreach ($k in $shells.Keys) {
                [pscustomobject]@{ Value = $k; Tip = $shells[$k] }
            }
            return & $emit $items 'ParameterValue'
        }
        return
    }

    # 4) Completing the argument to a value-bearing option
    if ($prevToken) {
        # Locale / game profile options.
        if ($valueOptions.ContainsKey($prevToken)) {
            $items = foreach ($v in $valueOptions[$prevToken]) {
                [pscustomobject]@{ Value = $v; Tip = $prevToken }
            }
            return & $emit $items 'ParameterValue'
        }
        # Property options: meaning depends on the subcommand.
        if ($prevToken -in @('-p', '--property')) {
            if ($subcommand -eq 'info') {
                $items = foreach ($v in $infoProperties) {
                    [pscustomobject]@{ Value = $v; Tip = 'info property' } }
                return & $emit $items 'ParameterValue'
            }
            elseif ($subcommand -eq 'list') {
                $items = foreach ($v in $listProperties) {
                    [pscustomobject]@{ Value = $v; Tip = 'list property' } }
                return & $emit $items 'ParameterValue'
            }
            # For 'verify', -p/--print is a flag (no value) -> fall through.
        }
        # Options that take a file/dir path -> let PowerShell complete paths.
        $pathOptions = @('-o', '--output', '-l', '--listfile', '-f', '--file',
                         '-p', '--path')
        if ($prevToken -in $pathOptions -and
            -not ($subcommand -in @('info', 'list') -and $prevToken -in @('-p', '--property'))) {
            return  # empty -> native path completion takes over
        }
    }

    # 5) Completing an option for the current subcommand
    if ($wordToComplete -like '-*' -or [string]::IsNullOrEmpty($wordToComplete)) {
        $opts = $optionSpec[$subcommand]
        if ($opts) {
            $items = foreach ($k in $opts.Keys) {
                [pscustomobject]@{ Value = $k; Tip = $opts[$k] }
            }
            $results = & $emit $items 'ParameterName'
            if ($wordToComplete -like '-*') { return $results }
            # When word is empty we still also allow path completion, so only
            # return option results if the user has started a dash.
            if ($results) { return $results }
        }
    }

    # 6) Fall back to native filesystem completion (archives/files)
    return
}
