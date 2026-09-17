#include "mpq/compact.h"

#include <iostream>
#include <optional>
#include <string>

#include <StormLib.h>

#include "util/format.h"

int CompactMpqArchive(HANDLE archive, const std::optional<std::string> &listfile_name) {
    std::cout << "[*] Compacting archive. This may take some time..." << std::endl;
    // Check if the user provided a listfile input
    const char *listfile = listfile_name.has_value() ? listfile_name->c_str() : nullptr;

    if (!SFileCompactArchive(archive, listfile, false)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to compact archive: (" << error << ") " << StormErrorString(error)
                  << std::endl;
        return 1;
    }
    return 0;
}
