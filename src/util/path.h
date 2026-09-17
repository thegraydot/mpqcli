#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

std::string NormalizeFilePath(const fs::path &path);
std::string WindowsifyFilePath(const fs::path &path);

/// Reports whether path resolves to a location inside base
bool IsWithinDirectory(const fs::path &base, const fs::path &path);

/// Builds the in-archive name for a local file
std::string ResolveArchiveName(const std::string &f, const std::optional<std::string> &path,
                               bool treat_as_directory = false);

std::vector<fs::path> ListFilesRecursive(const fs::path &directory, std::error_code &ec);

/// Returns the file's last-modification time as a Windows FILETIME value
/// (100-nanosecond intervals since 1601-01-01 UTC), or 0 if it cannot be read
uint64_t LocalFileTimestamp(const fs::path &path);

#endif
