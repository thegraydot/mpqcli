#include <mpqcli/version.h>
#include <ostream>

#include <CLI/CLI.hpp>

#include "commands.h"

void RegisterVersion(CLI::App &app, Context &context) {
    auto *sub = app.add_subcommand("version", "Prints program version");
    sub->callback([&context]() {
        context.out << mpqcli::version_string << "-" << mpqcli::git_commit_hash << std::endl;
    });
}
