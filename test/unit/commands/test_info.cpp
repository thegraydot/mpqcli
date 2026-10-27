#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sstream>

#include "commands/info.h"
#include "created_archive.h"

using Catch::Matchers::ContainsSubstring;
using mpqcli::Info;
using mpqcli::InfoOptions;
using mpqcli::testing::CreatedArchive;

TEST_CASE("Info", "[info]") {
    CreatedArchive created;
    InfoOptions options;
    options.archive = created.archive;
    std::ostringstream out;

    SECTION("prints only the value of a named property") {
        options.property = "format-version";
        Info(options, out);
        REQUIRE(out.str() == "1\n");
    }

    SECTION("prints every property by name when none is named") {
        Info(options, out);
        REQUIRE_THAT(out.str(), ContainsSubstring("Format version: 1\n"));
        REQUIRE_THAT(out.str(), ContainsSubstring("Signature type: None\n"));
    }
}
