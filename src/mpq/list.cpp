#include "mpq/list.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include <StormLib.h>

#include "errors.h"
#include "mpq/flags.h"
#include "mpq/query.h"
#include "util/format.h"
#include "util/locales.h"

namespace mpqcli {

void ListFiles(HANDLE archive, const std::optional<std::filesystem::path> &listfile_name,
               bool list_all, bool list_detailed, const std::vector<std::string> &properties,
               std::ostream &out, std::ostream &err) {
    const std::string listfile_string =
        listfile_name.has_value() ? listfile_name->string() : std::string();
    const char *listfile = listfile_name.has_value() ? listfile_string.c_str() : nullptr;

    SFILE_FIND_DATA find_data;
    HANDLE find_handle = SFileFindFirstFile(archive, "*", &find_data, listfile);
    if (find_handle == nullptr) {
        throw ArchiveError("Failed to find first file in MPQ archive.");
    }

    std::vector<std::string> properties_to_print =
        properties.empty() ? std::vector<std::string>{"file-size", "locale", "file-time"}
                           : properties;
    if (!properties.empty()) {
        // Named properties are only printed by the detailed listing
        list_detailed = true;
    }

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

    // The enumeration yields one entry per locale of a name and the detailed listing
    // prints every locale in one go, so later entries for a seen name are skipped
    std::set<std::string> seen_file_names;
    do {
        if (!list_all && std::find(special_mpq_files.begin(), special_mpq_files.end(),
                                   find_data.cFileName) != special_mpq_files.end()) {
            continue;
        }

        if (list_detailed) {
            if (seen_file_names.find(find_data.cFileName) != seen_file_names.end()) {
                continue;
            }
            seen_file_names.insert(find_data.cFileName);

            // Multiple files can be stored with identical filenames under different locales
            DWORD max_locales = 32; // Updated in place by SFileEnumLocales
            std::vector<LCID> file_locale_vec(max_locales);
            LCID *file_locales = file_locale_vec.data();

            DWORD result =
                SFileEnumLocales(archive, find_data.cFileName, file_locales, &max_locales, 0);

            if (result == ERROR_INVALID_PARAMETER) {
                // StormLib rejects a name it cannot resolve (IsPseudoFileName) before
                // touching file_locales or max_locales, so the unknown name is listed
                // once under the default locale
                max_locales = 1;
                file_locales[0] = default_locale;

            } else if (result == ERROR_INVALID_HANDLE || result == ERROR_NOT_SUPPORTED) {
                err << "[!] Internal error for file: " << find_data.cFileName << std::endl;
                continue;

            } else if (result == ERROR_INSUFFICIENT_BUFFER) {
                err << "[!] There are more than " << max_locales
                    << " locales for the file: " << find_data.cFileName << ". Will only list the "
                    << max_locales << " first files." << std::endl;
            }

            for (DWORD i = 0; i < max_locales; i++) {
                LCID locale = file_locales[i];
                SFileSetLocale(locale);
                HANDLE file;

                if (!SFileOpenFileEx(archive, find_data.cFileName, SFILE_OPEN_FROM_MPQ, &file)) {
                    err << "[!] Failed to open file: " << find_data.cFileName << std::endl;
                    continue;
                }

                for (const auto &prop : properties_to_print) {
                    auto it = property_info_class.find(prop);
                    if (it == property_info_class.end())
                        continue;

                    if (prop == "hash-index" || prop == "file-index") {
                        out << std::setw(5) << GetFileInfo<int32_t>(file, it->second) << " ";
                    } else if (prop == "name-hash1" || prop == "name-hash2") {
                        out << std::setfill('0') << std::hex << std::setw(8)
                            << GetFileInfo<int32_t>(file, it->second) << std::setfill(' ')
                            << std::dec << " ";
                    } else if (prop == "name-hash3") {
                        out << std::setfill('0') << std::hex << std::setw(16)
                            << GetFileInfo<int64_t>(file, it->second) << std::setfill(' ')
                            << std::dec << " ";
                    } else if (prop == "locale") {
                        // StormLib packs the hash entry's platform byte into bits 16 to 23 of
                        // the LCID (SFILE_MAKE_LCID); only the low 16 bits are the locale
                        const LCID file_locale = SFILE_LOCALE(GetFileInfo<LCID>(file, it->second));
                        out << std::setw(4) << LocaleToLang(file_locale) << " ";
                    } else if (prop == "byte-offset") {
                        out << std::hex << std::setw(8) << GetFileInfo<int64_t>(file, it->second)
                            << std::dec << " ";
                    } else if (prop == "file-time") {
                        out << std::setw(19)
                            << FileTimeToLsTime(GetFileInfo<int64_t>(file, it->second)) << " ";
                    } else if (prop == "file-size" || prop == "compressed-size") {
                        out << std::setw(8) << GetFileInfo<uint32_t>(file, it->second) << " ";
                    } else if (prop == "flags") {
                        out << std::setw(8)
                            << GetFlagString(GetFileInfo<uint32_t>(file, it->second)) << " ";
                    } else if (prop == "encryption-key" || prop == "encryption-key-raw") {
                        out << std::setfill('0') << std::hex << std::setw(8)
                            << GetFileInfo<int64_t>(file, it->second) << std::setfill(' ')
                            << std::dec << " ";
                    }
                }

                out << " " << find_data.cFileName << std::endl;
                SFileCloseFile(file);
            }
            SFileSetLocale(default_locale);
        } else {
            out << find_data.cFileName << std::endl;
        }

    } while (SFileFindNextFile(find_handle, &find_data));

    SFileFindClose(find_handle);
}

} // namespace mpqcli
