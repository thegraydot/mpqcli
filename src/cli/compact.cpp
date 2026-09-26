#include "mpq/compact.h"

#include <optional>
#include <ostream>
#include <string>

#include "cli/commands.h"
#include "mpq/archive.h"

namespace mpqcli {

int HandleCompact(const std::string &target, const std::optional<std::string> &listfile_name,
                  std::ostream &out) {
    Archive archive = Archive::Open(target, 0);
    CompactMpqArchive(archive.Handle(), listfile_name, out);
    archive.Close();
    return 0;
}

} // namespace mpqcli
