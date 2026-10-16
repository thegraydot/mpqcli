#ifndef READ_FILE_H
#define READ_FILE_H

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <stdexcept>
#include <string>

namespace mpqcli::testing {

/// Reads a file byte for byte; throws when it cannot be opened
///
/// Binary mode on every platform, so what came back is what is on disk.
inline std::string ReadFile(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("ReadFile: cannot open " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace mpqcli::testing

#endif // READ_FILE_H
