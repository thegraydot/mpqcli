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

static const std::vector<std::string> special_mpq_files = {"(listfile)", "(signature)",
                                                           "(attributes)"};

bool OpenMpqArchive(const std::string &filename, HANDLE *archive, int32_t flags) {
    if (!SFileOpenArchive(filename.c_str(), 0, flags, archive)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to open MPQ archive: " << filename << ": (" << error << ") "
                  << StormErrorString(error) << std::endl;
        return false;
    }
    return true;
}

bool CloseMpqArchive(HANDLE archive) {
    if (!SFileCloseArchive(archive)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to close MPQ archive: (" << error << ") "
                  << StormErrorString(error) << std::endl;
        return false;
    }
    return true;
}

bool FileExistsInArchiveForLocale(const HANDLE archive, const std::string &file_path,
                                  const LCID locale) {
    bool file_exists = false;
    SFileSetLocale(locale);
    HANDLE file;
    if (SFileOpenFileEx(archive, file_path.c_str(), SFILE_OPEN_FROM_MPQ, &file)) {
        const auto file_locale = GetFileInfo<int32_t>(file, SFileInfoLocale);
        if (file_locale == locale) {
            file_exists = true;
        }
        SFileCloseFile(file);
    }
    return file_exists;
}

bool SignMpqArchive(HANDLE archive) {
    if (!SFileSignArchive(archive, SIGNATURE_TYPE_WEAK)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to sign MPQ archive: (" << error << ") " << StormErrorString(error)
                  << std::endl;
        return false;
    }
    return true;
}

int ExtractFiles(HANDLE archive, const std::string &output,
                 const std::optional<std::string> &listfile_name, LCID preferred_locale) {
    SFileSetLocale(preferred_locale);
    // Check if the user provided a listfile input
    const char *listfile = listfile_name.has_value() ? listfile_name->c_str() : nullptr;

    SFILE_FIND_DATA find_data;
    HANDLE find_handle = SFileFindFirstFile(archive, "*", &find_data, listfile);
    if (find_handle == nullptr) {
        std::cerr << "[!] Failed to find first file in MPQ archive." << std::endl;
        return 1;
    }

    int32_t result = 0;
    do {
        result |= ExtractFile(archive, output, find_data.cFileName,
                              true, // Keep folder structure
                              preferred_locale);
    } while (SFileFindNextFile(find_handle, &find_data));

    SFileFindClose(find_handle);
    return result;
}

int ExtractFile(HANDLE archive, const std::string &output, const std::string &file_name,
                bool keep_folder_structure, LCID preferred_locale) {
    SFileSetLocale(preferred_locale);
    if (!FileExistsInArchiveForLocale(archive, file_name.c_str(), preferred_locale) &&
        !FileExistsInArchiveForLocale(archive, file_name.c_str(), default_locale)) {
        std::cerr << "[!] Failed: File doesn't exist"
                  << PrettyPrintLocale(preferred_locale, " for locale ", true) << ": " << file_name
                  << std::endl;
        return 1;
    }

    // Change forward slashes on non-Windows systems
    fs::path file_name_path(file_name);
    std::string file_name_string = NormalizeFilePath(file_name_path);

    // Remove folder structure if keepFolderStructure is false
    if (!keep_folder_structure) {
        file_name_path = fs::path(file_name_string);
        file_name_string = file_name_path.filename().u8string();
    }

    // Create output directory
    fs::path output_path_absolute = fs::canonical(output);
    fs::path output_path_base =
        output_path_absolute.parent_path() / output_path_absolute.filename();
    std::filesystem::create_directories(fs::path(output_path_base).parent_path());

    // Ensure sub-directories for folder-nested files exist before calling canonical
    fs::path output_file_path_name = output_path_base / file_name_string;
    std::filesystem::create_directories(output_file_path_name.parent_path());

    // Guard against path traversal attacks: resolve symlinks and ".." with canonical
    // (requires path to exist, hence create_directories above)
    fs::path resolved_output =
        fs::canonical(output_file_path_name.parent_path()) / output_file_path_name.filename();
    if (std::mismatch(output_path_base.begin(), output_path_base.end(), resolved_output.begin(),
                      resolved_output.end())
            .first != output_path_base.end()) {
        std::cerr << "[!] Blocked: path traversal attempt detected: " << file_name_string
                  << std::endl;
        return 1;
    }

    std::string output_file_name{resolved_output.u8string()};

    if (SFileExtractFile(archive, file_name.c_str(), output_file_name.c_str(), 0)) {
        std::cout << "[*] Extracted: " << file_name_string << std::endl;
    } else {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed: (" << error << ") " << StormErrorString(error) << ": "
                  << file_name << std::endl;
        return 1;
    }

    return 0;
}

HANDLE CreateMpqArchive(const std::string &output_archive_name, const uint32_t file_count,
                        const GameRules &game_rules) {
    // Check if file already exists
    if (fs::exists(output_archive_name)) {
        std::cerr << "[!] File already exists: " << output_archive_name << " Exiting..."
                  << std::endl;
        return nullptr;
    }

    HANDLE archive;

    // Use game-specific create settings
    const MpqCreateSettings &settings = game_rules.GetCreateSettings();

    SFILE_CREATE_MPQ create_info = {};
    // All logic for defaults and dependencies is handled in GameRules::OverrideCreateSettings
    create_info.cbSize = sizeof(SFILE_CREATE_MPQ);
    create_info.dwMpqVersion = settings.mpq_version;
    create_info.dwStreamFlags = settings.stream_flags;
    create_info.dwFileFlags1 = settings.file_flags1;
    create_info.dwFileFlags2 = settings.file_flags2;
    create_info.dwFileFlags3 = settings.file_flags3;
    create_info.dwAttrFlags = settings.attr_flags;
    create_info.dwSectorSize = settings.sector_size;
    create_info.dwRawChunkSize = settings.raw_chunk_size;
    create_info.dwMaxFileCount = file_count;

    const bool result = SFileCreateArchive2(output_archive_name.c_str(), &create_info, &archive);

    if (!result) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to create MPQ archive: " << output_archive_name << ": (" << error
                  << ") " << StormErrorString(error) << std::endl;
        return nullptr;
    }

    return archive;
}

int AddFiles(HANDLE archive, const std::string &input_path, const std::string &path_prefix,
             LCID locale, const GameRules &game_rules,
             const CompressionSettingsOverrides &overrides, bool overwrite, bool update) {
    fs::path target_path = fs::path(input_path);

    std::vector<fs::directory_entry> entries;
    for (const auto &entry : fs::recursive_directory_iterator(input_path)) {
        if (fs::is_regular_file(entry.path())) {
            entries.push_back(entry);
        }
    }
    std::sort(entries.begin(), entries.end(),
              [](const fs::directory_entry &a, const fs::directory_entry &b) {
                  return a.path() < b.path();
              });

    int files_added = 0;
    int files_skipped = 0;
    int files_failed = 0;

    for (const auto &entry : entries) {
        fs::path input_file_path = fs::relative(entry, target_path);
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

        if (update) {
            SFileSetLocale(locale);
            HANDLE file;
            if (SFileOpenFileEx(archive, archive_file_path.c_str(), SFILE_OPEN_FROM_MPQ, &file)) {
                int32_t file_locale = GetFileInfo<int32_t>(file, SFileInfoLocale);
                if (file_locale == locale) {
                    DWORD archived_size = SFileGetFileSize(file, nullptr);
                    SFileCloseFile(file);
                    uintmax_t disk_size = fs::file_size(entry.path());
                    if (disk_size == static_cast<uintmax_t>(archived_size)) {
                        std::cout << "[~] Skipping unchanged file: " << archive_file_path
                                  << std::endl;
                        files_skipped++;
                        continue;
                    }
                } else {
                    SFileCloseFile(file);
                }
            }
        }

        const int result = AddFile(archive, entry.path(), archive_file_path, locale, game_rules,
                                   overrides, overwrite);
        if (result == 0) {
            files_added++;
        } else {
            files_failed++;
        }
    }

    if (update) {
        std::cout << "[*] For " << input_path << ": " << files_added << " files added, "
                  << files_skipped << " files skipped, " << files_failed << " files failed."
                  << std::endl;
    }

    return files_failed;
}

int AddFile(HANDLE archive, const fs::path &local_file, const std::string &archive_file_path,
            const LCID locale, const GameRules &game_rules,
            const CompressionSettingsOverrides &overrides, bool overwrite) {
    // Return if file doesn't exist on disk
    if (!fs::exists(local_file)) {
        std::cerr << "[!] File doesn't exist on disk: " << local_file << std::endl;
        return 1;
    }

    // Check if file exists in MPQ archive
    SFileSetLocale(locale);
    HANDLE file;
    if (SFileOpenFileEx(archive, archive_file_path.c_str(), SFILE_OPEN_FROM_MPQ, &file)) {
        int32_t file_locale = GetFileInfo<int32_t>(file, SFileInfoLocale);
        SFileCloseFile(file);
        if (file_locale == locale && !overwrite) {
            std::cerr << "[!] File" << PrettyPrintLocale(locale, " for locale ")
                      << " already exists in MPQ archive: " << archive_file_path << " - Skipping..."
                      << std::endl;
            return 1;
        } else if (file_locale == locale) {
            std::cout << "[+] File" << PrettyPrintLocale(locale, " for locale ")
                      << " already exists in MPQ archive: " << archive_file_path
                      << " - Overwriting..." << std::endl;
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
    const std::uintmax_t raw_file_size = fs::file_size(local_file);
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

    if (overwrite) {
        flags += MPQ_FILE_REPLACEEXISTING;
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

int RemoveFile(HANDLE archive, const std::string &archive_file_path, LCID locale) {
    SFileSetLocale(locale);
    std::cout << "[-] Removing file" << PrettyPrintLocale(locale, " for locale ") << ": "
              << archive_file_path << std::endl;

    if (!FileExistsInArchiveForLocale(archive, archive_file_path, locale)) {
        std::cerr << "[!] Failed: File doesn't exist"
                  << PrettyPrintLocale(locale, " for locale ", true) << ": " << archive_file_path
                  << std::endl;
        return 1;
    }

    if (!SFileRemoveFile(archive, archive_file_path.c_str(), 0)) {
        std::cerr << "[!] Failed: File cannot be removed"
                  << PrettyPrintLocale(locale, " for locale ", true) << ": " << archive_file_path
                  << std::endl;
        return 1;
    }

    return 0;
}

std::string GetFlagString(uint32_t flags) {
    std::string result;

    // Preserve column-aligned flag-to-char mappings
    // clang-format off
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

int ListFiles(HANDLE archive, const std::optional<std::string> &listfile_name, bool list_all,
              bool list_detailed, const std::vector<std::string> &properties) {
    // Check if the user provided a listfile input
    const char *listfile = listfile_name.has_value() ? listfile_name->c_str() : nullptr;

    SFILE_FIND_DATA find_data;
    HANDLE find_handle = SFileFindFirstFile(archive, "*", &find_data, listfile);
    if (find_handle == nullptr) {
        std::cerr << "[!] Failed to find first file in MPQ archive." << std::endl;
        return -1;
    }

    std::vector<std::string> properties_to_print =
        properties.empty() ? std::vector<std::string>{"file-size", "locale", "file-time"}
                           : properties;
    if (!properties.empty()) {
        list_detailed =
            true; // If the user specified properties, we need to print the detailed output
    }

    // Map of property name to SFileInfoClass, defined once, outside the loop
    static const std::map<std::string, SFileInfoClass> property_info_class = {
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
        seen_file_names; // Used to prevent printing the same file name multiple times
    // Loop through all files in the MPQ archive
    do {
        // Skip special files unless user wants to list all (like ls -a)
        if (!list_all && std::find(special_mpq_files.begin(), special_mpq_files.end(),
                                   find_data.cFileName) != special_mpq_files.end()) {
            continue;
        }

        // Print the detailed (long) file listing (like ls -l)
        if (list_detailed) {
            if (seen_file_names.find(find_data.cFileName) != seen_file_names.end()) {
                // Filename has been seen before, and thus printed before. Skip over it.
                continue;
            }
            seen_file_names.insert(find_data.cFileName);

            // Multiple files can be stored with identical filenames under different locales.
            // Loop over all locales and print the file details for each locale.
            DWORD max_locales = 32; // This will be updated in the call to SFileEnumLocales
            std::vector<LCID> file_locale_vec(max_locales);
            LCID *file_locales = file_locale_vec.data();

            DWORD result =
                SFileEnumLocales(archive, find_data.cFileName, file_locales, &max_locales, 0);

            if (result == ERROR_INVALID_PARAMETER) {
                // This ought to mean that the file name is unknown, whereupon `SFileEnumLocales`
                // exits early since its check for `IsPseudoFileName` returns true. If that is the
                // case, it will not have populated `fileLocales` or have updated `maxLocales`. Just
                // set the maxLocales to 1 and list the file with the unknown name once.
                max_locales = 1;
                file_locales[0] = default_locale;

            } else if (result == ERROR_INVALID_HANDLE || result == ERROR_NOT_SUPPORTED) {
                std::cerr << "[!] Internal error for file: " << find_data.cFileName << std::endl;
                continue;

            } else if (result == ERROR_INSUFFICIENT_BUFFER) {
                std::cerr << "[!] There are more than " << max_locales
                          << " locales for the file: " << find_data.cFileName
                          << ". Will only list the " << max_locales << " first files." << std::endl;
            }

            // Loop through all found locales
            for (DWORD i = 0; i < max_locales; i++) {
                LCID locale = file_locales[i];
                SFileSetLocale(locale);
                HANDLE file;

                // We need to open the file to get detailed information
                // Use our custom GetFileInfo function
                if (!SFileOpenFileEx(archive, find_data.cFileName, SFILE_OPEN_FROM_MPQ, &file)) {
                    std::cerr << "[!] Failed to open file: " << find_data.cFileName << std::endl;
                    continue; // Skip to the next file
                }

                for (const auto &prop : properties_to_print) {
                    auto it = property_info_class.find(prop);
                    if (it == property_info_class.end())
                        continue;

                    if (prop == "hash-index" || prop == "file-index") {
                        std::cout << std::setw(5) << GetFileInfo<int32_t>(file, it->second) << " ";
                    } else if (prop == "name-hash1" || prop == "name-hash2") {
                        std::cout << std::setfill('0') << std::hex << std::setw(8)
                                  << GetFileInfo<int32_t>(file, it->second) << std::setfill(' ')
                                  << std::dec << " ";
                    } else if (prop == "name-hash3") {
                        std::cout << std::setfill('0') << std::hex << std::setw(16)
                                  << GetFileInfo<int64_t>(file, it->second) << std::setfill(' ')
                                  << std::dec << " ";
                    } else if (prop == "locale") {
                        std::cout << std::setw(4)
                                  << LocaleToLang(GetFileInfo<int32_t>(file, it->second)) << " ";
                    } else if (prop == "byte-offset") {
                        std::cout << std::hex << std::setw(8)
                                  << GetFileInfo<int64_t>(file, it->second) << std::dec << " ";
                    } else if (prop == "file-time") {
                        std::cout << std::setw(19)
                                  << FileTimeToLsTime(GetFileInfo<int64_t>(file, it->second))
                                  << " ";
                    } else if (prop == "file-size" || prop == "compressed-size") {
                        std::cout << std::setw(8) << GetFileInfo<int32_t>(file, it->second) << " ";
                    } else if (prop == "flags") {
                        std::cout << std::setw(8)
                                  << GetFlagString(GetFileInfo<int32_t>(file, it->second)) << " ";
                    } else if (prop == "encryption-key" || prop == "encryption-key-raw") {
                        std::cout << std::setfill('0') << std::hex << std::setw(8)
                                  << GetFileInfo<int64_t>(file, it->second) << std::setfill(' ')
                                  << std::dec << " ";
                    }
                }

                std::cout << " " << find_data.cFileName << std::endl;
                SFileCloseFile(file);
            }
            SFileSetLocale(default_locale); // Reset locale to default after changing it
        } else {
            // Print just the filename (like default ls command output)
            std::cout << find_data.cFileName << std::endl;
        }

    } while (SFileFindNextFile(find_handle, &find_data));

    SFileFindClose(find_handle);
    return 0;
}

std::unique_ptr<char[]> ReadFile(HANDLE archive, const char *file_name, unsigned int *file_size,
                                 LCID preferred_locale) {
    SFileSetLocale(preferred_locale);
    if (!FileExistsInArchiveForLocale(archive, file_name, preferred_locale) &&
        !FileExistsInArchiveForLocale(archive, file_name, default_locale)) {
        std::cerr << "[!] Failed: File doesn't exist"
                  << PrettyPrintLocale(preferred_locale, " for locale ", true) << ": " << file_name
                  << std::endl;
        return nullptr;
    }

    HANDLE file;
    if (!SFileOpenFileEx(archive, file_name, SFILE_OPEN_FROM_MPQ, &file)) {
        std::cerr << "[!] Failed: File cannot be opened: " << file_name << std::endl;
        return nullptr;
    }

    *file_size = SFileGetFileSize(file, nullptr);
    if (*file_size == SFILE_INVALID_SIZE) {
        std::cerr << "[!] Failed: Invalid file size for: " << file_name << std::endl;
        SFileCloseFile(file);
        return nullptr;
    }

    auto file_content = std::make_unique<char[]>(*file_size);
    DWORD bytes_read;
    if (!SFileReadFile(file, file_content.get(), *file_size, &bytes_read, nullptr)) {
        std::cerr << "[!] Failed: Cannot read file contents for: " << file_name << std::endl;
        SFileCloseFile(file);
        return nullptr;
    }

    SFileCloseFile(file);
    return file_content;
}

void PrintMpqInfo(HANDLE archive, const std::optional<std::string> &info_property) {
    // Map of property names to their corresponding actions
    std::map<std::string, std::function<void(bool)>> property_actions = {
        {"format-version",
         [&](bool print_name) {
             TMPQHeader header = GetFileInfo<TMPQHeader>(archive, SFileMpqHeader);
             uint16_t format_version =
                 header.wFormatVersion + 1; // Add +1 because StormLib starts at 0
             if (print_name) {
                 std::cout << "Format version: ";
             }
             std::cout << format_version << std::endl;
         }},
        {"header-offset",
         [&](bool print_name) {
             int64_t header_offset = GetFileInfo<int64_t>(archive, SFileMpqHeaderOffset);
             if (print_name) {
                 std::cout << "Header offset: ";
             }
             std::cout << header_offset << std::endl;
         }},
        {"header-size",
         [&](bool print_name) {
             int64_t header_size = GetFileInfo<int64_t>(archive, SFileMpqHeaderSize);
             if (print_name) {
                 std::cout << "Header size: ";
             }
             std::cout << header_size << std::endl;
         }},
        {"archive-size",
         [&](bool print_name) {
             int32_t archive_size = GetFileInfo<int32_t>(archive, SFileMpqArchiveSize);
             if (print_name) {
                 std::cout << "Archive size: ";
             }
             std::cout << archive_size << std::endl;
         }},
        {"file-count",
         [&](bool print_name) {
             int32_t number_of_files = GetFileInfo<int32_t>(archive, SFileMpqNumberOfFiles);
             if (print_name) {
                 std::cout << "File count: ";
             }
             std::cout << number_of_files << std::endl;
         }},
        {"max-files",
         [&](bool print_name) {
             int32_t max_files = GetFileInfo<int32_t>(archive, SFileMpqMaxFileCount);
             if (print_name) {
                 std::cout << "Max files: ";
             }
             std::cout << max_files << std::endl;
         }},
        {"signature-type", [&](bool print_name) {
             int32_t signature_type = GetFileInfo<int32_t>(archive, SFileMpqSignatures);
             if (print_name) {
                 std::cout << "Signature type: ";
             }
             if (signature_type == SIGNATURE_TYPE_NONE) {
                 std::cout << "None" << std::endl;
             } else if (signature_type == SIGNATURE_TYPE_WEAK) {
                 std::cout << "Weak" << std::endl;
             } else if (signature_type == SIGNATURE_TYPE_STRONG) {
                 std::cout << "Strong" << std::endl;
             }
         }}};

    // If infoProperty is not set, print all properties with their names (key)
    // Otherwise, print only the specified property value
    if (!info_property.has_value()) {
        for (const auto &[key, action] : property_actions) {
            action(true); // Print property name and value
        }
    } else {
        auto it = property_actions.find(info_property.value());
        if (it != property_actions.end()) {
            it->second(false); // Print only the value
        }
    }
}

uint32_t VerifyMpqArchive(HANDLE archive) {
    return SFileVerifyArchive(archive);
}

int CompactMpqArchive(HANDLE archive, const std::optional<std::string> &listfile_name) {
    std::cout << "[*] Compacting archive. This may take some time..." << std::endl;
    // Check if the user provided a listfile input
    const char *listfile = listfile_name.has_value() ? listfile_name->c_str() : nullptr;

    if (!SFileCompactArchive(archive, listfile, false)) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to compact archive: (" << error << ") " << StormErrorString(error)
                  << std::endl;
        return 1;
    }
    return 0;
}

int32_t PrintMpqSignature(HANDLE archive, const std::string &target) {
    // Determine if we have a strong or weak digital signature
    int32_t signature_type = GetFileInfo<int32_t>(archive, SFileMpqSignatures);

    std::vector<char> signature_content;

    if (signature_type == SIGNATURE_TYPE_NONE) {
        return 1;
    } else if (signature_type == SIGNATURE_TYPE_WEAK) {
        const char *file_name = "(signature)";
        uint32_t file_size;
        auto file_content = ReadFile(archive, file_name, &file_size, default_locale);

        if (!file_content) {
            std::cerr << "[!] Failed to read weak signature file." << std::endl;
            return -1;
        }
        signature_content.resize(file_size);
        std::copy(file_content.get(), file_content.get() + file_size, signature_content.begin());

        PrintAsBinary(file_content.get(), file_size);

    } else if (signature_type == SIGNATURE_TYPE_STRONG) {
        signature_content = GetFileInfo<std::vector<char>>(archive, SFileMpqStrongSignature);
        if (signature_content.empty()) {
            int64_t archive_size = GetFileInfo<int64_t>(archive, SFileMpqArchiveSize64);
            int64_t archive_offset = GetFileInfo<int64_t>(archive, SFileMpqHeaderOffset);

            const fs::path archive_path = fs::canonical(target);
            std::uintmax_t file_size = fs::file_size(archive_path);
            int64_t signature_length = file_size - archive_offset - archive_size;

            if (signature_length <= 0) {
                std::cerr << "[!] Invalid signature length: " << signature_length << std::endl;
                return -1;
            }

            std::ifstream file_mpq(archive_path, std::ios::binary);
            file_mpq.seekg(archive_offset + archive_size, std::ios::beg);
            signature_content.resize(static_cast<size_t>(signature_length));
            file_mpq.read(signature_content.data(), signature_content.size());
            file_mpq.close();

            PrintAsBinary(signature_content.data(),
                          static_cast<uint32_t>(signature_content.size()));
        }
    }

    return 0;
}
