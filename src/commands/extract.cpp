#include "commands/extract.h"

#include <atomic>
#include <filesystem>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "errors.h"
#include "mpq/archive.h"
#include "mpq/extract.h"
#include "mpq/query.h"
#include "util/locales.h"

namespace fs = std::filesystem;

namespace mpqcli {

bool Extract(const ExtractOptions &options, std::ostream &err, const std::atomic<bool> &cancelled) {
    std::error_code ec;
    fs::path effective_output;
    if (!options.output.has_value()) {
        fs::path archive_path = fs::absolute(options.archive, ec);
        if (ec) {
            throw FileError("Failed to resolve archive path", options.archive, ec);
        }
        effective_output = archive_path.parent_path() / archive_path.stem();
    } else {
        effective_output = options.output.value();
    }

    Archive archive = Archive::Open(options.archive, MPQ_OPEN_READ_ONLY);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;

    // Every name is settled before the output directory is created, so a
    // rejected --flat run leaves nothing on disk
    const std::vector<std::string> file_names =
        options.files.has_value()
            ? options.files.value()
            : ListArchivedFiles(archive.Handle(), UsableListfile(options.listfile, err), cancelled);
    if (file_names.empty()) {
        err << "[*] No files to extract" << std::endl;
        archive.Close();
        return true;
    }
    if (options.flat) {
        const auto collision = FindFlatCollision(file_names);
        if (collision.has_value()) {
            err << "[!] --flat would extract " << collision->first << " and " << collision->second
                << " to the same file" << std::endl;
            archive.Close();
            return false;
        }
    }

    // Remembered so a run that extracts nothing can take away the directory it made
    std::error_code exists_ec;
    const bool output_existed = fs::exists(effective_output, exists_ec);

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

    const int result = ExtractFiles(archive.Handle(), effective_output, file_names, options.flat,
                                    lcid, err, cancelled);
    archive.Close();

    if (result != 0) {
        err << "[!] Failed to extract all files." << std::endl;
        // fs::remove takes away an empty directory only, so a partial extraction stays
        std::error_code cleanup_ec;
        if (!output_existed && fs::is_empty(effective_output, cleanup_ec)) {
            fs::remove(effective_output, cleanup_ec);
        }
    }
    return result == 0;
}

} // namespace mpqcli
