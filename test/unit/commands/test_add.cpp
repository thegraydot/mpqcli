#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <sstream>
#include <string>

#include "commands/add.h"
#include "created_archive.h"
#include "errors.h"
#include "read_file.h"
#include "sample_files.h"
#include "write_file.h"

namespace fs = std::filesystem;

using mpqcli::Add;
using mpqcli::AddOptions;
using mpqcli::testing::ArchiveHasFile;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::ReadFile;
using mpqcli::testing::SampleFiles;
using mpqcli::testing::WriteFile;

TEST_CASE("Add", "[add]") {
    CreatedArchive created;
    const fs::path new_file = created.files.dir.Path() / "more" / "birds.txt";
    WriteFile(new_file, "This is a file about birds.\n");
    AddOptions options;
    options.archive = created.archive;
    options.files = {new_file};
    std::ostringstream err;

    SECTION("adds a file to the archive") {
        const std::atomic<bool> cancelled{false};
        REQUIRE(Add(options, err, cancelled));
        REQUIRE(ArchiveHasFile(created.archive, "birds.txt"));
    }

    SECTION("throws Interrupted when already cancelled, leaving the archive as it was") {
        const std::string before = ReadFile(created.archive);
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Add(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE(ReadFile(created.archive) == before);
        REQUIRE_FALSE(ArchiveHasFile(created.archive, "birds.txt"));
        for (const auto &entry : SampleFiles::Contents()) {
            CAPTURE(entry.first);
            REQUIRE(ArchiveHasFile(created.archive, entry.first));
        }
    }
}
