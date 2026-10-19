#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include "cancel_after.h"
#include "commands/extract.h"
#include "created_archive.h"
#include "errors.h"
#include "read_file.h"
#include "sample_files.h"

namespace fs = std::filesystem;

using mpqcli::Extract;
using mpqcli::ExtractOptions;
using mpqcli::testing::CancelAfter;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::CreateWithoutListfile;
using mpqcli::testing::ReadFile;
using mpqcli::testing::SampleFiles;

TEST_CASE("Extract", "[extract]") {
    CreatedArchive created;
    const fs::path output = created.files.dir.Path() / "out";
    ExtractOptions options;
    options.archive = created.archive;
    options.output = output;
    std::ostringstream err;

    SECTION("writes every file to the output directory byte for byte") {
        const std::atomic<bool> cancelled{false};
        REQUIRE(Extract(options, err, cancelled));
        for (const auto &[name, content] : SampleFiles::Contents()) {
            CAPTURE(name);
            REQUIRE(ReadFile(output / name) == content);
        }
    }

    SECTION("fails on a named file missing from the archive, leaving no output directory") {
        options.files = std::vector<std::string>{"nope.txt"};
        const std::atomic<bool> cancelled{false};
        REQUIRE_FALSE(Extract(options, err, cancelled));
        REQUIRE_FALSE(fs::exists(output));
    }

    SECTION("extracts nothing from an archive with no entries, creating no output directory") {
        const fs::path empty_dir = created.files.dir.Path() / "empty";
        fs::create_directories(empty_dir);
        options.archive = created.files.dir.Path() / "bare.mpq";
        CreateWithoutListfile(options.archive, empty_dir);
        const std::atomic<bool> cancelled{false};
        REQUIRE(Extract(options, err, cancelled));
        REQUIRE(err.str() == "[*] No files to extract\n");
        REQUIRE_FALSE(fs::exists(output));
    }

    SECTION("throws Interrupted when cancelled while listing, creating no output directory") {
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Extract(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE_FALSE(fs::exists(output));
    }

    SECTION("throws Interrupted when cancelled before the first named file, leaving no output "
            "directory") {
        options.files = std::vector<std::string>{"cats.txt", "dogs.txt"};
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Extract(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE_FALSE(fs::exists(output));
    }

    SECTION("keeps an output directory that already existed when cancelled") {
        fs::create_directories(output);
        options.files = std::vector<std::string>{"cats.txt", "dogs.txt"};
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Extract(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE(fs::is_directory(output));
        REQUIRE(fs::is_empty(output));
    }

    SECTION("stops after the first named file when cancelled part-way") {
        options.files = std::vector<std::string>{"cats.txt", "dogs.txt"};
        CancelAfter cancel("[*] Extracted:");
        REQUIRE_THROWS_AS(Extract(options, cancel.Err(), cancel.Cancelled()), mpqcli::Interrupted);
        REQUIRE(ReadFile(output / "cats.txt") == "This is a file about cats.\n");
        REQUIRE_FALSE(fs::exists(output / "dogs.txt"));
    }
}
