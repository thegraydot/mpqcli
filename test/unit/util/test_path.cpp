#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "temp_dir.h"
#include "util/path.h"
#include "write_file.h"

namespace fs = std::filesystem;

using mpqcli::ExpandStdinMarker;
using mpqcli::IsWithinDirectory;
using mpqcli::ListFilesRecursive;
using mpqcli::LocalFileTimestamp;
using mpqcli::NormalizeFilePath;
using mpqcli::ResolveArchiveName;
using mpqcli::WindowsifyFilePath;
using mpqcli::testing::TempDir;
using mpqcli::testing::WriteFile;

namespace {

// A Windows FILETIME counts 100-nanosecond intervals from 1601-01-01, which is this
// many seconds before the Unix epoch
constexpr uint64_t intervals_per_second = 10000000U;
constexpr uint64_t filetime_epoch_offset = 11644473600U;

} // namespace

TEST_CASE("NormalizeFilePath", "[path]") {
    SECTION("leaves forward slashes alone") {
        REQUIRE(NormalizeFilePath(fs::path("dir/sub/file.txt")) == "dir/sub/file.txt");
    }

    SECTION("makes an archive name's backslashes separators the platform reads") {
#ifdef _WIN32
        REQUIRE(NormalizeFilePath(fs::path("dir\\sub\\file.txt")) == "dir\\sub\\file.txt");
#else
        REQUIRE(NormalizeFilePath(fs::path("dir\\sub\\file.txt")) == "dir/sub/file.txt");
#endif
    }

    SECTION("returns a name outside ASCII as UTF-8") {
        REQUIRE(NormalizeFilePath(fs::u8path("caf\xC3\xA9.txt")) == "caf\xC3\xA9.txt");
    }
}

TEST_CASE("WindowsifyFilePath", "[path]") {
    SECTION("turns forward slashes into backslashes") {
        REQUIRE(WindowsifyFilePath(fs::path("dir/sub/file.txt")) == "dir\\sub\\file.txt");
    }

    SECTION("leaves backslashes alone") {
        REQUIRE(WindowsifyFilePath(fs::path("dir\\sub\\file.txt")) == "dir\\sub\\file.txt");
    }

    SECTION("gives a path joined with the native separator backslashes") {
        REQUIRE(WindowsifyFilePath(fs::path("dir") / "sub" / "file.txt") == "dir\\sub\\file.txt");
    }

    SECTION("returns a name outside ASCII as UTF-8") {
        REQUIRE(WindowsifyFilePath(fs::u8path("dir/caf\xC3\xA9.txt")) == "dir\\caf\xC3\xA9.txt");
    }
}

TEST_CASE("IsWithinDirectory", "[path]") {
    const fs::path base("out");

    SECTION("accepts a file directly under base") {
        REQUIRE(IsWithinDirectory(base, base / "a.txt"));
    }

    SECTION("accepts a file nested several levels under base") {
        REQUIRE(IsWithinDirectory(base, base / "sub" / "deeper" / "a.txt"));
    }

    SECTION("rejects a sibling whose name begins with base's name") {
        REQUIRE_FALSE(IsWithinDirectory(base, fs::path("outside") / "a.txt"));
        REQUIRE_FALSE(IsWithinDirectory(base, fs::path("outside")));
    }

    SECTION("rejects a file in a parent of base") {
        REQUIRE_FALSE(IsWithinDirectory(base / "sub", base / "a.txt"));
    }

    // Components are compared as they stand, so ".." only counts once the caller has
    // normalised the path, as ExtractFile does
    SECTION("rejects a normalised path that climbs out of base") {
        REQUIRE_FALSE(IsWithinDirectory(base, (base / ".." / "etc" / "passwd").lexically_normal()));
        REQUIRE_FALSE(
            IsWithinDirectory(base, (base / "sub" / ".." / ".." / "a.txt").lexically_normal()));
        REQUIRE_FALSE(
            IsWithinDirectory(base, (base / ".." / "outside" / "a.txt").lexically_normal()));
    }

    SECTION("accepts a normalised path whose .. stays inside base") {
        REQUIRE(IsWithinDirectory(base, (base / "sub" / ".." / "a.txt").lexically_normal()));
    }

    SECTION("rejects an absolute path outside an absolute base") {
        const fs::path root = fs::current_path().root_path();
        REQUIRE_FALSE(IsWithinDirectory(root / "out", root / "etc" / "passwd"));
        REQUIRE(IsWithinDirectory(root / "out", root / "out" / "a.txt"));
    }

    SECTION("rejects an absolute path against a relative base") {
        REQUIRE_FALSE(IsWithinDirectory(base, fs::current_path().root_path() / "out" / "a.txt"));
    }
}

TEST_CASE("ResolveArchiveName", "[path]") {
    const fs::path file = fs::path("inputs") / "animals" / "cats.txt";

    SECTION("uses the file's own name when no path is given") {
        REQUIRE(ResolveArchiveName(file, std::nullopt) == "cats.txt");
    }

    SECTION("stores a lone file at the given path") {
        REQUIRE(ResolveArchiveName(file, "data/felines.txt") == "data\\felines.txt");
    }

    SECTION("puts the file's name under the path when treated as a directory") {
        REQUIRE(ResolveArchiveName(file, "data/animals", true) == "data\\animals\\cats.txt");
    }

    SECTION("puts the file at the root when treated as a directory with no path") {
        REQUIRE(ResolveArchiveName(file, std::nullopt, true) == "cats.txt");
    }

    SECTION("accepts a path written with backslashes") {
        REQUIRE(ResolveArchiveName(file, "data\\animals", true) == "data\\animals\\cats.txt");
    }
}

TEST_CASE("ListFilesRecursive", "[path]") {
    TempDir dir;
    std::error_code ec;

    SECTION("lists every file at any depth, sorted") {
        WriteFile(dir.Path() / "b.txt", "b");
        WriteFile(dir.Path() / "a" / "c.txt", "c");
        WriteFile(dir.Path() / "a" / "b" / "d.txt", "d");

        const std::vector<fs::path> expected = {dir.Path() / "a" / "b" / "d.txt",
                                                dir.Path() / "a" / "c.txt", dir.Path() / "b.txt"};
        REQUIRE(ListFilesRecursive(dir.Path(), ec) == expected);
        REQUIRE_FALSE(ec);
    }

    SECTION("leaves out directories, empty or not") {
        fs::create_directories(dir.Path() / "empty");
        WriteFile(dir.Path() / "full" / "a.txt", "a");

        const std::vector<fs::path> expected = {dir.Path() / "full" / "a.txt"};
        REQUIRE(ListFilesRecursive(dir.Path(), ec) == expected);
        REQUIRE_FALSE(ec);
    }

    SECTION("returns nothing for an empty directory") {
        REQUIRE(ListFilesRecursive(dir.Path(), ec).empty());
        REQUIRE_FALSE(ec);
    }

    SECTION("reports a directory that does not exist") {
        REQUIRE(ListFilesRecursive(dir.Path() / "missing", ec).empty());
        REQUIRE(ec);
    }

    SECTION("skips a dangling symlink") {
        WriteFile(dir.Path() / "a.txt", "a");
        // Windows creates a symlink only with Developer Mode or elevation
        std::error_code link_ec;
        fs::create_symlink(dir.Path() / "missing", dir.Path() / "dangling", link_ec);
        if (link_ec) {
            SKIP("cannot create a symlink here: " << link_ec.message());
        }

        const std::vector<fs::path> expected = {dir.Path() / "a.txt"};
        REQUIRE(ListFilesRecursive(dir.Path(), ec) == expected);
        REQUIRE_FALSE(ec);
    }
}

TEST_CASE("ExpandStdinMarker", "[path]") {
    SECTION("returns the paths unchanged, without reading, when none is the marker") {
        std::istringstream in("unread.txt\n");

        const std::vector<std::string> expected = {"a.txt", "b.txt"};
        REQUIRE(ExpandStdinMarker({"a.txt", "b.txt"}, in) == expected);
        std::string rest;
        std::getline(in, rest);
        REQUIRE(rest == "unread.txt");
    }

    SECTION("replaces the marker in place with each line read") {
        std::istringstream in("b.txt\nc.txt\n");

        const std::vector<std::string> expected = {"a.txt", "b.txt", "c.txt", "d.txt"};
        REQUIRE(ExpandStdinMarker({"a.txt", "-", "d.txt"}, in) == expected);
    }

    SECTION("drops empty lines") {
        std::istringstream in("\nb.txt\n\n\nc.txt\n");

        const std::vector<std::string> expected = {"b.txt", "c.txt"};
        REQUIRE(ExpandStdinMarker({"-"}, in) == expected);
    }

    SECTION("strips a trailing carriage return, dropping a line left empty") {
        std::istringstream in("b.txt\r\n\r\nc.txt\r\n");

        const std::vector<std::string> expected = {"b.txt", "c.txt"};
        REQUIRE(ExpandStdinMarker({"-"}, in) == expected);
    }

    SECTION("keeps a last line with no newline") {
        std::istringstream in("b.txt\nc.txt");

        const std::vector<std::string> expected = {"b.txt", "c.txt"};
        REQUIRE(ExpandStdinMarker({"-"}, in) == expected);
    }

    SECTION("reads the stream once, so a later marker adds nothing") {
        std::istringstream in("a.txt\nb.txt\n");

        const std::vector<std::string> expected = {"a.txt", "b.txt", "x.txt"};
        REQUIRE(ExpandStdinMarker({"-", "x.txt", "-"}, in) == expected);
    }
}

TEST_CASE("LocalFileTimestamp", "[path]") {
    TempDir dir;
    const fs::path file = dir.Path() / "a.txt";
    WriteFile(file, "a");

    SECTION("converts the modification time to a FILETIME") {
        // C++17 has no clock_cast, so the fixed time reaches the file through the two
        // clocks' current readings
        const std::time_t unix_time = 1700000000;
        const auto system_time = std::chrono::system_clock::from_time_t(unix_time);
        fs::last_write_time(file, fs::file_time_type::clock::now() +
                                      std::chrono::duration_cast<fs::file_time_type::duration>(
                                          system_time - std::chrono::system_clock::now()));

        // The bridge can land a hair either side of the second, and only whole seconds
        // are kept
        const uint64_t expected =
            (static_cast<uint64_t>(unix_time) + filetime_epoch_offset) * intervals_per_second;
        const uint64_t actual = LocalFileTimestamp(file);
        REQUIRE(actual >= expected - intervals_per_second);
        REQUIRE(actual <= expected + intervals_per_second);
    }

    SECTION("returns 0 for a file that does not exist") {
        REQUIRE(LocalFileTimestamp(dir.Path() / "missing.txt") == 0U);
    }
}
