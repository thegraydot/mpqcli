#include "mpq/list.h"

#include <optional>
#include <string>
#include <vector>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"

namespace mpqcli {

int HandleList(const std::string &target, const std::optional<std::string> &listfile_name,
               bool list_all, bool list_detailed, const std::vector<std::string> &properties) {
    Archive archive = Archive::Open(target, MPQ_OPEN_READ_ONLY);
    ListFiles(archive.Handle(), listfile_name, list_all, list_detailed, properties);
    archive.Close();
    return 0;
}

} // namespace mpqcli
