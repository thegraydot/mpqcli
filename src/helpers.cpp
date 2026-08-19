#include "helpers.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <hash-library/crc32.h>
#include <hash-library/md5.h>
#include <iostream>
#include <sys/stat.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <StormLib.h>

namespace fs = std::filesystem;

std::string FileTimeToLsTime(int64_t file_time) {
    if (file_time == 0) {
        return "";
    }
    constexpr int64_t epoch_diff = 11644473600LL;
    int64_t unix_time = (file_time / 10000000) - epoch_diff;
    char buf[20];
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &unix_time);
#else
    localtime_r(&unix_time, &tm_buf);
#endif
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

std::string NormalizeFilePath(const fs::path &path) {
    std::string file_path = path.u8string();
#ifndef _WIN32
    std::replace(file_path.begin(), file_path.end(), '\\', '/');
    return file_path;
#else
    return file_path;
#endif
}

std::string WindowsifyFilePath(const fs::path &path) {
    std::string file_path = path.u8string();
    std::replace(file_path.begin(), file_path.end(), '/', '\\');
    return file_path;
}

std::string StormErrorString(uint32_t err) {
    switch (err) {
        // clang-format off
        case ERROR_SUCCESS:                return "Success";
        case ERROR_FILE_NOT_FOUND:         return "File not found";
        case ERROR_ACCESS_DENIED:          return "Access denied (archive may be read-only or have open files)";
        case ERROR_INVALID_HANDLE:         return "Invalid handle";
        case ERROR_NOT_ENOUGH_MEMORY:      return "Not enough memory";
        case ERROR_NOT_SUPPORTED:          return "Operation not supported";
        case ERROR_INVALID_PARAMETER:      return "Invalid parameter";
        case ERROR_DISK_FULL:              return "Disk full";
        case ERROR_ALREADY_EXISTS:         return "Already exists";
        case ERROR_INSUFFICIENT_BUFFER:    return "Insufficient buffer";
        case ERROR_BAD_FORMAT:             return "Bad MPQ format";
        case ERROR_NO_MORE_FILES:          return "No more files";
        case ERROR_HANDLE_EOF:             return "End of file";
        case ERROR_CAN_NOT_COMPLETE:       return "Cannot complete";
        case ERROR_FILE_CORRUPT:           return "File is corrupt";
        case ERROR_BUFFER_OVERFLOW:        return "Buffer overflow";
        case ERROR_INVALID_DATA:           return "Invalid data";
        case ERROR_NO_UNICODE_TRANSLATION: return "No Unicode translation";
        case ERROR_AVI_FILE:               return "Not an MPQ file (AVI file)";
        case ERROR_UNKNOWN_FILE_KEY:       return "Unknown file encryption key";
        case ERROR_CHECKSUM_ERROR:         return "Sector checksum mismatch";
        case ERROR_INTERNAL_FILE:          return "Operation not allowed on internal file";
        case ERROR_BASE_FILE_MISSING:      return "Base file missing for incremental patch";
        case ERROR_MARKED_FOR_DELETE:      return "File is marked as deleted in the MPQ";
        case ERROR_FILE_INCOMPLETE:        return "Required file part is missing";
        case ERROR_UNKNOWN_FILE_NAMES:     return "At least one file name is unknown (listfile is incomplete)";
        case ERROR_CANT_FIND_PATCH_PREFIX: return "Cannot find patch prefix";
        case ERROR_FAKE_MPQ_HEADER:        return "Fake MPQ header at this position";
        case ERROR_FILE_DELETED:           return "File contains delete marker";
        default:                           return std::strerror(static_cast<int>(err));
        // clang-format on
    }
}

uint32_t CalculateMpqMaxFileValue(const std::string &path) {
    uint32_t file_count = 0;

    // Determine the number of files in the target directory, recusively
    if (!fs::is_regular_file(path)) {
        for (const auto &entry : fs::recursive_directory_iterator(path)) {
            if (fs::is_regular_file(entry.path())) {
                ++file_count;
            }
        }
    }

    // Always add 3 for "special" files
    file_count += 3;

    // Based on file count, determine the max number of files an MPQ archive can hold
    // We always have a minimum of 32
    // Anything over is rounded up to the closest power of 2
    // For example: 64, 128, 256
    // This is examples behavior of WoW MPQ archives (patches and installs)
    if (file_count <= 32) {
        return 32;
    }

    return NextPowerOfTwo(file_count);
}

uint32_t NextPowerOfTwo(uint32_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

void PrintAsBinary(const char *buffer, uint32_t size) {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    std::cout.write(buffer, size);
}

// CRC32 (ZIP/gzip polynomial) and MD5 are provided by the hash-library
// submodule, matching the values StormLib stores in (attributes).
std::optional<uint32_t> ComputeFileCrc32(const fs::path &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return std::nullopt;
    }
    CRC32 crc32;
    char buf[65536];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        crc32.add(buf, static_cast<size_t>(f.gcount()));
    }
    // getHash yields the checksum as big-endian bytes; reassemble the value.
    unsigned char digest[CRC32::HashBytes];
    crc32.getHash(digest);
    return (static_cast<uint32_t>(digest[0]) << 24) | (static_cast<uint32_t>(digest[1]) << 16) |
           (static_cast<uint32_t>(digest[2]) << 8) | static_cast<uint32_t>(digest[3]);
}

bool ComputeFileMd5(const fs::path &path, uint8_t *md5_out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    MD5 md5;
    char buf[65536];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        md5.add(buf, static_cast<size_t>(f.gcount()));
    }
    md5.getHash(md5_out);
    return true;
}

// Returns the file's last-modification time as a Windows FILETIME value
// (100-nanosecond intervals since 1601-01-01 UTC).  Returns 0 on error.
uint64_t LocalFileTimestamp(const fs::path &path) {
#ifdef _WIN32
    // _wstat64 handles paths with non-ASCII characters, which the narrow
    // stat() would mangle on Windows.
    struct _stat64 st {};
    if (_wstat64(path.wstring().c_str(), &st) != 0) {
        return 0;
    }
#else
    struct stat st {};
    if (stat(path.string().c_str(), &st) != 0) {
        return 0;
    }
#endif
    constexpr int64_t epoch_diff = 11644473600LL;
    return static_cast<uint64_t>((static_cast<int64_t>(st.st_mtime) + epoch_diff) * 10000000LL);
}
