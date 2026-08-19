#ifndef HELPERS_H
#define HELPERS_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace fs = std::filesystem;

std::string FileTimeToLsTime(int64_t file_time);
std::string NormalizeFilePath(const fs::path &path);
std::string WindowsifyFilePath(const fs::path &path);
std::string StormErrorString(uint32_t err);
uint32_t CalculateMpqMaxFileValue(const std::string &path);
uint32_t NextPowerOfTwo(uint32_t n);
void PrintAsBinary(const char *buffer, uint32_t size);

// Local file checksum / timestamp helpers used by the --update logic.
// Each returns std::nullopt / false / 0 if the file cannot be read.
std::optional<uint32_t> ComputeFileCrc32(const fs::path &path);
bool ComputeFileMd5(const fs::path &path, uint8_t *md5_out);
uint64_t LocalFileTimestamp(const fs::path &path);

#endif
