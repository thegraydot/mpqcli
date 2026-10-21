#ifndef CREATED_ARCHIVE_H
#define CREATED_ARCHIVE_H

#include <atomic>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>

#include <StormLib.h>

#include "commands/create.h"
#include "mpq/archive.h"
#include "mpq/query.h"
#include "sample_files.h"
#include "util/locales.h"

namespace mpqcli::testing {

/// Reports whether the archive holds a file by that name under the default locale
///
/// Opens the archive read-only for the one query and closes it again, so the test can
/// go on to change or remove the archive, which Windows refuses while a handle is open.
inline bool ArchiveHasFile(const std::filesystem::path &archive, const std::string &name) {
    const Archive opened = Archive::Open(archive, MPQ_OPEN_READ_ONLY);
    return FileExistsInArchiveForLocale(opened.Handle(), name, default_locale);
}

/// An archive built over SampleFiles by calling Create directly, one per test
///
/// The unit layer's counterpart to SampleArchive, for tests that link the core: the
/// archive holds each sample file at its root under the generic profile. A failed
/// create throws, since that is a broken fixture rather than a result.
struct CreatedArchive {
    SampleFiles files;
    std::filesystem::path archive;

    CreatedArchive() : archive(files.dir.Path() / "files.mpq") {
        CreateOptions options;
        options.archive = archive;
        options.files = {files.files_dir};
        std::ostringstream err;
        const std::atomic<bool> cancelled{false};
        if (!Create(options, err, cancelled)) {
            throw std::runtime_error("CreatedArchive: create failed: " + err.str());
        }
    }
};

/// Creates an archive over input with no (listfile), by calling Create directly
///
/// The entries then have no names, and an empty directory as input gives an archive with
/// no entries at all. A failed create throws, since that is a broken fixture rather than
/// a result.
inline void CreateWithoutListfile(const std::filesystem::path &archive,
                                  const std::filesystem::path &input) {
    CreateOptions options;
    options.archive = archive;
    options.files = {input};
    options.create_overrides.file_flags1 = 0U;
    std::ostringstream err;
    const std::atomic<bool> cancelled{false};
    if (!Create(options, err, cancelled)) {
        throw std::runtime_error("CreateWithoutListfile: create failed: " + err.str());
    }
}

} // namespace mpqcli::testing

#endif // CREATED_ARCHIVE_H
