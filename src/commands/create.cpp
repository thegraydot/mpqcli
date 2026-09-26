#include "commands/create.h"

#include <cstdint>
#include <filesystem>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "gamerules/rules.h"
#include "mpq/add.h"
#include "mpq/archive.h"
#include "util/capacity.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

bool Create(const CreateOptions &options, std::ostream &out, std::ostream &err) {
    std::error_code ec;
    fs::path output_file_path;
    if (options.output.has_value()) {
        output_file_path = fs::absolute(options.output.value(), ec);
        if (ec) {
            err << "[!] Failed to resolve output path: (" << ec.value() << ") " << ec.message()
                << ": " << options.output.value() << std::endl;
            return false;
        }
    } else {
        output_file_path = fs::path(options.target);
        // If the path ends with a separator (e.g. "dir/"), strip the
        // trailing separator first so we get "dir.mpq"
        if (output_file_path.filename().empty()) {
            output_file_path = output_file_path.parent_path();
        }
        output_file_path.replace_extension(".mpq");
    }
    std::string output_file = output_file_path.u8string();

    GameProfile profile;
    if (options.game_profile.has_value()) {
        profile = GameRules::StringToProfile(options.game_profile.value());
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules game_rules(profile);

    out << "[*] Game profile: " << options.game_profile.value_or("default")
        << ", Output file: " << output_file << std::endl;

    game_rules.OverrideCreateSettings(options.create_overrides);

    // List the files up front: the archive's max file count is fixed at creation
    std::vector<fs::path> files;
    const bool is_directory = fs::is_directory(options.target, ec);
    if (is_directory) {
        files = ListFilesRecursive(options.target, ec);
        if (ec) {
            err << "[!] Failed to list directory: (" << ec.value() << ") " << ec.message() << ": "
                << options.target << std::endl;
            return false;
        }
    } else if (!fs::is_regular_file(options.target, ec)) {
        err << "[!] Not a file or directory: " << options.target << std::endl;
        return false;
    }
    const uint32_t file_count =
        CalculateMpqMaxFileValue(is_directory ? static_cast<uint32_t>(files.size()) : 1);

    // Create the MPQ archive and add files
    Archive archive = Archive::Create(output_file, file_count, game_rules);
    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;

    int result = 0;
    if (is_directory) {
        const std::string prefix = options.path.value_or("");
        result |= AddFiles(archive.Handle(), files, options.target, prefix, lcid, game_rules, out,
                           err, options.compression_overrides);
    } else {
        std::string archive_path = ResolveArchiveName(options.target, options.path);
        result |= AddFile(archive.Handle(), options.target, archive_path, lcid, game_rules, out,
                          err, options.compression_overrides);
    }

    if (options.sign) {
        archive.Sign();
    }
    archive.Close();

    return result == 0;
}

} // namespace mpqcli
