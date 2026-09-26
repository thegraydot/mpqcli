#ifndef COMMANDS_ADD_H
#define COMMANDS_ADD_H

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "gamerules/settings.h"

namespace mpqcli {

/// Settled arguments for the add subcommand
struct AddOptions {
    std::string archive;
    std::vector<std::string> files;
    std::optional<std::string> path;
    bool overwrite = false;
    bool update = false;
    std::optional<std::string> locale;
    std::optional<std::string> game_profile;
    CompressionSettingsOverrides compression_overrides;
};

/// Adds files and directories to an archive, returning false if any of them failed
bool Add(const AddOptions &options, std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
