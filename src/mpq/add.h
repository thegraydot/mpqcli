#ifndef MPQ_ADD_H
#define MPQ_ADD_H

#include <filesystem>
#include <string>
#include <vector>

#include <StormLib.h>

#include "gamerules.h"

namespace fs = std::filesystem;

int AddFiles(HANDLE archive, const std::vector<fs::path> &files, const fs::path &base_path,
             const std::string &path_prefix, LCID locale, const GameRules &game_rules,
             const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
             bool overwrite = false, bool update = false, int *skipped = nullptr);
int AddFile(HANDLE archive, const fs::path &local_file, const std::string &archive_file_path,
            LCID locale, const GameRules &game_rules,
            const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
            bool overwrite = false, bool update = false, int *skipped = nullptr);

#endif
