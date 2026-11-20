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

/// One stored variant of a file: its name, and the locale it is stored under
///
/// The locale is as StormLib reports it, with the platform in bits 16 to 23.
struct ArchivedFile {
    std::string name;
    LCID locale = 0;
};

/// A file to extract, under the locale to ask for it in
struct FileToExtract {
    std::string name;
    LCID locale = 0;
    /// The other locales the name is stored under, which are not extracted
    std::vector<LCID> left_out;
};

/// Lists every file the archive can name, once per locale it is stored under
///
/// An archive with no entries gives an empty list.
///
/// @throws StormError if StormLib cannot enumerate the archive's files
std::vector<ArchivedFile>
ListArchivedFiles(HANDLE archive, const std::optional<std::filesystem::path> &listfile_name,
                  const std::atomic<bool> &cancelled);

/// Picks one variant of each name, in the order the names are first listed
///
/// The variant under preferred_locale, else the neutral one, else the one with the
/// lowest locale. The others are named in left_out, lowest locale first.
std::vector<FileToExtract> ChooseVariants(const std::vector<ArchivedFile> &files,
                                          LCID preferred_locale);

/// Returns two different files that a flat extraction would write to the same path
std::optional<std::pair<std::string, std::string>>
FindFlatCollision(const std::vector<std::string> &file_names);

/// Extracts each file into output under its locale, returning non-zero if any failed
///
/// A file with other variants left out is followed by a line naming them.
int ExtractFiles(HANDLE archive, const std::filesystem::path &output,
                 const std::vector<FileToExtract> &files, bool flat, std::ostream &err,
                 const std::atomic<bool> &cancelled);
int ExtractFile(HANDLE archive, const std::filesystem::path &output, const std::string &file_name,
                bool flat, LCID preferred_locale, std::ostream &err);

} // namespace mpqcli

#endif
