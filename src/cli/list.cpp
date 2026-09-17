#include "mpq/list.h"

#include <optional>
#include <string>
#include <vector>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"

int HandleList(const std::string &target, const std::optional<std::string> &listfile_name,
               bool list_all, bool list_detailed, const std::vector<std::string> &properties) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }
    ListFiles(archive, listfile_name, list_all, list_detailed, properties);
    CloseMpqArchive(archive);
    return 0;
}
