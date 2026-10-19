#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

#include "cancel_after.h"
#include "commands/remove.h"
#include "created_archive.h"
#include "errors.h"
#include "read_file.h"

using mpqcli::Remove;
using mpqcli::RemoveOptions;
using mpqcli::testing::ArchiveHasFile;
using mpqcli::testing::CancelAfter;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::ReadFile;

TEST_CASE("Remove", "[remove]") {
    CreatedArchive created;
    RemoveOptions options;
    options.archive = created.archive;
    options.files = {"cats.txt"};
    std::ostringstream err;

    SECTION("removes the named file and keeps the rest") {
        const std::atomic<bool> cancelled{false};
        REQUIRE(Remove(options, err, cancelled));
        REQUIRE_FALSE(ArchiveHasFile(created.archive, "cats.txt"));
        REQUIRE(ArchiveHasFile(created.archive, "dogs.txt"));
    }

    SECTION("throws Interrupted when already cancelled, leaving the archive as it was") {
        const std::string before = ReadFile(created.archive);
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Remove(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE(ReadFile(created.archive) == before);
        REQUIRE(ArchiveHasFile(created.archive, "cats.txt"));
    }

    // Remove changes the archive in place, so a file removed before the interrupt stays gone
    SECTION("stops after the first name when cancelled part-way") {
        options.files = {"cats.txt", "dogs.txt"};
        CancelAfter cancel("[-] Removing file");
        REQUIRE_THROWS_AS(Remove(options, cancel.Err(), cancel.Cancelled()), mpqcli::Interrupted);
        REQUIRE_FALSE(ArchiveHasFile(created.archive, "cats.txt"));
        REQUIRE(ArchiveHasFile(created.archive, "dogs.txt"));
    }
}
