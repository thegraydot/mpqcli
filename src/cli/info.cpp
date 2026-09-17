#include "mpq/info.h"

#include <optional>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"

int HandleInfo(const std::string &target, const std::optional<std::string> &property) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }
    PrintMpqInfo(archive, property);
    CloseMpqArchive(archive);
    return 0;
}
