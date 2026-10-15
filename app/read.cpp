#include "commands/read.h"

#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "validators.h"

void RegisterRead(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::ReadOptions>();
    auto *sub = app.add_subcommand("read", "Read a file from an MPQ archive");

    sub->add_option("archive", options->archive, "MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("file", options->file, "File to read")->required();
    sub->add_option("--locale", options->locale, "Preferred locale for read file")
        ->check(locale_valid);

    sub->callback([options, &context]() {
        context.exit_code = mpqcli::Read(*options, context.out, context.err) ? 0 : 1;
    });
}
