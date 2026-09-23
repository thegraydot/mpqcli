#include <iostream>
#include <mpqcli/completion_data.h>

#include "commands.h"

void HandleCompletionBash() {
    std::cout << mpqcli::bash_completion_script;
}

void HandleCompletionZsh() {
    std::cout << mpqcli::zsh_completion_script;
}

void HandleCompletionPs() {
    std::cout << mpqcli::ps_completion_script;
}

void HandleCompletionFish() {
    std::cout << mpqcli::fish_completion_script;
}
