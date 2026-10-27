#ifndef ONE_LOCALE_ARCHIVE_H
#define ONE_LOCALE_ARCHIVE_H

#include <filesystem>
#include <stdexcept>
#include <string>

#include "many_locales_archive.h"
#include "subprocess_helper.h"
#include "temp_dir.h"
#include "test_environment.h"
#include "write_file.h"

namespace mpqcli::testing {

/// An archive holding cats.txt under esES alone, so no default-locale variant exists
///
/// What a lookup without a locale, or with the wrong one, falls back to when there is
/// nothing to fall back to.
struct OneLocaleArchive {
    static inline const std::string locale = "esES";

    TempDir dir;
    std::filesystem::path archive;

    OneLocaleArchive() : archive(dir.Path() / "mpq_with_one_locale.mpq") {
        const auto files_dir = dir.Path() / "locale_files";
        WriteFile(files_dir / "cats.txt", CatsContent(locale));
        const auto result = Run(TestEnvironment::Instance().binary_path,
                                {"create", "--format-version", "1", archive.string(),
                                 files_dir.string(), "--locale", locale});
        if (result.returncode != 0) {
            throw std::runtime_error("OneLocaleArchive: create exited " +
                                     std::to_string(result.returncode) + ": " +
                                     result.stderr_output);
        }
    }
};

} // namespace mpqcli::testing

#endif // ONE_LOCALE_ARCHIVE_H
