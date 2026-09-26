#ifndef COMMANDS_EXTRACT_H
#define COMMANDS_EXTRACT_H

#include <atomic>
#include <optional>
#include <ostream>
#include <string>

namespace mpqcli {

/// Settled arguments for the extract subcommand
struct ExtractOptions {
    std::string target;
    std::optional<std::string> output;
    std::optional<std::string> file;
    bool keep_folder_structure = false;
    std::optional<std::string> listfile;
    std::optional<std::string> locale;
};

/// Extracts one file or every file, returning false if any of them failed
bool Extract(const ExtractOptions &options, std::ostream &err, const std::atomic<bool> &cancelled);

} // namespace mpqcli

#endif
