#ifndef MPQ_RENAME_H
#define MPQ_RENAME_H

#include <string>

#include <StormLib.h>

int RenameFile(HANDLE archive, const std::string &old_archive_file_path,
               const std::string &new_archive_file_path, LCID locale);

#endif
