#ifndef MPQ_VERIFY_H
#define MPQ_VERIFY_H

#include <cstdint>
#include <string>

#include <StormLib.h>

namespace mpqcli {

uint32_t VerifyMpqArchive(HANDLE archive);
int32_t PrintMpqSignature(HANDLE archive, const std::string &target);

} // namespace mpqcli

#endif
