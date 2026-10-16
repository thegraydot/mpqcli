#include <catch2/catch_test_macros.hpp>

#include "util/string.h"

using mpqcli::ToLower;

TEST_CASE("ToLower", "[string]") {
    SECTION("lowercases every ASCII letter") {
        REQUIRE(ToLower("ABCXYZ") == "abcxyz");
        REQUIRE(ToLower("MiXeD.TxT") == "mixed.txt");
    }

    SECTION("leaves digits, punctuation and separators alone") {
        REQUIRE(ToLower("A1-B_2\\C/D.E") == "a1-b_2\\c/d.e");
    }

    SECTION("returns an empty string unchanged") {
        REQUIRE(ToLower("").empty());
    }

    SECTION("leaves bytes outside ASCII alone") {
        REQUIRE(ToLower("\xC3\x89T\xC3\x89") == "\xC3\x89t\xC3\x89");
    }
}
