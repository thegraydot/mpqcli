#ifndef COMMANDS_READ_H
#define COMMANDS_READ_H

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

namespace mpqcli {

/// Settled arguments for the read subcommand
struct ReadOptions {
    std::string file;
    std::filesystem::path target;
    std::optional<std::string> locale;
};

/// Writes one archived file's bytes to out, returning false if it could not be read
bool Read(const ReadOptions &options, std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
