#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "commands/remove.h"
#include "created_archive.h"
#include "errors.h"

using mpqcli::Remove;
using mpqcli::RemoveOptions;
using mpqcli::testing::ArchiveHasFile;
using mpqcli::testing::CreatedArchive;

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

    SECTION("throws Interrupted when already cancelled, removing nothing") {
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Remove(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE(ArchiveHasFile(created.archive, "cats.txt"));
    }
}
