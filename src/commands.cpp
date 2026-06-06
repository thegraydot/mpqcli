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
                               const bool treatAsDirectory = false) {
    fs::path filePath = path.value_or(fs::path(f).filename().u8string());
    if (treatAsDirectory) {
        const std::string filename = fs::path(f).filename().u8string();
        filePath = path.value_or("") / fs::path(filename);
    }
    return WindowsifyFilePath(filePath);
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
    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }
    PrintMpqInfo(hArchive, property);
    CloseMpqArchive(hArchive);
    return 0;
}

int HandleCreate(const std::string &target, const std::optional<std::string> &path,
                 const std::optional<std::string> &output, bool signArchive,
                 const std::optional<std::string> &locale,
                 const std::optional<std::string> &gameProfile, int32_t mpqVersion,
                 int64_t streamFlags, int64_t sectorSize, int64_t rawChunkSize, int64_t fileFlags1,
                 int64_t fileFlags2, int64_t fileFlags3, int64_t attrFlags, int64_t fileDwFlags,
                 int64_t fileDwCompression, int64_t fileDwCompressionNext) {
    fs::path outputFilePath;
    if (output.has_value()) {
        outputFilePath = fs::absolute(output.value());
    } else {
        outputFilePath = fs::path(target);
        // If the path ends with a separator (e.g. "dir/"), strip the
        // trailing separator first so we get "dir.mpq"
        if (outputFilePath.filename().empty()) {
            outputFilePath = outputFilePath.parent_path();
        }
        outputFilePath.replace_extension(".mpq");
    }
    std::string outputFile = outputFilePath.u8string();

    GameProfile profile;
    if (gameProfile.has_value()) {
        profile = GameRules::StringToProfile(gameProfile.value());
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules gameRules(profile);

    std::cout << "[*] Game profile: " << gameProfile.value_or("default")
              << ", Output file: " << outputFile << std::endl;

    if (mpqVersion > 0) {
        mpqVersion--;  // We label versions 1-4, but StormLib uses 0-3
    }

    // Apply MpqCreateSettings overrides if provided
    MpqCreateSettingsOverrides overrides;
    if (mpqVersion >= 0) overrides.mpqVersion = static_cast<DWORD>(mpqVersion);
    if (streamFlags >= 0) overrides.streamFlags = static_cast<DWORD>(streamFlags);
    if (fileFlags1 >= 0) overrides.fileFlags1 = static_cast<DWORD>(fileFlags1);
    if (fileFlags2 >= 0) overrides.fileFlags2 = static_cast<DWORD>(fileFlags2);
    if (fileFlags3 >= 0) overrides.fileFlags3 = static_cast<DWORD>(fileFlags3);
    if (attrFlags >= 0) overrides.attrFlags = static_cast<DWORD>(attrFlags);
    if (sectorSize >= 0) overrides.sectorSize = static_cast<DWORD>(sectorSize);
    if (rawChunkSize >= 0) overrides.rawChunkSize = static_cast<DWORD>(rawChunkSize);
    gameRules.OverrideCreateSettings(overrides);

    // Determine the number of files we are going to add
    uint32_t fileCount = CalculateMpqMaxFileValue(target);

    // Create the MPQ archive and add files
    int result = 0;
    HANDLE hArchive = CreateMpqArchive(outputFile, fileCount, gameRules);
    if (hArchive) {
        LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : defaultLocale;

        // Apply AddFileSettings overrides if provided
        CompressionSettingsOverrides addOverrides;
        if (fileDwFlags >= 0) addOverrides.dwFlags = static_cast<DWORD>(fileDwFlags);
        if (fileDwCompression >= 0)
            addOverrides.dwCompression = static_cast<DWORD>(fileDwCompression);
        if (fileDwCompressionNext >= 0)
            addOverrides.dwCompressionNext = static_cast<DWORD>(fileDwCompressionNext);

        if (fs::is_directory(target)) {
            const std::string prefix = path.value_or("");
            result |= AddFiles(hArchive, target, prefix, lcid, gameRules, addOverrides);

        } else if (fs::is_regular_file(target)) {
            std::string archivePath = ResolveArchiveName(target, path);
            result |= AddFile(hArchive, target, archivePath, lcid, gameRules, addOverrides);

        } else {
            std::cerr << "[!] Not a file or directory: " << target << std::endl;
            result |= 1;
        }

        if (signArchive) {
            SignMpqArchive(hArchive);
        }
        CloseMpqArchive(hArchive);
    } else {
        std::cerr << "[!] Failed to create MPQ archive." << std::endl;
        return 1;
    }

    return result;
}

int HandleAdd(const std::vector<std::string> &files, const std::string &target,
              const std::optional<std::string> &path, bool overwrite, bool update,
              const std::optional<std::string> &locale,
              const std::optional<std::string> &gameProfile, int64_t fileDwFlags,
              int64_t fileDwCompression, int64_t fileDwCompressionNext) {
    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, 0)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : defaultLocale;

    GameProfile profile;
    if (gameProfile.has_value()) {
        profile = GameRules::StringToProfile(gameProfile.value());
        std::cout << "[*] Using game profile: " << gameProfile.value() << std::endl;
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules gameRules(profile);

    CompressionSettingsOverrides addOverrides;
    if (fileDwFlags >= 0) addOverrides.dwFlags = static_cast<DWORD>(fileDwFlags);
    if (fileDwCompression >= 0) addOverrides.dwCompression = static_cast<DWORD>(fileDwCompression);
    if (fileDwCompressionNext >= 0)
        addOverrides.dwCompressionNext = static_cast<DWORD>(fileDwCompressionNext);

    bool hasDirectory = false;
    for (const auto &f : files) {
        if (fs::is_directory(f)) {
            hasDirectory = true;
            break;
        }
    }

    if (update && !hasDirectory) {
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
                AddFiles(hArchive, f, prefix, lcid, gameRules, addOverrides, overwrite, update);

        } else if (fs::is_regular_file(f)) {
            const bool treatAsDirectory = hasDirectory || files.size() > 1;
            std::string archivePath = ResolveArchiveName(f, path, treatAsDirectory);
            result |= AddFile(hArchive, f, archivePath, lcid, gameRules, addOverrides, overwrite);

        } else {
            std::cerr << "[!] Not a file or directory: " << f << std::endl;
        }
    }

    CloseMpqArchive(hArchive);
    return result;
}

int HandleRemove(const std::vector<std::string> &files, const std::string &target,
                 const std::optional<std::string> &locale) {
    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, 0)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : defaultLocale;
    std::unordered_set<std::string> seen;
    int overallResult = 0;
    for (const auto &f : files) {
        if (!seen.insert(f).second) {
            continue;
        }
        int result = RemoveFile(hArchive, f, lcid);
        if (result != 0) {
            overallResult = result;
        }
    }
    CloseMpqArchive(hArchive);
    return overallResult;
}

int HandleList(const std::string &target, const std::optional<std::string> &listfileName,
               bool listAll, bool listDetailed, const std::vector<std::string> &properties) {
    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }
    ListFiles(hArchive, listfileName, listAll, listDetailed, properties);
    CloseMpqArchive(hArchive);
    return 0;
}

int HandleExtract(const std::string &target, const std::optional<std::string> &output,
                  const std::optional<std::string> &file, bool keepFolderStructure,
                  const std::optional<std::string> &listfileName,
                  const std::optional<std::string> &locale) {
    // If no output directory specified, use MPQ path without extension
    // If output directory specified, create it if it doesn't exist
    std::string effectiveOutput;
    if (!output.has_value()) {
        fs::path outputPathAbsolute = fs::canonical(target);
        fs::path outputPath = outputPathAbsolute.parent_path() / outputPathAbsolute.stem();
        effectiveOutput = outputPath.u8string();
    } else {
        effectiveOutput = output.value();
    }
    if (!fs::create_directory(effectiveOutput) && !fs::is_directory(effectiveOutput)) {
        std::cerr << "[!] Failed to create output directory: " << effectiveOutput << std::endl;
        return 1;
    }

    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : defaultLocale;
    if (locale.has_value() && lcid == defaultLocale) {
        std::cout << "[!] Warning: The locale '" << locale.value()
                  << "' is unknown. Will use default locale instead." << std::endl;
    }

    int result;
    if (file.has_value()) {
        result = ExtractFile(hArchive, effectiveOutput, file.value(), keepFolderStructure, lcid);
    } else {
        result = ExtractFiles(hArchive, effectiveOutput, listfileName, lcid);
    }
    CloseMpqArchive(hArchive);

    if (result != 0) {
        std::cerr << std::endl << "[!] Failed to extract all files." << std::endl;
    }
    return result;
}

int HandleRead(const std::string &file, const std::string &target,
               const std::optional<std::string> &locale) {
    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : defaultLocale;
    if (locale.has_value() && lcid == defaultLocale) {
        std::cout << "[!] Warning: The locale '" << locale.value()
                  << "' is unknown. Will use default locale instead." << std::endl;
    }

    uint32_t fileSize;
    auto fileContent = ReadFile(hArchive, file.c_str(), &fileSize, lcid);
    if (!fileContent) {
        return 1;
    }

    PrintAsBinary(fileContent.get(), fileSize);

    CloseMpqArchive(hArchive);
    return 0;
}

int HandleVerify(const std::string &target, bool printSignature) {
    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    int result = 0;
    uint32_t verifyResult = VerifyMpqArchive(hArchive);
    if (verifyResult == ERROR_WEAK_SIGNATURE_OK || verifyResult == ERROR_STRONG_SIGNATURE_OK ||
        verifyResult == ERROR_WEAK_SIGNATURE_ERROR ||
        verifyResult == ERROR_STRONG_SIGNATURE_ERROR) {
        if (printSignature) {
            // If printing the signature, don't print success message
            // because the user might want to pipe/redirect the signature data
            PrintMpqSignature(hArchive, target);
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
    CloseMpqArchive(hArchive);
    return result;
}

int HandleCompact(const std::string &target, const std::optional<std::string> &listfileName) {
    HANDLE hArchive;
    if (!OpenMpqArchive(target, &hArchive, 0)) {
        return 1;
    }

    const int result = CompactMpqArchive(hArchive, listfileName);
    CloseMpqArchive(hArchive);
    return result;
}
