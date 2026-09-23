#include "mpq/add.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "gamerules/rules.h"
#include "mpq/query.h"
#include "util/capacity.h"
#include "util/format.h"
#include "util/locales.h"
#include "util/path.h"

namespace mpqcli {

namespace fs = std::filesystem;

int AddFiles(HANDLE archive, const std::vector<fs::path> &files, const fs::path &base_path,
             const std::string &path_prefix, LCID locale, const GameRules &game_rules,
             const CompressionSettingsOverrides &overrides, bool overwrite, bool update,
             int *skipped) {
    int files_added = 0;
    int files_skipped = 0;
    int files_failed = 0;

    for (const auto &file : files) {
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
            std::cout << "[*] Skipping special MPQ file: " << archive_file_path << std::endl;
            continue;
        }

        int file_skipped = 0;
        if (AddFile(archive, file, archive_file_path, locale, game_rules, overrides, overwrite,
                    update, &file_skipped) != 0) {
            files_failed++;
        } else if (file_skipped > 0) {
            files_skipped++;
        } else {
            files_added++;
        }
    }

    if (update) {
        std::cout << "[*] For " << base_path.u8string() << ": " << files_added << " files added, "
                  << files_skipped << " files skipped, " << files_failed << " files failed."
                  << std::endl;
    }

    if (skipped != nullptr) {
        *skipped += files_skipped;
    }

    return files_failed > 0 ? 1 : 0;
}
int AddFile(HANDLE archive, const fs::path &local_file, const std::string &archive_file_path,
            const LCID locale, const GameRules &game_rules,
            const CompressionSettingsOverrides &overrides, bool overwrite, bool update,
            int *skipped) {
    // Return if file doesn't exist on disk
    std::error_code ec;
    if (!fs::exists(local_file, ec)) {
        std::cerr << "[!] File doesn't exist on disk: " << local_file << std::endl;
        return 1;
    }

    // Check if file exists in MPQ archive
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
                std::cout << "[~] Skipping unchanged file: " << archive_file_path << " ("
                          << match_reason << ")" << std::endl;
                if (skipped != nullptr) {
                    (*skipped)++;
                }
                return 0;
            }

            // Without either flag, leaving the archived copy in place is the documented
            // default, so this is a skip rather than a failure.
            if (!overwrite && !update) {
                std::cerr << "[!] File" << PrettyPrintLocale(locale, " for locale ")
                          << " already exists in MPQ archive: " << archive_file_path
                          << " - Skipping..." << std::endl;
                if (skipped != nullptr) {
                    (*skipped)++;
                }
                return 0;
            }

            std::cout << "[+] File" << PrettyPrintLocale(locale, " for locale ")
                      << " already exists in MPQ archive: " << archive_file_path
                      << " - Overwriting..." << std::endl;
        } else {
            SFileCloseFile(file);
        }
    }
    std::cout << "[+] Adding file" << PrettyPrintLocale(locale, " for locale ") << ": "
              << archive_file_path << std::endl;

    // Verify that we are not exceeding maxFile size of the archive, and if we do, increase it
    int32_t number_of_files = GetFileInfo<int32_t>(archive, SFileMpqNumberOfFiles);
    int32_t max_files = GetFileInfo<int32_t>(archive, SFileMpqMaxFileCount);

    if (number_of_files + 1 > max_files) {
        uint32_t new_max_files = NextPowerOfTwo(static_cast<uint32_t>(number_of_files + 1));
        bool set_max_file_count = SFileSetMaxFileCount(archive, new_max_files);
        if (!set_max_file_count) {
            const auto error = SErrGetLastError();
            std::cerr << "[!] Failed to increase new max file count to " << new_max_files << ": ("
                      << error << ") " << StormErrorString(error) << std::endl;
            return 1;
        }
    }

    // Get file size for rule matching
    const std::uintmax_t raw_file_size = fs::file_size(local_file, ec);
    if (ec) {
        std::cerr << "[!] Failed to read file size: (" << ec.value() << ") " << ec.message() << ": "
                  << local_file << std::endl;
        return 1;
    }
    if (raw_file_size > std::numeric_limits<DWORD>::max()) {
        std::cerr << "[!] Warning: file exceeds 4GB, size-based compression rules may not apply "
                     "correctly: "
                  << local_file << std::endl;
    }
    const DWORD file_size = static_cast<DWORD>(
        std::min(raw_file_size, static_cast<std::uintmax_t>(std::numeric_limits<DWORD>::max())));

    // Get game-specific rules
    const auto settings = game_rules.GetCompressionSettings(archive_file_path, file_size);

    // Apply overrides where specified, otherwise use game rules
    DWORD flags = overrides.flags.value_or(settings.mpq_flags);
    DWORD compression = overrides.compression.value_or(settings.compression_first);
    DWORD compression_next = overrides.compression_next.value_or(settings.compression_next);

    // Both flags mean "replace what is already there"; --update has simply decided
    // beforehand that this particular file is worth replacing.
    if (overwrite || update) {
        flags |= MPQ_FILE_REPLACEEXISTING;
    }

    bool added_file =
        SFileAddFileEx(archive, local_file.u8string().c_str(), archive_file_path.c_str(), flags,
                       compression, compression_next);

    if (!added_file) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to add: " << archive_file_path << ": (" << error << ") "
                  << StormErrorString(error) << std::endl;
        return 1;
    }

    return 0;
}

} // namespace mpqcli
