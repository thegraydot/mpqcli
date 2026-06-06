# Use _filedir when available, otherwise fall back to compgen.
# Pass an extension (e.g. "mpq") to restrict completions to that type plus directories.
_mpqcli_filedir() {
    if declare -f _filedir > /dev/null 2>&1; then
        _filedir "${1-}"
    else
        if [[ -n "${1-}" ]]; then
            local -a _f _d
            mapfile -t _f < <(compgen -f -X "!*.$1" -- "$cur")
            mapfile -t _d < <(compgen -d -- "$cur")
            COMPREPLY=("${_f[@]}" "${_d[@]}")
        else
            mapfile -t COMPREPLY < <(compgen -f -- "$cur")
        fi
    fi
}

_mpqcli() {
    local cur prev words cword

    # Use bash-completion helpers when available, fall back to COMP_* variables.
    if declare -f _init_completion > /dev/null 2>&1; then
        _init_completion || return
    else
        COMPREPLY=()
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        words=("${COMP_WORDS[@]}")
        cword=$COMP_CWORD
    fi

    local subcommands="version about info create add remove list extract read verify completion"
    local -a locales=(
        default enUS zhTW csCZ deDE esES frFR itIT
        jaJP koKR nlNL plPL ptBR ruRU zhCN enGB esMX ptPT
    )
    local -a games=(
        generic diablo1 lordsofmagic starcraft1 warcraft2 diablo2
        warcraft3 warcraft3-map wow-vanilla wow-tbc wow-wotlk
        wow-cataclysm wow-mop starcraft2 diablo3
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
        mapfile -t COMPREPLY < <(compgen -W "$subcommands" -- "$cur")
        return
    fi

    local subcmd="${words[1]}"

    case "$subcmd" in
        info)
            case "$prev" in
                -p|--property)
                    mapfile -t COMPREPLY < <(compgen -W "${info_properties[*]}" -- "$cur")
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "-p --property" -- "$cur")
            else
                _mpqcli_filedir mpq
            fi
            ;;

        create)
            case "$prev" in
                --locale)
                    mapfile -t COMPREPLY < <(compgen -W "${locales[*]}" -- "$cur")
                    return ;;
                -g|--game)
                    mapfile -t COMPREPLY < <(compgen -W "${games[*]}" -- "$cur")
                    return ;;
                --version)
                    mapfile -t COMPREPLY < <(compgen -W "1 2 3 4" -- "$cur")
                    return ;;
                -p|--path|-o|--output|--stream-flags|--sector-size|\
                --raw-chunk-size|--file-flags1|--file-flags2|--file-flags3|--attr-flags|\
                --flags|--compression|--compression-next)
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "-p --path -o --output -s --sign \
--locale -g --game --version --stream-flags --sector-size --raw-chunk-size \
--file-flags1 --file-flags2 --file-flags3 --attr-flags \
--flags --compression --compression-next" -- "$cur")
            else
                _mpqcli_filedir
            fi
            ;;

        add)
            case "$prev" in
                --locale)
                    mapfile -t COMPREPLY < <(compgen -W "${locales[*]}" -- "$cur")
                    return ;;
                -g|--game)
                    mapfile -t COMPREPLY < <(compgen -W "${games[*]}" -- "$cur")
                    return ;;
                -p|--path|--flags|--compression|--compression-next)
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "-p --path -w --overwrite -u --update \
--locale -g --game \
--flags --compression --compression-next" -- "$cur")
            else
                _mpqcli_filedir
            fi
            ;;

        remove)
            case "$prev" in
                --locale)
                    mapfile -t COMPREPLY < <(compgen -W "${locales[*]}" -- "$cur")
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "--locale" -- "$cur")
            else
                _mpqcli_filedir
            fi
            ;;

        list)
            case "$prev" in
                -l|--listfile)
                    _mpqcli_filedir
                    return ;;
                -p|--property)
                    mapfile -t COMPREPLY < <(compgen -W "${list_properties[*]}" -- "$cur")
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "-l --listfile -d --detailed -a --all -p --property" -- "$cur")
            else
                _mpqcli_filedir mpq
            fi
            ;;

        extract)
            case "$prev" in
                -o|--output)
                    _mpqcli_filedir
                    return ;;
                -l|--listfile)
                    _mpqcli_filedir
                    return ;;
                --locale)
                    mapfile -t COMPREPLY < <(compgen -W "${locales[*]}" -- "$cur")
                    return ;;
                -f|--file)
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "-o --output -f --file -k --keep -l --listfile --locale" -- "$cur")
            else
                _mpqcli_filedir mpq
            fi
            ;;

        read)
            case "$prev" in
                --locale)
                    mapfile -t COMPREPLY < <(compgen -W "${locales[*]}" -- "$cur")
                    return ;;
            esac
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "--locale" -- "$cur")
            elif [[ $cword -ge 3 ]]; then
                # positional 1 is an in-archive path (no filesystem completion);
                # positional 2+ is the archive file
                _mpqcli_filedir mpq
            fi
            ;;

        verify)
            if [[ "$cur" == -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "-p --print" -- "$cur")
            else
                _mpqcli_filedir mpq
            fi
            ;;

        completion)
            if [[ "$cur" != -* ]]; then
                mapfile -t COMPREPLY < <(compgen -W "bash zsh powershell fish" -- "$cur")
            fi
            ;;

        version|about) ;;
    esac
}

complete -F _mpqcli mpqcli
