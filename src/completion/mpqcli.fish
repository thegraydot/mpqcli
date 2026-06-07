# Fish shell completions for mpqcli
# Source: generated from mpqcli/src/main.cpp
#
# Install:
#   cp mpqcli.fish ~/.config/fish/completions/mpqcli.fish

# Disable file completion globally; re-enable per argument where needed.
complete -c mpqcli -f

# Shared value sets (sourced directly from main.cpp)
set -l __mpqcli_locales \
    default \
    enUS enGB zhTW zhCN csCZ deDE esES esMX \
    frFR itIT jaJP koKR nlNL plPL ptBR ptPT ruRU

set -l __mpqcli_game_profiles \
    generic \
    diablo1 diablo d1 \
    lordsofmagic lomse \
    starcraft starcraft1 sc1 \
    warcraft2 wc2 war2 \
    diablo2 d2 \
    warcraft3 wc3 war3 \
    warcraft3-map wc3-map war3-map \
    wow1 wow-vanilla \
    wow2 wow-tbc \
    wow3 wow-wotlk \
    wow4 wow-cataclysm \
    wow5 wow-mop \
    starcraft2 sc2 \
    diablo3 d3

set -l __mpqcli_info_properties \
    format-version header-offset header-size archive-size \
    file-count max-files signature-type

set -l __mpqcli_list_properties \
    hash-index name-hash1 name-hash2 name-hash3 locale \
    file-index byte-offset file-time file-size compressed-size \
    flags encryption-key encryption-key-raw

# Top-level subcommands (no subcommand active yet)
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a version   -d 'Print program version'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a about     -d 'Print program information'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a info      -d 'Print info about an MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a create    -d 'Create an MPQ archive from a file or directory'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a add       -d 'Add files to an existing MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a remove    -d 'Remove files from an existing MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a list      -d 'List files in an MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a extract   -d 'Extract files from an MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a read      -d 'Read a file from an MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a verify      -d 'Verify an MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a compact     -d 'Compact the MPQ archive'
complete -c mpqcli -n 'not __fish_seen_subcommand_from version about info create add remove list extract read verify compact completion' \
    -a completion  -d 'Generate shell completion script'

# info
#   info <target.mpq> [-p/--property <value>]
# positional: existing file
complete -c mpqcli -n '__fish_seen_subcommand_from info' -F

complete -c mpqcli -n '__fish_seen_subcommand_from info' \
    -s p -l property -d 'Print only a specific property value' \
    -r -a "$__mpqcli_info_properties"

# create
#   create <target> [-p/--path] [-o/--output] [-s/--sign]
#          [--locale] [-g/--game]
#          [--version] [--stream-flags] [--sector-size] [--raw-chunk-size]
#          [--file-flags1/2/3] [--attr-flags]
#          [--flags] [--compression] [--compression-next]
complete -c mpqcli -n '__fish_seen_subcommand_from create' -F

complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s p -l path        -d 'Archive path for a single file, or prefix for a directory' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s o -l output      -d 'Output MPQ archive path' -r -F
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s s -l sign        -d 'Sign the MPQ archive'
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l locale           -d 'Locale to use for added files' \
    -r -a "$__mpqcli_locales"
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s g -l game        -d 'Game profile for MPQ creation' \
    -r -a "$__mpqcli_game_profiles"
# Game setting overrides
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l version          -d 'Override MPQ archive version (1-4)' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l stream-flags     -d 'Override stream flags' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l sector-size      -d 'Override sector size' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l raw-chunk-size   -d 'Override raw chunk size (MPQ v4)' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l file-flags1      -d 'Override file flags for (listfile)' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l file-flags2      -d 'Override file flags for (attributes)' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l file-flags3      -d 'Override file flags for (signature)' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l attr-flags       -d 'Override attribute flags (CRC32, FILETIME, MD5)' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l flags            -d 'Override MPQ file flags for added files' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l compression      -d 'Override compression for first sector of added files' -r
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l compression-next -d 'Override compression for subsequent sectors of added files' -r

# add
#   add <archive.mpq> <files...> [-p/--path] [-w/--overwrite] [-u/--update]
#       [--locale] [-g/--game]
#       [--flags] [--compression] [--compression-next]
complete -c mpqcli -n '__fish_seen_subcommand_from add' -F

complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s p -l path            -d 'Archive path for a single file, or prefix for a directory' -r
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s w -l overwrite       -d 'Overwrite file if it already exists in the archive'
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s u -l update          -d 'Skip files whose archived size matches on-disk size'
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l locale               -d 'Locale to use for added file' \
    -r -a "$__mpqcli_locales"
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s g -l game            -d 'Game profile for compression rules' \
    -r -a "$__mpqcli_game_profiles"
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l flags                -d 'Override MPQ file flags' -r
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l compression          -d 'Override compression for first sector' -r
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l compression-next     -d 'Override compression for subsequent sectors' -r

# remove
#   remove <archive.mpq> <files...> [--locale]
complete -c mpqcli -n '__fish_seen_subcommand_from remove' -F

complete -c mpqcli -n '__fish_seen_subcommand_from remove' \
    -l locale   -d 'Locale of the file to remove' \
    -r -a "$__mpqcli_locales"

# list
#   list <target.mpq> [-l/--listfile] [-d/--detailed] [-a/--all]
#        [-p/--property <value...>]
complete -c mpqcli -n '__fish_seen_subcommand_from list' -F

complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s l -l listfile    -d 'External file listing content of the archive' -r -F
complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s d -l detailed    -d 'Show additional columns'
complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s a -l all         -d 'Include hidden files'
complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s p -l property    -d 'Print only specific property values' \
    -r -a "$__mpqcli_list_properties"

# extract
#   extract <target.mpq> [-o/--output] [-f/--file] [-k/--keep]
#           [-l/--listfile] [--locale]
complete -c mpqcli -n '__fish_seen_subcommand_from extract' -F

complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s o -l output      -d 'Output directory' -r -F
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s f -l file        -d 'Target file to extract' -r
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s k -l keep        -d 'Keep folder structure'
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s l -l listfile    -d 'External file listing content of the archive' -r -F
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -l locale           -d 'Preferred locale for extracted file' \
    -r -a "$__mpqcli_locales"

# read
#   read <file> <target.mpq> [--locale]
complete -c mpqcli -n '__fish_seen_subcommand_from read' -F

complete -c mpqcli -n '__fish_seen_subcommand_from read' \
    -l locale   -d 'Preferred locale for read file' \
    -r -a "$__mpqcli_locales"

# verify
#   verify <target.mpq> [-p/--print]
complete -c mpqcli -n '__fish_seen_subcommand_from verify' -F

complete -c mpqcli -n '__fish_seen_subcommand_from verify' \
    -s p -l print   -d 'Print the digital signature (in hex)'

# compact
#   compact <target.mpq> [-l/--listfile]
complete -c mpqcli -n '__fish_seen_subcommand_from compact' -F

complete -c mpqcli -n '__fish_seen_subcommand_from compact' \
    -s l -l listfile    -d 'External file listing content of the archive' -r -F

# completion
#   completion <shell>
complete -c mpqcli -n '__fish_seen_subcommand_from completion' \
    -a bash        -d 'Generate bash completion script'
complete -c mpqcli -n '__fish_seen_subcommand_from completion' \
    -a zsh         -d 'Generate zsh completion script'
complete -c mpqcli -n '__fish_seen_subcommand_from completion' \
    -a powershell  -d 'Generate PowerShell completion script'
complete -c mpqcli -n '__fish_seen_subcommand_from completion' \
    -a fish        -d 'Generate fish completion script'
