#ifndef UTIL_FORMAT_H
#define UTIL_FORMAT_H

#include <cstdint>
#include <ostream>
#include <string>

namespace mpqcli {

std::string FileTimeToLsTime(int64_t file_time);
std::string StormErrorString(uint32_t err);

/// Writes size bytes of buffer to out without line-ending translation
void WriteBinary(std::ostream &out, const char *buffer, uint32_t size);

} // namespace mpqcli

#endif
