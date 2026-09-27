#ifndef COMMANDS_REMOVE_H
#define COMMANDS_REMOVE_H

#include <atomic>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace mpqcli {

/// Settled arguments for the remove subcommand
struct RemoveOptions {
    std::filesystem::path archive;
    std::vector<std::string> files;
    std::optional<std::string> locale;
};

/// Removes files from an archive, returning false if any of them could not be removed
bool Remove(const RemoveOptions &options, std::ostream &err, const std::atomic<bool> &cancelled);

} // namespace mpqcli

#endif
