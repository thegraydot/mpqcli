#include "util/capacity.h"

#include <cstdint>

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

    // Based on file count, determine the max number of files an MPQ archive can hold
    // We always have a minimum of 32
    // Anything over is rounded up to the closest power of 2
    // For example: 64, 128, 256
    // This is examples behavior of WoW MPQ archives (patches and installs)
    if (file_count <= 32) {
        return 32;
    }

    return NextPowerOfTwo(file_count);
}
