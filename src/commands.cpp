#include "commands.h"

#include <filesystem>
#include <iostream>
#include <unordered_set>

#include <StormLib.h>

#include "gamerules.h"
#include "helpers.h"
#include "locales.h"
#include "mpq.h"
#include "mpqcli.h"

namespace fs = std::filesystem;

std::string ResolveArchiveName(const std::string &f, const std::optional<std::string> &path,
                               const bool treat_as_directory = false) {
    fs::path file_path = path.value_or(fs::path(f).filename().u8string());
    if (treat_as_directory) {
        const std::string filename = fs::path(f).filename().u8string();
        file_path = path.value_or("") / fs::path(filename);
    }
    return WindowsifyFilePath(file_path);
}

int HandleVersion() {
    std::cout << MPQCLI_VERSION << "-" << GIT_COMMIT_HASH << std::endl;
    return 0;
}

int HandleAbout() {
    std::cout << "Name: mpqcli" << std::endl;
    std::cout << "Version: " << MPQCLI_VERSION << "-" << GIT_COMMIT_HASH << std::endl;
    std::cout << "Author: Thomas Laurenson" << std::endl;
    std::cout << "License: MIT" << std::endl;
    std::cout << "GitHub: https://github.com/thegraydot/mpqcli" << std::endl;
    std::cout << "Dependencies:" << std::endl;
    std::cout << " - StormLib (https://github.com/ladislav-zezula/StormLib)" << std::endl;
    std::cout << " - CLI11 (https://github.com/CLIUtils/CLI11)" << std::endl;
    return 0;
}

int HandleInfo(const std::string &target, const std::optional<std::string> &property) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }
    PrintMpqInfo(archive, property);
    CloseMpqArchive(archive);
    return 0;
}

int HandleCreate(const std::string &target, const std::optional<std::string> &path,
                 const std::optional<std::string> &output, bool sign_archive,
                 const std::optional<std::string> &locale,
                 const std::optional<std::string> &game_profile, int32_t mpq_version,
                 int64_t stream_flags, int64_t sector_size, int64_t raw_chunk_size,
                 int64_t file_flags1, int64_t file_flags2, int64_t file_flags3, int64_t attr_flags,
                 int64_t file_flags, int64_t file_compression, int64_t file_compression_next) {
    fs::path output_file_path;
    if (output.has_value()) {
        output_file_path = fs::absolute(output.value());
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

    std::cout << "[*] Game profile: " << game_profile.value_or("default")
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

    // Determine the number of files we are going to add
    uint32_t file_count = CalculateMpqMaxFileValue(target);

    // Create the MPQ archive and add files
    int result = 0;
    HANDLE archive = CreateMpqArchive(output_file, file_count, game_rules);
    if (archive) {
        LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;

        // Apply AddFileSettings overrides if provided
        CompressionSettingsOverrides add_overrides;
        if (file_flags >= 0)
            add_overrides.flags = static_cast<DWORD>(file_flags);
        if (file_compression >= 0)
            add_overrides.compression = static_cast<DWORD>(file_compression);
        if (file_compression_next >= 0)
            add_overrides.compression_next = static_cast<DWORD>(file_compression_next);

        if (fs::is_directory(target)) {
            const std::string prefix = path.value_or("");
            result |= AddFiles(archive, target, prefix, lcid, game_rules, add_overrides);

        } else if (fs::is_regular_file(target)) {
            std::string archive_path = ResolveArchiveName(target, path);
            result |= AddFile(archive, target, archive_path, lcid, game_rules, add_overrides);

        } else {
            std::cerr << "[!] Not a file or directory: " << target << std::endl;
            result |= 1;
        }

        if (sign_archive) {
            SignMpqArchive(archive);
        }
        CloseMpqArchive(archive);
    } else {
        std::cerr << "[!] Failed to create MPQ archive." << std::endl;
        return 1;
    }

    return result;
}

int HandleAdd(const std::vector<std::string> &files, const std::string &target,
              const std::optional<std::string> &path, bool overwrite, bool update,
              const std::optional<std::string> &locale,
              const std::optional<std::string> &game_profile, int64_t file_flags,
              int64_t file_compression, int64_t file_compression_next) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, 0)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;

    GameProfile profile;
    if (game_profile.has_value()) {
        profile = GameRules::StringToProfile(game_profile.value());
        std::cout << "[*] Using game profile: " << game_profile.value() << std::endl;
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

    bool has_directory = false;
    for (const auto &f : files) {
        if (fs::is_directory(f)) {
            has_directory = true;
            break;
        }
    }

    if (update && !has_directory) {
        std::cerr << "[!] Warning: --update is only meaningful when adding a directory"
                  << std::endl;
    }

    int result = 0;
    for (const auto &f : files) {
        if (!fs::exists(f)) {
            std::cerr << "[!] Path does not exist: " << f << std::endl;
            continue;
        }

        if (fs::is_directory(f)) {
            std::string prefix = path.value_or("");
            result |=
                AddFiles(archive, f, prefix, lcid, game_rules, add_overrides, overwrite, update);

        } else if (fs::is_regular_file(f)) {
            const bool treat_as_directory = has_directory || files.size() > 1;
            std::string archive_path = ResolveArchiveName(f, path, treat_as_directory);
            result |= AddFile(archive, f, archive_path, lcid, game_rules, add_overrides, overwrite);

        } else {
            std::cerr << "[!] Not a file or directory: " << f << std::endl;
        }
    }

    CloseMpqArchive(archive);
    return result;
}

int HandleRemove(const std::vector<std::string> &files, const std::string &target,
                 const std::optional<std::string> &locale) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, 0)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    std::unordered_set<std::string> seen;
    int overall_result = 0;
    for (const auto &f : files) {
        if (!seen.insert(f).second) {
            continue;
        }
        int result = RemoveFile(archive, f, lcid);
        if (result != 0) {
            overall_result = result;
        }
    }
    CloseMpqArchive(archive);
    return overall_result;
}

int HandleList(const std::string &target, const std::optional<std::string> &listfile_name,
               bool list_all, bool list_detailed, const std::vector<std::string> &properties) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }
    ListFiles(archive, listfile_name, list_all, list_detailed, properties);
    CloseMpqArchive(archive);
    return 0;
}

int HandleExtract(const std::string &target, const std::optional<std::string> &output,
                  const std::optional<std::string> &file, bool keep_folder_structure,
                  const std::optional<std::string> &listfile_name,
                  const std::optional<std::string> &locale) {
    // If no output directory specified, use MPQ path without extension
    // If output directory specified, create it if it doesn't exist
    std::string effective_output;
    if (!output.has_value()) {
        fs::path output_path_absolute = fs::canonical(target);
        fs::path output_path = output_path_absolute.parent_path() / output_path_absolute.stem();
        effective_output = output_path.u8string();
    } else {
        effective_output = output.value();
    }
    if (!fs::create_directory(effective_output) && !fs::is_directory(effective_output)) {
        std::cerr << "[!] Failed to create output directory: " << effective_output << std::endl;
        return 1;
    }

    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    if (locale.has_value() && lcid == default_locale) {
        std::cout << "[!] Warning: The locale '" << locale.value()
                  << "' is unknown. Will use default locale instead." << std::endl;
    }

    int result;
    if (file.has_value()) {
        result = ExtractFile(archive, effective_output, file.value(), keep_folder_structure, lcid);
    } else {
        result = ExtractFiles(archive, effective_output, listfile_name, lcid);
    }
    CloseMpqArchive(archive);

    if (result != 0) {
        std::cerr << std::endl << "[!] Failed to extract all files." << std::endl;
    }
    return result;
}

int HandleRead(const std::string &file, const std::string &target,
               const std::optional<std::string> &locale) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    if (locale.has_value() && lcid == default_locale) {
        std::cout << "[!] Warning: The locale '" << locale.value()
                  << "' is unknown. Will use default locale instead." << std::endl;
    }

    uint32_t file_size;
    auto file_content = ReadFile(archive, file.c_str(), &file_size, lcid);
    if (!file_content) {
        CloseMpqArchive(archive);
        return 1;
    }

    PrintAsBinary(file_content.get(), file_size);

    CloseMpqArchive(archive);
    return 0;
}

int HandleVerify(const std::string &target, bool print_signature) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    int result = 0;
    uint32_t verify_result = VerifyMpqArchive(archive);
    if (verify_result == ERROR_WEAK_SIGNATURE_OK || verify_result == ERROR_STRONG_SIGNATURE_OK ||
        verify_result == ERROR_WEAK_SIGNATURE_ERROR ||
        verify_result == ERROR_STRONG_SIGNATURE_ERROR) {
        if (print_signature) {
            // If printing the signature, don't print success message
            // because the user might want to pipe/redirect the signature data
            PrintMpqSignature(archive, target);
        } else {
            // Just print verification success
            std::cout << "[*] Verify success" << std::endl;
        }
        result = 0;
    } else {
        // Any other verify result is no signature, or error verifying
        std::cout << "[!] Verify failed" << std::endl;
        result = 1;
    }
    CloseMpqArchive(archive);
    return result;
}

int HandleCompact(const std::string &target, const std::optional<std::string> &listfile_name) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, 0)) {
        return 1;
    }

    const int result = CompactMpqArchive(archive, listfile_name);
    CloseMpqArchive(archive);
    return result;
}
