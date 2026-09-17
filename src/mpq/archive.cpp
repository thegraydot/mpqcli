#include "mpq/archive.h"

#include <cstdint>
#include <iostream>
#include <string>

#include <StormLib.h>

#include "util/format.h"

namespace mpqcli {

bool OpenMpqArchive(const std::string &filename, HANDLE *archive, int32_t flags) {
    if (!SFileOpenArchive(filename.c_str(), 0, flags, archive)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to open MPQ archive: " << filename << ": (" << error << ") "
                  << StormErrorString(error) << std::endl;
        return false;
    }
    return true;
}
bool CloseMpqArchive(HANDLE archive) {
    if (!SFileCloseArchive(archive)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to close MPQ archive: (" << error << ") "
                  << StormErrorString(error) << std::endl;
        return false;
    }
    return true;
}
bool SignMpqArchive(HANDLE archive) {
    if (!SFileSignArchive(archive, SIGNATURE_TYPE_WEAK)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to sign MPQ archive: (" << error << ") " << StormErrorString(error)
                  << std::endl;
        return false;
    }
    return true;
}

} // namespace mpqcli
