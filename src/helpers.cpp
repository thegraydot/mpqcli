#include "helpers.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>

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
    localtime_s(&tm_buf, &unixTime);
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
    return filePath;
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
