#ifndef MPQ_READ_H
#define MPQ_READ_H

#include <memory>
#include <ostream>

#include <StormLib.h>

namespace mpqcli {

std::unique_ptr<char[]> ReadArchivedFile(HANDLE archive, const char *file_name,
                                         unsigned int *file_size, LCID preferred_locale,
                                         std::ostream &err);

} // namespace mpqcli

#endif
