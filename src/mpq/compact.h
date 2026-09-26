#ifndef MPQ_COMPACT_H
#define MPQ_COMPACT_H

#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

namespace mpqcli {

/// Compacts the archive, resolving names it cannot from listfile_name when given
///
/// @throws StormError if StormLib cannot compact it
void CompactMpqArchive(HANDLE archive, const std::optional<std::string> &listfile_name,
                       std::ostream &err);

} // namespace mpqcli

#endif
