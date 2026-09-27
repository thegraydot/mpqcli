#include "commands/extract.h"

#include <atomic>
#include <filesystem>
#include <ostream>
#include <system_error>

#include <StormLib.h>

#include "errors.h"
#include "mpq/archive.h"
#include "mpq/extract.h"
#include "util/locales.h"

namespace fs = std::filesystem;

namespace mpqcli {

bool Extract(const ExtractOptions &options, std::ostream &err, const std::atomic<bool> &cancelled) {
    std::error_code ec;
    fs::path effective_output;
    if (!options.output.has_value()) {
        fs::path target_path = fs::absolute(options.target, ec);
        if (ec) {
            throw FileError("Failed to resolve archive path", options.target, ec);
        }
        effective_output = target_path.parent_path() / target_path.stem();
    } else {
        effective_output = options.output.value();
    }
    fs::create_directory(effective_output, ec);
    if (ec) {
        std::error_code query_ec;
        if (!fs::is_directory(effective_output, query_ec)) {
            throw FileError("Failed to create output directory", effective_output, ec);
        }
    }

    // ExtractFile can only check for symlink traversal where the OS can resolve
    // real paths; warn once up front on volumes where it cannot (RAM disks)
    static_cast<void>(fs::canonical(effective_output, ec));
    if (ec) {
        err << "[!] Output directory cannot be fully resolved, symlinks will not "
               "be checked during extraction: "
            << effective_output.string() << std::endl;
    }

    Archive archive = Archive::Open(options.target, MPQ_OPEN_READ_ONLY);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;
    if (options.locale.has_value() && lcid == default_locale) {
        err << "[!] The locale '" << options.locale.value()
            << "' is unknown. Will use default locale instead." << std::endl;
    }

    int result;
    if (options.file.has_value()) {
        result = ExtractFile(archive.Handle(), effective_output, options.file.value(),
                             options.keep_folder_structure, lcid, err);
    } else {
        result = ExtractFiles(archive.Handle(), effective_output, options.listfile, lcid, err,
                              cancelled);
    }
    archive.Close();

    if (result != 0) {
        err << std::endl << "[!] Failed to extract all files." << std::endl;
    }
    return result == 0;
}

} // namespace mpqcli
