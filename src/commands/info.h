#ifndef COMMANDS_INFO_H
#define COMMANDS_INFO_H

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

namespace mpqcli {

/// Settled arguments for the info subcommand
struct InfoOptions {
    std::filesystem::path target;
    std::optional<std::string> property;
};

/// Prints the archive's properties, or only the one named, to out
void Info(const InfoOptions &options, std::ostream &out);

} // namespace mpqcli

#endif
