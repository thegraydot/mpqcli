#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "mpq/extract.h"

using mpqcli::FindFlatCollision;

TEST_CASE("FindFlatCollision", "[extract]") {
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
