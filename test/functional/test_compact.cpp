#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "archive_helper.h"
#include "no_listfile_archive.h"
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

const std::string compacting_notice = "[*] Compacting archive. This may take some time...";
const std::string incomplete_listfile_error =
    "[!] Failed to compact archive: (10007) At least one file name is unknown (listfile is "
    "incomplete)";

// Bytes no compressor can shrink, the same on every run, so a removed file of
// this size leaves a gap compaction has to reclaim in full
std::string IncompressibleBytes(std::size_t size) {
    std::mt19937 generator(0);
    std::string bytes;
    bytes.reserve(size);
    while (bytes.size() < size) {
        const auto word = static_cast<std::uint32_t>(generator());
        for (int shift = 0; shift < 32 && bytes.size() < size; shift += 8) {
            bytes.push_back(static_cast<char>((word >> shift) & 0xFFU));
        }
    }
    return bytes;
}

} // namespace

TEST_CASE("compact: prints its help with --help", "[compact]") {
    auto result = Run(mpqcli_binary, {"compact", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("compact [OPTIONS] archive"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--listfile"));
}

TEST_CASE("compact: archive does not exist", "[compact]") {
    auto result = Run(mpqcli_binary, {"compact", "no-such-archive.mpq"});

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("File does not exist: no-such-archive.mpq"));
}

TEST_CASE("compact: listfile does not exist", "[compact]") {
    SampleArchive sample;

    auto result =
        Run(mpqcli_binary, {"compact", sample.archive.string(), "--listfile", "no-such-list.txt"});

    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("--listfile: File does not exist: no-such-list.txt"));
}

TEST_CASE("compact: fails when the archive is not an MPQ", "[compact]") {
    SampleFiles files;

    auto result = Run(mpqcli_binary, {"compact", (files.files_dir / "cats.txt").string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output).size() == 1);
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to open MPQ archive"));
}

TEST_CASE("compact: fails when the archive has no listfile", "[compact]") {
    NoListfileArchive archive;

    auto result = Run(mpqcli_binary, {"compact", archive.archive.string()});

    // Compaction rewrites every entry, so every name has to be known
    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{compacting_notice, incomplete_listfile_error});
}

TEST_CASE("compact: fails with a partial external listfile", "[compact]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, "cats.txt\n");

    auto result =
        Run(mpqcli_binary, {"compact", archive.archive.string(), "-l", listfile.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{compacting_notice, incomplete_listfile_error});
}

TEST_CASE("compact: succeeds with a complete external listfile", "[compact]") {
    const auto flag = GENERATE(as<std::string>{}, "-l", "--listfile");
    CAPTURE(flag);
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, "cats.txt\ndogs.txt\ncapybaras.txt\n");

    auto result =
        Run(mpqcli_binary, {"compact", archive.archive.string(), flag, listfile.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == std::vector<std::string>{compacting_notice});
    // Every entry survives with its content and locale
    for (const auto &entry : NoListfileArchive::Entries()) {
        CAPTURE(entry.name);
        const auto locale = entry.locale.empty() ? std::nullopt : std::optional(entry.locale);
        REQUIRE(ReadEntry(archive.archive, entry.name, locale) ==
                std::set<std::string>{entry.content});
    }
}

TEST_CASE("compact: ignores an empty listfile, with a warning", "[compact]") {
    // An archive with its own (listfile), so compacting succeeds without an external one
    SampleArchive sample;
    const auto listfile = sample.files.dir.Path() / "empty.txt";
    WriteFile(listfile, "");

    auto result = Run(mpqcli_binary, {"compact", sample.archive.string(), "-l", listfile.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToVector(result.stderr_output) ==
        std::vector<std::string>{"[!] Ignoring the listfile, which is empty: " + listfile.string(),
                                 compacting_notice});
    REQUIRE(ArchiveEntries(sample.archive) ==
            std::set<std::string>{"enUS  bytes", "enUS  cats.txt", "enUS  dogs.txt"});
}

TEST_CASE("compact: reclaims the space of removed files", "[compact]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "compaction_files";
    WriteFile(source_dir / "small.txt", "This is a small text file.\n");
    WriteFile(source_dir / "large.bin", IncompressibleBytes(512 * 1024));
    const auto archive = dir.Path() / "mpq_for_compaction.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);
    REQUIRE(Run(mpqcli_binary, {"remove", archive.string(), "large.bin"}).returncode == 0);
    const auto size_before = fs::file_size(archive);

    auto result = Run(mpqcli_binary, {"compact", archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == std::vector<std::string>{compacting_notice});
    const auto size_after = fs::file_size(archive);
    CAPTURE(size_before, size_after);
    // Removing only marks the entry; compaction is what drops the half-megabyte
    REQUIRE(size_after < size_before);
    REQUIRE(size_before - size_after > size_before / 2);
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  small.txt"});
    REQUIRE(ReadEntry(archive, "small.txt") == std::set<std::string>{"This is a small text file."});
}

TEST_CASE("compact: leaves an archive with nothing to reclaim as it was", "[compact]") {
    SampleArchive sample;
    const auto size_before = fs::file_size(sample.archive);
    const auto entries_before = ArchiveEntries(sample.archive, true);

    auto result = Run(mpqcli_binary, {"compact", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == std::vector<std::string>{compacting_notice});
    REQUIRE(fs::file_size(sample.archive) == size_before);
    REQUIRE(ArchiveEntries(sample.archive, true) == entries_before);
    REQUIRE(ReadEntry(sample.archive, "cats.txt") ==
            std::set<std::string>{"This is a file about cats."});
}
