#include "commands/compact.h"

#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"

void RegisterCompact(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::CompactOptions>();
    auto *sub = app.add_subcommand("compact", "Compact the MPQ archive");

    sub->add_option("target", options->target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("-l,--listfile", options->listfile, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);

    sub->callback([options, &context]() { mpqcli::Compact(*options, context.err); });
}
