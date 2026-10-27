#include <catch2/catch_test_macros.hpp>

#include "util/capacity.h"

using mpqcli::CalculateMpqMaxFileValue;
using mpqcli::NextPowerOfTwo;

TEST_CASE("NextPowerOfTwo", "[capacity]") {
    SECTION("returns an exact power of two unchanged") {
        REQUIRE(NextPowerOfTwo(1U) == 1U);
        REQUIRE(NextPowerOfTwo(2U) == 2U);
        REQUIRE(NextPowerOfTwo(64U) == 64U);
        REQUIRE(NextPowerOfTwo(0x80000000U) == 0x80000000U);
    }

    SECTION("rounds anything else up to the next power of two") {
        REQUIRE(NextPowerOfTwo(3U) == 4U);
        REQUIRE(NextPowerOfTwo(33U) == 64U);
        REQUIRE(NextPowerOfTwo(65U) == 128U);
        REQUIRE(NextPowerOfTwo(0x40000001U) == 0x80000000U);
    }
}

TEST_CASE("CalculateMpqMaxFileValue", "[capacity]") {
    SECTION("never declares fewer than 32 entries") {
        REQUIRE(CalculateMpqMaxFileValue(0U) == 32U);
        REQUIRE(CalculateMpqMaxFileValue(29U) == 32U);
    }

    SECTION("counts three special entries before rounding up") {
        // 30 files and the three special entries make 33, one more than fits in 32
        REQUIRE(CalculateMpqMaxFileValue(30U) == 64U);
        REQUIRE(CalculateMpqMaxFileValue(61U) == 64U);
        REQUIRE(CalculateMpqMaxFileValue(62U) == 128U);
    }
}
