#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>

#include "commands/read.h"
#include "created_archive.h"
#include "raw_archive.h"
#include "sample_files.h"
#include "util/locales.h"

using mpqcli::default_locale;
using mpqcli::Read;
using mpqcli::ReadOptions;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::SampleFiles;
using mpqcli::testing::WriteRawArchive;

TEST_CASE("Read", "[read]") {
    CreatedArchive created;

    SECTION("writes each file's bytes to out unchanged") {
        for (const auto &[name, content] : SampleFiles::Contents()) {
            CAPTURE(name);
            ReadOptions options;
            options.archive = created.archive;
            options.file = name;
            std::ostringstream out;
            std::ostringstream err;
            REQUIRE(Read(options, out, err));
            REQUIRE(out.str() == content);
        }
    }

    SECTION("reads a path an archive stores with forward slashes") {
        // Another tool may store a path with forward slashes, which only that spelling finds
        ReadOptions options;
        options.archive = created.files.dir.Path() / "slashes.mpq";
        WriteRawArchive(options.archive, {{"fwd/a.txt", default_locale}});
        options.file = "fwd/a.txt";
        std::ostringstream out;
        std::ostringstream err;
        REQUIRE(Read(options, out, err));
        REQUIRE(out.str() == "fwd/a.txt");
    }
}
