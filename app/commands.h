#ifndef COMMANDS_H
#define COMMANDS_H

#include <atomic>
#include <istream>
#include <ostream>

#include <CLI/CLI.hpp>

/// What main hands every subcommand: the real streams and a slot for the exit code
struct Context {
    std::ostream &out;
    std::ostream &err;
    std::istream &in;
    const std::atomic<bool> &cancelled;
    int exit_code = 0;
};

void RegisterVersion(CLI::App &app, Context &context);
void RegisterAbout(CLI::App &app, Context &context);
void RegisterInfo(CLI::App &app, Context &context);
void RegisterCreate(CLI::App &app, Context &context);
void RegisterAdd(CLI::App &app, Context &context);
void RegisterRemove(CLI::App &app, Context &context);
void RegisterRename(CLI::App &app, Context &context);
void RegisterList(CLI::App &app, Context &context);
void RegisterExtract(CLI::App &app, Context &context);
void RegisterRead(CLI::App &app, Context &context);
void RegisterVerify(CLI::App &app, Context &context);
void RegisterCompact(CLI::App &app, Context &context);
void RegisterCompletion(CLI::App &app, Context &context);

#endif
