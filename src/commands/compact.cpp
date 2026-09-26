#include "commands/compact.h"

#include <ostream>

#include "mpq/archive.h"
#include "mpq/compact.h"

namespace mpqcli {

bool Compact(const CompactOptions &options, std::ostream &err) {
    Archive archive = Archive::Open(options.target, 0);
    CompactMpqArchive(archive.Handle(), options.listfile, err);
    archive.Close();
    return true;
}

} // namespace mpqcli
