#include "commands/list.h"

#include <ostream>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/list.h"

namespace mpqcli {

void List(const ListOptions &options, std::ostream &out, std::ostream &err) {
    Archive archive = Archive::Open(options.target, MPQ_OPEN_READ_ONLY);
    ListFiles(archive.Handle(), options.listfile, options.all, options.detailed, options.properties,
              out, err);
    archive.Close();
}

} // namespace mpqcli
