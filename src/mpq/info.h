#ifndef MPQ_INFO_H
#define MPQ_INFO_H

#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

namespace mpqcli {

void PrintMpqInfo(HANDLE archive, const std::optional<std::string> &info_property,
                  std::ostream &out);

} // namespace mpqcli

#endif
