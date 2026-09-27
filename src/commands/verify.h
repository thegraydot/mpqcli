#ifndef COMMANDS_VERIFY_H
#define COMMANDS_VERIFY_H

#include <filesystem>
#include <ostream>

namespace mpqcli {

/// Settled arguments for the verify subcommand
struct VerifyOptions {
    std::filesystem::path target;
    bool print_signature = false;
};

/// Checks the archive's signature, returning false when it is missing or invalid
bool Verify(const VerifyOptions &options, std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
