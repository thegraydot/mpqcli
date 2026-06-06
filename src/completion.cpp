#include "completion.h"

#include <iostream>

#include "completion_data.h"

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
