#include "commands/add.h"

#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "gamerules/rules.h"
#include "util/path.h"
#include "validators.h"

void RegisterAdd(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::AddOptions>();
    auto *sub = app.add_subcommand("add", "Add files to an existing MPQ archive");

    sub->add_option("archive", options->archive, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("files", options->files,
                    "Files or directories to add; pass - to read paths from stdin")
        ->required()
        ->expected(-1);
    sub->add_option("-p,--path", options->path,
                    "Archive path for a single file, or prefix for a directory");
    CLI::Option *overwrite = sub->add_flag("-w,--overwrite", options->overwrite,
                                           "Replace every file that already exists in the archive");
    sub->add_flag("-u,--update", options->update,
                  "Replace only files that changed. Compares size, then timestamp/MD5/CRC32")
        ->excludes(overwrite);
    sub->add_option("--locale", options->locale, "Locale to use for added file")
        ->check(locale_valid);
    sub->add_option("-g,--game", options->game_profile,
                    "Game profile for compression rules. Valid options:\n" +
                        mpqcli::GameRules::GetAvailableProfiles())
        ->check(game_profile_valid);

    mpqcli::CompressionSettingsOverrides &compression = options->compression_overrides;
    sub->add_option("--flags", compression.flags, "Override MPQ file flags")
        ->group("Game setting overrides");
    sub->add_option("--compression", compression.compression,
                    "Override compression for first sector")
        ->group("Game setting overrides");
    sub->add_option("--compression-next", compression.compression_next,
                    "Override compression for subsequent sectors")
        ->group("Game setting overrides");

    sub->callback([options, &context]() {
        options->files = mpqcli::ExpandStdinMarker(options->files, context.in);
        context.exit_code = mpqcli::Add(*options, context.err) ? 0 : 1;
    });
}
