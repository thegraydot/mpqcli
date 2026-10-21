#ifndef UTIL_FORMAT_H
#define UTIL_FORMAT_H

#include <cstdint>
#include <ostream>
#include <string>

namespace mpqcli {

std::string FileTimeToLsTime(int64_t file_time);
/// Describes a StormLib error code in words that are the same on every platform
///
/// The code's value differs by platform, so it is left out. A code with no words of its
/// own is described in the platform's words instead, behind the code that names it.
std::string StormErrorString(uint32_t err);

/// Writes size bytes of buffer to out without line-ending translation
void WriteBinary(std::ostream &out, const char *buffer, uint32_t size);

} // namespace mpqcli

#endif
