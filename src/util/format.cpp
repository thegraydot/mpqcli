#include "util/format.h"

#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <ostream>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <StormLib.h>

// After StormLib.h, which includes it with NOMINMAX defined first
#ifdef _WIN32
#include <windows.h>
#endif

namespace mpqcli {

namespace {

/// Describes an error code StormLib passed through from the operating system
///
/// StormLib reports the platform's own codes: errno values on POSIX, which strerror
/// describes, and Win32 codes on Windows, which strerror would mistake for errno values.
std::string SystemErrorString(uint32_t err) {
#ifdef _WIN32
    char *buffer = nullptr;
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    if (length == 0 || buffer == nullptr) {
        return "Unknown error " + std::to_string(err);
    }
    std::string message(buffer, length);
    LocalFree(buffer);
    // The system text ends in a full stop and CRLF, which would break the line it is
    // printed into
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r' ||
                                message.back() == ' ' || message.back() == '.')) {
        message.pop_back();
    }
    return message;
#else
    return std::strerror(static_cast<int>(err));
#endif
}

} // namespace

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
        default:                           return "(" + std::to_string(err) + ") " + SystemErrorString(err);
        // clang-format on
    }
}

void WriteBinary(std::ostream &out, const char *buffer, uint32_t size) {
#ifdef _WIN32
    // The mode belongs to the file descriptor rather than the stream, so switching
    // it for a captured stream would change the real stdout for the rest of the run
    if (&out == &std::cout) {
        _setmode(_fileno(stdout), _O_BINARY);
    }
#endif
    out.write(buffer, size);
}

} // namespace mpqcli
