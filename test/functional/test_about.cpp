#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <string>
#include <vector>

#include "subprocess_helper.h"
#include "test_environment.h"

using namespace mpqcli::testing;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("about: prints the program information", "[about]") {
    const auto &env = TestEnvironment::Instance();

    auto result = Run(env.binary_path, {"about"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    // The version line carries what the version subcommand prints, so the
    // two cannot drift apart
    auto version = Run(env.binary_path, {"version"});
    REQUIRE(version.returncode == 0);
    // Fixed text in a fixed order: the lines are the contract, not a set
    REQUIRE(LinesToVector(result.stdout_output) ==
            std::vector<std::string>{
                "Name: mpqcli",
                "Version: " + TrimTrailingNewlines(version.stdout_output),
                "Author: Thomas Laurenson",
                "License: MIT",
                "GitHub: https://github.com/thegraydot/mpqcli",
                "Dependencies:",
                " - StormLib (https://github.com/ladislav-zezula/StormLib)",
                " - CLI11 (https://github.com/CLIUtils/CLI11)",
                " - hash-library (https://github.com/stbrumme/hash-library)",
            });
}

TEST_CASE("about: prints its help with --help", "[about]") {
    const auto &env = TestEnvironment::Instance();

    auto result = Run(env.binary_path, {"about", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("about [OPTIONS]"));
}

TEST_CASE("about: rejects extra arguments", "[about]") {
    const auto &env = TestEnvironment::Instance();

    auto result = Run(env.binary_path, {"about", "extra"});

    // 109 is CLI11's ExtrasError, forwarded unchanged by app.exit()
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: extra"));
}
