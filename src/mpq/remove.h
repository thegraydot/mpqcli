#ifndef MPQ_REMOVE_H
#define MPQ_REMOVE_H

#include <string>

#include <StormLib.h>

namespace mpqcli {

int RemoveFile(HANDLE archive, const std::string &archive_file_path, LCID locale);

} // namespace mpqcli

#endif
