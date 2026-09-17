#ifndef MPQ_ADD_H
#define MPQ_ADD_H

#include <filesystem>
#include <string>
#include <vector>

#include <StormLib.h>

#include "gamerules/rules.h"
#include "gamerules/settings.h"

namespace mpqcli {

int AddFiles(HANDLE archive, const std::vector<std::filesystem::path> &files,
             const std::filesystem::path &base_path, const std::string &path_prefix, LCID locale,
             const GameRules &game_rules,
             const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
             bool overwrite = false, bool update = false, int *skipped = nullptr);
int AddFile(HANDLE archive, const std::filesystem::path &local_file,
            const std::string &archive_file_path, LCID locale, const GameRules &game_rules,
            const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
            bool overwrite = false, bool update = false, int *skipped = nullptr);

} // namespace mpqcli

#endif
