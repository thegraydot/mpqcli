#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>

#include <StormLib.h>

#include "cancel_after.h"
#include "commands/add.h"
#include "created_archive.h"
#include "errors.h"
#include "mpq/archive.h"
#include "mpq/query.h"
#include "read_file.h"
#include "sample_files.h"
#include "write_file.h"

namespace fs = std::filesystem;

namespace {

// The size of the archive's file table, which StormLib rounds up beyond what Create asks for
std::uint32_t MaxFileCount(const fs::path &archive) {
    const mpqcli::Archive opened = mpqcli::Archive::Open(archive, MPQ_OPEN_READ_ONLY);
    return mpqcli::GetFileInfo<std::uint32_t>(opened.Handle(), SFileMpqMaxFileCount);
}

} // namespace

using mpqcli::Add;
using mpqcli::AddOptions;
using mpqcli::testing::ArchiveHasFile;
using mpqcli::testing::CancelAfter;
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

    SECTION("grows the file table part-way through one run") {
        const std::uint32_t max_files = MaxFileCount(created.archive);
        // As many new files as the table has slots, which is more than are free
        const fs::path fill_dir = created.files.dir.Path() / "fill";
        for (std::uint32_t i = 0; i < max_files; ++i) {
            WriteFile(fill_dir / ("f" + std::to_string(i) + ".txt"), "file " + std::to_string(i));
        }
        options.files = {fill_dir};
        const std::atomic<bool> cancelled{false};
        REQUIRE(Add(options, err, cancelled));
        REQUIRE(MaxFileCount(created.archive) > max_files);
        for (std::uint32_t i = 0; i < max_files; ++i) {
            const std::string name = "f" + std::to_string(i) + ".txt";
            CAPTURE(name);
            REQUIRE(ArchiveHasFile(created.archive, name));
        }
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

    // Add changes the archive in place, so a file added before the interrupt stays
    SECTION("stops after the first file of a directory when cancelled part-way") {
        WriteFile(new_file.parent_path() / "fish.txt", "This is a file about fish.\n");
        options.files = {new_file.parent_path()};
        CancelAfter cancel("[+] Adding file");
        REQUIRE_THROWS_AS(Add(options, cancel.Err(), cancel.Cancelled()), mpqcli::Interrupted);
        REQUIRE(ArchiveHasFile(created.archive, "birds.txt"));
        REQUIRE_FALSE(ArchiveHasFile(created.archive, "fish.txt"));
    }

    SECTION("stops after the first of several files when cancelled part-way") {
        const fs::path fish = new_file.parent_path() / "fish.txt";
        WriteFile(fish, "This is a file about fish.\n");
        options.files = {new_file, fish};
        CancelAfter cancel("[+] Adding file");
        REQUIRE_THROWS_AS(Add(options, cancel.Err(), cancel.Cancelled()), mpqcli::Interrupted);
        REQUIRE(ArchiveHasFile(created.archive, "birds.txt"));
        REQUIRE_FALSE(ArchiveHasFile(created.archive, "fish.txt"));
    }
}
