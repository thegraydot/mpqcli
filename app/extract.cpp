#include "commands/extract.h"

#include <memory>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "util/path.h"
#include "validators.h"

void RegisterExtract(CLI::App &app, Context &context) {
    struct Options {
        mpqcli::ExtractOptions extract;
        // Names inside the archive, or - for the lines on stdin, settled into extract.files
        // by the callback
        std::vector<std::string> files;
    };

    auto options = std::make_shared<Options>();
    auto *sub = app.add_subcommand("extract", "Extract files from the MPQ archive");

    sub->add_option("archive", options->extract.archive, "MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    CLI::Option *files =
        sub->add_option("files", options->files,
                        "Files to extract, or every file when omitted; pass - to read "
                        "names from stdin");
    sub->add_option("-o,--output", options->extract.output, "Output directory");
    sub->add_flag("--flat", options->extract.flat,
                  "Write files into the output directory without their folders");
    sub->add_option("-l,--listfile", options->extract.listfile,
                    "File listing content of an MPQ archive, when extracting every file")
        ->check(CLI::ExistingFile)
        ->excludes(files);
    sub->add_option("--locale", options->extract.locale, "Preferred locale for extracted files")
        ->check(locale_valid);

    sub->callback([options, &context]() {
        // An empty stdin extracts nothing rather than everything, so a pipeline whose
        // filter matched no names cannot unpack the whole archive
        if (!options->files.empty()) {
            options->extract.files = mpqcli::ExpandStdinMarker(options->files, context.in);
        }
        context.exit_code =
            mpqcli::Extract(options->extract, context.err, context.cancelled) ? 0 : 1;
    });
}
