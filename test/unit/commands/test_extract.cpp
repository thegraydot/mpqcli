#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include "commands/extract.h"
#include "created_archive.h"
#include "errors.h"
#include "read_file.h"
#include "sample_files.h"

namespace fs = std::filesystem;

using mpqcli::Extract;
using mpqcli::ExtractOptions;
using mpqcli::testing::CreatedArchive;
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

    SECTION("throws Interrupted when cancelled while listing, creating no output directory") {
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Extract(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE_FALSE(fs::exists(output));
    }

    SECTION("throws Interrupted when cancelled before the first named file, extracting nothing") {
        options.files = std::vector<std::string>{"cats.txt", "dogs.txt"};
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Extract(options, err, cancelled), mpqcli::Interrupted);
        const bool nothing_extracted = !fs::exists(output) || fs::is_empty(output);
        REQUIRE(nothing_extracted);
    }
}
