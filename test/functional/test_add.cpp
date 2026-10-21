#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <chrono>
#include <filesystem>
#include <initializer_list>
#include <set>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "archive_helper.h"
#include "compressible_text.h"
#include "many_locales_archive.h"
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

// What a fresh SampleArchive lists, before any add
const std::set<std::string> sample_entries = {"enUS  bytes", "enUS  cats.txt", "enUS  dogs.txt"};
const std::set<std::string> original_cats = {"This is a file about cats."};

std::set<std::string> Plus(std::set<std::string> entries,
                           std::initializer_list<std::string> extra) {
    entries.insert(extra.begin(), extra.end());
    return entries;
}

// The message add prints on stderr when it left pre-existing files alone
const std::string skipped_hint =
    "[*] 1 file(s) already in the archive were skipped. Use --overwrite to replace them, or "
    "--update to replace only the ones that changed.";

// The line --update prints once it has walked a directory
std::string UpdateSummary(const fs::path &dir, int added, int skipped) {
    return "[*] For " + dir.string() + ": " + std::to_string(added) + " files added, " +
           std::to_string(skipped) + " files skipped, 0 files failed.";
}

} // namespace

TEST_CASE("add: prints its help with --help", "[add]") {
    auto result = Run(mpqcli_binary, {"add", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("add [OPTIONS] archive files..."));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--update"));
    REQUIRE(result.stderr_output.empty());
}

TEST_CASE("add: archive does not exist", "[add]") {
    SampleFiles files;
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result =
        Run(mpqcli_binary, {"add", "no-such-archive.mpq", (files.files_dir / "cats.txt").string()},
            options);

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("File does not exist: no-such-archive.mpq"));
}

TEST_CASE("add: fails when the archive is not an MPQ", "[add]") {
    SampleFiles files;

    auto result = Run(mpqcli_binary, {"add", (files.files_dir / "cats.txt").string(),
                                      (files.files_dir / "dogs.txt").string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to open MPQ archive"));
    REQUIRE(LinesToVector(result.stderr_output).size() == 1);
}

TEST_CASE("add: input path does not exist", "[add]") {
    SampleArchive sample;
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), "no-such-file.txt"}, options);

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Path does not exist: no-such-file.txt"});
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("add: continues past a missing input and still fails", "[add]") {
    SampleArchive sample;
    const auto real = sample.files.dir.Path() / "real.txt";
    WriteFile(real, "real file");
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary,
                      {"add", sample.archive.string(), "no-such-file.txt", real.string()}, options);

    // The run carries on to the remaining inputs, but a missing one is still a failure
    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Path does not exist: no-such-file.txt",
                                  "[+] Adding file: real.txt"});
    REQUIRE(ArchiveEntries(sample.archive) == Plus(sample_entries, {"enUS  real.txt"}));
}

#ifndef _WIN32
// A FIFO exists but is neither a file nor a directory. Windows has no mkfifo,
// so the case is POSIX only.
TEST_CASE("add: rejects an input that is neither file nor directory", "[add]") {
    SampleArchive sample;
    const auto fifo = sample.files.dir.Path() / "pipe";
    REQUIRE(mkfifo(fifo.c_str(), 0600) == 0);

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), fifo.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Not a file or directory: " + fifo.string()});
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}
#endif

TEST_CASE("add: a single file", "[add]") {
    SampleArchive sample;
    const auto file = sample.files.dir.Path() / "test.txt";
    WriteFile(file, "This is a test file for MPQ addition.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), file.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[+] Adding file: test.txt"});
    REQUIRE(ArchiveEntries(sample.archive) == Plus(sample_entries, {"enUS  test.txt"}));
    REQUIRE(ReadEntry(sample.archive, "test.txt") ==
            std::set<std::string>{"This is a test file for MPQ addition."});
}

TEST_CASE("add: several files", "[add]") {
    SampleArchive sample;
    const auto alpha = sample.files.dir.Path() / "alpha.txt";
    const auto beta = sample.files.dir.Path() / "beta.txt";
    WriteFile(alpha, "Alpha file content.");
    WriteFile(beta, "Beta file content.");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), alpha.string(), beta.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: alpha.txt", "[+] Adding file: beta.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  alpha.txt", "enUS  beta.txt"}));
}

TEST_CASE("add: a single file with --path", "[add]") {
    const auto flag = GENERATE(as<std::string>{}, "-p", "--path");
    CAPTURE(flag);
    SampleArchive sample;
    const auto file = sample.files.dir.Path() / "test.txt";
    WriteFile(file, "This is a test file for MPQ addition.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), file.string(), flag,
                                      "important\\message.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: important\\message.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  important\\message.txt"}));
}

TEST_CASE("add: several files with --path prefix", "[add]") {
    SampleArchive sample;
    const auto test0 = sample.files.dir.Path() / "test0.txt";
    const auto test1 = sample.files.dir.Path() / "test1.txt";
    WriteFile(test0, "This is a test file for MPQ addition.");
    WriteFile(test1, "This is another test file for MPQ addition.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), test0.string(),
                                      test1.string(), "--path", "texts"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: texts\\test0.txt",
                                  "[+] Adding file: texts\\test1.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  texts\\test0.txt", "enUS  texts\\test1.txt"}));
}

TEST_CASE("add: a directory and a file together", "[add]") {
    SampleArchive sample;
    const auto extra_dir = sample.files.dir.Path() / "extra";
    WriteFile(extra_dir / "new.txt", "new file");
    const auto cats = sample.files.files_dir / "cats.txt";

    // With a directory among the inputs, a single file takes the prefix and
    // its own name rather than the whole --path as its archive path
    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), extra_dir.string(),
                                      cats.string(), "--path", "pre"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[+] Adding file: pre\\new.txt", "[+] Adding file: pre\\cats.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  pre\\new.txt", "enUS  pre\\cats.txt"}));
}

TEST_CASE("add: skips an existing file by default", "[add]") {
    SampleArchive sample;
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
    const auto cats = sample.files.dir.Path() / "cats.txt";
    WriteFile(cats, "Attempting to make this file about dogs.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), cats.string()});

    // Leaving the archived copy alone is the documented default, not a failure
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] File already exists in MPQ archive: cats.txt - Skipping...",
                                  skipped_hint});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
}

TEST_CASE("add: replaces an existing file with --overwrite", "[add]") {
    const auto flag = GENERATE(as<std::string>{}, "-w", "--overwrite");
    CAPTURE(flag);
    SampleArchive sample;
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
    const auto cats = sample.files.dir.Path() / "cats.txt";
    WriteFile(cats, "This file is suddenly about dogs.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), cats.string(), flag});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[~] File already exists in MPQ archive: cats.txt - Overwriting...",
                              "[+] Adding file: cats.txt"});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") ==
            std::set<std::string>{"This file is suddenly about dogs."});
}

TEST_CASE("add: --overwrite adds a file that is not in the archive", "[add]") {
    SampleArchive sample;
    const auto file = sample.files.dir.Path() / "test.txt";
    WriteFile(file, "This file is newly added.");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), file.string(), "--overwrite"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[+] Adding file: test.txt"});
    REQUIRE(ReadEntry(sample.archive, "test.txt") ==
            std::set<std::string>{"This file is newly added."});
}

TEST_CASE("add: rejects an unknown locale", "[add]") {
    SampleArchive sample;
    const auto horses = sample.files.dir.Path() / "horses.txt";
    WriteFile(horses, "This is a file about horses.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), horses.string(), "--locale",
                                      "illegal_locale"});

    // 105 is CLI11's ValidationError, from the locale check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("Locale must be nothing, or one of"));
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("add: stores the file under the given locale", "[add]") {
    const auto flag = GENERATE(as<std::string>{}, "-e", "--locale");
    // A language code, and a locale given as a hexadecimal LCID
    const auto locale = GENERATE(as<std::string>{}, "esES", "041D");
    CAPTURE(flag, locale);
    SampleArchive sample;
    const auto cats = sample.files.dir.Path() / "cats.txt";
    WriteFile(cats, "Este es un archivo sobre gatos.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), cats.string(), flag, locale});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file for locale " + locale + ": cats.txt"});
    // A second variant beside the default one, not a replacement of it
    REQUIRE(ArchiveEntries(sample.archive) == Plus(sample_entries, {locale + "  cats.txt"}));
    REQUIRE(ReadEntry(sample.archive, "cats.txt", locale) ==
            std::set<std::string>{"Este es un archivo sobre gatos."});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
}

TEST_CASE("add: replaces only the given locale's variant with --overwrite", "[add]") {
    SampleArchive sample;
    const auto cats = sample.files.dir.Path() / "cats.txt";
    WriteFile(cats, "Este es un archivo sobre gatos.");
    REQUIRE(Run(mpqcli_binary, {"add", sample.archive.string(), cats.string(), "--locale", "esES"})
                .returncode == 0);
    WriteFile(cats, "Este archivo ahora es sobre perros.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), cats.string(), "--locale",
                                      "esES", "--overwrite"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{
                "[~] File for locale esES already exists in MPQ archive: cats.txt - Overwriting...",
                "[+] Adding file for locale esES: cats.txt"});
    REQUIRE(ReadEntry(sample.archive, "cats.txt", "esES") ==
            std::set<std::string>{"Este archivo ahora es sobre perros."});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
}

TEST_CASE("add: re-adds a file after every locale variant was removed", "[add]") {
    ManyLocalesArchive many;
    for (const auto &variant : CatsVariants()) {
        std::vector<std::string> args = {"remove", many.archive.string(), "cats.txt"};
        if (!variant.locale.empty()) {
            args.insert(args.end(), {"--locale", variant.locale});
        }
        REQUIRE(Run(mpqcli_binary, args).returncode == 0);
    }
    REQUIRE(ArchiveEntries(many.archive).empty());
    const auto cats = many.files_dir / "cats.txt";
    WriteFile(cats, "This is a file about cats.");

    auto result = Run(mpqcli_binary, {"add", many.archive.string(), cats.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[+] Adding file: cats.txt"});
    REQUIRE(ReadEntry(many.archive, "cats.txt") == original_cats);
}

TEST_CASE("add: applies the game profile's compression", "[add]") {
    // Profile, and the exact flags a .txt file added under it gets, as for create
    const auto [profile, expected_flags] = GENERATE(table<std::string, std::string>({
        {"generic", "cex"},
        {"diablo1", "iex"},
        {"starcraft1", "ce2x"},
        {"wow1", "cx"},
        {"wow2", "crx"},
        {"starcraft2", "cux"},
        {"diablo3", "cux"},
    }));
    CAPTURE(profile);
    SampleArchive sample;
    const auto name = "test_" + profile + ".txt";
    const auto file = sample.files.dir.Path() / name;
    WriteFile(file, "Test file for " + profile + " profile.");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), file.string(), "--game", profile});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[*] Using game profile: " + profile, "[+] Adding file: " + name});
    REQUIRE(EntryProperty(sample.archive, name, "flags") == expected_flags);
}

TEST_CASE("add: rejects an unknown game profile", "[add]") {
    SampleArchive sample;
    const auto file = sample.files.dir.Path() / "test_invalid.txt";
    WriteFile(file, "Test file for invalid profile.");

    auto result = Run(mpqcli_binary,
                      {"add", sample.archive.string(), file.string(), "-g", "invalid_profile"});

    // 105 is CLI11's ValidationError, from the game profile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("Game profile must be one of"));
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("add: accepts every game profile", "[add]") {
    const auto profile =
        GENERATE(as<std::string>{}, "generic", "diablo1", "lordsofmagic", "starcraft1", "warcraft2",
                 "diablo2", "warcraft3", "warcraft3-map", "wow1", "wow2", "wow3", "wow4", "wow5",
                 "starcraft2", "diablo3");
    CAPTURE(profile);
    SampleArchive sample;
    const auto name = "test_all_" + profile + ".txt";
    const auto file = sample.files.dir.Path() / name;
    WriteFile(file, "Test file for " + profile + " profile.");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), file.string(), "-g", profile});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[*] Using game profile: " + profile, "[+] Adding file: " + name});
    REQUIRE(ArchiveEntries(sample.archive) == Plus(sample_entries, {"enUS  " + name}));
    // Every profile compresses or implodes a text file
    const auto flags = EntryProperty(sample.archive, name, "flags");
    CAPTURE(flags);
    REQUIRE((flags.find('c') != std::string::npos || flags.find('i') != std::string::npos));
}

TEST_CASE("add: a directory keeps its structure", "[add]") {
    SampleArchive sample;
    const auto add_dir = sample.files.dir.Path() / "add_dir";
    WriteFile(add_dir / "sub" / "a.txt", "Sub file A.");
    WriteFile(add_dir / "b.txt", "Root file B.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), add_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: sub\\a.txt", "[+] Adding file: b.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  sub\\a.txt", "enUS  b.txt"}));
}

TEST_CASE("add: a directory with --path prefix", "[add]") {
    SampleArchive sample;
    const auto add_dir = sample.files.dir.Path() / "add_dir_prefix";
    WriteFile(add_dir / "sub" / "a.txt", "Sub file A.");
    WriteFile(add_dir / "b.txt", "Root file B.");

    auto result = Run(mpqcli_binary,
                      {"add", sample.archive.string(), add_dir.string(), "--path", "textures"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: textures\\sub\\a.txt",
                                  "[+] Adding file: textures\\b.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  textures\\sub\\a.txt", "enUS  textures\\b.txt"}));
}

TEST_CASE("add: replaces existing files from a directory with --overwrite", "[add]") {
    SampleArchive sample;
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
    const auto add_dir = sample.files.dir.Path() / "add_dir_overwrite";
    WriteFile(add_dir / "cats.txt", "This is the replaced cats file.");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), add_dir.string(), "--overwrite"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[~] File already exists in MPQ archive: cats.txt - Overwriting...",
                              "[+] Adding file: cats.txt"});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") ==
            std::set<std::string>{"This is the replaced cats file."});
}

TEST_CASE("add: skips existing files from a directory by default", "[add]") {
    SampleArchive sample;
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
    const auto add_dir = sample.files.dir.Path() / "add_dir_conflict";
    WriteFile(add_dir / "cats.txt", "Conflicting cats content.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), add_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] File already exists in MPQ archive: cats.txt - Skipping...",
                                  skipped_hint});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") == original_cats);
}

TEST_CASE("add: skips special files inside a directory", "[add]") {
    SampleArchive sample;
    const auto add_dir = sample.files.dir.Path() / "special";
    WriteFile(add_dir / "(listfile)", "fake content");
    WriteFile(add_dir / "real.txt", "real file");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), add_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Skipping special MPQ file: (listfile)",
                                  "[+] Adding file: real.txt"});
    // The archive's own (listfile) is the only one, and the planted copy never went in
    REQUIRE(ArchiveEntries(sample.archive, true) ==
            Plus(sample_entries, {"enUS  (listfile)", "enUS  real.txt"}));
    REQUIRE(InfoProperty(sample.archive, "file-count") == "5");
}

TEST_CASE("add: skips the archive inside an input directory", "[add]") {
    TempDir dir;
    const auto input_dir = dir.Path() / "mod";
    WriteFile(input_dir / "a.txt", "A file.");
    const auto other = dir.Path() / "b.txt";
    WriteFile(other, "B file.");
    const auto archive = input_dir / "mod.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), other.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"add", archive.string(), input_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Skipping the archive itself: " + archive.string(),
                                  "[+] Adding file: a.txt"});
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  a.txt", "enUS  b.txt"});
}

TEST_CASE("add: skips the archive named as an input", "[add]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Skipping the archive itself: " + sample.archive.string()});
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("add: grows the file table when the archive is full", "[add]") {
    SampleArchive sample;
    REQUIRE(InfoProperty(sample.archive, "max-files") == "64");
    // Three sample files and the (listfile) leave sixty free slots
    const auto fill_dir = sample.files.dir.Path() / "fill";
    for (int i = 0; i < 60; ++i) {
        WriteFile(fill_dir / ("f" + std::to_string(i) + ".txt"), "file " + std::to_string(i));
    }
    REQUIRE(Run(mpqcli_binary, {"add", sample.archive.string(), fill_dir.string()}).returncode ==
            0);
    REQUIRE(InfoProperty(sample.archive, "file-count") == "64");
    REQUIRE(InfoProperty(sample.archive, "max-files") == "64");
    const auto extra = sample.files.dir.Path() / "extra.txt";
    WriteFile(extra, "one more");

    // A second invocation, so the table is grown on reopening an archive that is
    // already full
    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), extra.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: extra.txt"});
    REQUIRE(InfoProperty(sample.archive, "file-count") == "65");
    REQUIRE(InfoProperty(sample.archive, "max-files") == "128");
}

TEST_CASE("add: grows the file table part-way through one run", "[add]") {
    SampleArchive sample;
    REQUIRE(InfoProperty(sample.archive, "max-files") == "64");
    // One more than the sixty free slots, so the table fills mid-run
    const auto fill_dir = sample.files.dir.Path() / "fill";
    std::set<std::string> expected;
    for (int i = 0; i < 61; ++i) {
        const auto name = "f" + std::to_string(i) + ".txt";
        WriteFile(fill_dir / name, "file " + std::to_string(i));
        expected.insert("[+] Adding file: " + name);
    }

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), fill_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == expected);
    REQUIRE(InfoProperty(sample.archive, "file-count") == "65");
    REQUIRE(InfoProperty(sample.archive, "max-files") == "128");
}

TEST_CASE("add: --update skips unchanged files by MD5", "[add]") {
    // wow1 stores CRC32, MD5 and FILETIME. The files are written fresh, so the
    // timestamp check fails and the decision falls through to MD5
    SampleArchive sample({"--game", "wow1"});
    const auto update_dir = sample.files.dir.Path() / "update_dir_unchanged";
    WriteFile(update_dir / "cats.txt", "This is a file about cats.\n");
    WriteFile(update_dir / "dogs.txt", "This is a file about dogs.\n");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), update_dir.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Skipping unchanged file: cats.txt (MD5 matches)",
                                  "[*] Skipping unchanged file: dogs.txt (MD5 matches)",
                                  UpdateSummary(update_dir, 0, 2)});
}

TEST_CASE("add: --update replaces changed files", "[add]") {
    SampleArchive sample({"--game", "wow1"});
    const auto update_dir = sample.files.dir.Path() / "update_dir_changed";
    WriteFile(update_dir / "cats.txt", "This cat content is completely different and longer now.");
    WriteFile(update_dir / "dogs.txt", "This is a file about dogs.\n");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), update_dir.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{
                "[~] File already exists in MPQ archive: cats.txt - Overwriting...",
                "[+] Adding file: cats.txt", "[*] Skipping unchanged file: dogs.txt (MD5 matches)",
                UpdateSummary(update_dir, 1, 1)});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") ==
            std::set<std::string>{"This cat content is completely different and longer now."});
}

TEST_CASE("add: --update is idempotent", "[add]") {
    SampleArchive sample({"--game", "wow1"});
    const auto update_dir = sample.files.dir.Path() / "update_dir_idempotent";
    // Content that differs from the archived copy, so the first run replaces it
    WriteFile(update_dir / "cats.txt", "This cat content is completely different and longer now.");
    const std::vector<std::string> args = {"add", sample.archive.string(), update_dir.string(),
                                           "--update"};
    const auto first = Run(mpqcli_binary, args);
    REQUIRE(first.returncode == 0);
    REQUIRE_THAT(first.stderr_output, ContainsSubstring(UpdateSummary(update_dir, 1, 0)));

    auto result = Run(mpqcli_binary, args);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, !ContainsSubstring("[+] Adding file:"));
    REQUIRE_THAT(result.stderr_output, ContainsSubstring(UpdateSummary(update_dir, 0, 1)));
    // The skip and the summary
    REQUIRE(LinesToVector(result.stderr_output).size() == 2);
    REQUIRE(ReadEntry(sample.archive, "cats.txt") ==
            std::set<std::string>{"This cat content is completely different and longer now."});
}

TEST_CASE("add: --update skips an unchanged single file", "[add]") {
    const auto flag = GENERATE(as<std::string>{}, "-u", "--update");
    CAPTURE(flag);
    SampleArchive sample({"--game", "wow1"});
    const auto cats = sample.files.dir.Path() / "update_single_unchanged" / "cats.txt";
    WriteFile(cats, "This is a file about cats.\n");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), cats.string(), flag});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    // A single file gets the skip line alone: the summary is for directories
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Skipping unchanged file: cats.txt (MD5 matches)"});
}

TEST_CASE("add: --update replaces a changed single file", "[add]") {
    SampleArchive sample({"--game", "wow1"});
    const auto cats = sample.files.dir.Path() / "update_single_changed" / "cats.txt";
    WriteFile(cats, "This cat content is completely different and longer now.");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), cats.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[~] File already exists in MPQ archive: cats.txt - Overwriting...",
                              "[+] Adding file: cats.txt"});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") ==
            std::set<std::string>{"This cat content is completely different and longer now."});
}

TEST_CASE("add: --update adds a file that is not in the archive", "[add]") {
    SampleArchive sample({"--game", "wow1"});
    const auto file = sample.files.dir.Path() / "new.txt";
    WriteFile(file, "new file");

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), file.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[+] Adding file: new.txt"});
    REQUIRE(ReadEntry(sample.archive, "new.txt") == std::set<std::string>{"new file"});
}

TEST_CASE("add: --update and --overwrite exclude each other", "[add]") {
    SampleArchive sample;
    const auto cats = sample.files.files_dir / "cats.txt";

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), cats.string(), "-u", "-w"});

    // 108 is CLI11's ExcludesError
    REQUIRE(result.returncode == 108);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("--overwrite excludes --update"));
}

TEST_CASE("add: --update skips unchanged files by CRC32", "[add]") {
    // warcraft3 stores CRC32 and FILETIME but no MD5, and a fresh mtime rules
    // the timestamp out, so CRC32 is the check that decides
    SampleArchive sample({"--game", "warcraft3"});
    const auto update_dir = sample.files.dir.Path() / "update_dir_crc32_unchanged";
    WriteFile(update_dir / "cats.txt", "This is a file about cats.\n");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), update_dir.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Skipping unchanged file: cats.txt (CRC32 matches)",
                                  UpdateSummary(update_dir, 0, 1)});
}

TEST_CASE("add: --update replaces same-size changed files by CRC32", "[add]") {
    SampleArchive sample({"--game", "warcraft3"});
    const auto update_dir = sample.files.dir.Path() / "update_dir_crc32_changed";
    // The same 27 bytes as the archived copy, so only the content check can tell
    WriteFile(update_dir / "cats.txt", "This is a file about CATS.\n");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), update_dir.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[~] File already exists in MPQ archive: cats.txt - Overwriting...",
                              "[+] Adding file: cats.txt", UpdateSummary(update_dir, 1, 0)});
    REQUIRE(ReadEntry(sample.archive, "cats.txt") ==
            std::set<std::string>{"This is a file about CATS."});
}

TEST_CASE("add: --update skips unchanged files by timestamp", "[add]") {
    // FILETIME only: attr-flags 2 leaves no checksum for the decision to fall to
    SampleArchive sample({"--game", "warcraft3", "--attr-flags", "2"});
    const auto update_dir = sample.files.dir.Path() / "update_dir_ts_unchanged";
    const auto cats = update_dir / "cats.txt";
    WriteFile(cats, "This is a file about cats.\n");
    fs::last_write_time(cats, SampleFileTime());

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), update_dir.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Skipping unchanged file: cats.txt (Timestamp matches)",
                                  UpdateSummary(update_dir, 0, 1)});
}

TEST_CASE("add: --update replaces files whose timestamp changed", "[add]") {
    SampleArchive sample({"--game", "warcraft3", "--attr-flags", "2"});
    const auto update_dir = sample.files.dir.Path() / "update_dir_ts_changed";
    const auto cats = update_dir / "cats.txt";
    WriteFile(cats, "This is a file about cats.\n");
    fs::last_write_time(cats, SampleFileTime() + std::chrono::hours(1));

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), update_dir.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[~] File already exists in MPQ archive: cats.txt - Overwriting...",
                              "[+] Adding file: cats.txt", UpdateSummary(update_dir, 1, 0)});
}

TEST_CASE("add: --update always re-adds without attributes", "[add]") {
    // The default profile writes no (attributes), so there is nothing to compare
    SampleArchive sample;
    const auto update_dir = sample.files.dir.Path() / "update_dir_no_attrs";
    WriteFile(update_dir / "cats.txt", "This is a file about cats.\n");

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), update_dir.string(), "--update"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[~] File already exists in MPQ archive: cats.txt - Overwriting...",
                              "[+] Adding file: cats.txt", UpdateSummary(update_dir, 1, 0)});
}

TEST_CASE("add: reads input paths from stdin with -", "[add]") {
    SampleArchive sample;
    const auto stdin_a = sample.files.dir.Path() / "stdin_a.txt";
    const auto stdin_b = sample.files.dir.Path() / "stdin_b.txt";
    WriteFile(stdin_a, "Stdin file A.");
    WriteFile(stdin_b, "Stdin file B.");
    RunOptions options;
    options.stdin_input = stdin_a.string() + "\n" + stdin_b.string() + "\n";

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: stdin_a.txt", "[+] Adding file: stdin_b.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  stdin_a.txt", "enUS  stdin_b.txt"}));
}

TEST_CASE("add: strips Windows line endings from paths on stdin", "[add]") {
    SampleArchive sample;
    const auto stdin_a = sample.files.dir.Path() / "stdin_a.txt";
    const auto stdin_b = sample.files.dir.Path() / "stdin_b.txt";
    WriteFile(stdin_a, "Stdin file A.");
    WriteFile(stdin_b, "Stdin file B.");
    RunOptions options;
    options.stdin_input = stdin_a.string() + "\r\n" + stdin_b.string() + "\r\n";

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[+] Adding file: stdin_a.txt", "[+] Adding file: stdin_b.txt"});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Plus(sample_entries, {"enUS  stdin_a.txt", "enUS  stdin_b.txt"}));
}

TEST_CASE("add: - with empty stdin adds nothing", "[add]") {
    SampleArchive sample;
    RunOptions options;
    options.stdin_input = "";

    auto result = Run(mpqcli_binary, {"add", sample.archive.string(), "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(result.stderr_output.empty());
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("add: --flags sets the added file's flags", "[add]") {
    // Flag value, then the letters the flags column must and must not carry
    const auto [flags, present, absent] = GENERATE(table<std::string, std::string, std::string>({
        {"0", "", "ce"},     // Stored raw: neither compressed nor encrypted
        {"512", "c", "e"},   // MPQ_FILE_COMPRESS
        {"66048", "ce", ""}, // MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED
    }));
    CAPTURE(flags);
    SampleArchive sample;
    const auto file = sample.files.dir.Path() / "text.txt";
    WriteFile(file, CompressibleText(12000));

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), file.string(), "--flags", flags});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[+] Adding file: text.txt"});
    const auto stored = EntryProperty(sample.archive, "text.txt", "flags");
    CAPTURE(stored);
    for (const char letter : present) {
        CAPTURE(letter);
        REQUIRE(stored.find(letter) != std::string::npos);
    }
    for (const char letter : absent) {
        CAPTURE(letter);
        REQUIRE(stored.find(letter) == std::string::npos);
    }
}

TEST_CASE("add: --flags 0 stores the file uncompressed", "[add]") {
    SampleArchive sample;
    const auto file = sample.files.dir.Path() / "text.txt";
    WriteFile(file, CompressibleText(12000));

    auto result =
        Run(mpqcli_binary, {"add", sample.archive.string(), file.string(), "--flags", "0"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[+] Adding file: text.txt"});
    REQUIRE(EntryNumber(sample.archive, "text.txt", "compressed-size") ==
            EntryNumber(sample.archive, "text.txt", "file-size"));
}

TEST_CASE("add: --compression selects the algorithm", "[add]") {
    TempDir dir;
    const auto file = dir.Path() / "text.txt";
    WriteFile(file, CompressibleText(12000));
    // A fresh archive each time, with the same algorithm for every sector so
    // only --compression is under test
    const auto add_with = [&](const std::string &compression) {
        SampleFiles files;
        const auto archive = dir.Path() / ("compression_" + compression + ".mpq");
        REQUIRE(
            Run(mpqcli_binary, {"create", archive.string(), files.files_dir.string()}).returncode ==
            0);
        auto result = Run(mpqcli_binary, {"add", archive.string(), file.string(), "--compression",
                                          compression, "--compression-next", compression});
        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToSet(result.stderr_output) ==
                std::set<std::string>{"[+] Adding file: text.txt"});
        return archive;
    };

    const auto zlib = add_with("2");   // MPQ_COMPRESSION_ZLIB
    const auto bzip2 = add_with("16"); // MPQ_COMPRESSION_BZIP2

    const auto raw_size = EntryNumber(zlib, "text.txt", "file-size");
    const auto zlib_size = EntryNumber(zlib, "text.txt", "compressed-size");
    const auto bzip2_size = EntryNumber(bzip2, "text.txt", "compressed-size");
    CAPTURE(raw_size, zlib_size, bzip2_size);
    REQUIRE(zlib_size < raw_size);
    REQUIRE(bzip2_size < raw_size);
    REQUIRE(zlib_size != bzip2_size);
}

TEST_CASE("add: --compression-next selects the algorithm after the first sector", "[add]") {
    TempDir dir;
    const auto file = dir.Path() / "text.txt";
    // Well over one sector, so there are subsequent sectors for the option to reach
    WriteFile(file, CompressibleText(12000));
    const auto add_with = [&](const std::string &compression_next) {
        SampleFiles files;
        const auto archive = dir.Path() / ("next_" + compression_next + ".mpq");
        REQUIRE(
            Run(mpqcli_binary, {"create", archive.string(), files.files_dir.string()}).returncode ==
            0);
        auto result = Run(mpqcli_binary, {"add", archive.string(), file.string(), "--compression",
                                          "2", "--compression-next", compression_next});
        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToSet(result.stderr_output) ==
                std::set<std::string>{"[+] Adding file: text.txt"});
        return archive;
    };

    const auto all_zlib = add_with("2");
    const auto then_bzip2 = add_with("16");

    const auto raw_size = EntryNumber(all_zlib, "text.txt", "file-size");
    const auto all_zlib_size = EntryNumber(all_zlib, "text.txt", "compressed-size");
    const auto then_bzip2_size = EntryNumber(then_bzip2, "text.txt", "compressed-size");
    CAPTURE(raw_size, all_zlib_size, then_bzip2_size);
    REQUIRE(all_zlib_size < raw_size);
    REQUIRE(then_bzip2_size < raw_size);
    REQUIRE(all_zlib_size != then_bzip2_size);
}
