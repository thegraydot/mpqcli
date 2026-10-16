#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <sstream>
#include <vector>

#include "commands/create.h"
#include "created_archive.h"
#include "errors.h"
#include "sample_files.h"

namespace fs = std::filesystem;

using mpqcli::Create;
using mpqcli::CreateOptions;
using mpqcli::testing::ArchiveHasFile;
using mpqcli::testing::SampleFiles;

TEST_CASE("Create", "[create]") {
    SampleFiles files;
    CreateOptions options;
    options.archive = files.dir.Path() / "out.mpq";
    options.files = {files.files_dir};
    fs::path partial = options.archive;
    partial += ".partial";
    std::ostringstream err;

    SECTION("writes an archive holding every input file") {
        const std::atomic<bool> cancelled{false};
        REQUIRE(Create(options, err, cancelled));
        REQUIRE(fs::exists(options.archive));
        REQUIRE_FALSE(fs::exists(partial));
        for (const auto &entry : SampleFiles::Contents()) {
            CAPTURE(entry.first);
            REQUIRE(ArchiveHasFile(options.archive, entry.first));
        }
    }

    SECTION("throws Interrupted when already cancelled, leaving nothing behind") {
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_AS(Create(options, err, cancelled), mpqcli::Interrupted);
        REQUIRE_FALSE(fs::exists(options.archive));
        REQUIRE_FALSE(fs::exists(partial));

        // Nothing at all is left beside the inputs, whatever name it might have had
        const std::vector<fs::path> entries(fs::directory_iterator(files.dir.Path()),
                                            fs::directory_iterator{});
        const std::vector<fs::path> expected = {files.files_dir};
        REQUIRE(entries == expected);
    }
}
