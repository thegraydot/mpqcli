#include "commands/info.h"

#include <ostream>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/info.h"

namespace mpqcli {

void Info(const InfoOptions &options, std::ostream &out) {
    Archive archive = Archive::Open(options.target, MPQ_OPEN_READ_ONLY);
    PrintMpqInfo(archive.Handle(), options.property, out);
    archive.Close();
}

} // namespace mpqcli
