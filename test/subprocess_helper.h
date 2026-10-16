#ifndef SUBPROCESS_HELPER_H
#define SUBPROCESS_HELPER_H

#include <chrono>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace mpqcli::testing {

/// How to run the binary: optional stdin content, a working directory and a wall-clock limit
struct RunOptions {
    std::optional<std::string> stdin_input;
    std::optional<std::filesystem::path> working_dir;
    std::chrono::seconds timeout{60};
};

/// What the binary did: both streams as raw bytes, its exit code, and whether the limit hit
///
/// returncode is -1 when the process was killed for exceeding the timeout.
struct RunResult {
    std::string stdout_output;
    std::string stderr_output;
    int returncode = -1;
    bool timed_out = false;
};

/// Spawns binary with args, waits for it to exit, and returns what it did
///
/// Every functional test is written against this signature; how it is implemented is
/// the helper's own business. Throws std::runtime_error if the process cannot be spawned
/// at all, which is a broken test environment rather than a result to assert on.
///
/// @param binary Path to the executable to run.
/// @param args Arguments after the program name, one string each, unquoted.
/// @param options Optional stdin content, working directory and timeout.
/// @return The captured streams and exit code.
RunResult Run(const std::filesystem::path &binary, const std::vector<std::string> &args,
              const RunOptions &options = {});

/// Splits output into a set of lines for order-independent comparison
///
/// A trailing carriage return is stripped from each line, so output captured on Windows
/// compares equal to the same output on POSIX.
inline std::set<std::string> LinesToSet(const std::string &output, bool skip_empty = false) {
    std::set<std::string> result;
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (skip_empty && line.empty()) {
            continue;
        }
        result.insert(line);
    }
    return result;
}

/// Splits output into its lines in order, for output where order or repetition matters
///
/// A trailing carriage return is stripped from each line, as LinesToSet does. An empty
/// output gives no lines.
inline std::vector<std::string> LinesToVector(const std::string &output) {
    std::vector<std::string> result;
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        result.push_back(line);
    }
    return result;
}

/// Returns output with trailing newlines removed, for a command that prints one value
inline std::string TrimTrailingNewlines(std::string output) {
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    return output;
}

} // namespace mpqcli::testing

#endif // SUBPROCESS_HELPER_H
