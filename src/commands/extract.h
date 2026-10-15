#ifndef COMMANDS_EXTRACT_H
#define COMMANDS_EXTRACT_H

#include <atomic>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace mpqcli {

/// Settled arguments for the extract subcommand
struct ExtractOptions {
    std::filesystem::path archive;
    std::optional<std::filesystem::path> output;
    /// Files inside the archive; unset extracts every file the archive can name
    std::optional<std::vector<std::string>> files;
    bool flat = false;
    std::optional<std::filesystem::path> listfile;
    std::optional<std::string> locale;
};

/// Extracts the named files, or every file, returning false if any of them failed
bool Extract(const ExtractOptions &options, std::ostream &err, const std::atomic<bool> &cancelled);

} // namespace mpqcli

#endif
