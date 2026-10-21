#ifndef MPQ_LIST_H
#define MPQ_LIST_H

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <StormLib.h>

namespace mpqcli {

/// Prints the archive's files to out, one per line, with the requested properties
///
/// An archive with no entries prints nothing.
///
/// @throws StormError if StormLib cannot enumerate the archive's files
void ListFiles(HANDLE archive, const std::optional<std::filesystem::path> &listfile_name,
               bool list_all, bool list_detailed, const std::vector<std::string> &properties,
               std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
