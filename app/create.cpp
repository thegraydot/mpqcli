#include "commands/create.h"

#include <cstdint>
#include <memory>
#include <optional>

#include <CLI/CLI.hpp>
#include <StormLib.h>

#include "commands.h"
#include "gamerules/rules.h"
#include "validators.h"

void RegisterCreate(CLI::App &app, Context &context) {
    struct Options {
        mpqcli::CreateOptions create;
        // The documented numbering is 1 to 4; StormLib counts the same formats from 0
        std::optional<int32_t> version;
    };

    auto options = std::make_shared<Options>();
    auto *sub = app.add_subcommand("create", "Create an MPQ archive from target file or directory");

    sub->add_option("target", options->create.target, "Directory or file to put in MPQ archive")
        ->required()
        ->check(CLI::ExistingPath);
    sub->add_option("-p,--path", options->create.path,
                    "Archive path for a single file, or prefix for a directory");
    sub->add_option("-o,--output", options->create.output, "Output MPQ archive");
    sub->add_flag("-s,--sign", options->create.sign, "Sign the MPQ archive (default false)");
    sub->add_option("--locale", options->create.locale, "Locale to use for added files")
        ->check(locale_valid);
    sub->add_option("-g,--game", options->create.game_profile,
                    "Game profile for MPQ creation. Valid options:\n" +
                        mpqcli::GameRules::GetAvailableProfiles())
        ->check(game_profile_valid);

    mpqcli::MpqCreateSettingsOverrides &create = options->create.create_overrides;
    sub->add_option("--version", options->version, "Override the MPQ archive version")
        ->check(CLI::Range(1, 4))
        ->group("Game setting overrides");
    sub->add_option("--stream-flags", create.stream_flags, "Override stream flags")
        ->group("Game setting overrides");
    sub->add_option("--sector-size", create.sector_size, "Override sector size")
        ->group("Game setting overrides");
    sub->add_option("--raw-chunk-size", create.raw_chunk_size, "Override raw chunk size for MPQ v4")
        ->group("Game setting overrides");
    sub->add_option("--file-flags1", create.file_flags1, "Override file flags for (listfile)")
        ->group("Game setting overrides");
    sub->add_option("--file-flags2", create.file_flags2, "Override file flags for (attributes)")
        ->group("Game setting overrides");
    sub->add_option("--file-flags3", create.file_flags3, "Override file flags for (signature)")
        ->group("Game setting overrides");
    sub->add_option("--attr-flags", create.attr_flags,
                    "Override attribute flags (CRC32, FILETIME, MD5)")
        ->group("Game setting overrides");

    mpqcli::CompressionSettingsOverrides &compression = options->create.compression_overrides;
    sub->add_option("--flags", compression.flags, "Override MPQ file flags for added files")
        ->group("Game setting overrides");
    sub->add_option("--compression", compression.compression,
                    "Override compression for first sector of added files")
        ->group("Game setting overrides");
    sub->add_option("--compression-next", compression.compression_next,
                    "Override compression for subsequent sectors of added files")
        ->group("Game setting overrides");

    sub->callback([options, &context]() {
        if (options->version.has_value()) {
            options->create.create_overrides.mpq_version =
                static_cast<DWORD>(options->version.value() - 1);
        }
        context.exit_code = mpqcli::Create(options->create, context.err, context.cancelled) ? 0 : 1;
    });
}
