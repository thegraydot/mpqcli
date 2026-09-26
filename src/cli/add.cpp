#include "mpq/add.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "cli/commands.h"
#include "gamerules/rules.h"
#include "mpq/archive.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

int HandleAdd(const std::vector<std::string> &files, const std::string &target,
              const std::optional<std::string> &path, bool overwrite, bool update,
              const std::optional<std::string> &locale,
              const std::optional<std::string> &game_profile, int64_t file_flags,
              int64_t file_compression, int64_t file_compression_next, std::ostream &out,
              std::ostream &err) {
    Archive archive = Archive::Open(target, 0);

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;

    GameProfile profile;
    if (game_profile.has_value()) {
        profile = GameRules::StringToProfile(game_profile.value());
        out << "[*] Using game profile: " << game_profile.value() << std::endl;
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules game_rules(profile);

    CompressionSettingsOverrides add_overrides;
    if (file_flags >= 0)
        add_overrides.flags = static_cast<DWORD>(file_flags);
    if (file_compression >= 0)
        add_overrides.compression = static_cast<DWORD>(file_compression);
    if (file_compression_next >= 0)
        add_overrides.compression_next = static_cast<DWORD>(file_compression_next);

    std::error_code ec;
    bool has_directory = false;
    for (const auto &f : files) {
        if (fs::is_directory(f, ec)) {
            has_directory = true;
            break;
        }
    }

    int result = 0;
    int files_skipped = 0;
    for (const auto &f : files) {
        if (!fs::exists(f, ec)) {
            err << "[!] Path does not exist: " << f << std::endl;
            result |= 1;
            continue;
        }

        if (fs::is_directory(f, ec)) {
            std::vector<fs::path> directory_files = ListFilesRecursive(f, ec);
            if (ec) {
                err << "[!] Failed to list directory: (" << ec.value() << ") " << ec.message()
                    << ": " << f << std::endl;
                result |= 1;
                continue;
            }
            std::string prefix = path.value_or("");
            result |= AddFiles(archive.Handle(), directory_files, f, prefix, lcid, game_rules, out,
                               err, add_overrides, overwrite, update, &files_skipped);

        } else if (fs::is_regular_file(f, ec)) {
            const bool treat_as_directory = has_directory || files.size() > 1;
            std::string archive_path = ResolveArchiveName(f, path, treat_as_directory);
            result |= AddFile(archive.Handle(), f, archive_path, lcid, game_rules, out, err,
                              add_overrides, overwrite, update, &files_skipped);

        } else {
            err << "[!] Not a file or directory: " << f << std::endl;
            result |= 1;
        }
    }

    // Skipping pre-existing files is the default, so point at the flags that change it
    // rather than letting the run look like it silently did nothing.
    if (!overwrite && !update && files_skipped > 0) {
        err << "[*] " << files_skipped
            << " file(s) already in the archive were skipped. Use --overwrite to replace "
               "them, or --update to replace only the ones that changed."
            << std::endl;
    }

    archive.Close();
    return result;
}

} // namespace mpqcli
