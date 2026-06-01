#include "mpq.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include <StormLib.h>

#include "gamerules.h"
#include "helpers.h"
#include "locales.h"

namespace fs = std::filesystem;

static const std::vector<std::string> kSpecialMpqFiles = {"(listfile)", "(signature)",
                                                          "(attributes)"};

bool OpenMpqArchive(const std::string &filename, HANDLE *hArchive, int32_t flags) {
    if (!SFileOpenArchive(filename.c_str(), 0, flags, hArchive)) {
        std::cerr << "[!] Failed to open: " << filename << std::endl;
        return false;
    }
    return true;
}

bool CloseMpqArchive(HANDLE hArchive) {
    if (!SFileCloseArchive(hArchive)) {
        std::cerr << "[!] Failed to close MPQ archive." << std::endl;
        return false;
    }
    return true;
}

bool FileExistsInArchiveForLocale(const HANDLE hArchive, const std::string &filePath,
                                  const LCID locale) {
    bool fileExists = false;
    SFileSetLocale(locale);
    HANDLE hFile;
    if (SFileOpenFileEx(hArchive, filePath.c_str(), SFILE_OPEN_FROM_MPQ, &hFile)) {
        const auto fileLocale = GetFileInfo<int32_t>(hFile, SFileInfoLocale);
        if (fileLocale == locale) {
            fileExists = true;
        }
        SFileCloseFile(hFile);
    }
    return fileExists;
}

bool SignMpqArchive(HANDLE hArchive) {
    if (!SFileSignArchive(hArchive, SIGNATURE_TYPE_WEAK)) {
        std::cerr << "[!] Failed to sign MPQ archive." << std::endl;
        return false;
    }
    return true;
}

int ExtractFiles(HANDLE hArchive, const std::string &output,
                 const std::optional<std::string> &listfileName, LCID preferredLocale) {
    SFileSetLocale(preferredLocale);
    // Check if the user provided a listfile input
    const char *listfile = listfileName.has_value() ? listfileName->c_str() : nullptr;

    SFILE_FIND_DATA findData;
    HANDLE findHandle = SFileFindFirstFile(hArchive, "*", &findData, listfile);
    if (findHandle == nullptr) {
        std::cerr << "[!] Failed to find first file in MPQ archive." << std::endl;
        SFileCloseArchive(hArchive);
        return 1;
    }

    int32_t result = 0;
    do {
        result |= ExtractFile(hArchive, output, findData.cFileName,
                              true,  // Keep folder structure
                              preferredLocale);
    } while (SFileFindNextFile(findHandle, &findData));

    SFileFindClose(findHandle);
    return result;
}

int ExtractFile(HANDLE hArchive, const std::string &output, const std::string &fileName,
                bool keepFolderStructure, LCID preferredLocale) {
    SFileSetLocale(preferredLocale);
    const char *szFileName = fileName.c_str();
    if (!FileExistsInArchiveForLocale(hArchive, szFileName, preferredLocale) &&
        !FileExistsInArchiveForLocale(hArchive, szFileName, defaultLocale)) {
        std::cerr << "[!] Failed: File doesn't exist"
                  << PrettyPrintLocale(preferredLocale, " for locale ", true) << ": " << szFileName
                  << std::endl;
        return 1;
    }

    // Change forward slashes on non-Windows systems
    fs::path fileNamePath(fileName);
    std::string fileNameString = NormalizeFilePath(fileNamePath);

    // Remove folder structure if keepFolderStructure is false
    if (!keepFolderStructure) {
        fileNamePath = fs::path(fileNameString);
        fileNameString = fileNamePath.filename().u8string();
    }

    // Create output directory
    fs::path outputPathAbsolute = fs::canonical(output);
    fs::path outputPathBase = outputPathAbsolute.parent_path() / outputPathAbsolute.filename();
    std::filesystem::create_directories(fs::path(outputPathBase).parent_path());

    // Ensure sub-directories for folder-nested files exist before calling canonical
    fs::path outputFilePathName = outputPathBase / fileNameString;
    std::filesystem::create_directories(outputFilePathName.parent_path());

    // Guard against path traversal attacks: resolve symlinks and ".." with canonical
    // (requires path to exist, hence create_directories above)
    fs::path resolvedOutput =
        fs::canonical(outputFilePathName.parent_path()) / outputFilePathName.filename();
    if (std::mismatch(outputPathBase.begin(), outputPathBase.end(), resolvedOutput.begin(),
                      resolvedOutput.end())
            .first != outputPathBase.end()) {
        std::cerr << "[!] Blocked: path traversal attempt detected: " << fileNameString
                  << std::endl;
        return 1;
    }

    std::string outputFileName{resolvedOutput.u8string()};

    if (SFileExtractFile(hArchive, szFileName, outputFileName.c_str(), 0)) {
        std::cout << "[*] Extracted: " << fileNameString << std::endl;
    } else {
        int32_t error = SErrGetLastError();
        std::cerr << "[!] Failed: " << "(" << error << ") " << szFileName << std::endl;
        return 1;
    }

    return 0;
}

HANDLE CreateMpqArchive(const std::string &outputArchiveName, const uint32_t fileCount,
                        const GameRules &gameRules) {
    // Check if file already exists
    if (fs::exists(outputArchiveName)) {
        std::cerr << "[!] File already exists: " << outputArchiveName << " Exiting..." << std::endl;
        return nullptr;
    }

    HANDLE hMpq;

    // Use game-specific create settings
    const MpqCreateSettings &settings = gameRules.GetCreateSettings();

    SFILE_CREATE_MPQ createInfo = {};
    // All logic for defaults and dependencies is handled in GameRules::OverrideCreateSettings
    createInfo.cbSize = sizeof(SFILE_CREATE_MPQ);
    createInfo.dwMpqVersion = settings.mpqVersion;
    createInfo.dwStreamFlags = settings.streamFlags;
    createInfo.dwFileFlags1 = settings.fileFlags1;
    createInfo.dwFileFlags2 = settings.fileFlags2;
    createInfo.dwFileFlags3 = settings.fileFlags3;
    createInfo.dwAttrFlags = settings.attrFlags;
    createInfo.dwSectorSize = settings.sectorSize;
    createInfo.dwRawChunkSize = settings.rawChunkSize;
    createInfo.dwMaxFileCount = fileCount;

    const bool result = SFileCreateArchive2(outputArchiveName.c_str(), &createInfo, &hMpq);

    if (!result) {
        std::cerr << "[!] Failed to create MPQ archive: " << outputArchiveName << std::endl;
        int32_t error = SErrGetLastError();
        std::cout << "[!] Error: " << error << std::endl;
        return nullptr;
    }

    return hMpq;
}

int AddFiles(HANDLE hArchive, const std::string &inputPath, const std::string &pathPrefix,
             LCID locale, const GameRules &gameRules,
             const CompressionSettingsOverrides &overrides, bool overwrite, bool update) {
    fs::path targetPath = fs::path(inputPath);

    int filesAdded = 0;
    int filesSkipped = 0;

    for (const auto &entry : fs::recursive_directory_iterator(inputPath)) {
        if (!fs::is_regular_file(entry.path())) continue;

        fs::path inputFilePath = fs::relative(entry, targetPath);
        std::string archiveFilePath;

        if (pathPrefix.empty()) {
            archiveFilePath = WindowsifyFilePath(inputFilePath.u8string());
        } else {
            archiveFilePath = WindowsifyFilePath((fs::path(pathPrefix) / inputFilePath).u8string());
        }

        if (std::find(kSpecialMpqFiles.begin(), kSpecialMpqFiles.end(), archiveFilePath) !=
            kSpecialMpqFiles.end()) {
            std::cout << "[*] Skipping special MPQ file: " << archiveFilePath << std::endl;
            continue;
        }

        if (update) {
            SFileSetLocale(locale);
            HANDLE hFile;
            if (SFileOpenFileEx(hArchive, archiveFilePath.c_str(), SFILE_OPEN_FROM_MPQ, &hFile)) {
                int32_t fileLocale = GetFileInfo<int32_t>(hFile, SFileInfoLocale);
                if (fileLocale == locale) {
                    DWORD archivedSize = SFileGetFileSize(hFile, nullptr);
                    SFileCloseFile(hFile);
                    uintmax_t diskSize = fs::file_size(entry.path());
                    if (diskSize == static_cast<uintmax_t>(archivedSize)) {
                        std::cout << "[~] Skipping unchanged file: " << archiveFilePath << std::endl;
                        filesSkipped++;
                        continue;
                    }
                } else {
                    SFileCloseFile(hFile);
                }
            }
        }

        int result =
            AddFile(hArchive, entry.path(), archiveFilePath, locale, gameRules, overrides, overwrite);
        if (result == 0) {
            filesAdded++;
        }
    }

    if (update) {
        std::cout << "[*] " << filesAdded << " files added, " << filesSkipped << " files skipped"
                  << std::endl;
    }

    return 0;
}

int AddFile(HANDLE hArchive, const fs::path &localFile, const std::string &archiveFilePath,
            const LCID locale, const GameRules &gameRules,
            const CompressionSettingsOverrides &overrides, bool overwrite) {
    // Return if file doesn't exist on disk
    if (!fs::exists(localFile)) {
        std::cerr << "[!] File doesn't exist on disk: " << localFile << std::endl;
        return -1;
    }

    // Check if file exists in MPQ archive
    SFileSetLocale(locale);
    HANDLE hFile;
    if (SFileOpenFileEx(hArchive, archiveFilePath.c_str(), SFILE_OPEN_FROM_MPQ, &hFile)) {
        int32_t fileLocale = GetFileInfo<int32_t>(hFile, SFileInfoLocale);
        SFileCloseFile(hFile);
        if (fileLocale == locale && !overwrite) {
            std::cerr << "[!] File" << PrettyPrintLocale(locale, " for locale ")
                      << " already exists in MPQ archive: " << archiveFilePath << " - Skipping..."
                      << std::endl;
            return -1;
        } else if (fileLocale == locale) {
            std::cout << "[+] File" << PrettyPrintLocale(locale, " for locale ")
                      << " already exists in MPQ archive: " << archiveFilePath
                      << " - Overwriting..." << std::endl;
        }
    }
    std::cout << "[+] Adding file" << PrettyPrintLocale(locale, " for locale ") << ": "
              << archiveFilePath << std::endl;

    // Verify that we are not exceeding maxFile size of the archive, and if we do, increase it
    int32_t numberOfFiles = GetFileInfo<int32_t>(hArchive, SFileMpqNumberOfFiles);
    int32_t maxFiles = GetFileInfo<int32_t>(hArchive, SFileMpqMaxFileCount);

    if (numberOfFiles + 1 > maxFiles) {
        uint32_t newMaxFiles = NextPowerOfTwo(static_cast<uint32_t>(numberOfFiles + 1));
        bool setMaxFileCount = SFileSetMaxFileCount(hArchive, newMaxFiles);
        if (!setMaxFileCount) {
            int32_t error = SErrGetLastError();
            std::cerr << "[!] Error: " << error
                      << " Failed to increase new max file count to: " << newMaxFiles << std::endl;
            return -1;
        }
    }

    // Get file size for rule matching
    const std::uintmax_t rawFileSize = fs::file_size(localFile);
    if (rawFileSize > std::numeric_limits<DWORD>::max()) {
        std::cerr << "[!] Warning: file exceeds 4GB, size-based compression rules may not apply "
                     "correctly: "
                  << localFile << std::endl;
    }
    const DWORD fileSize = static_cast<DWORD>(
        std::min(rawFileSize, static_cast<std::uintmax_t>(std::numeric_limits<DWORD>::max())));

    // Get game-specific rules
    auto [flags, compressionFirst, compressionNext] =
        gameRules.GetCompressionSettings(archiveFilePath, fileSize);

    // Apply overrides where specified, otherwise use game rules
    DWORD dwFlags = overrides.dwFlags.value_or(flags);
    DWORD dwCompression = overrides.dwCompression.value_or(compressionFirst);
    DWORD dwCompressionNext = overrides.dwCompressionNext.value_or(compressionNext);

    if (overwrite) {
        dwFlags += MPQ_FILE_REPLACEEXISTING;
    }

    bool addedFile = SFileAddFileEx(hArchive, localFile.u8string().c_str(), archiveFilePath.c_str(),
                                    dwFlags, dwCompression, dwCompressionNext);

    if (!addedFile) {
        int32_t error = SErrGetLastError();
        std::cerr << "[!] Error: " << error << " Failed to add: " << archiveFilePath << std::endl;
        return -1;
    }

    return 0;
}

int RemoveFile(HANDLE hArchive, const std::string &archiveFilePath, LCID locale) {
    SFileSetLocale(locale);
    std::cout << "[-] Removing file" << PrettyPrintLocale(locale, " for locale ") << ": "
              << archiveFilePath << std::endl;

    if (!FileExistsInArchiveForLocale(hArchive, archiveFilePath, locale)) {
        std::cerr << "[!] Failed: File doesn't exist"
                  << PrettyPrintLocale(locale, " for locale ", true) << ": " << archiveFilePath
                  << std::endl;
        return 1;
    }

    if (!SFileRemoveFile(hArchive, archiveFilePath.c_str(), 0)) {
        std::cerr << "[!] Failed: File cannot be removed"
                  << PrettyPrintLocale(locale, " for locale ", true) << ": " << archiveFilePath
                  << std::endl;
        return 1;
    }

    return 0;
}

std::string GetFlagString(uint32_t flags) {
    std::string result;

    // clang-format off: preserve column-aligned flag-to-char mappings
    if (flags & MPQ_FILE_IMPLODE)            result += 'i';
    if (flags & MPQ_FILE_COMPRESS)           result += 'c';
    if (flags & MPQ_FILE_ENCRYPTED)          result += 'e';
    if (flags & MPQ_FILE_KEY_V2)             result += '2';
    if (flags & MPQ_FILE_PATCH_FILE)         result += 'p';
    if (flags & MPQ_FILE_SINGLE_UNIT)        result += 'u';
    if (flags & MPQ_FILE_DELETE_MARKER)      result += 'd';
    if (flags & MPQ_FILE_SECTOR_CRC)         result += 'r';
    if (flags & MPQ_FILE_SIGNATURE)          result += 's';
    if (flags & MPQ_FILE_EXISTS)             result += 'x';
    if (flags & MPQ_FILE_COMPRESS_MASK)      result += 'm';
    if (flags & MPQ_FILE_DEFAULT_INTERNAL)   result += 'n';
    if (flags & MPQ_FILE_FIX_KEY)            result += 'f';
    // clang-format on

    return result;
}

int ListFiles(HANDLE hArchive, const std::optional<std::string> &listfileName, bool listAll,
              bool listDetailed, const std::vector<std::string> &properties) {
    // Check if the user provided a listfile input
    const char *listfile = listfileName.has_value() ? listfileName->c_str() : nullptr;

    SFILE_FIND_DATA findData;
    HANDLE findHandle = SFileFindFirstFile(hArchive, "*", &findData, listfile);
    if (findHandle == nullptr) {
        std::cerr << "[!] Failed to find first file in MPQ archive." << std::endl;
        SFileCloseArchive(hArchive);
        return -1;
    }

    std::vector<std::string> propertiesToPrint =
        properties.empty() ? std::vector<std::string>{"file-size", "locale", "file-time"}
                           : properties;
    if (!properties.empty()) {
        listDetailed =
            true;  // If the user specified properties, we need to print the detailed output
    }

    // Map of property name to SFileInfoClass — defined once, outside the loop
    static const std::map<std::string, SFileInfoClass> kPropertyInfoClass = {
        {"hash-index", SFileInfoHashIndex},
        {"name-hash1", SFileInfoNameHash1},
        {"name-hash2", SFileInfoNameHash2},
        {"name-hash3", SFileInfoNameHash3},
        {"locale", SFileInfoLocale},
        {"file-index", SFileInfoFileIndex},
        {"byte-offset", SFileInfoByteOffset},
        {"file-time", SFileInfoFileTime},
        {"file-size", SFileInfoFileSize},
        {"compressed-size", SFileInfoCompressedSize},
        {"flags", SFileInfoFlags},
        {"encryption-key", SFileInfoEncryptionKey},
        {"encryption-key-raw", SFileInfoEncryptionKeyRaw},
    };

    std::set<std::string>
        seenFileNames;  // Used to prevent printing the same file name multiple times
    // Loop through all files in the MPQ archive
    do {
        // Skip special files unless user wants to list all (like ls -a)
        if (!listAll && std::find(kSpecialMpqFiles.begin(), kSpecialMpqFiles.end(),
                                  findData.cFileName) != kSpecialMpqFiles.end()) {
            continue;
        }

        // Print the detailed (long) file listing (like ls -l)
        if (listDetailed) {
            if (seenFileNames.find(findData.cFileName) != seenFileNames.end()) {
                // Filename has been seen before, and thus printed before. Skip over it.
                continue;
            }
            seenFileNames.insert(findData.cFileName);

            // Multiple files can be stored with identical filenames under different locales.
            // Loop over all locales and print the file details for each locale.
            DWORD maxLocales = 32;  // This will be updated in the call to SFileEnumLocales
            std::vector<LCID> fileLocaleVec(maxLocales);
            LCID *fileLocales = fileLocaleVec.data();

            DWORD result =
                SFileEnumLocales(hArchive, findData.cFileName, fileLocales, &maxLocales, 0);

            if (result == ERROR_INVALID_PARAMETER) {
                // This ought to mean that the file name is unknown, whereupon `SFileEnumLocales`
                // exits early since its check for `IsPseudoFileName` returns true. If that is the
                // case, it will not have populated `fileLocales` or have updated `maxLocales`. Just
                // set the maxLocales to 1 and list the file with the unknown name once.
                maxLocales = 1;
                fileLocales[0] = defaultLocale;

            } else if (result == ERROR_INVALID_HANDLE || result == ERROR_NOT_SUPPORTED) {
                std::cerr << "[!] Internal error for file: " << findData.cFileName << std::endl;
                continue;

            } else if (result == ERROR_INSUFFICIENT_BUFFER) {
                std::cerr << "[!] There are more than " << maxLocales
                          << " locales for the file: " << findData.cFileName
                          << ". Will only list the " << maxLocales << " first files." << std::endl;
            }

            // Loop through all found locales
            for (DWORD i = 0; i < maxLocales; i++) {
                LCID locale = fileLocales[i];
                SFileSetLocale(locale);
                HANDLE hFile;

                // We need to open the file to get detailed information
                // Use our custom GetFileInfo function
                if (!SFileOpenFileEx(hArchive, findData.cFileName, SFILE_OPEN_FROM_MPQ, &hFile)) {
                    std::cerr << "[!] Failed to open file: " << findData.cFileName << std::endl;
                    continue;  // Skip to the next file
                }

                for (const auto &prop : propertiesToPrint) {
                    auto it = kPropertyInfoClass.find(prop);
                    if (it == kPropertyInfoClass.end()) continue;

                    if (prop == "hash-index" || prop == "file-index") {
                        std::cout << std::setw(5) << GetFileInfo<int32_t>(hFile, it->second) << " ";
                    } else if (prop == "name-hash1" || prop == "name-hash2") {
                        std::cout << std::setfill('0') << std::hex << std::setw(8)
                                  << GetFileInfo<int32_t>(hFile, it->second) << std::setfill(' ')
                                  << std::dec << " ";
                    } else if (prop == "name-hash3") {
                        std::cout << std::setfill('0') << std::hex << std::setw(16)
                                  << GetFileInfo<int64_t>(hFile, it->second) << std::setfill(' ')
                                  << std::dec << " ";
                    } else if (prop == "locale") {
                        std::cout << std::setw(4)
                                  << LocaleToLang(GetFileInfo<int32_t>(hFile, it->second)) << " ";
                    } else if (prop == "byte-offset") {
                        std::cout << std::hex << std::setw(8)
                                  << GetFileInfo<int64_t>(hFile, it->second) << std::dec << " ";
                    } else if (prop == "file-time") {
                        std::cout << std::setw(19)
                                  << FileTimeToLsTime(GetFileInfo<int64_t>(hFile, it->second))
                                  << " ";
                    } else if (prop == "file-size" || prop == "compressed-size") {
                        std::cout << std::setw(8) << GetFileInfo<int32_t>(hFile, it->second) << " ";
                    } else if (prop == "flags") {
                        std::cout << std::setw(8)
                                  << GetFlagString(GetFileInfo<int32_t>(hFile, it->second)) << " ";
                    } else if (prop == "encryption-key" || prop == "encryption-key-raw") {
                        std::cout << std::setfill('0') << std::hex << std::setw(8)
                                  << GetFileInfo<int64_t>(hFile, it->second) << std::setfill(' ')
                                  << std::dec << " ";
                    }
                }

                std::cout << " " << findData.cFileName << std::endl;
                SFileCloseFile(hFile);
            }
            SFileSetLocale(defaultLocale);  // Reset locale to default after changing it
        } else {
            // Print just the filename (like default ls command output)
            std::cout << findData.cFileName << std::endl;
        }

    } while (SFileFindNextFile(findHandle, &findData));

    SFileFindClose(findHandle);
    return 0;
}

std::unique_ptr<char[]> ReadFile(HANDLE hArchive, const char *szFileName, unsigned int *fileSize,
                                 LCID preferredLocale) {
    SFileSetLocale(preferredLocale);
    if (!FileExistsInArchiveForLocale(hArchive, szFileName, preferredLocale) &&
        !FileExistsInArchiveForLocale(hArchive, szFileName, defaultLocale)) {
        std::cerr << "[!] Failed: File doesn't exist"
                  << PrettyPrintLocale(preferredLocale, " for locale ", true) << ": " << szFileName
                  << std::endl;
        return nullptr;
    }

    HANDLE hFile;
    if (!SFileOpenFileEx(hArchive, szFileName, SFILE_OPEN_FROM_MPQ, &hFile)) {
        std::cerr << "[!] Failed: File cannot be opened: " << szFileName << std::endl;
        return nullptr;
    }

    *fileSize = SFileGetFileSize(hFile, nullptr);
    if (*fileSize == SFILE_INVALID_SIZE) {
        std::cerr << "[!] Failed: Invalid file size for: " << szFileName << std::endl;
        SFileCloseFile(hFile);
        return nullptr;
    }

    auto fileContent = std::make_unique<char[]>(*fileSize);
    DWORD dwBytesRead;
    if (!SFileReadFile(hFile, fileContent.get(), *fileSize, &dwBytesRead, nullptr)) {
        std::cerr << "[!] Failed: Cannot read file contents for: " << szFileName << std::endl;
        SFileCloseFile(hFile);
        return nullptr;
    }

    SFileCloseFile(hFile);
    return fileContent;
}

void PrintMpqInfo(HANDLE hArchive, const std::optional<std::string> &infoProperty) {
    // Map of property names to their corresponding actions
    std::map<std::string, std::function<void(bool)>> propertyActions = {
        {"format-version",
         [&](bool printName) {
             TMPQHeader header = GetFileInfo<TMPQHeader>(hArchive, SFileMpqHeader);
             uint16_t formatVersion =
                 header.wFormatVersion + 1;  // Add +1 because StormLib starts at 0
             if (printName) {
                 std::cout << "Format version: ";
             }
             std::cout << formatVersion << std::endl;
         }},
        {"header-offset",
         [&](bool printName) {
             int64_t headerOffset = GetFileInfo<int64_t>(hArchive, SFileMpqHeaderOffset);
             if (printName) {
                 std::cout << "Header offset: ";
             }
             std::cout << headerOffset << std::endl;
         }},
        {"header-size",
         [&](bool printName) {
             int64_t headerSize = GetFileInfo<int64_t>(hArchive, SFileMpqHeaderSize);
             if (printName) {
                 std::cout << "Header size: ";
             }
             std::cout << headerSize << std::endl;
         }},
        {"archive-size",
         [&](bool printName) {
             int32_t archiveSize = GetFileInfo<int32_t>(hArchive, SFileMpqArchiveSize);
             if (printName) {
                 std::cout << "Archive size: ";
             }
             std::cout << archiveSize << std::endl;
         }},
        {"file-count",
         [&](bool printName) {
             int32_t numberOfFiles = GetFileInfo<int32_t>(hArchive, SFileMpqNumberOfFiles);
             if (printName) {
                 std::cout << "File count: ";
             }
             std::cout << numberOfFiles << std::endl;
         }},
        {"max-files",
         [&](bool printName) {
             int32_t maxFiles = GetFileInfo<int32_t>(hArchive, SFileMpqMaxFileCount);
             if (printName) {
                 std::cout << "Max files: ";
             }
             std::cout << maxFiles << std::endl;
         }},
        {"signature-type", [&](bool printName) {
             int32_t signatureType = GetFileInfo<int32_t>(hArchive, SFileMpqSignatures);
             if (printName) {
                 std::cout << "Signature type: ";
             }
             if (signatureType == SIGNATURE_TYPE_NONE) {
                 std::cout << "None" << std::endl;
             } else if (signatureType == SIGNATURE_TYPE_WEAK) {
                 std::cout << "Weak" << std::endl;
             } else if (signatureType == SIGNATURE_TYPE_STRONG) {
                 std::cout << "Strong" << std::endl;
             }
         }}};

    // If infoProperty is not set, print all properties with their names (key)
    // Otherwise, print only the specified property value
    if (!infoProperty.has_value()) {
        for (const auto &[key, action] : propertyActions) {
            action(true);  // Print property name and value
        }
    } else {
        auto it = propertyActions.find(infoProperty.value());
        if (it != propertyActions.end()) {
            it->second(false);  // Print only the value
        }
    }
}

uint32_t VerifyMpqArchive(HANDLE hArchive) {
    return SFileVerifyArchive(hArchive);
}

int32_t PrintMpqSignature(HANDLE hArchive, const std::string &target) {
    // Determine if we have a strong or weak digital signature
    int32_t signatureType = GetFileInfo<int32_t>(hArchive, SFileMpqSignatures);

    std::vector<char> signatureContent;

    if (signatureType == SIGNATURE_TYPE_NONE) {
        return 1;
    } else if (signatureType == SIGNATURE_TYPE_WEAK) {
        const char *szFileName = "(signature)";
        uint32_t fileSize;
        auto fileContent = ReadFile(hArchive, szFileName, &fileSize, defaultLocale);

        if (!fileContent) {
            std::cerr << "[!] Failed to read weak signature file." << std::endl;
            return -1;
        }
        signatureContent.resize(fileSize);
        std::copy(fileContent.get(), fileContent.get() + fileSize, signatureContent.begin());

        PrintAsBinary(fileContent.get(), fileSize);

    } else if (signatureType == SIGNATURE_TYPE_STRONG) {
        signatureContent = GetFileInfo<std::vector<char>>(hArchive, SFileMpqStrongSignature);
        if (signatureContent.empty()) {
            int64_t archiveSize = GetFileInfo<int64_t>(hArchive, SFileMpqArchiveSize64);
            int64_t archiveOffset = GetFileInfo<int64_t>(hArchive, SFileMpqHeaderOffset);

            const fs::path archivePath = fs::canonical(target);
            std::uintmax_t fileSize = fs::file_size(archivePath);
            int64_t signatureLength = fileSize - archiveOffset - archiveSize;

            if (signatureLength <= 0) {
                std::cerr << "[!] Invalid signature length: " << signatureLength << std::endl;
                return -1;
            }

            std::ifstream file_mpq(archivePath, std::ios::binary);
            file_mpq.seekg(archiveOffset + archiveSize, std::ios::beg);
            signatureContent.resize(static_cast<size_t>(signatureLength));
            file_mpq.read(signatureContent.data(), signatureContent.size());
            file_mpq.close();

            PrintAsBinary(signatureContent.data(), static_cast<uint32_t>(signatureContent.size()));
        }
    }

    return 0;
}
