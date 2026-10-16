#ifndef WRITE_FILE_H
#define WRITE_FILE_H

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>

namespace mpqcli::testing {

/// Writes content to path byte for byte, creating parent directories as needed
///
/// Binary mode on every platform, so a "\n" in the content stays one byte on Windows
/// and the sizes a test expects hold everywhere.
inline void WriteFile(const std::filesystem::path &path, const std::string &content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

} // namespace mpqcli::testing

#endif // WRITE_FILE_H
