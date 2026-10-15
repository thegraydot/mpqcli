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
    if (options.files.empty()) {
        throw Error("No files or directories to add");
    }

    std::error_code ec;
    const fs::path output_file_path = fs::absolute(options.archive, ec);
    if (ec) {
        throw FileError("Failed to resolve archive path", options.archive, ec);
    }
    if (fs::exists(output_file_path, ec)) {
        throw ArchiveError("File already exists: " + output_file_path.string());
    }

    // Count every input up front: the archive's max file count is fixed at creation,
    // and an input that cannot be read fails here, before the archive exists
    uint32_t input_file_count = 0;
    for (const auto &f : options.files) {
        if (!fs::exists(f, ec)) {
            throw FileError("Path does not exist", f);
        }
        if (fs::is_directory(f, ec)) {
            const std::vector<fs::path> directory_files = ListFilesRecursive(f, ec);
            if (ec) {
                throw FileError("Failed to list directory", f, ec);
            }
            input_file_count += static_cast<uint32_t>(directory_files.size());
        } else if (fs::is_regular_file(f, ec)) {
            input_file_count++;
        } else {
            throw FileError("Not a file or directory", f);
        }
    }
    const uint32_t file_count = CalculateMpqMaxFileValue(input_file_count);

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

    int files_skipped = 0;
    const int result =
        AddInputs(archive.Handle(), partial_path, options.files, options.path, lcid, game_rules,
                  err, cancelled, options.compression_overrides, false, false, &files_skipped);
    if (files_skipped > 0) {
        err << "[*] " << files_skipped
            << " file(s) were skipped because an earlier input added a file by the same name"
            << std::endl;
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
