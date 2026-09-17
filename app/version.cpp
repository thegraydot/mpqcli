#include <iostream>
#include <mpqcli/version.h>

#include "subcommands.h"

int HandleVersion() {
    std::cout << MPQCLI_VERSION << "-" << GIT_COMMIT_HASH << std::endl;
    return 0;
}
