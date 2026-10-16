#include "commands/list.h"

#include <ostream>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/list.h"
#include "mpq/query.h"

namespace mpqcli {

void List(const ListOptions &options, std::ostream &out, std::ostream &err) {
    Archive archive = Archive::Open(options.archive, MPQ_OPEN_READ_ONLY);
    ListFiles(archive.Handle(), UsableListfile(options.listfile, err), options.all,
              options.detailed, options.properties, out, err);
    archive.Close();
}

} // namespace mpqcli
