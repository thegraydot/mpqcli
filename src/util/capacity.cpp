#include "util/capacity.h"

#include <cstdint>

namespace mpqcli {

uint32_t NextPowerOfTwo(uint32_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

uint32_t CalculateMpqMaxFileValue(uint32_t file_count) {
    // Always add 3 for "special" files
    file_count += 3;

    // The table holds at least 32 entries; above that the count rounds up to the
    // next power of two (64, 128, 256), which matches the WoW patch and install
    // archives
    if (file_count <= 32) {
        return 32;
    }

    return NextPowerOfTwo(file_count);
}

} // namespace mpqcli
