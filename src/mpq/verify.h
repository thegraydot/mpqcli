#ifndef MPQ_VERIFY_H
#define MPQ_VERIFY_H

#include <cstdint>
#include <ostream>
#include <string>

#include <StormLib.h>

namespace mpqcli {

uint32_t VerifyMpqArchive(HANDLE archive);

/// Writes the archive's weak or strong signature bytes to out
///
/// @throws ArchiveError if the signature cannot be read
void PrintMpqSignature(HANDLE archive, const std::string &target, std::ostream &out,
                       std::ostream &err);

} // namespace mpqcli

#endif
