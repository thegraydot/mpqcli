#include "mpq/info.h"

#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"

namespace mpqcli {

int HandleInfo(const std::string &target, const std::optional<std::string> &property,
               std::ostream &out) {
    Archive archive = Archive::Open(target, MPQ_OPEN_READ_ONLY);
    PrintMpqInfo(archive.Handle(), property, out);
    archive.Close();
    return 0;
}

} // namespace mpqcli
