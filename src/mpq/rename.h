#ifndef MPQ_RENAME_H
#define MPQ_RENAME_H

#include <ostream>
#include <string>

#include <StormLib.h>

namespace mpqcli {

int RenameFile(HANDLE archive, const std::string &old_archive_file_path,
               const std::string &new_archive_file_path, LCID locale, std::ostream &err);

} // namespace mpqcli

#endif
