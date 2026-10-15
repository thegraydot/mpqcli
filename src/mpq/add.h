#ifndef MPQ_ADD_H
#define MPQ_ADD_H

#include <atomic>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <StormLib.h>

#include "gamerules/rules.h"
#include "gamerules/settings.h"

namespace mpqcli {

int AddFiles(HANDLE archive, const std::vector<std::filesystem::path> &files,
             const std::filesystem::path &base_path, const std::string &path_prefix, LCID locale,
             const GameRules &game_rules, std::ostream &err, const std::atomic<bool> &cancelled,
             const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
             bool overwrite = false, bool update = false, int *skipped = nullptr);
int AddFile(HANDLE archive, const std::filesystem::path &local_file,
            const std::string &archive_file_path, LCID locale, const GameRules &game_rules,
            std::ostream &err,
            const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
            bool overwrite = false, bool update = false, int *skipped = nullptr);

/// Adds each local file or directory in inputs, returning 1 if any of them failed
///
/// A directory's contents go under path, or at the archive root without it. A lone file is
/// stored at path when given; beside other inputs or any directory, path is a prefix.
int AddInputs(HANDLE archive, const std::vector<std::filesystem::path> &inputs,
              const std::optional<std::string> &path, LCID locale, const GameRules &game_rules,
              std::ostream &err, const std::atomic<bool> &cancelled,
              const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
              bool overwrite = false, bool update = false, int *skipped = nullptr);

} // namespace mpqcli

#endif
