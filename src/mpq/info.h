#ifndef MPQ_INFO_H
#define MPQ_INFO_H

#include <optional>
#include <string>

#include <StormLib.h>

namespace mpqcli {

void PrintMpqInfo(HANDLE archive, const std::optional<std::string> &info_property);

} // namespace mpqcli

#endif
