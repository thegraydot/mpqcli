#include <iostream>
#include <mpqcli/version.h>

#include "commands.h"

int HandleVersion() {
    std::cout << mpqcli::version_string << "-" << mpqcli::git_commit_hash << std::endl;
    return 0;
}
