#ifndef COMMANDS_COMPACT_H
#define COMMANDS_COMPACT_H

#include <filesystem>
#include <optional>
#include <ostream>

namespace mpqcli {

/// Settled arguments for the compact subcommand
struct CompactOptions {
    std::filesystem::path target;
    std::optional<std::filesystem::path> listfile;
};

/// Compacts the archive
void Compact(const CompactOptions &options, std::ostream &err);

} // namespace mpqcli

#endif
