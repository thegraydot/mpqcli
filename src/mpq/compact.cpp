#include "mpq/compact.h"

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

#include "errors.h"

namespace mpqcli {

void CompactMpqArchive(HANDLE archive, const std::optional<std::filesystem::path> &listfile_name,
                       std::ostream &err) {
    err << "[*] Compacting archive. This may take some time..." << std::endl;
    const std::string listfile_string =
        listfile_name.has_value() ? listfile_name->string() : std::string();
    const char *listfile = listfile_name.has_value() ? listfile_string.c_str() : nullptr;

    if (!SFileCompactArchive(archive, listfile, false)) {
        throw StormError("Failed to compact archive", SErrGetLastError());
    }
}

} // namespace mpqcli
