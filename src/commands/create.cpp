#include "commands/create.h"

#include <atomic>
#include <cstdint>
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
#include "util/capacity.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

namespace {

/// Removes the archive being written on the way out unless it was moved into place
struct PartialArchive {
    fs::path path;
    bool keep = false;

    ~PartialArchive() {
        if (!keep) {
            std::error_code ec;
            fs::remove(path, ec);
        }
    }
};

} // namespace

bool Create(const CreateOptions &options, std::ostream &err, const std::atomic<bool> &cancelled) {
    std::error_code ec;
    fs::path output_file_path;
    if (options.output.has_value()) {
        output_file_path = fs::absolute(options.output.value(), ec);
        if (ec) {
            throw FileError("Failed to resolve output path", options.output.value(), ec);
        }
    } else {
        output_file_path = options.target;
        // If the path ends with a separator (e.g. "dir/"), strip the
        // trailing separator first so we get "dir.mpq"
        if (output_file_path.filename().empty()) {
            output_file_path = output_file_path.parent_path();
        }
        output_file_path.replace_extension(".mpq");
    }
    if (fs::exists(output_file_path, ec)) {
        throw ArchiveError("File already exists: " + output_file_path.string());
    }

    GameProfile profile;
    if (options.game_profile.has_value()) {
        profile = GameRules::StringToProfile(options.game_profile.value());
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules game_rules(profile);

    err << "[*] Game profile: " << options.game_profile.value_or("default")
        << ", Output file: " << output_file_path.string() << std::endl;

    game_rules.OverrideCreateSettings(options.create_overrides);

    // List the files up front: the archive's max file count is fixed at creation
    std::vector<fs::path> files;
    const bool is_directory = fs::is_directory(options.target, ec);
    if (is_directory) {
        files = ListFilesRecursive(options.target, ec);
        if (ec) {
            throw FileError("Failed to list directory", options.target, ec);
        }
    } else if (!fs::is_regular_file(options.target, ec)) {
        throw FileError("Not a file or directory", options.target);
    }
    const uint32_t file_count =
        CalculateMpqMaxFileValue(is_directory ? static_cast<uint32_t>(files.size()) : 1);

    // The archive is invalid until it is closed, so it is written beside the output
    // and renamed into place once complete; an interrupt or error before that leaves
    // nothing at the output path. A partial left by an earlier interrupted run would
    // otherwise be refused as an existing file.
    fs::path partial_path = output_file_path;
    partial_path += ".partial";
    fs::remove(partial_path, ec);
    PartialArchive partial{partial_path};

    Archive archive = Archive::Create(partial_path, file_count, game_rules);
    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;

    int result = 0;
    if (is_directory) {
        const std::string prefix = options.path.value_or("");
        result |= AddFiles(archive.Handle(), files, options.target, prefix, lcid, game_rules, err,
                           cancelled, options.compression_overrides);
    } else {
        std::string archive_path = ResolveArchiveName(options.target, options.path);
        result |= AddFile(archive.Handle(), options.target, archive_path, lcid, game_rules, err,
                          options.compression_overrides);
    }

    if (options.sign) {
        archive.Sign();
    }
    archive.Close();

    // A failed rename must not delete the complete archive it was about to move
    partial.keep = true;
    fs::rename(partial_path, output_file_path, ec);
    if (ec) {
        throw FileError("Failed to move " + partial_path.string() + " into place", output_file_path,
                        ec);
    }

    return result == 0;
}

} // namespace mpqcli
