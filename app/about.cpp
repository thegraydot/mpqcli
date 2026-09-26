#include <mpqcli/version.h>
#include <ostream>

#include <CLI/CLI.hpp>

#include "commands.h"

void RegisterAbout(CLI::App &app, Context &context) {
    auto *sub = app.add_subcommand("about", "Prints program information");
    sub->callback([&context]() {
        context.out << "Name: mpqcli" << std::endl;
        context.out << "Version: " << mpqcli::version_string << "-" << mpqcli::git_commit_hash
                    << std::endl;
        context.out << "Author: Thomas Laurenson" << std::endl;
        context.out << "License: MIT" << std::endl;
        context.out << "GitHub: https://github.com/thegraydot/mpqcli" << std::endl;
        context.out << "Dependencies:" << std::endl;
        context.out << " - StormLib (https://github.com/ladislav-zezula/StormLib)" << std::endl;
        context.out << " - CLI11 (https://github.com/CLIUtils/CLI11)" << std::endl;
        context.out << " - hash-library (https://github.com/stbrumme/hash-library)" << std::endl;
    });
}
