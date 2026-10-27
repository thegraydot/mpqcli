#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "commands/rename.h"
#include "created_archive.h"

using mpqcli::Rename;
using mpqcli::RenameOptions;
using mpqcli::testing::ArchiveHasFile;
using mpqcli::testing::CreatedArchive;

TEST_CASE("Rename", "[rename]") {
    CreatedArchive created;
    std::ostringstream err;

    SECTION("gives the file its new name") {
        RenameOptions options;
        options.archive = created.archive;
        options.old_file = "cats.txt";
        options.new_file = "felines.txt";
        REQUIRE(Rename(options, err));
        REQUIRE(ArchiveHasFile(created.archive, "felines.txt"));
        REQUIRE_FALSE(ArchiveHasFile(created.archive, "cats.txt"));
    }
}
