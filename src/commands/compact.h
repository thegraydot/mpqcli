#ifndef COMMANDS_COMPACT_H
#define COMMANDS_COMPACT_H

#include <optional>
#include <ostream>
#include <string>

namespace mpqcli {

/// Settled arguments for the compact subcommand
struct CompactOptions {
    std::string target;
    std::optional<std::string> listfile;
};

/// Compacts the archive
bool Compact(const CompactOptions &options, std::ostream &err);

} // namespace mpqcli

#endif
