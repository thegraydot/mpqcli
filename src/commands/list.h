#ifndef COMMANDS_LIST_H
#define COMMANDS_LIST_H

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace mpqcli {

/// Settled arguments for the list subcommand
struct ListOptions {
    std::filesystem::path target;
    std::optional<std::filesystem::path> listfile;
    bool detailed = false;
    bool all = false;
    std::vector<std::string> properties;
};

/// Prints the archive's files to out
void List(const ListOptions &options, std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
