#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <cstdint>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

// CRC32 (ZIP/gzip polynomial) and MD5 match the values StormLib stores in
// (attributes), which is what lets the --update check compare them directly.

/// Returns the CRC32 of path, or nullopt if it cannot be read
std::optional<uint32_t> ComputeFileCrc32(const fs::path &path);

/// Writes the MD5 of path to md5_out, returning false if it cannot be read
bool ComputeFileMd5(const fs::path &path, uint8_t *md5_out);

#endif
