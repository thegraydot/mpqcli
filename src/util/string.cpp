#include "util/string.h"

#include <algorithm>
#include <string>

namespace mpqcli {

std::string ToLower(const std::string &str) {
    std::string result = str;
    // Mapped by hand rather than with std::tolower, which follows the C locale and in a
    // single-byte code page would lowercase letters outside ASCII as well
    std::transform(result.begin(), result.end(), result.begin(), [](char c) {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    });
    return result;
}

} // namespace mpqcli
