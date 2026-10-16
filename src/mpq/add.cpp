#include "mpq/add.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "errors.h"
#include "gamerules/rules.h"
#include "mpq/query.h"
#include "util/capacity.h"
#include "util/format.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

namespace {

/// Whether file is the archive's own file, reporting the skip when it is
bool SkipIfArchiveFile(const fs::path &file, const fs::path &archive_file, std::ostream &err) {
    // Compared by identity rather than by path, so a symlink to the archive or another
    // spelling of its path is caught too
    std::error_code ec;
    if (!fs::equivalent(file, archive_file, ec)) {
        return false;
    }
    err << "[*] Skipping the archive itself: " << file.string() << std::endl;
    return true;
}

/// Doubles the archive's file table, reporting a failure on err
bool GrowFileTable(HANDLE archive, std::ostream &err) {
    const auto max_files = GetFileInfo<uint32_t>(archive, SFileMpqMaxFileCount);
    const uint32_t new_max_files = NextPowerOfTwo(max_files + 1);
    if (!SFileSetMaxFileCount(archive, new_max_files)) {
        const auto error = SErrGetLastError();
        err << "[!] Failed to increase new max file count to " << new_max_files << ": (" << error
            << ") " << StormErrorString(error) << std::endl;
        return false;
    }
    return true;
}

} // namespace

int AddFiles(HANDLE archive, const std::vector<fs::path> &files, const fs::path &base_path,
             const std::string &path_prefix, LCID locale, const GameRules &game_rules,
             std::ostream &err, const std::atomic<bool> &cancelled,
             const CompressionSettingsOverrides &overrides, bool overwrite, bool update,
             int *skipped) {
    int files_added = 0;
    int files_skipped = 0;
    int files_failed = 0;

    for (const auto &file : files) {
        ThrowIfCancelled(cancelled);

        // Determine relative path lexically rather than with fs::relative, which
        // resolves paths through the OS and throws on volumes that cannot report
        // real paths (RAM disks, some network shares).
        fs::path input_file_path = file.lexically_relative(base_path);
        std::string archive_file_path;

        if (path_prefix.empty()) {
            archive_file_path = WindowsifyFilePath(input_file_path.u8string());
        } else {
            archive_file_path =
                WindowsifyFilePath((fs::path(path_prefix) / input_file_path).u8string());
        }

        if (std::find(special_mpq_files.begin(), special_mpq_files.end(), archive_file_path) !=
            special_mpq_files.end()) {
            err << "[*] Skipping special MPQ file: " << archive_file_path << std::endl;
            continue;
        }

        int file_skipped = 0;
        if (AddFile(archive, file, archive_file_path, locale, game_rules, err, overrides, overwrite,
                    update, &file_skipped) != 0) {
            files_failed++;
        } else if (file_skipped > 0) {
            files_skipped++;
        } else {
            files_added++;
        }
    }

    if (update) {
        err << "[*] For " << base_path.string() << ": " << files_added << " files added, "
            << files_skipped << " files skipped, " << files_failed << " files failed." << std::endl;
    }

    if (skipped != nullptr) {
        *skipped += files_skipped;
    }

    return files_failed > 0 ? 1 : 0;
}

int AddFile(HANDLE archive, const fs::path &local_file, const std::string &archive_file_path,
            const LCID locale, const GameRules &game_rules, std::ostream &err,
            const CompressionSettingsOverrides &overrides, bool overwrite, bool update,
            int *skipped) {
    std::error_code ec;
    if (!fs::exists(local_file, ec)) {
        err << "[!] File doesn't exist on disk: " << local_file.string() << std::endl;
        return 1;
    }

    SFileSetLocale(locale);
    HANDLE file;
    if (SFileOpenFileEx(archive, archive_file_path.c_str(), SFILE_OPEN_FROM_MPQ, &file)) {
        const auto file_locale = GetFileInfo<LCID>(file, SFileInfoLocale);
        if (file_locale == locale) {
            // --update: leave the archived copy alone while it still matches the local file
            std::string match_reason;
            const bool unchanged =
                update && ArchivedFileMatches(archive, file, local_file, match_reason);
            SFileCloseFile(file);

            if (unchanged) {
                err << "[*] Skipping unchanged file: " << archive_file_path << " (" << match_reason
                    << ")" << std::endl;
                if (skipped != nullptr) {
                    (*skipped)++;
                }
                return 0;
            }

            // Without either flag, leaving the archived copy in place is the documented
            // default, so this is a skip rather than a failure.
            if (!overwrite && !update) {
                err << "[!] File" << PrettyPrintLocale(locale, " for locale ")
                    << " already exists in MPQ archive: " << archive_file_path << " - Skipping..."
                    << std::endl;
                if (skipped != nullptr) {
                    (*skipped)++;
                }
                return 0;
            }

            err << "[~] File" << PrettyPrintLocale(locale, " for locale ")
                << " already exists in MPQ archive: " << archive_file_path << " - Overwriting..."
                << std::endl;
        } else {
            SFileCloseFile(file);
        }
    }
    err << "[+] Adding file" << PrettyPrintLocale(locale, " for locale ") << ": "
        << archive_file_path << std::endl;

    const std::uintmax_t raw_file_size = fs::file_size(local_file, ec);
    if (ec) {
        err << "[!] Failed to read file size: (" << ec.value() << ") " << ec.message() << ": "
            << local_file.string() << std::endl;
        return 1;
    }
    if (raw_file_size > std::numeric_limits<DWORD>::max()) {
        err << "[!] File exceeds 4GB, size-based compression rules may not apply "
               "correctly: "
            << local_file.string() << std::endl;
    }
    const DWORD file_size = static_cast<DWORD>(
        std::min(raw_file_size, static_cast<std::uintmax_t>(std::numeric_limits<DWORD>::max())));

    const auto settings = game_rules.GetCompressionSettings(archive_file_path, file_size);

    DWORD flags = overrides.flags.value_or(settings.mpq_flags);
    DWORD compression = overrides.compression.value_or(settings.compression_first);
    DWORD compression_next = overrides.compression_next.value_or(settings.compression_next);

    // Both flags mean "replace what is already there"; --update has simply decided
    // beforehand that this particular file is worth replacing.
    if (overwrite || update) {
        flags |= MPQ_FILE_REPLACEEXISTING;
    }

    const auto add = [&]() {
        return SFileAddFileEx(archive, local_file.string().c_str(), archive_file_path.c_str(),
                              flags, compression, compression_next);
    };

    // The table grows once StormLib reports it full, not when the file count reaches
    // the maximum: while the archive is open StormLib holds back a slot for each special
    // file it writes on close, and does not report how many, so a count comparison
    // fires a few files too late. A full table fails before any data is written, so
    // the add is safe to retry.
    bool added_file = add();
    if (!added_file && SErrGetLastError() == ERROR_DISK_FULL) {
        if (!GrowFileTable(archive, err)) {
            return 1;
        }
        added_file = add();
    }

    if (!added_file) {
        const auto error = SErrGetLastError();
        err << "[!] Failed to add: " << archive_file_path << ": (" << error << ") "
            << StormErrorString(error) << std::endl;
        return 1;
    }

    return 0;
}

int AddInputs(HANDLE archive, const fs::path &archive_file, const std::vector<fs::path> &inputs,
              const std::optional<std::string> &path, LCID locale, const GameRules &game_rules,
              std::ostream &err, const std::atomic<bool> &cancelled,
              const CompressionSettingsOverrides &overrides, bool overwrite, bool update,
              int *skipped) {
    std::error_code ec;
    bool has_directory = false;
    for (const auto &f : inputs) {
        if (fs::is_directory(f, ec)) {
            has_directory = true;
            break;
        }
    }

    int result = 0;
    for (const auto &f : inputs) {
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
            // A directory can hold the archive being written, as "create out.mpq ." does
            const auto skip_archive = [&](const fs::path &file) {
                return SkipIfArchiveFile(file, archive_file, err);
            };
            directory_files.erase(
                std::remove_if(directory_files.begin(), directory_files.end(), skip_archive),
                directory_files.end());
            std::string prefix = path.value_or("");
            result |= AddFiles(archive, directory_files, f, prefix, locale, game_rules, err,
                               cancelled, overrides, overwrite, update, skipped);

        } else if (fs::is_regular_file(f, ec)) {
            if (SkipIfArchiveFile(f, archive_file, err)) {
                continue;
            }
            const bool treat_as_directory = has_directory || inputs.size() > 1;
            std::string archive_path = ResolveArchiveName(f, path, treat_as_directory);
            result |= AddFile(archive, f, archive_path, locale, game_rules, err, overrides,
                              overwrite, update, skipped);

        } else {
            err << "[!] Not a file or directory: " << f.string() << std::endl;
            result |= 1;
        }
    }

    return result;
}

} // namespace mpqcli
