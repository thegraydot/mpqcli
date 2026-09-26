#include "mpq/compact.h"

#include <optional>
#include <string>

#include "cli/commands.h"
#include "mpq/archive.h"

namespace mpqcli {

int HandleCompact(const std::string &target, const std::optional<std::string> &listfile_name) {
    Archive archive = Archive::Open(target, 0);
    CompactMpqArchive(archive.Handle(), listfile_name);
    archive.Close();
    return 0;
}

} // namespace mpqcli
