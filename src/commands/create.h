#ifndef COMMANDS_CREATE_H
#define COMMANDS_CREATE_H

#include <atomic>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

#include "gamerules/settings.h"

namespace mpqcli {

/// Settled arguments for the create subcommand
struct CreateOptions {
    std::filesystem::path target;
    std::optional<std::string> path;
    std::optional<std::filesystem::path> output;
    bool sign = false;
    std::optional<std::string> locale;
    std::optional<std::string> game_profile;
    MpqCreateSettingsOverrides create_overrides;
    CompressionSettingsOverrides compression_overrides;
};

/// Creates an archive from a file or directory, returning false if any file failed to add
bool Create(const CreateOptions &options, std::ostream &err, const std::atomic<bool> &cancelled);

} // namespace mpqcli

#endif
