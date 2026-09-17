#ifndef MPQ_REMOVE_H
#define MPQ_REMOVE_H

#include <string>

#include <StormLib.h>

int RemoveFile(HANDLE archive, const std::string &archive_file_path, LCID locale);

#endif
