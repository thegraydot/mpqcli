#include "mpq/compact.h"

#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

#include "errors.h"

namespace mpqcli {

void CompactMpqArchive(HANDLE archive, const std::optional<std::string> &listfile_name,
                       std::ostream &out) {
    out << "[*] Compacting archive. This may take some time..." << std::endl;
    const char *listfile = listfile_name.has_value() ? listfile_name->c_str() : nullptr;

    if (!SFileCompactArchive(archive, listfile, false)) {
        throw StormError("Failed to compact archive", SErrGetLastError());
    }
}

} // namespace mpqcli
