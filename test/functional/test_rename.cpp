#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
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

// What a fresh SampleArchive lists, before any rename
const std::set<std::string> sample_entries = {"enUS  bytes", "enUS  cats.txt", "enUS  dogs.txt"};

// What ManyLocalesArchive lists: cats.txt under the default locale and three others
const std::set<std::string> all_variants = {"enUS  cats.txt", "041D  cats.txt", "deDE  cats.txt",
                                            "esES  cats.txt"};

// The complete listfile for NoListfileArchive, with the names the cases rename to
const std::string complete_listfile = "cats.txt\ndogs.txt\ncapybaras.txt\nrenamed.txt\n";

// entries with one line swapped for another
std::set<std::string> Renamed(std::set<std::string> entries, const std::string &from,
                              const std::string &to) {
    entries.erase(from);
    entries.insert(to);
    return entries;
}

// The line rename prints to stderr before trying; a locale is named only when
// one was given, and the new name is shown as it will be stored
std::string Renaming(const std::string &from, const std::string &to,
                     const std::string &locale = "") {
    return "[~] Renaming file" + (locale.empty() ? "" : " for locale " + locale) + ": " + from +
           " -> " + to;
}

// The failure when the entry is not stored under the locale looked for, which
// is named even when it is the default one
std::string MissingUnderLocale(const std::string &locale, const std::string &name) {
    return "[!] Failed: File doesn't exist for locale " + locale + ": " + name;
}

// The failure when StormLib refuses the rename itself
std::string CannotRename(const std::string &locale, const std::string &from,
                         const std::string &to) {
    return "[!] Failed: File cannot be renamed for locale " + locale + ": " + from + " -> " + to;
}

} // namespace

TEST_CASE("rename: prints its help with --help", "[rename]") {
    auto result = Run(mpqcli_binary, {"rename", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE_THAT(result.stdout_output,
                 ContainsSubstring("rename [OPTIONS] archive old-file new-file"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--locale"));
}

TEST_CASE("rename: archive does not exist", "[rename]") {
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result =
        Run(mpqcli_binary, {"rename", "no-such-archive.mpq", "cats.txt", "felines.txt"}, options);

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("File does not exist: no-such-archive.mpq"));
}

TEST_CASE("rename: fails when the archive is not an MPQ", "[rename]") {
    SampleFiles files;

    auto result = Run(mpqcli_binary, {"rename", (files.files_dir / "cats.txt").string(), "cats.txt",
                                      "felines.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output).size() == 1);
    REQUIRE_THAT(result.stderr_output, ContainsSubstring(open_failure));
}

TEST_CASE("rename: rejects an unknown locale", "[rename]") {
    SampleArchive sample;

    // Like create, add and remove, rename validates the locale at parse time
    auto result = Run(mpqcli_binary, {"rename", sample.archive.string(), "cats.txt", "felines.txt",
                                      "--locale", "nosuchlocale"});

    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("Locale must be nothing, or one of"));
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("rename: rejects extra arguments", "[rename]") {
    SampleArchive sample;

    auto result =
        Run(mpqcli_binary, {"rename", sample.archive.string(), "cats.txt", "felines.txt", "extra"});

    // 109 is CLI11's ExtrasError
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: extra"));
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("rename: fails when the archive has no such entry", "[rename]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary,
                      {"rename", sample.archive.string(), "does-not-exist.txt", "renamed.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    // The attempt is announced before it fails
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("does-not-exist.txt", "renamed.txt"),
                                     MissingUnderLocale("enUS", "does-not-exist.txt")});
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("rename: a single file", "[rename]") {
    SampleArchive sample;
    const auto size_before = fs::file_size(sample.archive);

    auto result =
        Run(mpqcli_binary, {"rename", sample.archive.string(), "cats.txt", "felines.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("cats.txt", "felines.txt")});
    // The internal (listfile) follows, so the new name lists without help
    REQUIRE(ArchiveEntries(sample.archive) ==
            Renamed(sample_entries, "enUS  cats.txt", "enUS  felines.txt"));
    REQUIRE(ArchiveEntries(sample.archive, true).count("enUS  (listfile)") == 1);
    REQUIRE(ReadEntry(sample.archive, "felines.txt") ==
            std::set<std::string>{"This is a file about cats."});
    REQUIRE(Run(mpqcli_binary, {"read", sample.archive.string(), "cats.txt"}).returncode == 1);
    // The (listfile) sits at the archive's end and is rewritten in place, so
    // the archive moves by the name-length difference alone, with no gap left
    // for compact to reclaim: the no-fragmentation claim the docs page makes
    REQUIRE(fs::file_size(sample.archive) ==
            size_before + std::string("felines.txt").size() - std::string("cats.txt").size());
}

TEST_CASE("rename: matches the old name without regard to case", "[rename]") {
    SampleArchive sample;

    auto result =
        Run(mpqcli_binary, {"rename", sample.archive.string(), "DOGS.TXT", "canines.txt"});

    // MPQ name hashes are computed over the upper-cased name; the message
    // echoes the spelling given
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("DOGS.TXT", "canines.txt")});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Renamed(sample_entries, "enUS  dogs.txt", "enUS  canines.txt"));
}

TEST_CASE("rename: refuses the same name or a change of case alone", "[rename]") {
    const auto new_name = GENERATE(as<std::string>{}, "cats.txt", "CATS.TXT");
    CAPTURE(new_name);
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"rename", sample.archive.string(), "cats.txt", new_name});

    // Both names hash to the same entry, so StormLib sees a name already taken
    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("cats.txt", new_name),
                                     CannotRename("enUS", "cats.txt", new_name)});
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
}

TEST_CASE("rename: refuses a name another entry holds under the same locale", "[rename]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"rename", sample.archive.string(), "cats.txt", "dogs.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("cats.txt", "dogs.txt"),
                                     CannotRename("enUS", "cats.txt", "dogs.txt")});
    // Neither entry is touched
    REQUIRE(ArchiveEntries(sample.archive) == sample_entries);
    REQUIRE(ReadEntry(sample.archive, "dogs.txt") ==
            std::set<std::string>{"This is a file about dogs."});
}

TEST_CASE("rename: allows a name another entry holds under a different locale", "[rename]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, complete_listfile);

    // cats.txt is stored under deDE alone, so the default locale is free to take it
    auto result =
        Run(mpqcli_binary, {"rename", archive.archive.string(), "capybaras.txt", "cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("capybaras.txt", "cats.txt")});
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) ==
            std::set<std::string>{"enUS  cats.txt", "deDE  cats.txt", "041D  dogs.txt"});
    // Each variant keeps its own content
    REQUIRE(ReadEntry(archive.archive, "cats.txt") ==
            std::set<std::string>{"This is a file about capybaras."});
    REQUIRE(ReadEntry(archive.archive, "cats.txt", "deDE") ==
            std::set<std::string>{"Dies ist eine Datei ueber Katzen."});
}

TEST_CASE("rename: refuses the special entries", "[rename]") {
    const auto name = GENERATE(as<std::string>{}, "(listfile)", "(attributes)", "(signature)");
    CAPTURE(name);
    std::vector<std::string> args = AttributedArchiveArgs();
    args.push_back("-s");
    SampleArchive sample(args);
    const auto before = ArchiveEntries(sample.archive, true);
    REQUIRE(before.count("enUS  " + name) == 1);

    auto result = Run(mpqcli_binary, {"rename", sample.archive.string(), name, "other.txt"});

    // StormLib keeps its own entries: the name is found but the rename is refused
    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming(name, "other.txt"),
                                     CannotRename("enUS", name, "other.txt")});
    REQUIRE(ArchiveEntries(sample.archive, true) == before);
}

TEST_CASE("rename: stores a new name given with forward slashes using backslashes", "[rename]") {
    SampleArchive sample;

    auto result =
        Run(mpqcli_binary, {"rename", sample.archive.string(), "cats.txt", "deep/renamed.txt"});

    // As add and create store paths, and as the message already shows it
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("cats.txt", "deep\\renamed.txt")});
    REQUIRE(ArchiveEntries(sample.archive) ==
            Renamed(sample_entries, "enUS  cats.txt", "enUS  deep\\renamed.txt"));
    REQUIRE(ReadEntry(sample.archive, "deep\\renamed.txt") ==
            std::set<std::string>{"This is a file about cats."});
}

TEST_CASE("rename: a nested entry by its backslash path", "[rename]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "root.txt", "root file");
    WriteFile(source_dir / "sub" / "nested.txt", "nested file");
    const auto archive = dir.Path() / "nested.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"rename", archive.string(), "sub\\nested.txt", "top.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("sub\\nested.txt", "top.txt")});
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  root.txt", "enUS  top.txt"});
    REQUIRE(ReadEntry(archive, "top.txt") == std::set<std::string>{"nested file"});
}

TEST_CASE("rename: the default-locale variant when no locale is given", "[rename]") {
    ManyLocalesArchive locales;

    auto result =
        Run(mpqcli_binary, {"rename", locales.archive.string(), "cats.txt", "renamed_enus.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("cats.txt", "renamed_enus.txt")});
    // The other variants keep their name
    REQUIRE(ArchiveEntries(locales.archive) ==
            Renamed(all_variants, "enUS  cats.txt", "enUS  renamed_enus.txt"));
    REQUIRE(ReadEntry(locales.archive, "renamed_enus.txt") ==
            std::set<std::string>{CatsContent("")});
    REQUIRE(ReadEntry(locales.archive, "cats.txt", "deDE") ==
            std::set<std::string>{CatsContent("deDE")});
}

TEST_CASE("rename: the variant under the given locale", "[rename]") {
    // The locale as given, how the message names it, the listing's label for
    // it, and the variant whose content must now read under the new name: a
    // language code, the hexadecimal LCID the locale table does not name, the
    // hexadecimal LCID of one it does (0407 is deDE), and the "default" alias
    // the validator accepts, which names no locale in the message
    const auto [locale, named, label, variant] =
        GENERATE(table<std::string, std::string, std::string, std::string>({
            {"esES", "esES", "esES", "esES"},
            {"041D", "041D", "041D", "041D"},
            {"0407", "deDE", "deDE", "deDE"},
            {"default", "", "enUS", ""},
        }));
    const auto flag = GENERATE(as<std::string>{}, "-e", "--locale");
    CAPTURE(locale, flag);
    ManyLocalesArchive locales;

    auto result = Run(mpqcli_binary, {"rename", locales.archive.string(), "cats.txt", "renamed.txt",
                                      flag, locale});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("cats.txt", "renamed.txt", named)});
    REQUIRE(ArchiveEntries(locales.archive) ==
            Renamed(all_variants, label + "  cats.txt", label + "  renamed.txt"));
    REQUIRE(ReadEntry(locales.archive, "renamed.txt", locale) ==
            std::set<std::string>{CatsContent(variant)});
}

TEST_CASE("rename: each variant in turn without touching the others", "[rename]") {
    ManyLocalesArchive locales;
    auto expected = all_variants;

    for (const auto &[locale, label] : std::vector<std::pair<std::string, std::string>>{
             {"", "enUS"}, {"esES", "esES"}, {"041D", "041D"}, {"deDE", "deDE"}}) {
        CAPTURE(locale);
        const auto new_name = "renamed_" + label + ".txt";
        std::vector<std::string> args = {"rename", locales.archive.string(), "cats.txt", new_name};
        if (!locale.empty()) {
            args.push_back("--locale");
            args.push_back(locale);
        }

        auto result = Run(mpqcli_binary, args);

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToVector(result.stderr_output) ==
                std::vector<std::string>{Renaming("cats.txt", new_name, locale)});
        expected = Renamed(expected, label + "  cats.txt", label + "  " + new_name);
        REQUIRE(ArchiveEntries(locales.archive) == expected);
    }
}

TEST_CASE("rename: fails for a locale the entry is not stored under", "[rename]") {
    SECTION("an entry stored under one locale") {
        NoListfileArchive archive;
        const auto before = ArchiveEntries(archive.archive);

        for (const auto &entry : NoListfileArchive::Entries()) {
            CAPTURE(entry.name);

            auto result = Run(mpqcli_binary, {"rename", archive.archive.string(), entry.name,
                                              "renamed.txt", "--locale", "ptPT"});

            // Whether the entry is under the default locale, a named one or a bare
            // LCID, a locale it lacks is a miss with no fallback
            REQUIRE(result.returncode == 1);
            REQUIRE(result.stdout_output.empty());
            REQUIRE(LinesToVector(result.stderr_output) ==
                    std::vector<std::string>{Renaming(entry.name, "renamed.txt", "ptPT"),
                                             MissingUnderLocale("ptPT", entry.name)});
        }
        REQUIRE(ArchiveEntries(archive.archive) == before);
    }

    SECTION("an entry stored under several locales") {
        ManyLocalesArchive locales;

        auto result = Run(mpqcli_binary, {"rename", locales.archive.string(), "cats.txt",
                                          "renamed.txt", "--locale", "ptPT"});

        REQUIRE(result.returncode == 1);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToVector(result.stderr_output) ==
                std::vector<std::string>{Renaming("cats.txt", "renamed.txt", "ptPT"),
                                         MissingUnderLocale("ptPT", "cats.txt")});
        REQUIRE(ArchiveEntries(locales.archive) == all_variants);
    }
}

TEST_CASE("rename: fails without a locale for an entry stored only under another", "[rename]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, complete_listfile);
    const std::set<std::string> before = {"enUS  capybaras.txt", "deDE  cats.txt",
                                          "041D  dogs.txt"};
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) == before);

    // dogs.txt is stored under 041D alone: the default locale has no such entry
    auto without =
        Run(mpqcli_binary, {"rename", archive.archive.string(), "dogs.txt", "renamed.txt"});

    REQUIRE(without.returncode == 1);
    REQUIRE(without.stdout_output.empty());
    REQUIRE(LinesToVector(without.stderr_output) ==
            std::vector<std::string>{Renaming("dogs.txt", "renamed.txt"),
                                     MissingUnderLocale("enUS", "dogs.txt")});
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) == before);

    auto with = Run(mpqcli_binary, {"rename", archive.archive.string(), "dogs.txt", "renamed.txt",
                                    "--locale", "041D"});

    REQUIRE(with.returncode == 0);
    REQUIRE(with.stdout_output.empty());
    REQUIRE(LinesToVector(with.stderr_output) ==
            std::vector<std::string>{Renaming("dogs.txt", "renamed.txt", "041D")});
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) ==
            Renamed(before, "041D  dogs.txt", "041D  renamed.txt"));
    REQUIRE(ReadEntry(archive.archive, "renamed.txt", "041D") ==
            std::set<std::string>{"Detta aer en fil om hundar."});
}

TEST_CASE("rename: finds an entry by name in an archive without a listfile", "[rename]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, complete_listfile);
    const auto size_before = fs::file_size(archive.archive);

    // The name hashes to its entry whether or not the archive can list it
    auto result =
        Run(mpqcli_binary, {"rename", archive.archive.string(), "capybaras.txt", "renamed.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToVector(result.stderr_output) ==
            std::vector<std::string>{Renaming("capybaras.txt", "renamed.txt")});
    REQUIRE(ArchiveEntries(archive.archive, false, listfile) ==
            std::set<std::string>{"enUS  renamed.txt", "deDE  cats.txt", "041D  dogs.txt"});
    REQUIRE(ReadEntry(archive.archive, "renamed.txt") ==
            std::set<std::string>{"This is a file about capybaras."});
    REQUIRE(Run(mpqcli_binary, {"read", archive.archive.string(), "capybaras.txt"}).returncode ==
            1);
    // With no (listfile) to rewrite, a rename is a hash-table edit alone and
    // the archive keeps its size
    REQUIRE(fs::file_size(archive.archive) == size_before);
}
