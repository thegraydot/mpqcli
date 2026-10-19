#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "integration_data.h"
#include "read_file.h"
#include "sample_archive.h"
#include "sample_files.h"
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

const std::string open_failure = "[!] Failed to open MPQ archive";

// The value of every property info knows, for one archive
struct Properties {
    std::string format_version;
    std::string header_offset;
    std::string header_size;
    std::string archive_size;
    std::string file_count;
    std::string max_files;
    std::string signature_type;

    // Property name and value pairs, as --property takes and prints them
    std::vector<std::pair<std::string, std::string>> Values() const {
        return {
            {"format-version", format_version}, {"header-offset", header_offset},
            {"header-size", header_size},       {"archive-size", archive_size},
            {"file-count", file_count},         {"max-files", max_files},
            {"signature-type", signature_type},
        };
    }

    // The lines the full listing prints, one labelled property each
    std::set<std::string> Lines() const {
        return {
            "Format version: " + format_version, "Header offset: " + header_offset,
            "Header size: " + header_size,       "Archive size: " + archive_size,
            "File count: " + file_count,         "Max files: " + max_files,
            "Signature type: " + signature_type,
        };
    }
};

// What the attributed archive recipe over the sample files produces under each
// MPQ format version. The header sizes are StormLib's fixed per-version header
// lengths. The archive sizes were read back from the binary and hold on every
// platform because the sample files carry LF endings and a pinned modification
// time, which the (attributes) block stores.
struct FormatVersion {
    std::string version;
    std::string header_size;
    std::string archive_size;
};

const std::vector<FormatVersion> format_versions = {
    {"1", "32", "1381"},
    {"2", "44", "1377"},
    {"3", "68", "1601"},
    {"4", "208", "1742"},
};

// The properties of an attributed archive: five entries, the three sample files
// plus the (listfile) and (attributes) the recipe writes
Properties AttributedProperties(const FormatVersion &format) {
    return {format.version, "0", format.header_size, format.archive_size, "5", "64", "None"};
}

// How create's arguments decide the special entries an archive carries, and so
// its file count and signature type
struct SpecialEntries {
    std::vector<std::string> create_args;
    std::string file_count;
    std::string signature_type;
};

const std::vector<SpecialEntries> special_entries = {
    {{}, "4", "None"},                              // The three files and a (listfile)
    {{"--file-flags1", "0"}, "3", "None"},          // No (listfile)
    {{"-s"}, "5", "Weak"},                          // A (signature) as well
    {{"-s", "--format-version", "4"}, "5", "Weak"}, // Weak signing holds on the newest format
    {AttributedArchiveArgs(), "5", "None"},         // An (attributes) block instead
};

// The two Blizzard patches make fetch_integration_data downloads: the only
// archives with a genuine strong signature, and with a weak one Blizzard made.
// The first is the archive the docs page's example output was taken from. The
// strong signature sits after the archive proper, so the archive size is 260
// bytes short of the file's own.
struct PatchArchive {
    std::string name;
    Properties expected;
};

const std::vector<PatchArchive> patch_archives = {
    {"v1_weak_signature_patch.mpq", {"1", "0", "32", "1798918", "65", "128", "Weak"}},
    {"v2_strong_signature_patch.mpq", {"2", "0", "44", "8720276", "70", "128", "Strong"}},
};

} // namespace

TEST_CASE("info: prints its help with --help", "[info]") {
    auto result = Run(mpqcli_binary, {"info", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("info [OPTIONS] archive"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--property"));
}

TEST_CASE("info: archive does not exist", "[info]") {
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary, {"info", "no-such-archive.mpq"}, options);

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("archive: File does not exist: no-such-archive.mpq"));
}

TEST_CASE("info: rejects a directory as the archive", "[info]") {
    SampleFiles files;

    auto result = Run(mpqcli_binary, {"info", files.files_dir.string()});

    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("File is actually a directory"));
}

TEST_CASE("info: rejects extra arguments", "[info]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"info", sample.archive.string(), "extra"});

    // 109 is CLI11's ExtrasError: a second archive is not silently ignored
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: extra"));
}

TEST_CASE("info: rejects an unknown property", "[info]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"info", "-q", "not-real", sample.archive.string()});

    // The CLI::IsMember check rejects the name before the archive is opened
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("--property: not-real not in {"));
}

TEST_CASE("info: rejects -p, which is not one of its options", "[info]") {
    SampleArchive sample;
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result =
        Run(mpqcli_binary, {"info", "-p", "file-count", sample.archive.string()}, options);

    // The unknown -p is set aside and the property name fills the archive
    // positional, whose CLI::ExistingFile check fails first: 105 is CLI11's
    // ValidationError
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("archive: File does not exist: file-count"));
}

TEST_CASE("info: fails when the archive cannot be opened as an MPQ", "[info]") {
    SampleArchive sample;

    SECTION("a text file") {
        auto result = Run(mpqcli_binary, {"info", (sample.files.files_dir / "cats.txt").string()});

        REQUIRE(result.returncode == 1);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToSet(result.stderr_output).size() == 1);
        REQUIRE_THAT(result.stderr_output, ContainsSubstring(open_failure));
        // StormLib's error code and text are forwarded, not just the failure
        REQUIRE_THAT(result.stderr_output, ContainsSubstring("(1000) Bad MPQ format"));
    }

    SECTION("an archive cut short of its tables") {
        const auto truncated = sample.files.dir.Path() / "truncated.mpq";
        WriteFile(truncated, ReadFile(sample.archive).substr(0, 700));

        auto result = Run(mpqcli_binary, {"info", truncated.string()});

        REQUIRE(result.returncode == 1);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToSet(result.stderr_output).size() == 1);
        REQUIRE_THAT(result.stderr_output, ContainsSubstring(open_failure));
    }
}

TEST_CASE("info: prints every property with its name", "[info]") {
    const auto format = GENERATE(from_range(format_versions));
    CAPTURE(format.version);
    SampleArchive sample(AttributedArchiveArgs(format.version));

    auto result = Run(mpqcli_binary, {"info", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE(LinesToSet(result.stdout_output) == AttributedProperties(format).Lines());
}

TEST_CASE("info: prints one property's bare value with --property", "[info]") {
    const auto flag = GENERATE(as<std::string>{}, "-q", "--property");
    const auto format = GENERATE(from_range(format_versions));
    CAPTURE(flag, format.version);
    SampleArchive sample(AttributedArchiveArgs(format.version));

    for (const auto &[property, value] : AttributedProperties(format).Values()) {
        CAPTURE(property);

        auto result = Run(mpqcli_binary, {"info", flag, property, sample.archive.string()});

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stderr_output.empty());
        // The value alone on one line, with nothing repeated or labelled
        REQUIRE(TrimTrailingNewlines(result.stdout_output) == value);
    }
}

TEST_CASE("info: file count and signature type follow the special entries", "[info]") {
    const auto entries = GENERATE(from_range(special_entries));
    CAPTURE(entries.create_args);
    SampleArchive sample(entries.create_args);

    auto result = Run(mpqcli_binary, {"info", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    const auto lines = LinesToSet(result.stdout_output);
    REQUIRE(lines.count("File count: " + entries.file_count) == 1);
    REQUIRE(lines.count("Signature type: " + entries.signature_type) == 1);
}

TEST_CASE("info: prints the documented values for a Blizzard patch archive", "[info]") {
    const auto patch = GENERATE(from_range(patch_archives));
    CAPTURE(patch.name);
    const auto archive = IntegrationArchive(patch.name);

    auto result = Run(mpqcli_binary, {"info", archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE(LinesToSet(result.stdout_output) == patch.expected.Lines());
    for (const auto &[property, value] : patch.expected.Values()) {
        CAPTURE(property);
        auto bare = Run(mpqcli_binary, {"info", "-q", property, archive.string()});
        REQUIRE(bare.returncode == 0);
        REQUIRE(bare.stderr_output.empty());
        REQUIRE(TrimTrailingNewlines(bare.stdout_output) == value);
    }
}

TEST_CASE("info: reads an archive it has no permission to write", "[info]") {
    SampleArchive sample;
    fs::permissions(sample.archive,
                    fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read,
                    fs::perm_options::replace);

    auto result = Run(mpqcli_binary, {"info", "-q", "file-count", sample.archive.string()});

    // Windows cannot delete a read-only file, so give the bit back before the
    // fixture removes its directory
    fs::permissions(sample.archive, fs::perms::owner_write, fs::perm_options::add);
    // The archive is opened read-only, so a lack of write access is no obstacle
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE(TrimTrailingNewlines(result.stdout_output) == "4");
}

TEST_CASE("info: reports the offset of an archive embedded after leading bytes", "[info]") {
    // StormLib looks for the header at 512-byte steps, so both offsets are found
    const auto prefix = GENERATE(as<std::size_t>{}, 512, 1024);
    CAPTURE(prefix);
    SampleArchive sample;
    const auto embedded = sample.files.dir.Path() / "embedded.mpq";
    WriteFile(embedded, std::string(prefix, '\0') + ReadFile(sample.archive));
    // Everything but the offset reads the same as the bare archive, including
    // the archive size, which excludes the leading bytes
    auto bare = Run(mpqcli_binary, {"info", sample.archive.string()});
    REQUIRE(bare.returncode == 0);
    REQUIRE(bare.stderr_output.empty());
    auto expected = LinesToSet(bare.stdout_output);
    REQUIRE(expected.erase("Header offset: 0") == 1);
    expected.insert("Header offset: " + std::to_string(prefix));

    auto result = Run(mpqcli_binary, {"info", embedded.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE(LinesToSet(result.stdout_output) == expected);
}
