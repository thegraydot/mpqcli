#include "util/string.h"

#include <algorithm>
#include <cctype>
#include <string>

std::string ToLower(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}
