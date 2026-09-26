#include "commands/rename.h"

#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "validators.h"

void RegisterRename(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::RenameOptions>();
    auto *sub = app.add_subcommand("rename", "Rename a file in an existing MPQ archive");

    sub->add_option("archive", options->archive, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("old-file", options->old_file, "Current archive path of the file")->required();
    sub->add_option("new-file", options->new_file, "New archive path of the file")->required();
    sub->add_option("--locale", options->locale, "Locale of file to rename")->check(locale_valid);

    sub->callback([options, &context]() {
        context.exit_code = mpqcli::Rename(*options, context.err) ? 0 : 1;
    });
}
