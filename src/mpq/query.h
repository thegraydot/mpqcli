#ifndef MPQ_QUERY_H
#define MPQ_QUERY_H

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <StormLib.h>

namespace mpqcli {

/// Files the archive maintains for itself rather than on a user's behalf
inline const std::vector<std::string> special_mpq_files = {"(listfile)", "(signature)",
                                                           "(attributes)"};

bool FileExistsInArchiveForLocale(HANDLE archive, const std::string &file_path, LCID locale);

/// Reports whether the archived copy of local_file is byte-for-byte current
bool ArchivedFileMatches(HANDLE archive, HANDLE file, const std::filesystem::path &local_file,
                         std::string &match_reason);

/// Returns the listfile to hand StormLib, leaving out one that is empty
///
/// StormLib rejects an empty listfile as corrupt, yet it names nothing and so changes no
/// result. An empty one is reported on err and dropped, so the command goes on as if no
/// listfile had been given.
std::optional<std::filesystem::path>
UsableListfile(const std::optional<std::filesystem::path> &listfile, std::ostream &err);

template <typename T> T GetFileInfo(HANDLE file, SFileInfoClass info_class) {
    T value{};
    if (!SFileGetFileInfo(file, info_class, &value, sizeof(T), nullptr)) {
        return T{};
    }
    return value;
}

} // namespace mpqcli

#endif
