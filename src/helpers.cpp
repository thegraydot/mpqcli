#include "helpers.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
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

// ---- CRC32 --------------------------------------------------------------
// Standard ZIP/gzip CRC32 (polynomial 0xEDB88320, reflected bit order).

static const std::array<uint32_t, 256> kCrc32Table = []() {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j)
            c = (c >> 1) ^ (c & 1u ? 0xEDB88320u : 0u);
        t[i] = c;
    }
    return t;
}();

std::optional<uint32_t> ComputeFileCrc32(const fs::path &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return std::nullopt;
    }
    uint32_t crc = 0xFFFFFFFFu;
    char buf[65536];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        for (std::streamsize i = 0; i < f.gcount(); ++i)
            crc = (crc >> 8) ^ kCrc32Table[(crc ^ static_cast<uint8_t>(buf[i])) & 0xFFu];
    }
    return crc ^ 0xFFFFFFFFu;
}

// ---- MD5 ----------------------------------------------------------------
// Compact RFC 1321 implementation – no external dependencies.

namespace {

constexpr uint32_t kMd5K[64] = {
    // Round 1
    0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
    0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
    0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
    0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
    // Round 2
    0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
    0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
    0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
    0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
    // Round 3
    0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
    0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
    0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
    0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
    // Round 4
    0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
    0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
    0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
    0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u,
};

constexpr uint8_t kMd5S[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

struct Md5State {
    uint32_t a{0x67452301u};
    uint32_t b{0xEFCDAB89u};
    uint32_t c{0x98BADCFEu};
    uint32_t d{0x10325476u};
    uint64_t count{0};
    uint8_t  buf[64]{};
};

static uint32_t RotLeft32(uint32_t x, uint8_t n) { return (x << n) | (x >> (32u - n)); }

static void Md5Block(Md5State &s, const uint8_t *blk) {
    uint32_t w[16];
    for (int i = 0; i < 16; ++i) {
        w[i] =  static_cast<uint32_t>(blk[4 * i])
             | (static_cast<uint32_t>(blk[4 * i + 1]) << 8u)
             | (static_cast<uint32_t>(blk[4 * i + 2]) << 16u)
             | (static_cast<uint32_t>(blk[4 * i + 3]) << 24u);
    }
    uint32_t a = s.a, b = s.b, c = s.c, d = s.d;
    for (int i = 0; i < 64; ++i) {
        uint32_t f = 0;
        uint32_t g = 0;
        if (i < 16) {
            f = (b & c) | (~b & d);
            g = static_cast<uint32_t>(i);
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = static_cast<uint32_t>(5 * i + 1) % 16u;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = static_cast<uint32_t>(3 * i + 5) % 16u;
        } else {
            f = c ^ (b | ~d);
            g = static_cast<uint32_t>(7 * i) % 16u;
        }
        f += a + kMd5K[i] + w[g];
        a = d;
        d = c;
        c = b;
        b += RotLeft32(f, kMd5S[i]);
    }
    s.a += a;
    s.b += b;
    s.c += c;
    s.d += d;
}

static void Md5Update(Md5State &s, const uint8_t *data, size_t len) {
    uint32_t offset = s.count & 63u;
    s.count += len;
    while (len > 0) {
        size_t chunk = std::min(len, static_cast<size_t>(64u - offset));
        std::memcpy(s.buf + offset, data, chunk);
        data += chunk;
        len -= chunk;
        offset += static_cast<uint32_t>(chunk);
        if (offset == 64u) {
            Md5Block(s, s.buf);
            offset = 0;
        }
    }
}

static void Md5Final(Md5State &s, uint8_t *digest) {
    const uint64_t bits = s.count * 8u;
    uint8_t pad = 0x80u;
    Md5Update(s, &pad, 1);
    pad = 0;
    while ((s.count & 63u) != 56u) Md5Update(s, &pad, 1);
    uint8_t bits_le[8];
    for (int i = 0; i < 8; ++i)
        bits_le[i] = static_cast<uint8_t>(bits >> (8u * static_cast<unsigned>(i)));
    Md5Update(s, bits_le, 8);
    const uint32_t parts[4] = {s.a, s.b, s.c, s.d};
    for (int i = 0; i < 4; ++i) {
        digest[4 * i]     = static_cast<uint8_t>(parts[i]);
        digest[4 * i + 1] = static_cast<uint8_t>(parts[i] >> 8u);
        digest[4 * i + 2] = static_cast<uint8_t>(parts[i] >> 16u);
        digest[4 * i + 3] = static_cast<uint8_t>(parts[i] >> 24u);
    }
}

}  // namespace

bool ComputeFileMd5(const fs::path &path, uint8_t *md5_out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    Md5State state;
    uint8_t buf[65536];
    while (f.read(reinterpret_cast<char *>(buf), sizeof(buf)) || f.gcount() > 0)
        Md5Update(state, buf, static_cast<size_t>(f.gcount()));
    Md5Final(state, md5_out);
    return true;
}

// ---- Timestamp ----------------------------------------------------------
// Returns the file's last-modification time as a Windows FILETIME value
// (100-nanosecond intervals since 1601-01-01 UTC).  Returns 0 on error.

uint64_t LocalFileTimestamp(const fs::path &path) {
    struct stat st {};
    if (stat(path.string().c_str(), &st) != 0) {
        return 0;
    }
    constexpr int64_t kEpochDiff = 11644473600LL;
    return static_cast<uint64_t>((static_cast<int64_t>(st.st_mtime) + kEpochDiff) * 10000000LL);
}
