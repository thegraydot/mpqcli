#include "mpq/query.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

#include <StormLib.h>

#include "util/hash.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

bool FileExistsInArchiveForLocale(const HANDLE archive, const std::string &file_path,
                                  const LCID locale) {
    bool file_exists = false;
    SFileSetLocale(locale);
    HANDLE file;
    if (SFileOpenFileEx(archive, file_path.c_str(), SFILE_OPEN_FROM_MPQ, &file)) {
        const auto file_locale = GetFileInfo<LCID>(file, SFileInfoLocale);
        if (file_locale == locale) {
            file_exists = true;
        }
        SFileCloseFile(file);
    }
    return file_exists;
}

bool ArchivedFileMatches(HANDLE archive, HANDLE file, const fs::path &local_file,
                         std::string &match_reason) {
    const DWORD archived_size = SFileGetFileSize(file, nullptr);
    std::error_code ec;
    const uintmax_t disk_size = fs::file_size(local_file, ec);
    if (ec || disk_size != static_cast<uintmax_t>(archived_size)) {
        return false;
    }

    const DWORD attr_flags = SFileGetAttributes(archive);

    // Timestamp first: the cheapest check, with no local file I/O
    if (attr_flags & MPQ_ATTRIBUTE_FILETIME) {
        const uint64_t archived_time = GetFileInfo<uint64_t>(file, SFileInfoFileTime);
        const uint64_t local_time = LocalFileTimestamp(local_file);
        // Compare at second resolution: stat() has only second precision
        if (archived_time != 0 && local_time != 0 &&
            archived_time / 10000000u == local_time / 10000000u) {
            match_reason = "Timestamp matches";
            return true;
        }
    }

    // MD5 when the timestamp did not match or was unavailable
    if (attr_flags & MPQ_ATTRIBUTE_MD5) {
        uint8_t archived_md5[MD5_DIGEST_SIZE]{};
        if (SFileGetFileInfo(file, SFileInfoMD5, archived_md5, sizeof(archived_md5), nullptr)) {
            // An all-zero digest means "no MD5 stored". A file whose
            // real MD5 is all zeroes is astronomically unlikely; the
            // worst case is a redundant re-add.
            const uint8_t zero_md5[MD5_DIGEST_SIZE]{};
            if (std::memcmp(archived_md5, zero_md5, MD5_DIGEST_SIZE) != 0) {
                uint8_t local_md5[MD5_DIGEST_SIZE]{};
                if (ComputeFileMd5(local_file, local_md5) &&
                    std::memcmp(local_md5, archived_md5, MD5_DIGEST_SIZE) == 0) {
                    match_reason = "MD5 matches";
                    return true;
                }
            }
        }
    }

    // CRC32 when neither timestamp nor MD5 matched or was available
    if (attr_flags & MPQ_ATTRIBUTE_CRC32) {
        const DWORD archived_crc32 = GetFileInfo<DWORD>(file, SFileInfoCRC32);
        // Zero means "no CRC32 stored"; a file whose real CRC32 is
        // zero just gets a redundant re-add.
        if (archived_crc32 != 0) {
            if (auto local_crc32 = ComputeFileCrc32(local_file)) {
                if (*local_crc32 == archived_crc32) {
                    match_reason = "CRC32 matches";
                    return true;
                }
            }
        }
    }

    // No attributes present, or none matched: re-add the file
    return false;
}

} // namespace mpqcli
