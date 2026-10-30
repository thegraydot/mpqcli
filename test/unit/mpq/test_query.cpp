#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <optional>
#include <sstream>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/query.h"
#include "raw_archive.h"
#include "temp_dir.h"
#include "util/locales.h"
#include "write_file.h"

namespace fs = std::filesystem;

using mpqcli::Archive;
using mpqcli::default_locale;
using mpqcli::StoredFileName;
using mpqcli::UsableListfile;
using mpqcli::testing::TempDir;
using mpqcli::testing::WriteFile;
using mpqcli::testing::WriteRawArchive;

namespace {

constexpr LCID german = 0x407;

} // namespace

TEST_CASE("StoredFileName", "[query]") {
    TempDir dir;
    const auto path = dir.Path() / "slashes.mpq";
    WriteRawArchive(path, {{"back\\slash.txt", default_locale},
                           {"forward/slash.txt", default_locale},
                           {"both/forms.txt", default_locale},
                           {"both\\forms.txt", default_locale},
                           {"caf\xC3\xA9\\menu.txt", default_locale},
                           {"german\\only.txt", german}});
    const auto archive = Archive::Open(path, MPQ_OPEN_READ_ONLY);

    SECTION("finds a path stored with backslashes when given forward slashes") {
        REQUIRE(StoredFileName(archive.Handle(), "back/slash.txt", default_locale) ==
                "back\\slash.txt");
    }

    SECTION("keeps a path given with the backslashes it is stored under") {
        REQUIRE(StoredFileName(archive.Handle(), "back\\slash.txt", default_locale) ==
                "back\\slash.txt");
    }

    SECTION("keeps a path stored with forward slashes as given") {
        REQUIRE(StoredFileName(archive.Handle(), "forward/slash.txt", default_locale) ==
                "forward/slash.txt");
    }

    SECTION("prefers the path as given when the archive stores both forms") {
        REQUIRE(StoredFileName(archive.Handle(), "both/forms.txt", default_locale) ==
                "both/forms.txt");
        REQUIRE(StoredFileName(archive.Handle(), "both\\forms.txt", default_locale) ==
                "both\\forms.txt");
    }

    SECTION("keeps the bytes of a name outside ASCII") {
        // On Windows a conversion through filesystem::path would re-encode these bytes
        // from the ANSI code page, and the lookup would miss
        REQUIRE(StoredFileName(archive.Handle(), "caf\xC3\xA9/menu.txt", default_locale) ==
                "caf\xC3\xA9\\menu.txt");
    }

    SECTION("returns a path found in neither form as given") {
        REQUIRE(StoredFileName(archive.Handle(), "missing/file.txt", default_locale) ==
                "missing/file.txt");
    }

    SECTION("looks the path up under the given locale") {
        REQUIRE(StoredFileName(archive.Handle(), "german/only.txt", german) == "german\\only.txt");
        REQUIRE(StoredFileName(archive.Handle(), "german/only.txt", default_locale) ==
                "german/only.txt");
    }
}

TEST_CASE("UsableListfile", "[query]") {
    TempDir dir;
    std::ostringstream err;

    SECTION("passes no listfile through as none") {
        REQUIRE_FALSE(UsableListfile(std::nullopt, err).has_value());
        REQUIRE(err.str().empty());
    }

    SECTION("returns a listfile with names in it unchanged") {
        const auto listfile = dir.Path() / "listfile.txt";
        WriteFile(listfile, "cats.txt\n");
        REQUIRE(UsableListfile(listfile, err) == std::optional<fs::path>(listfile));
        REQUIRE(err.str().empty());
    }

    SECTION("drops an empty listfile with a warning") {
        const auto listfile = dir.Path() / "empty.txt";
        WriteFile(listfile, "");
        REQUIRE_FALSE(UsableListfile(listfile, err).has_value());
        REQUIRE(err.str() ==
                "[!] Ignoring the listfile, which is empty: " + listfile.string() + "\n");
    }

    SECTION("leaves a missing listfile for StormLib to report") {
        const auto listfile = dir.Path() / "missing.txt";
        REQUIRE(UsableListfile(listfile, err) == std::optional<fs::path>(listfile));
        REQUIRE(err.str().empty());
    }
}
