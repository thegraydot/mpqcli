#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>

#include "subprocess_helper.h"
#include "test_environment.h"

using namespace mpqcli::testing;

TEST_CASE("Run: survives a command that exits without reading its stdin", "[helper]") {
    // More than a pipe holds, so the write cannot finish before the child exits and is
    // certain to meet a closed pipe; a few bytes would only race the exit
    constexpr std::size_t more_than_a_pipe_holds = 1024 * 1024;
    RunOptions options;
    options.stdin_input = std::string(more_than_a_pipe_holds, 'x');

    // version rejects the extra argument before it would ever read stdin
    auto result = Run(TestEnvironment::Instance().binary_path, {"version", "extra"}, options);

    // 109 is CLI11's ExtrasError
    REQUIRE(result.returncode == 109);
    REQUIRE_FALSE(result.timed_out);
}
