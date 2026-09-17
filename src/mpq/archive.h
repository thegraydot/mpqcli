#ifndef MPQ_ARCHIVE_H
#define MPQ_ARCHIVE_H

#include <cstdint>
#include <string>

#include <StormLib.h>

namespace mpqcli {

bool OpenMpqArchive(const std::string &filename, HANDLE *archive, int32_t flags);
bool CloseMpqArchive(HANDLE archive);
bool SignMpqArchive(HANDLE archive);

} // namespace mpqcli

#endif
