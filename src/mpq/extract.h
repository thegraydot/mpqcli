#ifndef MPQ_EXTRACT_H
#define MPQ_EXTRACT_H

#include <atomic>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <StormLib.h>

namespace mpqcli {

/// Lists every file the archive can name, once per locale it is stored under
///
/// @throws ArchiveError if the archive's files cannot be enumerated
std::vector<std::string>
ListArchivedFiles(HANDLE archive, const std::optional<std::filesystem::path> &listfile_name,
                  const std::atomic<bool> &cancelled);

/// Returns two different files that a flat extraction would write to the same path
std::optional<std::pair<std::string, std::string>>
FindFlatCollision(const std::vector<std::string> &file_names);

/// Extracts each named file into output, returning non-zero if any failed
int ExtractFiles(HANDLE archive, const std::filesystem::path &output,
                 const std::vector<std::string> &file_names, bool flat, LCID preferred_locale,
                 std::ostream &err, const std::atomic<bool> &cancelled);
int ExtractFile(HANDLE archive, const std::filesystem::path &output, const std::string &file_name,
                bool flat, LCID preferred_locale, std::ostream &err);

} // namespace mpqcli

#endif
