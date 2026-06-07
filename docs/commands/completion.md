# completion

Print a shell completion script to stdout.

## Supported Shells

- bash
- zsh
- fish
- PowerShell

## Bash

Write the completion script to a file and source it from your shell profile.

```bash
$ mpqcli completion bash > ~/.bash_completion.d/mpqcli
$ source ~/.bash_completion.d/mpqcli
```

Alternatively, write the script to a system-wide completions directory (requires root):

```bash
$ mpqcli completion bash > /etc/bash_completion.d/mpqcli
```

## Zsh

Write the completion script to a directory that is on your `$fpath`.

```zsh
$ mpqcli completion zsh > "${fpath[1]}/_mpqcli"
```

## PowerShell

Append the completion script to your PowerShell profile so it loads automatically.

```powershell
PS> mpqcli completion powershell >> $PROFILE
```

## Fish

Write the completion script to the fish completions directory.

```fish
$ mpqcli completion fish > ~/.config/fish/completions/mpqcli.fish
```
