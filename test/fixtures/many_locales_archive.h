#ifndef MANY_LOCALES_ARCHIVE_H
#define MANY_LOCALES_ARCHIVE_H

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "subprocess_helper.h"
#include "temp_dir.h"
#include "test_environment.h"
#include "write_file.h"

namespace mpqcli::testing {

/// One locale variant of cats.txt; an empty locale is the default one
struct CatsVariant {
    std::string locale;
    std::string content;
};

/// The variants the locale fixtures store, in the order they go in
inline std::vector<CatsVariant> CatsVariants() {
    return {
        {"", "This is a file about cats."},
        {"041D", "Detta aer en fil om katter."},
        {"deDE", "Dies ist eine Datei ueber Katzen."},
        {"esES", "Este es un archivo sobre gatos."},
    };
}

/// The content stored under locale, or under the default locale when empty
inline std::string CatsContent(const std::string &locale) {
    for (const auto &variant : CatsVariants()) {
        if (variant.locale == locale) {
            return variant.content;
        }
    }
    throw std::runtime_error("CatsContent: no variant for locale " + locale);
}

/// An archive holding cats.txt under the default locale and three others
///
/// Created from the default variant, then each other variant is added under its
/// locale, one of them the hexadecimal 041D that the locale table does not name. What
/// the read, extract, remove and rename tests use to tell the variants apart. A failed
/// create or add throws, since that is a broken environment rather than a result.
struct ManyLocalesArchive {
    TempDir dir;
    std::filesystem::path files_dir;
    std::filesystem::path archive;

    ManyLocalesArchive()
        : files_dir(dir.Path() / "locale_files"),
          archive(dir.Path() / "mpq_with_many_locales.mpq") {
        const auto &binary = TestEnvironment::Instance().binary_path;
        const auto cats = files_dir / "cats.txt";
        for (const auto &variant : CatsVariants()) {
            WriteFile(cats, variant.content);
            const auto result =
                variant.locale.empty()
                    ? Run(binary,
                          {"create", "--format-version", "1", archive.string(), files_dir.string()})
                    : Run(binary,
                          {"add", archive.string(), cats.string(), "--locale", variant.locale});
            if (result.returncode != 0) {
                throw std::runtime_error("ManyLocalesArchive: storing locale '" + variant.locale +
                                         "' exited " + std::to_string(result.returncode) + ": " +
                                         result.stderr_output);
            }
        }
    }
};

} // namespace mpqcli::testing

#endif // MANY_LOCALES_ARCHIVE_H
