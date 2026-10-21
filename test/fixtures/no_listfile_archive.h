#ifndef NO_LISTFILE_ARCHIVE_H
#define NO_LISTFILE_ARCHIVE_H

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "subprocess_helper.h"
#include "temp_dir.h"
#include "test_environment.h"
#include "write_file.h"

namespace mpqcli::testing {

/// An archive with no internal (listfile) and one entry per locale
///
/// Created with --file-flags1 0 so no (listfile) is written, then two more files are
/// added under a language code and a hexadecimal LCID. Without a listfile the entries
/// list as File0000000N.xxx, which is what the list, rename and compact tests need; a
/// test that wants the names back writes an external listfile of its own. A failed
/// create or add throws, since that is a broken environment rather than a result.
struct NoListfileArchive {
    /// Name, locale and content of one entry; an empty locale is the default one
    struct Entry {
        std::string name;
        std::string locale;
        std::string content;
    };

    TempDir dir;
    std::filesystem::path files_dir;
    std::filesystem::path archive;

    /// The entries in the order they went in
    static std::vector<Entry> Entries() {
        return {
            {"capybaras.txt", "", "This is a file about capybaras."},
            {"cats.txt", "deDE", "Dies ist eine Datei ueber Katzen."},
            {"dogs.txt", "041D", "Detta aer en fil om hundar."},
        };
    }

    NoListfileArchive()
        : files_dir(dir.Path() / "locale_files"),
          archive(dir.Path() / "mpq_without_internal_listfile.mpq") {
        const auto &binary = TestEnvironment::Instance().binary_path;
        for (const auto &entry : Entries()) {
            const auto path = files_dir / entry.name;
            WriteFile(path, entry.content);
            // The first entry creates the archive from the directory, which holds
            // only that file at this point; the rest are added under their locale
            const auto result =
                entry.locale.empty()
                    ? Run(binary,
                          {"create", archive.string(), "--file-flags1", "0", files_dir.string()})
                    : Run(binary,
                          {"add", archive.string(), path.string(), "--locale", entry.locale});
            if (result.returncode != 0) {
                throw std::runtime_error("NoListfileArchive: building " + entry.name + " exited " +
                                         std::to_string(result.returncode) + ": " +
                                         result.stderr_output);
            }
        }
    }
};

} // namespace mpqcli::testing

#endif // NO_LISTFILE_ARCHIVE_H
