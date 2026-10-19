#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <fstream>
#include <hash-library/md5.h>
#include <ios>
#include <string>
#include <vector>

#include "archive_helper.h"
#include "integration_data.h"
#include "sample_archive.h"
#include "subprocess_helper.h"
#include "temp_dir.h"
#include "test_environment.h"
#include "write_file.h"

namespace fs = std::filesystem;

using namespace mpqcli::testing;
using Catch::Matchers::ContainsSubstring;

namespace {

// The binary every case spawns: a reference into the singleton, so it is the
// baked path itself rather than a copy that could drift from it
const fs::path &mpqcli_binary = TestEnvironment::Instance().binary_path;

const std::vector<std::string> verify_success = {"[*] Verify success"};

// Create arguments for a weakly signed archive whose files are compressed but
// not encrypted: 512 is MPQ_FILE_COMPRESS
const std::vector<std::string> weak_signature_args = {
    "-s", "--file-flags1",      "4294967295", "--file-flags2", "4294967295", "--file-flags3",
    "0",  "--attr-flags",       "15",         "--flags",       "512",        "--compression",
    "2",  "--compression-next", "4294967295"};

// Long enough that its compressed data runs well past the sector offset table
// at its start, whose corruption fails the read rather than the signature
const std::string tamper_text = "This is a longer file about cats, with enough content to give us "
                                "several bytes of compressed data to safely flip a bit in.\n";

// Inverts the byte at offset in place
void FlipByte(const fs::path &file, std::streamoff offset) {
    std::fstream stream(file, std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(stream.is_open());
    stream.seekg(offset);
    const int byte = stream.get();
    REQUIRE(byte != std::char_traits<char>::eof());
    stream.seekp(offset);
    stream.put(static_cast<char>(byte ^ 0xFF));
    REQUIRE(stream.good());
}

} // namespace

TEST_CASE("verify: prints its help with --help", "[verify]") {
    auto result = Run(mpqcli_binary, {"verify", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("verify [OPTIONS] archive"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("-x,"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--print"));
}

TEST_CASE("verify: archive does not exist", "[verify]") {
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary, {"verify", "no-such-archive.mpq"}, options);

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("archive: File does not exist: no-such-archive.mpq"));
}

TEST_CASE("verify: rejects -p", "[verify]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"verify", "-p", sample.archive.string()});

    // 109 is CLI11's ExtrasError: verify has no option spelled -p
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("not expected: -p"));
}

TEST_CASE("verify: fails when the archive has no signature", "[verify]") {
    SampleArchive sample(AttributedArchiveArgs());

    auto result = Run(mpqcli_binary, {"verify", sample.archive.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{"[!] Verify failed: archive has no signature"});
}

TEST_CASE("verify: fails when the signature no longer matches the content", "[verify]") {
    TempDir dir;
    const auto files_dir = dir.Path() / "tamper_files";
    WriteFile(files_dir / "cats.txt", tamper_text);
    const auto archive = dir.Path() / "mpq_with_tampered_signature.mpq";
    auto create = Run(mpqcli_binary, {"create", "-s", archive.string(), files_dir.string()});
    REQUIRE(create.returncode == 0);
    // The midpoint of the entry's compressed data, clear of its sector offset table
    const auto offset = EntryNumber(archive, "cats.txt", "byte-offset");
    const auto compressed_size = EntryNumber(archive, "cats.txt", "compressed-size");
    FlipByte(archive, static_cast<std::streamoff>(offset + compressed_size / 2));

    auto result = Run(mpqcli_binary, {"verify", archive.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{"[!] Verify failed: signature is present but invalid"});
}

TEST_CASE("verify: succeeds on a valid weak signature", "[verify]") {
    SampleArchive sample(weak_signature_args);

    auto result = Run(mpqcli_binary, {"verify", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == verify_success);
}

TEST_CASE("verify: --print writes a weak signature's bytes to stdout", "[verify]") {
    const auto archive = IntegrationArchive("v1_weak_signature_patch.mpq");

    auto result = Run(mpqcli_binary, {"verify", "--print", archive.string()});

    REQUIRE(result.returncode == 0);
    // The (signature) entry, raw rather than as text
    MD5 md5;
    REQUIRE(md5(result.stdout_output) == "e434b57cdc6b730c17e18f2bb2fa8fd9");
    REQUIRE(LinesToVector(result.stderr_output) == verify_success);
}

TEST_CASE("verify: --print writes a strong signature's bytes to stdout", "[verify]") {
    const auto archive = IntegrationArchive("v2_strong_signature_patch.mpq");

    auto result = Run(mpqcli_binary, {"verify", "--print", archive.string()});

    REQUIRE(result.returncode == 0);
    // The block that follows the archive in the file, raw rather than as text
    MD5 md5;
    REQUIRE(md5(result.stdout_output) == "931fc40bb6bff48b89d39b5acb169bbd");
    REQUIRE(LinesToVector(result.stderr_output) == verify_success);
}

TEST_CASE("verify: -x prints the same signature as --print", "[verify]") {
    SampleArchive sample({"-s"});

    auto short_form = Run(mpqcli_binary, {"verify", "-x", sample.archive.string()});
    auto long_form = Run(mpqcli_binary, {"verify", "--print", sample.archive.string()});

    REQUIRE(short_form.returncode == 0);
    REQUIRE_FALSE(short_form.stdout_output.empty());
    REQUIRE(LinesToVector(short_form.stderr_output) == verify_success);
    REQUIRE(long_form.returncode == 0);
    REQUIRE(LinesToVector(long_form.stderr_output) == verify_success);
    REQUIRE(short_form.stdout_output == long_form.stdout_output);
}
