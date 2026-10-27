#include "commands/compact.h"

#include <ostream>

#include "mpq/archive.h"
#include "mpq/compact.h"
#include "mpq/query.h"

namespace mpqcli {

void Compact(const CompactOptions &options, std::ostream &err) {
    Archive archive = Archive::Open(options.archive, 0);
    CompactMpqArchive(archive.Handle(), UsableListfile(options.listfile, err), err);
    archive.Close();
}

} // namespace mpqcli
