#include "mpq/flags.h"

#include <cstdint>
#include <string>

#include <StormLib.h>

std::string GetFlagString(uint32_t flags) {
    std::string result;

    // Preserve column-aligned flag-to-char mappings
    // clang-format off
    if (flags & MPQ_FILE_IMPLODE)            result += 'i';
    if (flags & MPQ_FILE_COMPRESS)           result += 'c';
    if (flags & MPQ_FILE_ENCRYPTED)          result += 'e';
    if (flags & MPQ_FILE_KEY_V2)             result += '2';
    if (flags & MPQ_FILE_PATCH_FILE)         result += 'p';
    if (flags & MPQ_FILE_SINGLE_UNIT)        result += 'u';
    if (flags & MPQ_FILE_DELETE_MARKER)      result += 'd';
    if (flags & MPQ_FILE_SECTOR_CRC)         result += 'r';
    if (flags & MPQ_FILE_SIGNATURE)          result += 's';
    if (flags & MPQ_FILE_EXISTS)             result += 'x';
    if (flags & MPQ_FILE_COMPRESS_MASK)      result += 'm';
    if (flags & MPQ_FILE_DEFAULT_INTERNAL)   result += 'n';
    if (flags & MPQ_FILE_FIX_KEY)            result += 'f';
    // clang-format on

    return result;
}
