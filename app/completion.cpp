#include <mpqcli/completion_data.h>

#include <CLI/CLI.hpp>

#include "commands.h"

void RegisterCompletion(CLI::App &app, Context &context) {
    auto *sub = app.add_subcommand("completion", "Generate shell completion script");
    sub->require_subcommand(1);
    sub->add_subcommand("bash", "Generate bash completion script")->callback([&context]() {
        context.out << mpqcli::bash_completion_script;
    });
    sub->add_subcommand("zsh", "Generate zsh completion script")->callback([&context]() {
        context.out << mpqcli::zsh_completion_script;
    });
    sub->add_subcommand("powershell", "Generate PowerShell completion script")
        ->callback([&context]() { context.out << mpqcli::ps_completion_script; });
    sub->add_subcommand("fish", "Generate fish completion script")->callback([&context]() {
        context.out << mpqcli::fish_completion_script;
    });
}
