#ifndef MPQ_LIST_H
#define MPQ_LIST_H

#include <optional>
#include <string>
#include <vector>

#include <StormLib.h>

namespace mpqcli {

/// Prints the archive's files, one per line, with the requested properties
///
/// @throws ArchiveError if the archive's files cannot be enumerated
void ListFiles(HANDLE archive, const std::optional<std::string> &listfile_name, bool list_all,
               bool list_detailed, const std::vector<std::string> &properties);

} // namespace mpqcli

#endif
