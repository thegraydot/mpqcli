#ifndef COMMANDS_VERIFY_H
#define COMMANDS_VERIFY_H

#include <ostream>
#include <string>

namespace mpqcli {

/// Settled arguments for the verify subcommand
struct VerifyOptions {
    std::string target;
    bool print_signature = false;
};

/// Checks the archive's signature, returning false when it is missing or invalid
bool Verify(const VerifyOptions &options, std::ostream &out, std::ostream &err);

} // namespace mpqcli

#endif
