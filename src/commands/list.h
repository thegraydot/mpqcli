#ifndef COMMANDS_LIST_H
#define COMMANDS_LIST_H

#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace mpqcli {

/// Settled arguments for the list subcommand
struct ListOptions {
    std::string target;
    std::optional<std::string> listfile;
    bool detailed = false;
    bool all = false;
    std::vector<std::string> properties;
};

/// Prints the archive's files to out
bool List(const ListOptions &options, std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
