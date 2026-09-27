#include "commands/info.h"

#include <memory>
#include <set>
#include <string>

#include <CLI/CLI.hpp>

#include "commands.h"

namespace {

const std::set<std::string> valid_properties = {
    "format-version", "header-offset", "header-size",    "archive-size",
    "file-count",     "max-files",     "signature-type",
};

} // namespace

void RegisterInfo(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::InfoOptions>();
    auto *sub = app.add_subcommand("info", "Prints info about an MPQ archive");

    sub->add_option("target", options->target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("-p,--property", options->property, "Prints only a specific property value")
        ->check(CLI::IsMember(valid_properties));

    sub->callback([options, &context]() { mpqcli::Info(*options, context.out); });
}
