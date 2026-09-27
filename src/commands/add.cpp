#include "commands/add.h"

#include <atomic>
#include <filesystem>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "errors.h"
#include "gamerules/rules.h"
#include "mpq/add.h"
#include "mpq/archive.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

bool Add(const AddOptions &options, std::ostream &err, const std::atomic<bool> &cancelled) {
    Archive archive = Archive::Open(options.archive, 0);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;

    GameProfile profile;
    if (options.game_profile.has_value()) {
        profile = GameRules::StringToProfile(options.game_profile.value());
        err << "[*] Using game profile: " << options.game_profile.value() << std::endl;
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules game_rules(profile);

    std::error_code ec;
    bool has_directory = false;
    for (const auto &f : options.files) {
        if (fs::is_directory(f, ec)) {
            has_directory = true;
            break;
        }
    }

    int result = 0;
    int files_skipped = 0;
    for (const auto &f : options.files) {
        ThrowIfCancelled(cancelled);

        if (!fs::exists(f, ec)) {
            err << "[!] Path does not exist: " << f.string() << std::endl;
            result |= 1;
            continue;
        }

        if (fs::is_directory(f, ec)) {
            std::vector<fs::path> directory_files = ListFilesRecursive(f, ec);
            if (ec) {
                err << "[!] Failed to list directory: (" << ec.value() << ") " << ec.message()
                    << ": " << f.string() << std::endl;
                result |= 1;
                continue;
            }
            std::string prefix = options.path.value_or("");
            result |= AddFiles(archive.Handle(), directory_files, f, prefix, lcid, game_rules, err,
                               cancelled, options.compression_overrides, options.overwrite,
                               options.update, &files_skipped);

        } else if (fs::is_regular_file(f, ec)) {
            const bool treat_as_directory = has_directory || options.files.size() > 1;
            std::string archive_path = ResolveArchiveName(f, options.path, treat_as_directory);
            result |= AddFile(archive.Handle(), f, archive_path, lcid, game_rules, err,
                              options.compression_overrides, options.overwrite, options.update,
                              &files_skipped);

        } else {
            err << "[!] Not a file or directory: " << f.string() << std::endl;
            result |= 1;
        }
    }

    // Skipping pre-existing files is the default, so point at the flags that change it
    // rather than letting the run look like it silently did nothing.
    if (!options.overwrite && !options.update && files_skipped > 0) {
        err << "[*] " << files_skipped
            << " file(s) already in the archive were skipped. Use --overwrite to replace "
               "them, or --update to replace only the ones that changed."
            << std::endl;
    }

    archive.Close();
    return result == 0;
}

} // namespace mpqcli
