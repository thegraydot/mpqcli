#include "commands/extract.h"

#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"

void RegisterExtract(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::ExtractOptions>();
    auto *sub = app.add_subcommand("extract", "Extract files from the MPQ archive");

    sub->add_option("target", options->target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("-o,--output", options->output, "Output directory");
    sub->add_option("-f,--file", options->file, "Target file to extract");
    sub->add_flag("-k,--keep", options->keep_folder_structure,
                  "Keep folder structure (default false)");
    sub->add_option("-l,--listfile", options->listfile, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);
    sub->add_option("--locale", options->locale, "Preferred locale for extracted file");

    sub->callback([options, &context]() {
        context.exit_code = mpqcli::Extract(*options, context.err, context.cancelled) ? 0 : 1;
    });
}
