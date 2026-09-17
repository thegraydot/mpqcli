#ifndef UTIL_FORMAT_H
#define UTIL_FORMAT_H

#include <cstdint>
#include <string>

std::string FileTimeToLsTime(int64_t file_time);
std::string StormErrorString(uint32_t err);

/// Writes size bytes of buffer to stdout without line-ending translation
void PrintAsBinary(const char *buffer, uint32_t size);

#endif
