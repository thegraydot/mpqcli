#ifndef UTIL_CAPACITY_H
#define UTIL_CAPACITY_H

#include <cstdint>

namespace mpqcli {

/// Rounds n up to the next power of two
uint32_t NextPowerOfTwo(uint32_t n);

/// Returns the max file count an archive holding file_count files should declare
uint32_t CalculateMpqMaxFileValue(uint32_t file_count);

} // namespace mpqcli

#endif
