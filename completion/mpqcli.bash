# Bash completion for mpqcli. Uses the bash-completion helpers when they are
# loaded and falls back to plain compgen otherwise, so it also runs on bash 3.2

# Fill COMPREPLY with the words in $1 that match the current word
_mpqcli_words() {
    local word
    COMPREPLY=()
    while IFS= read -r word; do
        COMPREPLY+=("$word")
    done < <(compgen -W "$1" -- "$cur")
}

# Complete filesystem paths. With no argument every path is offered, with -d
# only directories, and with an extension such as mpq only files of that type
# in either case plus directories, so an archive can be reached by descending
_mpqcli_filedir() {
    if declare -f _filedir > /dev/null 2>&1; then
        _filedir "${1-}"
        return
    fi

    local path upper
    COMPREPLY=()
    compopt -o filenames 2> /dev/null
    if [[ "${1-}" == -d ]]; then
        while IFS= read -r path; do
            COMPREPLY+=("$path")
        done < <(compgen -d -- "$cur")
    elif [[ -n "${1-}" ]]; then
        upper=$(printf '%s' "$1" | tr '[:lower:]' '[:upper:]')
        while IFS= read -r path; do
            if [[ -d "$path" || "$path" == *."$1" || "$path" == *."$upper" ]]; then
                COMPREPLY+=("$path")
            fi
        done < <(compgen -f -- "$cur")
    else
        while IFS= read -r path; do
            COMPREPLY+=("$path")
        done < <(compgen -f -- "$cur")
    fi
}

# Set pos to the index, from zero, of the positional argument under the cursor.
# Every option is assumed to take a value in the following word, except the
# flags passed as arguments, and a lone - counts as a positional (stdin)
_mpqcli_positional_index() {
    local i flag
    pos=0
    for ((i = 2; i < cword; i++)); do
        case "${words[i]}" in
            -) pos=$((pos + 1)) ;;
            --*=*) ;;
            -*)
                for flag in "$@"; do
                    [[ "${words[i]}" == "$flag" ]] && continue 2
                done
                i=$((i + 1))
                ;;
            *) pos=$((pos + 1)) ;;
        esac
    done
}

_mpqcli() {
    local cur prev words cword pos

    # Use bash-completion helpers when available, fall back to COMP_* variables.
    # The -s splits --option=value so the value completes like a separate word
    if declare -f _init_completion > /dev/null 2>&1; then
        _init_completion -s || return
    else
        COMPREPLY=()
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        words=("${COMP_WORDS[@]}")
        cword=$COMP_CWORD
    fi

    local subcommands="version about info create add remove rename list extract read verify compact completion"
    local -a locales=(
        default enUS zhTW csCZ deDE esES frFR itIT
        jaJP koKR nlNL plPL ptBR ruRU zhCN enGB esMX ptPT
    )
    local -a games=(
        generic
        diablo1 diablo d1
        lordsofmagic lomse
        starcraft starcraft1 sc sc1
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
    local -a info_properties=(
        format-version header-offset header-size archive-size
        file-count max-files signature-type
    )
    local -a list_properties=(
        hash-index name-hash1 name-hash2 name-hash3 locale
        file-index byte-offset file-time file-size compressed-size
        flags encryption-key encryption-key-raw
    )

    if [[ $cword -eq 1 ]]; then
        _mpqcli_words "$subcommands"
        return
    fi

    case "${words[1]}" in
        info)
            case "$prev" in
                -p|--property) _mpqcli_words "${info_properties[*]}"; return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "-p --property"
            else
                _mpqcli_positional_index
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        create)
            case "$prev" in
                --locale) _mpqcli_words "${locales[*]}"; return ;;
                -g|--game) _mpqcli_words "${games[*]}"; return ;;
                --version) _mpqcli_words "1 2 3 4"; return ;;
                -o|--output) _mpqcli_filedir; return ;;
                -p|--path|--stream-flags|--sector-size|--raw-chunk-size|\
                --file-flags1|--file-flags2|--file-flags3|--attr-flags|\
                --flags|--compression|--compression-next)
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "-p --path -o --output -s --sign --locale -g --game --version \
--stream-flags --sector-size --raw-chunk-size --file-flags1 --file-flags2 --file-flags3 \
--attr-flags --flags --compression --compression-next"
            else
                _mpqcli_positional_index -s --sign
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir
                fi
            fi
            ;;

        add)
            case "$prev" in
                --locale) _mpqcli_words "${locales[*]}"; return ;;
                -g|--game) _mpqcli_words "${games[*]}"; return ;;
                -p|--path|--flags|--compression|--compression-next) return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "-p --path -w --overwrite -u --update --locale -g --game \
--flags --compression --compression-next"
            else
                # Positional 1 is the archive; the rest are files or directories to add
                _mpqcli_positional_index -w --overwrite -u --update
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                else
                    _mpqcli_filedir
                fi
            fi
            ;;

        remove)
            case "$prev" in
                --locale) _mpqcli_words "${locales[*]}"; return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "--locale"
            else
                # Positional 1 is the archive; the rest are in-archive paths, so
                # they get no filesystem completion
                _mpqcli_positional_index
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        rename)
            case "$prev" in
                --locale) _mpqcli_words "${locales[*]}"; return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "--locale"
            else
                # Positional 1 is the archive; positionals 2 and 3 are in-archive
                # paths, so they get no filesystem completion
                _mpqcli_positional_index
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        list)
            case "$prev" in
                -l|--listfile) _mpqcli_filedir; return ;;
                -p|--property) _mpqcli_words "${list_properties[*]}"; return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "-l --listfile -d --detailed -a --all -p --property"
            else
                _mpqcli_positional_index -d --detailed -a --all
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        extract)
            case "$prev" in
                -o|--output) _mpqcli_filedir -d; return ;;
                -l|--listfile) _mpqcli_filedir; return ;;
                --locale) _mpqcli_words "${locales[*]}"; return ;;
                -f|--file) return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "-o --output -f --file -k --keep -l --listfile --locale"
            else
                _mpqcli_positional_index -k --keep
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        read)
            case "$prev" in
                --locale) _mpqcli_words "${locales[*]}"; return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "--locale"
            else
                # Positional 1 is an in-archive path, so it gets no filesystem
                # completion; positional 2 is the archive
                _mpqcli_positional_index
                if [[ $pos -eq 1 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        verify)
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "-p --print"
            else
                _mpqcli_positional_index -p --print
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        compact)
            case "$prev" in
                -l|--listfile) _mpqcli_filedir; return ;;
            esac
            if [[ "$cur" == -* ]]; then
                _mpqcli_words "-l --listfile"
            else
                _mpqcli_positional_index
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_filedir mpq
                fi
            fi
            ;;

        completion)
            if [[ "$cur" != -* ]]; then
                _mpqcli_positional_index
                if [[ $pos -eq 0 ]]; then
                    _mpqcli_words "bash zsh powershell fish"
                fi
            fi
            ;;

        version|about) ;;
    esac
}

complete -F _mpqcli mpqcli
