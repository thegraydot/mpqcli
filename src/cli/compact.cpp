#include "mpq/compact.h"

#include <optional>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"

namespace mpqcli {

int HandleCompact(const std::string &target, const std::optional<std::string> &listfile_name) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, 0)) {
        return 1;
    }

    const int result = CompactMpqArchive(archive, listfile_name);
    CloseMpqArchive(archive);
    return result;
}

} // namespace mpqcli
