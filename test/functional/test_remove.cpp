#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "archive_helper.h"
#include "many_locales_archive.h"
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

const std::string open_failure = "[!] Failed to open MPQ archive";

// What a fresh SampleArchive lists, before any remove
const std::set<std::string> sample_entries = {"enUS  bytes", "enUS  cats.txt", "enUS  dogs.txt"};

// What ManyLocalesArchive lists: cats.txt under the default locale and three others
const std::set<std::string> all_variants = {"enUS  cats.txt", "041D  cats.txt", "deDE  cats.txt",
                                            "esES  cats.txt"};

std::set<std::string> Minus(std::set<std::string> entries,
                            std::initializer_list<std::string> gone) {
    for (const auto &entry : gone) {
        entries.erase(entry);
    }
    return entries;
}

// The line remove prints to stderr for each name before trying it; a locale is
// named only when one was given
std::string Removing(const std::string &name, const std::string &locale = "") {
    return "[-] Removing file" + (locale.empty() ? "" : " for locale " + locale) + ": " + name;
}

// The failure when the entry is not stored under the locale looked for, which
// is named even when it is the default one
std::string MissingUnderLocale(const std::string &locale, const std::string &name) {
    return "[!] Failed: File doesn't exist for locale " + locale + ": " + name;
}

} // namespace

TEST_CASE("remove: prints its help with --help", "[remove]") {
    auto result = Run(mpqcli_binary, {"remove", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("remove [OPTIONS] archive files..."));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--locale"));
}

TEST_CASE("remove: archive does not exist", "[remove]") {
    auto result = Run(mpqcli_binary, {"remove", "no-such-archive.mpq", "cats.txt"});

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("File does not exist: no-such-archive.mpq"));
}

TEST_CASE("remove: fails when the archive is not an MPQ", "[remove]") {
    SampleFiles files;

    auto result =
        Run(mpqcli_binary, {"remove", (files.files_dir / "cats.txt").string(), "cats.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output).size() == 1);
    REQUIRE_THAT(result.stderr_output, ContainsSubstring(open_failure));
}

TEST_CASE("remove: rejects an unknown locale", "[remove]") {
    SampleArchive sample;

    // Like create and add, remove validates the locale at parse time
    auto result = Run(mpqcli_binary,
                      {"remove", sample.archive.string(), "cats.txt", "--locale", "nosuchlocale"});

    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("Locale must be nothing, or one of"));
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("remove: fails when the archive has no such entry", "[remove]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "does-not-exist.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    // The attempt is announced before it fails
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("does-not-exist.txt"),
                                     MissingUnderLocale("enUS", "does-not-exist.txt")});
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("remove: a single file", "[remove]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == std::vector<std::string>{Removing("cats.txt")});
    REQUIRE(ArchiveEntries(sample.archive) == Minus(sample_entries, {"enUS  cats.txt"}));
    // Gone for read as well, not just from the listing
    REQUIRE(Run(mpqcli_binary, {"read", sample.archive.string(), "cats.txt"}).returncode == 1);
}

TEST_CASE("remove: several files in one run", "[remove]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "cats.txt", "dogs.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    // Announced in the order given
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("cats.txt"), Removing("dogs.txt")});
    REQUIRE(ArchiveEntries(sample.archive) == std::set<std::string>{"enUS  bytes"});
}

TEST_CASE("remove: a name given twice is removed once", "[remove]") {
    // The repeat is dropped rather than failing as a second, missing entry
    SECTION("without a locale") {
        SampleArchive sample;

        auto result =
            Run(mpqcli_binary, {"remove", sample.archive.string(), "cats.txt", "cats.txt"});

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToVector(result.stderr_output) ==
                std::vector<std::string>{Removing("cats.txt")});
        REQUIRE(ArchiveEntries(sample.archive) == Minus(sample_entries, {"enUS  cats.txt"}));
    }

    SECTION("with a locale") {
        ManyLocalesArchive locales;

        auto result = Run(mpqcli_binary, {"remove", locales.archive.string(), "cats.txt",
                                          "cats.txt", "--locale", "esES"});

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToVector(result.stderr_output) ==
                std::vector<std::string>{Removing("cats.txt", "esES")});
        REQUIRE(ArchiveEntries(locales.archive) == Minus(all_variants, {"esES  cats.txt"}));
    }
}

TEST_CASE("remove: continues past a missing entry and still fails", "[remove]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary,
                      {"remove", sample.archive.string(), "cats.txt", "ghost.txt", "dogs.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("cats.txt"), Removing("ghost.txt"),
                                     MissingUnderLocale("enUS", "ghost.txt"),
                                     Removing("dogs.txt")});
    // The names on either side of the missing one are still removed
    REQUIRE(ArchiveEntries(sample.archive) == std::set<std::string>{"enUS  bytes"});
}

TEST_CASE("remove: every user file leaves only the special entries", "[remove]") {
    SampleArchive sample(AttributedArchiveArgs());

    auto result =
        Run(mpqcli_binary, {"remove", sample.archive.string(), "cats.txt", "dogs.txt", "bytes"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == std::vector<std::string>{Removing("cats.txt"),
                                                                            Removing("dogs.txt"),
                                                                            Removing("bytes")});
    REQUIRE(ArchiveEntries(sample.archive).empty());
    REQUIRE(ArchiveEntries(sample.archive, true) ==
            std::set<std::string>{"enUS  (listfile)", "enUS  (attributes)"});
    REQUIRE(InfoProperty(sample.archive, "file-count") == "2");
}

TEST_CASE("remove: refuses the special entries", "[remove]") {
    const auto name = GENERATE(as<std::string>{}, "(listfile)", "(attributes)", "(signature)");
    CAPTURE(name);
    std::vector<std::string> args = AttributedArchiveArgs();
    args.push_back("-s");
    SampleArchive sample(args);
    const auto before = ArchiveEntries(sample.archive, true);
    REQUIRE(before.count("enUS  " + name) == 1);

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), name});

    // StormLib keeps its own entries: the name is found but the removal is refused
    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{
                Removing(name), "[!] Failed: File cannot be removed for locale enUS: " + name});
    REQUIRE(ArchiveEntries(sample.archive, true) == before);
    REQUIRE(InfoProperty(sample.archive, "signature-type") == "Weak");
}

TEST_CASE("remove: matches the entry name without regard to case", "[remove]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "CATS.TXT"});

    // MPQ name hashes are computed over the upper-cased name; the message
    // echoes the spelling given
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == std::vector<std::string>{Removing("CATS.TXT")});
    REQUIRE(ArchiveEntries(sample.archive) == Minus(sample_entries, {"enUS  cats.txt"}));
}

TEST_CASE("remove: a nested entry by its backslash path", "[remove]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "root.txt", "root file");
    WriteFile(source_dir / "sub" / "nested.txt", "nested file");
    const auto archive = dir.Path() / "nested.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"remove", archive.string(), "sub\\nested.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("sub\\nested.txt")});
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  root.txt"});
}

TEST_CASE("remove: the default-locale variant when no locale is given", "[remove]") {
    ManyLocalesArchive locales;

    auto result = Run(mpqcli_binary, {"remove", locales.archive.string(), "cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) == std::vector<std::string>{Removing("cats.txt")});
    REQUIRE(ArchiveEntries(locales.archive) == Minus(all_variants, {"enUS  cats.txt"}));
    // The other variants are untouched: the German one still reads, and a read
    // with no locale has nothing left to fall back to
    REQUIRE(ReadEntry(locales.archive, "cats.txt", "deDE") ==
            std::set<std::string>{CatsContent("deDE")});
    auto fallback = Run(mpqcli_binary, {"read", locales.archive.string(), "cats.txt"});
    REQUIRE(fallback.returncode == 1);
    REQUIRE(fallback.stdout_output.empty());
    REQUIRE(LinesToSet(fallback.stderr_output) ==
            std::set<std::string>{MissingUnderLocale("enUS", "cats.txt")});
}

TEST_CASE("remove: the variant under the given locale", "[remove]") {
    // The locale as given, how the message names it, and the entry that goes:
    // a language code, the hexadecimal LCID the locale table does not name,
    // the hexadecimal LCID of one it does (0407 is deDE), and the "default"
    // alias the validator accepts, which names no locale in the message
    const auto [locale, named, entry] = GENERATE(table<std::string, std::string, std::string>({
        {"esES", "esES", "esES  cats.txt"},
        {"041D", "041D", "041D  cats.txt"},
        {"0407", "deDE", "deDE  cats.txt"},
        {"default", "", "enUS  cats.txt"},
    }));
    const auto flag = GENERATE(as<std::string>{}, "-e", "--locale");
    CAPTURE(locale, flag);
    ManyLocalesArchive locales;

    auto result =
        Run(mpqcli_binary, {"remove", locales.archive.string(), "cats.txt", flag, locale});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("cats.txt", named)});
    REQUIRE(ArchiveEntries(locales.archive) == Minus(all_variants, {entry}));
}

TEST_CASE("remove: each variant in turn until none is left", "[remove]") {
    ManyLocalesArchive locales;
    auto remaining = all_variants;

    for (const auto &[locale, entry] :
         std::vector<std::pair<std::string, std::string>>{{"", "enUS  cats.txt"},
                                                          {"esES", "esES  cats.txt"},
                                                          {"041D", "041D  cats.txt"},
                                                          {"deDE", "deDE  cats.txt"}}) {
        CAPTURE(locale);
        std::vector<std::string> args = {"remove", locales.archive.string(), "cats.txt"};
        if (!locale.empty()) {
            args.push_back("--locale");
            args.push_back(locale);
        }

        auto result = Run(mpqcli_binary, args);

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToVector(result.stderr_output) ==
                std::vector<std::string>{Removing("cats.txt", locale)});
        remaining.erase(entry);
        REQUIRE(ArchiveEntries(locales.archive) == remaining);
    }
    REQUIRE(ArchiveEntries(locales.archive, true) == std::set<std::string>{"enUS  (listfile)"});
}

TEST_CASE("remove: fails for a locale the entry is not stored under", "[remove]") {
    SECTION("an entry stored under one locale") {
        NoListfileArchive archive;
        const auto before = ArchiveEntries(archive.archive);

        for (const auto &entry : NoListfileArchive::Entries()) {
            CAPTURE(entry.name);

            auto result = Run(mpqcli_binary,
                              {"remove", archive.archive.string(), entry.name, "--locale", "ptPT"});

            // Whether the entry is under the default locale, a named one or a bare
            // LCID, a locale it lacks is a miss with no fallback
            REQUIRE(result.returncode == 1);
            REQUIRE(result.stdout_output.empty());
            REQUIRE(LinesToVector(result.stderr_output) ==
                    std::vector<std::string>{Removing(entry.name, "ptPT"),
                                             MissingUnderLocale("ptPT", entry.name)});
        }
        REQUIRE(ArchiveEntries(archive.archive) == before);
    }

    SECTION("an entry stored under several locales") {
        ManyLocalesArchive locales;

        auto result = Run(mpqcli_binary,
                          {"remove", locales.archive.string(), "cats.txt", "--locale", "ptPT"});

        REQUIRE(result.returncode == 1);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToVector(result.stderr_output) ==
                std::vector<std::string>{Removing("cats.txt", "ptPT"),
                                         MissingUnderLocale("ptPT", "cats.txt")});
        REQUIRE(ArchiveEntries(locales.archive) == all_variants);
    }
}

TEST_CASE("remove: fails without a locale for an entry stored only under another", "[remove]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, "cats.txt\ndogs.txt\ncapybaras.txt");
    const std::set<std::string> before = {"enUS  capybaras.txt", "deDE  cats.txt",
                                          "041D  dogs.txt"};
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) == before);

    // dogs.txt is stored under 041D alone: the default locale has no such entry
    auto without = Run(mpqcli_binary, {"remove", archive.archive.string(), "dogs.txt"});

    REQUIRE(without.returncode == 1);
    REQUIRE(without.stdout_output.empty());
    REQUIRE(LinesToVector(without.stderr_output) ==
            std::vector<std::string>{Removing("dogs.txt"), MissingUnderLocale("enUS", "dogs.txt")});
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) == before);

    auto with =
        Run(mpqcli_binary, {"remove", archive.archive.string(), "dogs.txt", "--locale", "041D"});

    REQUIRE(with.returncode == 0);
    REQUIRE(with.stdout_output.empty());
    REQUIRE(LinesToVector(with.stderr_output) ==
            std::vector<std::string>{Removing("dogs.txt", "041D")});
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) == Minus(before, {"041D  dogs.txt"}));
}

TEST_CASE("remove: finds an entry by name in an archive without a listfile", "[remove]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, "cats.txt\ndogs.txt\ncapybaras.txt");

    // The name hashes to its entry whether or not the archive can list it
    auto result = Run(mpqcli_binary, {"remove", archive.archive.string(), "capybaras.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("capybaras.txt")});
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) ==
            std::set<std::string>{"deDE  cats.txt", "041D  dogs.txt"});
}

TEST_CASE("remove: reads names from stdin with -", "[remove]") {
    SampleArchive sample;
    RunOptions options;
    // Blank lines are skipped
    options.stdin_input = "\ncats.txt\n\ndogs.txt\n";

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("cats.txt"), Removing("dogs.txt")});
    REQUIRE(ArchiveEntries(sample.archive) == std::set<std::string>{"enUS  bytes"});
}

TEST_CASE("remove: strips Windows line endings from names read from stdin", "[remove]") {
    SampleArchive sample;
    RunOptions options;
    // A name that kept its carriage return would match no entry, and the blank
    // line would become a name of its own
    options.stdin_input = "cats.txt\r\n\r\ndogs.txt\r\n";

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("cats.txt"), Removing("dogs.txt")});
    REQUIRE(ArchiveEntries(sample.archive) == std::set<std::string>{"enUS  bytes"});
}

TEST_CASE("remove: - with empty stdin removes nothing", "[remove]") {
    SampleArchive sample;
    RunOptions options;
    options.stdin_input = "";

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(result.stderr_output.empty());
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("remove: mixes names given as arguments with names from stdin", "[remove]") {
    SampleArchive sample;
    RunOptions options;
    options.stdin_input = "dogs.txt\n";

    auto result = Run(mpqcli_binary, {"remove", sample.archive.string(), "cats.txt", "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Removing("cats.txt"), Removing("dogs.txt")});
    REQUIRE(ArchiveEntries(sample.archive) == std::set<std::string>{"enUS  bytes"});
}
