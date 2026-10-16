#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "subprocess_helper.h"
#include "test_environment.h"

using namespace mpqcli::testing;

TEST_CASE("version: prints the version and commit hash", "[version]") {
    const auto &env = TestEnvironment::Instance();
    auto result = Run(env.binary_path, {"version"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());

    // One line: the project version, a dash, then either the 40-digit commit
    // hash or "unknown" for a build that had no git to ask
    auto lines = LinesToSet(result.stdout_output);
    REQUIRE(lines.size() == 1);
    REQUIRE_THAT(*lines.begin(),
                 Catch::Matchers::Matches("[0-9]+\\.[0-9]+\\.[0-9]+-([0-9a-f]{40}|unknown)"));
}

TEST_CASE("version: rejects extra arguments", "[version]") {
    const auto &env = TestEnvironment::Instance();
    auto result = Run(env.binary_path, {"version", "extra"});

    // 109 is CLI11's ExtrasError, forwarded unchanged by app.exit()
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, Catch::Matchers::ContainsSubstring(
                                           "The following argument was not expected: extra"));
}
