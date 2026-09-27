#include "mpq/extract.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>

#include <StormLib.h>

#include "errors.h"
#include "mpq/query.h"
#include "util/format.h"
#include "util/locales.h"
#include "util/path.h"

namespace fs = std::filesystem;

namespace mpqcli {

int ExtractFiles(HANDLE archive, const fs::path &output,
                 const std::optional<fs::path> &listfile_name, LCID preferred_locale,
                 std::ostream &err, const std::atomic<bool> &cancelled) {
    SFileSetLocale(preferred_locale);
    const std::string listfile_string =
        listfile_name.has_value() ? listfile_name->string() : std::string();
    const char *listfile = listfile_name.has_value() ? listfile_string.c_str() : nullptr;

    SFILE_FIND_DATA find_data;
    HANDLE find_handle = SFileFindFirstFile(archive, "*", &find_data, listfile);
    if (find_handle == nullptr) {
        throw ArchiveError("Failed to find first file in MPQ archive.");
    }

    int32_t result = 0;
    do {
        // Closed by hand rather than through ThrowIfCancelled, so the find handle
        // does not leak on the way out
        if (cancelled.load(std::memory_order_relaxed)) {
            SFileFindClose(find_handle);
            throw Interrupted{};
        }
        result |= ExtractFile(archive, output, find_data.cFileName,
                              true, // Keep folder structure
                              preferred_locale, err);
    } while (SFileFindNextFile(find_handle, &find_data));

    SFileFindClose(find_handle);
    return result;
}

int ExtractFile(HANDLE archive, const fs::path &output, const std::string &file_name,
                bool keep_folder_structure, LCID preferred_locale, std::ostream &err) {
    SFileSetLocale(preferred_locale);
    if (!FileExistsInArchiveForLocale(archive, file_name.c_str(), preferred_locale) &&
        !FileExistsInArchiveForLocale(archive, file_name.c_str(), default_locale)) {
        err << "[!] Failed: File doesn't exist"
            << PrettyPrintLocale(preferred_locale, " for locale ", true) << ": " << file_name
            << std::endl;
        return 1;
    }

    fs::path file_name_path(file_name);
    std::string file_name_string = NormalizeFilePath(file_name_path);

    if (!keep_folder_structure) {
        file_name_path = fs::path(file_name_string);
        file_name_string = file_name_path.filename().u8string();
    }

    std::error_code ec;
    fs::path output_path_base = fs::absolute(output, ec).lexically_normal();
    if (ec) {
        err << "[!] Failed to resolve output directory: (" << ec.value() << ") " << ec.message()
            << ": " << output.string() << std::endl;
        return 1;
    }
    if (output_path_base.filename().empty()) {
        output_path_base = output_path_base.parent_path();
    }

    // Guard against path traversal attacks in two stages. First lexically, so a
    // ".." entry is rejected before anything is created on disk.
    fs::path output_file_path_name = (output_path_base / file_name_string).lexically_normal();
    if (!IsWithinDirectory(output_path_base, output_file_path_name)) {
        err << "[!] Blocked: path traversal attempt detected: " << file_name_string << std::endl;
        return 1;
    }

    // Ensure sub-directories for folder-nested files exist before resolving
    fs::create_directories(output_file_path_name.parent_path(), ec);
    if (ec) {
        err << "[!] Failed to create output directory: (" << ec.value() << ") " << ec.message()
            << ": " << output_file_path_name.parent_path().string() << std::endl;
        return 1;
    }

    // Second, through the OS to also catch symlinks. Volumes that cannot report
    // real paths (RAM disks) fail here, in which case the lexical check above is
    // the only guard, and the caller warns about that once.
    fs::path resolved_base = fs::canonical(output_path_base, ec);
    if (!ec) {
        fs::path resolved_output = fs::canonical(output_file_path_name.parent_path(), ec) /
                                   output_file_path_name.filename();
        if (ec) {
            err << "[!] Failed to resolve output path: (" << ec.value() << ") " << ec.message()
                << ": " << output_file_path_name.string() << std::endl;
            return 1;
        }
        if (!IsWithinDirectory(resolved_base, resolved_output)) {
            err << "[!] Blocked: path traversal attempt detected: " << file_name_string
                << std::endl;
            return 1;
        }
    }

    std::string output_file_name{output_file_path_name.string()};

    if (SFileExtractFile(archive, file_name.c_str(), output_file_name.c_str(), 0)) {
        err << "[*] Extracted: " << file_name_string << std::endl;
    } else {
        const auto error = SErrGetLastError();
        err << "[!] Failed: (" << error << ") " << StormErrorString(error) << ": " << file_name
            << std::endl;
        return 1;
    }

    return 0;
}

} // namespace mpqcli
