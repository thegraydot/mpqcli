#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>
#include <ostream>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "errors.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
// The manifest makes UTF-8 the process code page, but a console keeps code pages of
// its own, usually the OEM one, so names would print as mojibake. The console is
// shared with the shell that started mpqcli, so what it had is put back on the way
// out. Without a console both calls fail, which is harmless.
class Utf8Console {
public:
    Utf8Console() : input_(GetConsoleCP()), output_(GetConsoleOutputCP()) {
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }
    Utf8Console(const Utf8Console &) = delete;
    Utf8Console &operator=(const Utf8Console &) = delete;
    ~Utf8Console() {
        if (input_ != 0) {
            SetConsoleCP(input_);
        }
        if (output_ != 0) {
            SetConsoleOutputCP(output_);
        }
    }

private:
    UINT input_;
    UINT output_;
};
#endif

// The only mutable global state: a signal handler takes no arguments and has nowhere
// else to write, and it may touch nothing but lock-free atomics. interrupted is what
// the core polls; interrupt_signal is which signal set it, so main can end by that
// same signal. It starts as SIGINT, which is what main used before it was recorded.
std::atomic<bool> interrupted{false};
std::atomic<int> interrupt_signal{SIGINT};
static_assert(decltype(interrupted)::is_always_lock_free,
              "a signal handler may only touch a lock-free atomic");
static_assert(decltype(interrupt_signal)::is_always_lock_free,
              "a signal handler may only touch a lock-free atomic");

extern "C" void OnInterrupt(int signal_number) {
    interrupt_signal.store(signal_number, std::memory_order_relaxed);
    interrupted.store(true, std::memory_order_relaxed);
}

} // namespace

int main(int argc, char **argv) {
#ifdef _WIN32
    const Utf8Console console;
#endif
    Context context{std::cout, std::cerr, std::cin, interrupted};

    CLI::App app{
        "A command line tool to create, add, remove, list, extract, read, rename, and verify MPQ "
        "archives using the StormLib library"};
    app.require_subcommand(1);
    // No option here starts with a slash, so on Windows a path such as /data/patch.mpq
    // is an argument rather than an option
    app.allow_windows_style_options(false);

    RegisterVersion(app, context);
    RegisterAbout(app, context);
    RegisterInfo(app, context);
    RegisterCreate(app, context);
    RegisterAdd(app, context);
    RegisterRemove(app, context);
    RegisterRename(app, context);
    RegisterList(app, context);
    RegisterExtract(app, context);
    RegisterRead(app, context);
    RegisterVerify(app, context);
    RegisterCompact(app, context);
    RegisterCompletion(app, context);

    // Windows never delivers SIGTERM; registering it there is harmless and keeps
    // one code path
    std::signal(SIGINT, OnInterrupt);
    std::signal(SIGTERM, OnInterrupt);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // An empty command line is a request for help rather than a mistake. The argc
        // check is what keeps an unknown subcommand, which raises the same error, failing.
        if (argc == 1 && e.get_exit_code() == static_cast<int>(CLI::ExitCodes::RequiredError)) {
            context.out << app.help() << std::endl;
            return 0;
        }
        return app.exit(e, context.out, context.err);
    } catch (const mpqcli::Interrupted &) {
        // The user asked for this, so nothing is reported
#ifdef _WIN32
        // A re-raised SIGINT would end the process with 3 here, so return what a POSIX
        // shell reports for one instead
        return 130;
#else
        // Dying of the same signal rather than returning a code is what lets a shell or
        // a supervisor see how mpqcli ended: 130 after SIGINT, 143 after SIGTERM
        const int signal_number = interrupt_signal.load(std::memory_order_relaxed);
        std::signal(signal_number, SIG_DFL);
        std::raise(signal_number);
        return 1; // Not reached
#endif
    } catch (const mpqcli::Error &e) {
        context.err << "[!] " << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        context.err << "[!] Unexpected error: " << e.what() << std::endl;
        return 2;
    }
    return context.exit_code;
}
