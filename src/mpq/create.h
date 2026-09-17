#ifndef MPQ_CREATE_H
#define MPQ_CREATE_H

#include <cstdint>
#include <string>

#include <StormLib.h>

#include "gamerules/rules.h"

namespace mpqcli {

HANDLE CreateMpqArchive(const std::string &output_archive_name, uint32_t file_count,
                        const GameRules &game_rules);

} // namespace mpqcli

#endif
