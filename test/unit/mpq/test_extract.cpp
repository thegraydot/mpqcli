#include <algorithm>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <vector>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/extract.h"
#include "raw_archive.h"
#include "temp_dir.h"

using mpqcli::Archive;
using mpqcli::ArchivedFile;
using mpqcli::ChooseVariants;
using mpqcli::FindFlatCollision;
using mpqcli::ListArchivedFiles;
using mpqcli::testing::TempDir;
using mpqcli::testing::WriteRawArchive;

namespace {

constexpr LCID neutral = 0;
constexpr LCID german = 0x407;
constexpr LCID spanish = 0x40a;
constexpr LCID french = 0x40c;

} // namespace

TEST_CASE("ListArchivedFiles", "[mpq/extract]") {
    SECTION("lists a name once per locale, with that locale") {
        TempDir dir;
        const auto path = dir.Path() / "locales.mpq";
        WriteRawArchive(path, {{"cats.txt", neutral}, {"cats.txt", german}});
        const auto archive = Archive::Open(path, MPQ_OPEN_READ_ONLY);
        const std::atomic<bool> cancelled{false};

        const auto files = ListArchivedFiles(archive.Handle(), std::nullopt, cancelled);

        std::vector<LCID> cats_locales;
        for (const auto &file : files) {
            if (file.name == "cats.txt") {
                cats_locales.push_back(file.locale);
            }
        }
        std::sort(cats_locales.begin(), cats_locales.end());
        REQUIRE(cats_locales == std::vector<LCID>{neutral, german});
    }
}

TEST_CASE("ChooseVariants", "[mpq/extract]") {
    const std::vector<ArchivedFile> three = {
        {"cats.txt", spanish}, {"cats.txt", neutral}, {"cats.txt", german}};

    SECTION("picks the variant under the preferred locale") {
        const auto chosen = ChooseVariants(three, german);
        REQUIRE(chosen.size() == 1);
        REQUIRE(chosen[0].name == "cats.txt");
        REQUIRE(chosen[0].locale == german);
        REQUIRE(chosen[0].left_out == std::vector<LCID>{neutral, spanish});
    }

    SECTION("picks the neutral variant when the preferred locale is not stored") {
        const auto chosen = ChooseVariants(three, french);
        REQUIRE(chosen.size() == 1);
        REQUIRE(chosen[0].locale == neutral);
        REQUIRE(chosen[0].left_out == std::vector<LCID>{german, spanish});
    }

    SECTION("picks the only variant, stored under another locale") {
        const auto chosen = ChooseVariants({{"cats.txt", german}}, neutral);
        REQUIRE(chosen.size() == 1);
        REQUIRE(chosen[0].locale == german);
        REQUIRE(chosen[0].left_out.empty());
    }

    SECTION("picks the lowest locale when neither the preferred nor neutral is stored") {
        const auto chosen = ChooseVariants({{"cats.txt", spanish}, {"cats.txt", german}}, french);
        REQUIRE(chosen.size() == 1);
        REQUIRE(chosen[0].locale == german);
        REQUIRE(chosen[0].left_out == std::vector<LCID>{spanish});
    }

    SECTION("matches the preferred locale whatever the platform, and keeps the platform") {
        // SFILE_MAKE_LCID(german, 1), which -Wold-style-cast rejects
        const LCID german_on_platform_1 = german | (1U << 16);
        const auto chosen =
            ChooseVariants({{"cats.txt", neutral}, {"cats.txt", german_on_platform_1}}, german);
        REQUIRE(chosen.size() == 1);
        REQUIRE(chosen[0].locale == german_on_platform_1);
    }

    SECTION("lists each name once, in the order it first appears") {
        const auto chosen = ChooseVariants(
            {{"dogs.txt", neutral}, {"cats.txt", german}, {"dogs.txt", german}}, neutral);
        REQUIRE(chosen.size() == 2);
        REQUIRE(chosen[0].name == "dogs.txt");
        REQUIRE(chosen[0].locale == neutral);
        REQUIRE(chosen[0].left_out == std::vector<LCID>{german});
        REQUIRE(chosen[1].name == "cats.txt");
        REQUIRE(chosen[1].locale == german);
        REQUIRE(chosen[1].left_out.empty());
    }

    SECTION("chooses nothing from an empty list") {
        REQUIRE(ChooseVariants({}, neutral).empty());
    }
}

TEST_CASE("FindFlatCollision", "[mpq/extract]") {
    SECTION("finds nothing when every file name differs") {
        REQUIRE_FALSE(FindFlatCollision({"a\\cats.txt", "b\\dogs.txt", "bytes"}).has_value());
    }

    SECTION("finds nothing in an empty list") {
        REQUIRE_FALSE(FindFlatCollision({}).has_value());
    }

    SECTION("finds two folders holding the same name") {
        const auto collision = FindFlatCollision({"a\\cats.txt", "b\\cats.txt"});
        REQUIRE(collision.has_value());
        REQUIRE(collision->first == "a\\cats.txt");
        REQUIRE(collision->second == "b\\cats.txt");
    }

    SECTION("finds names that differ only in case") {
        const auto collision = FindFlatCollision({"a\\Cats.txt", "b\\cats.TXT"});
        REQUIRE(collision.has_value());
        REQUIRE(collision->first == "a\\Cats.txt");
        REQUIRE(collision->second == "b\\cats.TXT");
    }

    SECTION("pairs the colliding file with the first of that name") {
        const auto collision =
            FindFlatCollision({"a\\cats.txt", "b\\dogs.txt", "c\\dogs.txt", "d\\cats.txt"});
        REQUIRE(collision.has_value());
        REQUIRE(collision->first == "b\\dogs.txt");
        REQUIRE(collision->second == "c\\dogs.txt");
    }

    SECTION("finds nothing when the same file is named twice") {
        REQUIRE_FALSE(FindFlatCollision({"a\\cats.txt", "a\\cats.txt"}).has_value());
    }

    SECTION("finds nothing when the same file is named twice in different cases") {
        REQUIRE_FALSE(FindFlatCollision({"a\\cats.txt", "A\\CATS.TXT"}).has_value());
    }
}
