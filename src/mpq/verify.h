#ifndef MPQ_VERIFY_H
#define MPQ_VERIFY_H

#include <cstdint>
#include <filesystem>
#include <ostream>

#include <StormLib.h>

namespace mpqcli {

uint32_t VerifyMpqArchive(HANDLE archive);

/// Writes the archive's weak or strong signature bytes to out
///
/// @throws ArchiveError if the signature cannot be read
/// @throws FileError if the archive's size on disk cannot be read
void PrintMpqSignature(HANDLE archive, const std::filesystem::path &target, std::ostream &out,
                       std::ostream &err);

} // namespace mpqcli

#endif
