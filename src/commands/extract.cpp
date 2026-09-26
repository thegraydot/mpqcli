#include "commands/extract.h"

#include <filesystem>
#include <ostream>
#include <string>
#include <system_error>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/extract.h"
#include "util/locales.h"

namespace fs = std::filesystem;

namespace mpqcli {

bool Extract(const ExtractOptions &options, std::ostream &out, std::ostream &err) {
    // If no output directory specified, use MPQ path without extension
    // If output directory specified, create it if it doesn't exist
    std::error_code ec;
    std::string effective_output;
    if (!options.output.has_value()) {
        fs::path target_path = fs::absolute(options.target, ec);
        if (ec) {
            err << "[!] Failed to resolve archive path: (" << ec.value() << ") " << ec.message()
                << ": " << options.target << std::endl;
            return false;
        }
        effective_output = (target_path.parent_path() / target_path.stem()).u8string();
    } else {
        effective_output = options.output.value();
    }
    fs::create_directory(effective_output, ec);
    if (ec) {
        std::error_code query_ec;
        if (!fs::is_directory(effective_output, query_ec)) {
            err << "[!] Failed to create output directory: (" << ec.value() << ") " << ec.message()
                << ": " << effective_output << std::endl;
            return false;
        }
    }

    // ExtractFile can only check for symlink traversal where the OS can resolve
    // real paths; warn once up front on volumes where it cannot (RAM disks)
    static_cast<void>(fs::canonical(effective_output, ec));
    if (ec) {
        out << "[!] Warning: Output directory cannot be fully resolved, symlinks will not "
               "be checked during extraction: "
            << effective_output << std::endl;
    }

    Archive archive = Archive::Open(options.target, MPQ_OPEN_READ_ONLY);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;
    if (options.locale.has_value() && lcid == default_locale) {
        out << "[!] Warning: The locale '" << options.locale.value()
            << "' is unknown. Will use default locale instead." << std::endl;
    }

    int result;
    if (options.file.has_value()) {
        result = ExtractFile(archive.Handle(), effective_output, options.file.value(),
                             options.keep_folder_structure, lcid, out, err);
    } else {
        result = ExtractFiles(archive.Handle(), effective_output, options.listfile, lcid, out, err);
    }
    archive.Close();

    if (result != 0) {
        err << std::endl << "[!] Failed to extract all files." << std::endl;
    }
    return result == 0;
}

} // namespace mpqcli
