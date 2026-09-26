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
#include "mpq/add.h"
#include "mpq/archive.h"
#include "util/capacity.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

int HandleCreate(const std::string &target, const std::optional<std::string> &path,
                 const std::optional<std::string> &output, bool sign_archive,
                 const std::optional<std::string> &locale,
                 const std::optional<std::string> &game_profile, int32_t mpq_version,
                 int64_t stream_flags, int64_t sector_size, int64_t raw_chunk_size,
                 int64_t file_flags1, int64_t file_flags2, int64_t file_flags3, int64_t attr_flags,
                 int64_t file_flags, int64_t file_compression, int64_t file_compression_next,
                 std::ostream &out, std::ostream &err) {
    std::error_code ec;
    fs::path output_file_path;
    if (output.has_value()) {
        output_file_path = fs::absolute(output.value(), ec);
        if (ec) {
            err << "[!] Failed to resolve output path: (" << ec.value() << ") " << ec.message()
                << ": " << output.value() << std::endl;
            return 1;
        }
    } else {
        output_file_path = fs::path(target);
        // If the path ends with a separator (e.g. "dir/"), strip the
        // trailing separator first so we get "dir.mpq"
        if (output_file_path.filename().empty()) {
            output_file_path = output_file_path.parent_path();
        }
        output_file_path.replace_extension(".mpq");
    }
    std::string output_file = output_file_path.u8string();

    GameProfile profile;
    if (game_profile.has_value()) {
        profile = GameRules::StringToProfile(game_profile.value());
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules game_rules(profile);

    out << "[*] Game profile: " << game_profile.value_or("default")
        << ", Output file: " << output_file << std::endl;

    if (mpq_version > 0) {
        mpq_version--; // We label versions 1-4, but StormLib uses 0-3
    }

    // Apply MpqCreateSettings overrides if provided
    MpqCreateSettingsOverrides overrides;
    if (mpq_version >= 0)
        overrides.mpq_version = static_cast<DWORD>(mpq_version);
    if (stream_flags >= 0)
        overrides.stream_flags = static_cast<DWORD>(stream_flags);
    if (file_flags1 >= 0)
        overrides.file_flags1 = static_cast<DWORD>(file_flags1);
    if (file_flags2 >= 0)
        overrides.file_flags2 = static_cast<DWORD>(file_flags2);
    if (file_flags3 >= 0)
        overrides.file_flags3 = static_cast<DWORD>(file_flags3);
    if (attr_flags >= 0)
        overrides.attr_flags = static_cast<DWORD>(attr_flags);
    if (sector_size >= 0)
        overrides.sector_size = static_cast<DWORD>(sector_size);
    if (raw_chunk_size >= 0)
        overrides.raw_chunk_size = static_cast<DWORD>(raw_chunk_size);
    game_rules.OverrideCreateSettings(overrides);

    // List the files up front: the archive's max file count is fixed at creation
    std::vector<fs::path> files;
    const bool is_directory = fs::is_directory(target, ec);
    if (is_directory) {
        files = ListFilesRecursive(target, ec);
        if (ec) {
            err << "[!] Failed to list directory: (" << ec.value() << ") " << ec.message() << ": "
                << target << std::endl;
            return 1;
        }
    } else if (!fs::is_regular_file(target, ec)) {
        err << "[!] Not a file or directory: " << target << std::endl;
        return 1;
    }
    const uint32_t file_count =
        CalculateMpqMaxFileValue(is_directory ? static_cast<uint32_t>(files.size()) : 1);

    // Create the MPQ archive and add files
    Archive archive = Archive::Create(output_file, file_count, game_rules);
    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;

    // Apply AddFileSettings overrides if provided
    CompressionSettingsOverrides add_overrides;
    if (file_flags >= 0)
        add_overrides.flags = static_cast<DWORD>(file_flags);
    if (file_compression >= 0)
        add_overrides.compression = static_cast<DWORD>(file_compression);
    if (file_compression_next >= 0)
        add_overrides.compression_next = static_cast<DWORD>(file_compression_next);

    int result = 0;
    if (is_directory) {
        const std::string prefix = path.value_or("");
        result |= AddFiles(archive.Handle(), files, target, prefix, lcid, game_rules, out, err,
                           add_overrides);
    } else {
        std::string archive_path = ResolveArchiveName(target, path);
        result |= AddFile(archive.Handle(), target, archive_path, lcid, game_rules, out, err,
                          add_overrides);
    }

    if (sign_archive) {
        archive.Sign();
    }
    archive.Close();

    return result;
}

} // namespace mpqcli
