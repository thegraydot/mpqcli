#include "mpq/extract.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"
#include "util/locales.h"

namespace fs = std::filesystem;

int HandleExtract(const std::string &target, const std::optional<std::string> &output,
                  const std::optional<std::string> &file, bool keep_folder_structure,
                  const std::optional<std::string> &listfile_name,
                  const std::optional<std::string> &locale) {
    // If no output directory specified, use MPQ path without extension
    // If output directory specified, create it if it doesn't exist
    std::error_code ec;
    std::string effective_output;
    if (!output.has_value()) {
        fs::path target_path = fs::absolute(target, ec);
        if (ec) {
            std::cerr << "[!] Failed to resolve archive path: (" << ec.value() << ") "
                      << ec.message() << ": " << target << std::endl;
            return 1;
        }
        effective_output = (target_path.parent_path() / target_path.stem()).u8string();
    } else {
        effective_output = output.value();
    }
    fs::create_directory(effective_output, ec);
    if (ec) {
        std::error_code query_ec;
        if (!fs::is_directory(effective_output, query_ec)) {
            std::cerr << "[!] Failed to create output directory: (" << ec.value() << ") "
                      << ec.message() << ": " << effective_output << std::endl;
            return 1;
        }
    }

    // ExtractFile can only check for symlink traversal where the OS can resolve
    // real paths; warn once up front on volumes where it cannot (RAM disks)
    static_cast<void>(fs::canonical(effective_output, ec));
    if (ec) {
        std::cout << "[!] Warning: Output directory cannot be fully resolved, symlinks will not "
                     "be checked during extraction: "
                  << effective_output << std::endl;
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
