#ifndef MPQ_EXTRACT_H
#define MPQ_EXTRACT_H

#include <atomic>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

namespace mpqcli {

/// Extracts every file the archive can name into output, returning non-zero if any failed
///
/// @throws ArchiveError if the archive's files cannot be enumerated
int ExtractFiles(HANDLE archive, const std::filesystem::path &output,
                 const std::optional<std::filesystem::path> &listfile_name, LCID preferred_locale,
                 std::ostream &err, const std::atomic<bool> &cancelled);
int ExtractFile(HANDLE archive, const std::filesystem::path &output, const std::string &file_name,
                bool keep_folder_structure, LCID preferred_locale, std::ostream &err);

} // namespace mpqcli

#endif
