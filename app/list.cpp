#include "commands/list.h"

#include <memory>
#include <set>
#include <string>

#include <CLI/CLI.hpp>

#include "commands.h"

namespace {

const std::set<std::string> valid_properties = {
    "hash-index", "name-hash1",     "name-hash2",         "name-hash3", "locale",
    "file-index", "byte-offset",    "file-time",          "file-size",  "compressed-size",
    "flags",      "encryption-key", "encryption-key-raw",
};

} // namespace

void RegisterList(CLI::App &app, Context &context) {
    auto options = std::make_shared<mpqcli::ListOptions>();
    auto *sub = app.add_subcommand("list", "List files from the MPQ archive");

    sub->add_option("target", options->target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    sub->add_option("-l,--listfile", options->listfile, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);
    sub->add_flag("-d,--detailed", options->detailed,
                  "File listing with additional columns (default false)");
    sub->add_flag("-a,--all", options->all, "File listing including hidden files (default false)");
    sub->add_option("-p,--property", options->properties, "Prints only specific property values")
        ->check(CLI::IsMember(valid_properties));

    sub->callback([options, &context]() { mpqcli::List(*options, context.out, context.err); });
}
