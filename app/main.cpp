#include <exception>
#include <iostream>
#include <ostream>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "errors.h"

int main(int argc, char **argv) {
    Context context{std::cout, std::cerr, std::cin};

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

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // An empty command line is a request for help rather than a mistake. The argc
        // check is what keeps an unknown subcommand, which raises the same error, failing
        if (argc == 1 && e.get_exit_code() == static_cast<int>(CLI::ExitCodes::RequiredError)) {
            context.out << app.help() << std::endl;
            return 0;
        }
        return app.exit(e, context.out, context.err);
    } catch (const mpqcli::Error &e) {
        context.err << "[!] " << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        context.err << "[!] Unexpected error: " << e.what() << std::endl;
        return 2;
    }
    return context.exit_code;
}
