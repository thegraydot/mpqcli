#include <iostream>
#include <mpqcli/version.h>

#include "commands.h"

int HandleAbout() {
    std::cout << "Name: mpqcli" << std::endl;
    std::cout << "Version: " << mpqcli::version_string << "-" << mpqcli::git_commit_hash
              << std::endl;
    std::cout << "Author: Thomas Laurenson" << std::endl;
    std::cout << "License: MIT" << std::endl;
    std::cout << "GitHub: https://github.com/thegraydot/mpqcli" << std::endl;
    std::cout << "Dependencies:" << std::endl;
    std::cout << " - StormLib (https://github.com/ladislav-zezula/StormLib)" << std::endl;
    std::cout << " - CLI11 (https://github.com/CLIUtils/CLI11)" << std::endl;
    std::cout << " - hash-library (https://github.com/stbrumme/hash-library)" << std::endl;
    return 0;
}
