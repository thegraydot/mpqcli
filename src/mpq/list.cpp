#include "mpq/list.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <StormLib.h>

#include "mpq/flags.h"
#include "mpq/query.h"
#include "util/format.h"
#include "util/locales.h"

namespace mpqcli {

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
                                  << LocaleToLang(GetFileInfo<LCID>(file, it->second)) << " ";
                    } else if (prop == "byte-offset") {
                        std::cout << std::hex << std::setw(8)
                                  << GetFileInfo<int64_t>(file, it->second) << std::dec << " ";
                    } else if (prop == "file-time") {
                        std::cout << std::setw(19)
                                  << FileTimeToLsTime(GetFileInfo<int64_t>(file, it->second))
                                  << " ";
                    } else if (prop == "file-size" || prop == "compressed-size") {
                        std::cout << std::setw(8) << GetFileInfo<uint32_t>(file, it->second) << " ";
                    } else if (prop == "flags") {
                        std::cout << std::setw(8)
                                  << GetFlagString(GetFileInfo<uint32_t>(file, it->second)) << " ";
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

} // namespace mpqcli
