#ifndef RAW_ARCHIVE_H
#define RAW_ARCHIVE_H

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <StormLib.h>

namespace mpqcli::testing {

/// Writes an archive holding each name under its locale, through StormLib directly
///
/// mpqcli stores every path with backslashes, but StormLib stores a name exactly as given,
/// so this is how a test gets an archive another tool wrote with forward slashes. Each file
/// holds its own name as its content, so a read shows which entry it found. Throws
/// std::runtime_error when StormLib refuses, since that is a broken fixture.
inline void WriteRawArchive(const std::filesystem::path &path,
                            const std::vector<std::pair<std::string, LCID>> &entries) {
    HANDLE archive = nullptr;
    if (!SFileCreateArchive(path.string().c_str(), MPQ_CREATE_LISTFILE | MPQ_CREATE_ARCHIVE_V1, 16,
                            &archive)) {
        throw std::runtime_error("WriteRawArchive: cannot create " + path.string());
    }
    for (const auto &[name, locale] : entries) {
        const auto size = static_cast<DWORD>(name.size());
        HANDLE file = nullptr;
        if (!SFileCreateFile(archive, name.c_str(), 0, size, locale, 0, &file) ||
            !SFileWriteFile(file, name.data(), size, 0) || !SFileFinishFile(file)) {
            SFileCloseArchive(archive);
            throw std::runtime_error("WriteRawArchive: cannot add " + name);
        }
    }
    if (!SFileCloseArchive(archive)) {
        throw std::runtime_error("WriteRawArchive: cannot close " + path.string());
    }
}

} // namespace mpqcli::testing

#endif // RAW_ARCHIVE_H
