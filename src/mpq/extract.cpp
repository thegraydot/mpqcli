#include "mpq/extract.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <StormLib.h>

#include "errors.h"
#include "mpq/query.h"
#include "util/format.h"
#include "util/locales.h"
#include "util/path.h"
#include "util/string.h"

namespace fs = std::filesystem;

namespace mpqcli {

namespace {

/// Returns the path, relative to the output directory, a file is extracted to
std::string ExtractedName(const std::string &file_name, bool flat) {
    std::string name = NormalizeFilePath(fs::path(file_name));
    if (flat) {
        name = fs::path(name).filename().u8string();
    }
    return name;
}

} // namespace

std::vector<ArchivedFile> ListArchivedFiles(HANDLE archive,
                                            const std::optional<fs::path> &listfile_name,
                                            const std::atomic<bool> &cancelled) {
    const std::string listfile_string =
        listfile_name.has_value() ? listfile_name->string() : std::string();
    const char *listfile = listfile_name.has_value() ? listfile_string.c_str() : nullptr;

    SFILE_FIND_DATA find_data;
    HANDLE find_handle = SFileFindFirstFile(archive, "*", &find_data, listfile);
    if (find_handle == nullptr) {
        // ERROR_NO_MORE_FILES is an archive with no entries: nothing to name, not a failure
        const auto error = SErrGetLastError();
        if (error == ERROR_NO_MORE_FILES) {
            return {};
        }
        throw StormError("Failed to find first file in MPQ archive", error);
    }

    std::vector<ArchivedFile> files;
    do {
        // Closed by hand rather than through ThrowIfCancelled, so the find handle
        // does not leak on the way out
        if (cancelled.load(std::memory_order_relaxed)) {
            SFileFindClose(find_handle);
            throw Interrupted{};
        }
        files.push_back({find_data.cFileName, find_data.lcLocale});
    } while (SFileFindNextFile(find_handle, &find_data));

    SFileFindClose(find_handle);
    return files;
}

std::vector<FileToExtract> ChooseVariants(const std::vector<ArchivedFile> &files,
                                          LCID preferred_locale) {
    std::vector<std::string> names;
    std::map<std::string, std::vector<LCID>> locales_by_name;
    for (const auto &file : files) {
        auto &locales = locales_by_name[file.name];
        if (locales.empty()) {
            names.push_back(file.name);
        }
        locales.push_back(file.locale);
    }

    // Compared without the platform StormLib packs into bits 16 to 23, as list
    // prints them; the chosen locale keeps it, so the lookup finds that exact entry
    const auto by_locale = [](LCID a, LCID b) { return SFILE_LOCALE(a) < SFILE_LOCALE(b); };
    const auto stored_under = [](const std::vector<LCID> &locales, LCID wanted) {
        return std::find_if(locales.begin(), locales.end(), [wanted](LCID locale) {
            return SFILE_LOCALE(locale) == SFILE_LOCALE(wanted);
        });
    };

    std::vector<FileToExtract> chosen;
    for (const auto &name : names) {
        auto locales = locales_by_name[name];
        std::stable_sort(locales.begin(), locales.end(), by_locale);
        auto pick = stored_under(locales, preferred_locale);
        if (pick == locales.end()) {
            pick = stored_under(locales, default_locale);
        }
        if (pick == locales.end()) {
            pick = locales.begin();
        }
        const LCID locale = *pick;
        locales.erase(pick);
        chosen.push_back({name, locale, locales});
    }
    return chosen;
}

std::optional<std::pair<std::string, std::string>>
FindFlatCollision(const std::vector<std::string> &file_names) {
    // Compared without case: MPQ names ignore it, and so do the default Windows
    // and macOS filesystems, where the second file would overwrite the first
    std::map<std::string, std::string> first_by_extracted_name;
    for (const auto &name : file_names) {
        auto [it, inserted] =
            first_by_extracted_name.emplace(ToLower(ExtractedName(name, true)), name);
        // The same file named twice, or listed once per locale, lands on itself
        if (!inserted &&
            ToLower(ExtractedName(it->second, false)) != ToLower(ExtractedName(name, false))) {
            return std::make_pair(it->second, name);
        }
    }
    return std::nullopt;
}

int ExtractFiles(HANDLE archive, const fs::path &output, const std::vector<FileToExtract> &files,
                 bool flat, std::ostream &err, const std::atomic<bool> &cancelled) {
    int result = 0;
    for (const auto &file : files) {
        ThrowIfCancelled(cancelled);
        result |= ExtractFile(archive, output, file.name, flat, file.locale, err);
        if (!file.left_out.empty()) {
            err << "[*] Left out other locales of " << file.name << ": ";
            for (std::size_t i = 0; i < file.left_out.size(); i++) {
                err << (i == 0 ? "" : ", ") << LocaleToLang(SFILE_LOCALE(file.left_out[i]));
            }
            err << std::endl;
        }
    }
    return result;
}

int ExtractFile(HANDLE archive, const fs::path &output, const std::string &file_name, bool flat,
                LCID preferred_locale, std::ostream &err) {
    SFileSetLocale(preferred_locale);
    const std::string stored_name = StoredFileName(archive, file_name, preferred_locale);
    const bool has_preferred = FileExistsInArchiveForLocale(archive, stored_name, preferred_locale);
    if (!has_preferred && !FileExistsInArchiveForLocale(archive, stored_name, default_locale)) {
        err << "[!] Failed: File doesn't exist"
            << PrettyPrintLocale(preferred_locale, " for locale ", true) << ": " << stored_name
            << std::endl;
        return 1;
    }
    // Only a named file can fall back: extracting everything asks for each
    // variant under the locale it is stored in
    if (!has_preferred) {
        ReportLocaleFallback(stored_name, preferred_locale, err);
    }

    const std::string file_name_string = ExtractedName(stored_name, flat);

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

    if (SFileExtractFile(archive, stored_name.c_str(), output_file_name.c_str(), 0)) {
        err << "[*] Extracted: " << file_name_string << std::endl;
    } else {
        const auto error = SErrGetLastError();
        err << "[!] Failed: " << StormErrorString(error) << ": " << stored_name << std::endl;
        return 1;
    }

    return 0;
}

} // namespace mpqcli
