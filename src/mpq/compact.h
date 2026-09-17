#ifndef MPQ_COMPACT_H
#define MPQ_COMPACT_H

#include <optional>
#include <string>

#include <StormLib.h>

int CompactMpqArchive(HANDLE archive, const std::optional<std::string> &listfile_name);

#endif
