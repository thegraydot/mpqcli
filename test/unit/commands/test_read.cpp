#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "commands/read.h"
#include "created_archive.h"
#include "sample_files.h"

using mpqcli::Read;
using mpqcli::ReadOptions;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::SampleFiles;

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
}
