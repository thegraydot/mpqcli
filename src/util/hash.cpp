#include "util/hash.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <hash-library/crc32.h>
#include <hash-library/md5.h>
#include <optional>

namespace fs = std::filesystem;

namespace mpqcli {

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

} // namespace mpqcli
