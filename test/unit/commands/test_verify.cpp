#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "commands/create.h"
#include "commands/verify.h"
#include "created_archive.h"
#include "sample_files.h"

using mpqcli::Create;
using mpqcli::CreateOptions;
using mpqcli::Verify;
using mpqcli::VerifyOptions;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::SampleFiles;

TEST_CASE("Verify", "[verify]") {
    std::ostringstream out;
    std::ostringstream err;

    SECTION("accepts an archive created with a signature") {
        SampleFiles files;
        CreateOptions create;
        create.archive = files.dir.Path() / "signed.mpq";
        create.files = {files.files_dir};
        create.sign = true;
        const std::atomic<bool> cancelled{false};
        REQUIRE(Create(create, err, cancelled));

        VerifyOptions options;
        options.archive = create.archive;
        REQUIRE(Verify(options, out, err));
    }

    SECTION("rejects an archive with no signature") {
        CreatedArchive created;
        VerifyOptions options;
        options.archive = created.archive;
        REQUIRE_FALSE(Verify(options, out, err));
    }
}
