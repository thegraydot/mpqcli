#ifndef COMMANDS_EXTRACT_H
#define COMMANDS_EXTRACT_H

#include <atomic>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>

namespace mpqcli {

/// Settled arguments for the extract subcommand
struct ExtractOptions {
    std::filesystem::path target;
    std::optional<std::filesystem::path> output;
    std::optional<std::string> file;
    bool keep_folder_structure = false;
    std::optional<std::filesystem::path> listfile;
    std::optional<std::string> locale;
};

/// Extracts one file or every file, returning false if any of them failed
bool Extract(const ExtractOptions &options, std::ostream &err, const std::atomic<bool> &cancelled);

} // namespace mpqcli

#endif
