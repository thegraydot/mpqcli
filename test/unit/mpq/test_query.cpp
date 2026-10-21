#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <optional>
#include <sstream>

#include "mpq/query.h"
#include "temp_dir.h"
#include "write_file.h"

namespace fs = std::filesystem;

using mpqcli::UsableListfile;
using mpqcli::testing::TempDir;
using mpqcli::testing::WriteFile;

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
