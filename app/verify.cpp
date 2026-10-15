#include "commands/verify.h"

#include <memory>

#include <CLI/CLI.hpp>

#include "commands.h"

void RegisterVerify(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::VerifyOptions>();
    auto *sub = app.add_subcommand("verify", "Verify the MPQ archive");

    sub->add_option("archive", options->archive, "MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_flag("-x,--print", options->print_signature, "Print the digital signature (in hex)");

    sub->callback([options, &context]() {
        context.exit_code = mpqcli::Verify(*options, context.out, context.err) ? 0 : 1;
    });
}
