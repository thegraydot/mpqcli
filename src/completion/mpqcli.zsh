#compdef mpqcli

_mpqcli_locales=(
    default enUS zhTW csCZ deDE esES frFR itIT
    jaJP koKR nlNL plPL ptBR ruRU zhCN enGB esMX ptPT
)

_mpqcli_games=(
    generic
    diablo1 diablo d1
    lordsofmagic lomse
    starcraft starcraft1 sc1
    warcraft2 wc2 war2
    diablo2 d2
    warcraft3 wc3 war3
    warcraft3-map wc3-map war3-map
    wow1 wow-vanilla
    wow2 wow-tbc
    wow3 wow-wotlk
    wow4 wow-cataclysm
    wow5 wow-mop
    starcraft2 sc2
    diablo3 d3
)

_mpqcli_info_properties=(
    format-version header-offset header-size archive-size
    file-count max-files signature-type
)

_mpqcli_list_properties=(
    hash-index name-hash1 name-hash2 name-hash3 locale
    file-index byte-offset file-time file-size compressed-size
    flags encryption-key encryption-key-raw
)

_mpqcli() {
    local state
    typeset -A opt_args

    _arguments -C \
        '1: :_mpqcli_cmds' \
        '*:: :->subcmd'

    case $state in
        subcmd)
            case $words[1] in
                info)       _mpqcli_info ;;
                create)     _mpqcli_create ;;
                add)        _mpqcli_add ;;
                remove)     _mpqcli_remove ;;
                list)       _mpqcli_list ;;
                extract)    _mpqcli_extract ;;
                read)       _mpqcli_read ;;
                verify)     _mpqcli_verify ;;
                compact)    _mpqcli_compact ;;
                completion) _mpqcli_completion ;;
            esac
            ;;
    esac
}

_mpqcli_cmds() {
    local cmds=(
        'version:Print program version'
        'about:Print program information'
        'info:Print info about an MPQ archive'
        'create:Create an MPQ archive from target file or directory'
        'add:Add files to an existing MPQ archive'
        'remove:Remove files from an existing MPQ archive'
        'list:List files from the MPQ archive'
        'extract:Extract files from the MPQ archive'
        'read:Read a file from an MPQ archive'
        'verify:Verify the MPQ archive'
        'compact:Compact the MPQ archive'
        'completion:Generate shell completion script'
    )
    _describe 'subcommand' cmds
}

_mpqcli_info() {
    local props="${_mpqcli_info_properties[*]}"
    _arguments \
        '1:archive:_files' \
        '(-p --property)'{-p,--property}'[print only a specific property]:property:('"$props"')'
}

_mpqcli_create() {
    _arguments \
        '1:target:_files' \
        '(-p --path)'{-p,--path}'[archive path for a single file, or prefix for a directory]:path' \
        '(-o --output)'{-o,--output}'[output archive]:file:_files' \
        '(-s --sign)'{-s,--sign}'[sign the archive]' \
        '--locale[locale for added files]:locale:('"${_mpqcli_locales[@]}"')' \
        '(-g --game)'{-g,--game}'[game profile]:profile:('"${_mpqcli_games[@]}"')' \
        '--version[MPQ archive version (1-4)]:version:(1 2 3 4)' \
        '--stream-flags[override stream flags]:flags' \
        '--sector-size[override sector size]:size' \
        '--raw-chunk-size[override raw chunk size for MPQ v4]:size' \
        '--file-flags1[override file flags for listfile]:flags' \
        '--file-flags2[override file flags for attributes]:flags' \
        '--file-flags3[override file flags for signature]:flags' \
        '--attr-flags[override attribute flags]:flags' \
        '--flags[override MPQ file flags for added files]:flags' \
        '--compression[override compression for first sector]:compression' \
        '--compression-next[override compression for subsequent sectors]:compression'
}

_mpqcli_add() {
    _arguments \
        '1:archive:_files' \
        '*:files:_files' \
        '(-p --path)'{-p,--path}'[archive path or prefix for directory add]:path' \
        '(-w --overwrite)'{-w,--overwrite}'[overwrite existing file]' \
        '(-u --update)'{-u,--update}'[skip unchanged files in directory add]' \
        '--locale[locale for added file]:locale:('"${_mpqcli_locales[@]}"')' \
        '(-g --game)'{-g,--game}'[game profile]:profile:('"${_mpqcli_games[@]}"')' \
        '--flags[override MPQ file flags]:flags' \
        '--compression[override compression for first sector]:compression' \
        '--compression-next[override compression for subsequent sectors]:compression'
}

_mpqcli_remove() {
    _arguments \
        '1:archive:_files' \
        '*:archive paths' \
        '--locale[locale of file to remove]:locale:('"${_mpqcli_locales[@]}"')'
}

_mpqcli_list() {
    local props="${_mpqcli_list_properties[*]}"
    _arguments \
        '1:archive:_files' \
        '(-l --listfile)'{-l,--listfile}'[listfile path]:file:_files' \
        '(-d --detailed)'{-d,--detailed}'[detailed listing with extra columns]' \
        '(-a --all)'{-a,--all}'[include hidden files]' \
        '(-p --property)'{-p,--property}'[print specific property values]:property:('"$props"')'
}

_mpqcli_extract() {
    _arguments \
        '1:archive:_files' \
        '(-o --output)'{-o,--output}'[output directory]:dir:_files -/' \
        '(-f --file)'{-f,--file}'[target file to extract]:file' \
        '(-k --keep)'{-k,--keep}'[keep folder structure]' \
        '(-l --listfile)'{-l,--listfile}'[listfile path]:file:_files' \
        '--locale[preferred locale for extracted file]:locale:('"${_mpqcli_locales[@]}"')'
}

_mpqcli_read() {
    _arguments \
        '1:file-in-archive' \
        '2:archive:_files' \
        '--locale[preferred locale for read file]:locale:('"${_mpqcli_locales[@]}"')'
}

_mpqcli_verify() {
    _arguments \
        '1:archive:_files' \
        '(-p --print)'{-p,--print}'[print the digital signature in hex]'
}

_mpqcli_compact() {
    _arguments \
        '1:archive:_files' \
        '(-l --listfile)'{-l,--listfile}'[listfile path]:file:_files'
}

_mpqcli_completion() {
    local shells=(
        'bash:Generate bash completion script'
        'zsh:Generate zsh completion script'
        'powershell:Generate PowerShell completion script'
        'fish:Generate fish completion script'
    )
    _describe 'shell' shells
}

_mpqcli "$@"
