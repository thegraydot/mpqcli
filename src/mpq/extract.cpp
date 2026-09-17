#include "mpq/extract.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include <StormLib.h>

#include "mpq/query.h"
#include "util/format.h"
#include "util/locales.h"
#include "util/path.h"

namespace mpqcli {

namespace fs = std::filesystem;

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

    std::error_code ec;
    fs::path output_path_base = fs::absolute(output, ec).lexically_normal();
    if (ec) {
        std::cerr << "[!] Failed to resolve output directory: (" << ec.value() << ") "
                  << ec.message() << ": " << output << std::endl;
        return 1;
    }
    if (output_path_base.filename().empty()) {
        output_path_base = output_path_base.parent_path();
    }

    // Guard against path traversal attacks in two stages. First lexically, so a
    // ".." entry is rejected before anything is created on disk
    fs::path output_file_path_name = (output_path_base / file_name_string).lexically_normal();
    if (!IsWithinDirectory(output_path_base, output_file_path_name)) {
        std::cerr << "[!] Blocked: path traversal attempt detected: " << file_name_string
                  << std::endl;
        return 1;
    }

    // Ensure sub-directories for folder-nested files exist before resolving
    fs::create_directories(output_file_path_name.parent_path(), ec);
    if (ec) {
        std::cerr << "[!] Failed to create output directory: (" << ec.value() << ") "
                  << ec.message() << ": " << output_file_path_name.parent_path().u8string()
                  << std::endl;
        return 1;
    }

    // Second, through the OS to also catch symlinks. Volumes that cannot report
    // real paths (RAM disks) fail here, in which case the lexical check above is
    // the only guard; HandleExtract warns about this once
    fs::path resolved_base = fs::canonical(output_path_base, ec);
    if (!ec) {
        fs::path resolved_output = fs::canonical(output_file_path_name.parent_path(), ec) /
                                   output_file_path_name.filename();
        if (ec) {
            std::cerr << "[!] Failed to resolve output path: (" << ec.value() << ") "
                      << ec.message() << ": " << output_file_path_name.u8string() << std::endl;
            return 1;
        }
        if (!IsWithinDirectory(resolved_base, resolved_output)) {
            std::cerr << "[!] Blocked: path traversal attempt detected: " << file_name_string
                      << std::endl;
            return 1;
        }
    }

    std::string output_file_name{output_file_path_name.u8string()};

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

} // namespace mpqcli
