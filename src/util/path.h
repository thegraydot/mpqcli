#ifndef UTIL_PATH_H
#define UTIL_PATH_H

#include <cstdint>
#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace mpqcli {

std::string NormalizeFilePath(const std::filesystem::path &path);
std::string WindowsifyFilePath(const std::filesystem::path &path);

/// Reports whether path lies lexically under base, without resolving either
bool IsWithinDirectory(const std::filesystem::path &base, const std::filesystem::path &path);

/// Builds the in-archive name for a local file
std::string ResolveArchiveName(const std::filesystem::path &f,
                               const std::optional<std::string> &path,
                               bool treat_as_directory = false);

std::vector<std::filesystem::path> ListFilesRecursive(const std::filesystem::path &directory,
                                                      std::error_code &ec);

/// Replaces each "-" in paths with the non-empty lines read from in
std::vector<std::string> ExpandStdinMarker(const std::vector<std::string> &paths, std::istream &in);

/// Returns the file's last-modification time as a Windows FILETIME value
/// (100-nanosecond intervals since 1601-01-01 UTC), or 0 if it cannot be read
uint64_t LocalFileTimestamp(const std::filesystem::path &path);

} // namespace mpqcli

#endif
