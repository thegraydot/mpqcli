#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <sstream>

#include <StormLib.h>

#include "commands/compact.h"
#include "created_archive.h"
#include "mpq/archive.h"
#include "sample_files.h"

namespace fs = std::filesystem;

using mpqcli::Archive;
using mpqcli::Compact;
using mpqcli::CompactOptions;
using mpqcli::testing::ArchiveHasFile;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::SampleFiles;

TEST_CASE("Compact", "[compact]") {
    CreatedArchive created;
    CompactOptions options;
    options.archive = created.archive;
    std::ostringstream err;

    SECTION("keeps every file") {
        Compact(options, err);
        for (const auto &entry : SampleFiles::Contents()) {
            CAPTURE(entry.first);
            REQUIRE(ArchiveHasFile(created.archive, entry.first));
        }
    }

    SECTION("reclaims the space a removed file left behind") {
        {
            Archive archive = Archive::Open(created.archive, 0);
            REQUIRE(SFileRemoveFile(archive.Handle(), "cats.txt", 0));
            archive.Close();
        }
        const auto size_before = fs::file_size(created.archive);
        Compact(options, err);
        REQUIRE(fs::file_size(created.archive) < size_before);
        REQUIRE(ArchiveHasFile(created.archive, "dogs.txt"));
    }
}
