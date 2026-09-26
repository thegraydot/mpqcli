#ifndef COMMANDS_RENAME_H
#define COMMANDS_RENAME_H

#include <optional>
#include <ostream>
#include <string>

namespace mpqcli {

/// Settled arguments for the rename subcommand
struct RenameOptions {
    std::string archive;
    std::string old_file;
    std::string new_file;
    std::optional<std::string> locale;
};

/// Renames a file inside an archive, returning false if it could not be renamed
bool Rename(const RenameOptions &options, std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
