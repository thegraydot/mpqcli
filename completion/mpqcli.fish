# Fish completion for mpqcli
#
# Install:
#   mpqcli completion fish > ~/.config/fish/completions/mpqcli.fish

# Disable file completion globally; re-enable per argument where needed
complete -c mpqcli -f

# Index, from one, of the positional argument under the cursor. The command
# name and subcommand are skipped, as are options and the value following any
# option not listed as a flag in the arguments. A lone - counts as a positional
function __mpqcli_positional_index
    set -l index 1
    set -l skip 0
    for token in (commandline -poc)[3..-1]
        if test $skip -eq 1
            set skip 0
            continue
        end
        switch $token
            case '-'
                set index (math $index + 1)
            case '--*=*'
            case '-*'
                if not contains -- $token $argv
                    set skip 1
                end
            case '*'
                set index (math $index + 1)
        end
    end
    echo $index
end

# Shared value sets, kept in step with app/main.cpp, app/info.cpp, app/list.cpp,
# src/util/locales.cpp and src/gamerules/profiles.cpp
set -l __mpqcli_subcommands \
    version about info create add remove rename list extract read verify compact completion

set -l __mpqcli_locales \
    default \
    enUS enGB zhTW zhCN csCZ deDE esES esMX \
    frFR itIT jaJP koKR nlNL plPL ptBR ptPT ruRU

set -l __mpqcli_game_profiles \
    generic \
    diablo1 diablo d1 \
    lordsofmagic lomse \
    starcraft starcraft1 sc sc1 \
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
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a version    -d 'Prints program version'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a about      -d 'Prints program information'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a info       -d 'Prints info about an MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a create     -d 'Create an MPQ archive from target file or directory'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a add        -d 'Add files to an existing MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a remove     -d 'Remove files from an existing MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a rename     -d 'Rename a file in an existing MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a list       -d 'List files from the MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a extract    -d 'Extract files from the MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a read       -d 'Read a file from an MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a verify     -d 'Verify the MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a compact    -d 'Compact the MPQ archive'
complete -c mpqcli -n "not __fish_seen_subcommand_from $__mpqcli_subcommands" \
    -a completion -d 'Generate shell completion script'

# info
#   info <target.mpq> [-p/--property <value>]
complete -c mpqcli -n '__fish_seen_subcommand_from info; and test (__mpqcli_positional_index) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from info' \
    -s p -l property -d 'Print only a specific property value' \
    -r -f -a "$__mpqcli_info_properties"

# create
#   create <target> [-p/--path] [-o/--output] [-s/--sign]
#          [--locale] [-g/--game]
#          [--version] [--stream-flags] [--sector-size] [--raw-chunk-size]
#          [--file-flags1/2/3] [--attr-flags]
#          [--flags] [--compression] [--compression-next]
complete -c mpqcli -n '__fish_seen_subcommand_from create; and test (__mpqcli_positional_index -s --sign) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s p -l path        -d 'Archive path for a single file, or prefix for a directory' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s o -l output      -d 'Output MPQ archive path' -r -F
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s s -l sign        -d 'Sign the MPQ archive'
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l locale           -d 'Locale to use for added files' \
    -r -f -a "$__mpqcli_locales"
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -s g -l game        -d 'Game profile for MPQ creation' \
    -r -f -a "$__mpqcli_game_profiles"
# Game setting overrides
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l version          -d 'Override MPQ archive version (1-4)' -r -f -a '1 2 3 4'
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l stream-flags     -d 'Override stream flags' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l sector-size      -d 'Override sector size' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l raw-chunk-size   -d 'Override raw chunk size (MPQ v4)' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l file-flags1      -d 'Override file flags for (listfile)' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l file-flags2      -d 'Override file flags for (attributes)' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l file-flags3      -d 'Override file flags for (signature)' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l attr-flags       -d 'Override attribute flags (CRC32, FILETIME, MD5)' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l flags            -d 'Override MPQ file flags for added files' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l compression      -d 'Override compression for first sector of added files' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from create' \
    -l compression-next -d 'Override compression for subsequent sectors of added files' -r -f

# add
#   add <archive.mpq> <files...> [-p/--path] [-w/--overwrite | -u/--update]
#       [--locale] [-g/--game]
#       [--flags] [--compression] [--compression-next]
# Positional 1 is the archive and the rest are files or directories to add, so
# every positional takes a path
complete -c mpqcli -n '__fish_seen_subcommand_from add' -F

complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s p -l path            -d 'Archive path for a single file, or prefix for a directory' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s w -l overwrite       -d 'Replace every file that already exists in the archive'
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s u -l update          -d 'Replace only files that changed'
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l locale               -d 'Locale to use for added file' \
    -r -f -a "$__mpqcli_locales"
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -s g -l game            -d 'Game profile for compression rules' \
    -r -f -a "$__mpqcli_game_profiles"
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l flags                -d 'Override MPQ file flags' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l compression          -d 'Override compression for first sector' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from add' \
    -l compression-next     -d 'Override compression for subsequent sectors' -r -f

# remove
#   remove <archive.mpq> <files...> [--locale]
# Positional 1 is the archive; the rest are in-archive paths, so they get no
# filesystem completion
complete -c mpqcli -n '__fish_seen_subcommand_from remove; and test (__mpqcli_positional_index) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from remove' \
    -l locale   -d 'Locale of the file to remove' \
    -r -f -a "$__mpqcli_locales"

# rename
#   rename <archive.mpq> <old path> <new path> [--locale]
# Positional 1 is the archive; positionals 2 and 3 are in-archive paths, so
# they get no filesystem completion
complete -c mpqcli -n '__fish_seen_subcommand_from rename; and test (__mpqcli_positional_index) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from rename' \
    -l locale   -d 'Locale of the file to rename' \
    -r -f -a "$__mpqcli_locales"

# list
#   list <target.mpq> [-l/--listfile] [-d/--detailed] [-a/--all]
#        [-p/--property <value...>]
complete -c mpqcli -n '__fish_seen_subcommand_from list; and test (__mpqcli_positional_index -d --detailed -a --all) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s l -l listfile    -d 'External file listing content of the archive' -r -F
complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s d -l detailed    -d 'Show additional columns'
complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s a -l all         -d 'Include hidden files'
complete -c mpqcli -n '__fish_seen_subcommand_from list' \
    -s p -l property    -d 'Print only specific property values' \
    -r -f -a "$__mpqcli_list_properties"

# extract
#   extract <target.mpq> [-o/--output] [-f/--file] [-k/--keep]
#           [-l/--listfile] [--locale]
complete -c mpqcli -n '__fish_seen_subcommand_from extract; and test (__mpqcli_positional_index -k --keep) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s o -l output      -d 'Output directory' -r -f -a '(__fish_complete_directories)'
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s f -l file        -d 'Target file to extract' -r -f
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s k -l keep        -d 'Keep folder structure'
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -s l -l listfile    -d 'External file listing content of the archive' -r -F
complete -c mpqcli -n '__fish_seen_subcommand_from extract' \
    -l locale           -d 'Preferred locale for extracted file' \
    -r -f -a "$__mpqcli_locales"

# read
#   read <file> <target.mpq> [--locale]
# Positional 1 is an in-archive path, so it gets no filesystem completion;
# positional 2 is the archive
complete -c mpqcli -n '__fish_seen_subcommand_from read; and test (__mpqcli_positional_index) -eq 2' -F

complete -c mpqcli -n '__fish_seen_subcommand_from read' \
    -l locale   -d 'Preferred locale for read file' \
    -r -f -a "$__mpqcli_locales"

# verify
#   verify <target.mpq> [-p/--print]
complete -c mpqcli -n '__fish_seen_subcommand_from verify; and test (__mpqcli_positional_index -p --print) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from verify' \
    -s p -l print   -d 'Print the digital signature (in hex)'

# compact
#   compact <target.mpq> [-l/--listfile]
complete -c mpqcli -n '__fish_seen_subcommand_from compact; and test (__mpqcli_positional_index) -eq 1' -F

complete -c mpqcli -n '__fish_seen_subcommand_from compact' \
    -s l -l listfile    -d 'External file listing content of the archive' -r -F

# completion
#   completion <shell>
complete -c mpqcli -n '__fish_seen_subcommand_from completion; and not __fish_seen_subcommand_from bash zsh powershell fish' \
    -a bash        -d 'Generate bash completion script'
complete -c mpqcli -n '__fish_seen_subcommand_from completion; and not __fish_seen_subcommand_from bash zsh powershell fish' \
    -a zsh         -d 'Generate zsh completion script'
complete -c mpqcli -n '__fish_seen_subcommand_from completion; and not __fish_seen_subcommand_from bash zsh powershell fish' \
    -a powershell  -d 'Generate PowerShell completion script'
complete -c mpqcli -n '__fish_seen_subcommand_from completion; and not __fish_seen_subcommand_from bash zsh powershell fish' \
    -a fish        -d 'Generate fish completion script'
