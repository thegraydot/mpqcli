#ifndef MPQ_LIST_H
#define MPQ_LIST_H

#include <optional>
#include <string>
#include <vector>

#include <StormLib.h>

int ListFiles(HANDLE archive, const std::optional<std::string> &listfile_name, bool list_all,
              bool list_detailed, const std::vector<std::string> &properties);

#endif
