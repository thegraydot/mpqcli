#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstddef>
#include <string>

#ifndef _WIN32
#include <csignal>
#endif

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

// mpqcli cannot be made to crash or hang on demand, so a shell stands in for it. There is
// no /bin/sh on Windows, and no signals to report.
#ifndef _WIN32
TEST_CASE("Run: reports a child killed by a signal as a shell does", "[helper]") {
    // The expected message comes first, so only the exit code tells a crash from a failure
    auto result = Run("/bin/sh", {"-c", "echo '[!] expected message' >&2; kill -SEGV $$"});

    REQUIRE(result.returncode == 128 + SIGSEGV);
    REQUIRE(result.stderr_output == "[!] expected message\n");
    REQUIRE_FALSE(result.timed_out);
}

TEST_CASE("Run: kills a child that outlives its timeout", "[helper]") {
    RunOptions options;
    options.timeout = std::chrono::seconds(1);

    // exec, so the kill reaches sleep itself rather than a shell that would leave sleep
    // holding the pipes open
    auto result = Run("/bin/sh", {"-c", "exec sleep 30"}, options);

    REQUIRE(result.timed_out);
    REQUIRE(result.returncode == -1);
}

TEST_CASE("Run: reads the output while a large stdin is still being written", "[helper]") {
    // The child ignores its stdin and fills its stdout, each with more than a pipe holds,
    // which deadlocks a helper that writes all the input before reading any output
    constexpr std::size_t more_than_a_pipe_holds = 1024 * 1024;
    RunOptions options;
    options.stdin_input = std::string(more_than_a_pipe_holds, 'x');
    options.timeout = std::chrono::seconds(10);

    auto result = Run("/bin/sh", {"-c", "exec head -c 1048576 /dev/zero"}, options);

    REQUIRE_FALSE(result.timed_out);
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.size() == more_than_a_pipe_holds);
}
#endif
