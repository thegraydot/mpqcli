#ifndef MPQ_READ_H
#define MPQ_READ_H

#include <memory>

#include <StormLib.h>

namespace mpqcli {

std::unique_ptr<char[]> ReadFile(HANDLE archive, const char *file_name, unsigned int *file_size,
                                 LCID preferred_locale);

} // namespace mpqcli

#endif
