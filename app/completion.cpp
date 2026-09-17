#include <iostream>

#include "completion_data.h"
#include "subcommands.h"

void HandleCompletionBash() {
    std::cout << BashCompletionScript;
}

void HandleCompletionZsh() {
    std::cout << ZshCompletionScript;
}

void HandleCompletionPs() {
    std::cout << PsCompletionScript;
}

void HandleCompletionFish() {
    std::cout << FishCompletionScript;
}
