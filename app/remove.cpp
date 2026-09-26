#include "commands/remove.h"

#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "util/path.h"
#include "validators.h"

void RegisterRemove(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::RemoveOptions>();
    auto *sub = app.add_subcommand("remove", "Remove files from an existing MPQ archive");

    sub->add_option("archive", options->archive, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("files", options->files,
                    "Archive paths of files to remove; pass - to read paths from stdin")
        ->required()
        ->expected(-1);
    sub->add_option("--locale", options->locale, "Locale of file to remove")->check(locale_valid);

    sub->callback([options, &context]() {
        options->files = mpqcli::ExpandStdinMarker(options->files, context.in);
        context.exit_code = mpqcli::Remove(*options, context.err, context.cancelled) ? 0 : 1;
    });
}
