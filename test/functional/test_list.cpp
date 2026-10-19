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
#include <vector>

#include "integration_data.h"
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

// Asserts output is exactly these lines. Any order, since list prints entries
// in hash-table order, but none missing and none repeated, which a set alone
// would not catch
void RequireLines(const std::string &output, const std::set<std::string> &expected) {
    const auto lines = LinesToVector(output);
    CAPTURE(lines);
    REQUIRE(lines.size() == expected.size());
    REQUIRE(std::set<std::string>(lines.begin(), lines.end()) == expected);
}

// The entries of an archive built with AttributedArchiveArgs(): the sample
// files, the (listfile), and the (attributes) block that gives them a time
const std::set<std::string> user_entries = {"bytes", "cats.txt", "dogs.txt"};
const std::set<std::string> all_entries = {"bytes", "cats.txt", "dogs.txt", "(listfile)",
                                           "(attributes)"};

// The default detailed columns: size, locale and time. The time is the sample
// files' pinned modification time, set and printed in local time, so it reads
// the same in any zone. The special entries carry no time of their own.
const std::set<std::string> detailed_entries = {
    "       8 enUS 2025-07-29 14:31:00  bytes",
    "      27 enUS 2025-07-29 14:31:00  cats.txt",
    "      27 enUS 2025-07-29 14:31:00  dogs.txt",
    "      27 enUS                      (listfile)",
    "     149 enUS                      (attributes)",
};

// Every property list accepts, with the lines "-a -d -q <property>" prints for
// the attributed archive. Read back from the binary; they hold everywhere for
// the reasons the info tests give. byte-offset prints in hexadecimal with no
// prefix, and name-hash3 is all zeros because a version 1 archive has no HET
// table. The (listfile) and (attributes) keys differ between encryption-key
// and its raw form because their f flag folds the offset into the key.
struct PropertyListing {
    std::string property;
    std::set<std::string> lines;
};

const std::vector<PropertyListing> property_listings = {
    {"hash-index",
     {"   35  bytes", "   44  cats.txt", "    0  dogs.txt", "   25  (listfile)",
      "   14  (attributes)"}},
    {"name-hash1",
     {"147178ed  bytes", "0fd58937  cats.txt", "eb30456b  dogs.txt", "fd657910  (listfile)",
      "d38437cb  (attributes)"}},
    {"name-hash2",
     {"c99b9ee2  bytes", "70ab788e  cats.txt", "48345fbb  dogs.txt", "4e9b98a7  (listfile)",
      "07dfeaec  (attributes)"}},
    {"name-hash3",
     {"0000000000000000  bytes", "0000000000000000  cats.txt", "0000000000000000  dogs.txt",
      "0000000000000000  (listfile)", "0000000000000000  (attributes)"}},
    {"locale",
     {"enUS  bytes", "enUS  cats.txt", "enUS  dogs.txt", "enUS  (listfile)", "enUS  (attributes)"}},
    {"file-index",
     {"    0  bytes", "    1  cats.txt", "    2  dogs.txt", "    3  (listfile)",
      "    4  (attributes)"}},
    {"byte-offset",
     {"      20  bytes", "      30  cats.txt", "      53  dogs.txt", "      76  (listfile)",
      "      99  (attributes)"}},
    {"file-time",
     {"2025-07-29 14:31:00  bytes", "2025-07-29 14:31:00  cats.txt",
      "2025-07-29 14:31:00  dogs.txt", "                     (listfile)",
      "                     (attributes)"}},
    {"file-size",
     {"       8  bytes", "      27  cats.txt", "      27  dogs.txt", "      27  (listfile)",
      "     149  (attributes)"}},
    {"compressed-size",
     {"      16  bytes", "      35  cats.txt", "      35  dogs.txt", "      35  (listfile)",
      "     124  (attributes)"}},
    {"flags",
     {"     cex  bytes", "     cex  cats.txt", "     cex  dogs.txt", "    ce2x  (listfile)",
      "    ce2x  (attributes)"}},
    {"encryption-key",
     {"eaa753f9  bytes", "935a7772  cats.txt", "a073c614  dogs.txt", "2d2f0b11  (listfile)",
      "50e315dd  (attributes)"}},
    {"encryption-key-raw",
     {"eaa753f9  bytes", "935a7772  cats.txt", "a073c614  dogs.txt", "2d2f0a94  (listfile)",
      "50e314af  (attributes)"}},
};

// The two Blizzard patches make fetch_integration_data downloads, with the
// entries each lists. A strong signature is appended after the archive rather
// than stored as an entry, so the strong-signed patch has no (signature).
struct PatchListing {
    std::string name;
    std::size_t user_entries;
    std::set<std::string> special_entries;
};

const std::vector<PatchListing> patch_listings = {
    {"v1_weak_signature_patch.mpq", 62, {"(listfile)", "(attributes)", "(signature)"}},
    {"v2_strong_signature_patch.mpq", 68, {"(listfile)", "(attributes)"}},
};

} // namespace

TEST_CASE("list: prints its help with --help", "[list]") {
    auto result = Run(mpqcli_binary, {"list", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("list [OPTIONS] archive"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--detailed"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--listfile"));
}

TEST_CASE("list: archive does not exist", "[list]") {
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary, {"list", "no-such-archive.mpq"}, options);

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("archive: File does not exist: no-such-archive.mpq"));
}

TEST_CASE("list: listfile does not exist", "[list]") {
    SampleArchive sample;
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary,
                      {"list", sample.archive.string(), "--listfile", "no-such-list.txt"}, options);

    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("--listfile: File does not exist: no-such-list.txt"));
}

TEST_CASE("list: fails when the archive is not an MPQ", "[list]") {
    SampleFiles files;

    auto result = Run(mpqcli_binary, {"list", (files.files_dir / "cats.txt").string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output).size() == 1);
    REQUIRE_THAT(result.stderr_output, ContainsSubstring(open_failure));
}

TEST_CASE("list: rejects an unknown property", "[list]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"list", "-q", "not-real", sample.archive.string()});

    // The CLI::IsMember check rejects the name before the archive is opened
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("--property: not-real not in {"));
}

TEST_CASE("list: rejects -p, which is not one of its options", "[list]") {
    SampleArchive sample;
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary, {"list", "-p", "locale", sample.archive.string()}, options);

    // The unknown -p is set aside and the property name fills the archive
    // positional, whose CLI::ExistingFile check fails first: 105 is CLI11's
    // ValidationError
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("archive: File does not exist: locale"));
}

TEST_CASE("list: rejects extra arguments", "[list]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"list", sample.archive.string(), "extra"});

    // 109 is CLI11's ExtrasError: a second archive is not silently ignored
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: extra"));
}

TEST_CASE("list: prints the entry names alone by default", "[list]") {
    SampleArchive sample(AttributedArchiveArgs());

    auto result = Run(mpqcli_binary, {"list", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    // The special entries stay hidden, as ls hides dotfiles
    RequireLines(result.stdout_output, user_entries);
}

TEST_CASE("list: includes the special entries with --all", "[list]") {
    const auto flag = GENERATE(as<std::string>{}, "-a", "--all");
    CAPTURE(flag);
    // Signed as well, so every special entry an archive can carry is present
    std::vector<std::string> args = AttributedArchiveArgs();
    args.push_back("-s");
    SampleArchive sample(args);

    auto result = Run(mpqcli_binary, {"list", flag, sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    RequireLines(result.stdout_output,
                 {"bytes", "cats.txt", "dogs.txt", "(listfile)", "(attributes)", "(signature)"});
}

TEST_CASE("list: prints size, locale and time with --detailed", "[list]") {
    const auto flag = GENERATE(as<std::string>{}, "-d", "--detailed");
    CAPTURE(flag);
    SampleArchive sample(AttributedArchiveArgs());

    auto result = Run(mpqcli_binary, {"list", "-a", flag, sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    RequireLines(result.stdout_output, detailed_entries);
}

TEST_CASE("list: shows the signature entry stored raw", "[list]") {
    std::vector<std::string> args = AttributedArchiveArgs();
    args.push_back("-s");
    SampleArchive sample(args);

    auto result = Run(mpqcli_binary, {"list", "-a", "-d", "-q", "flags", "-q", "compressed-size",
                                      sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    // The (signature) is neither compressed nor encrypted, and signing adds a
    // row to the (attributes) block, which is why it is larger than unsigned
    RequireLines(result.stdout_output, {
                                           "     cex       16  bytes",
                                           "     cex       35  cats.txt",
                                           "     cex       35  dogs.txt",
                                           "    ce2x       35  (listfile)",
                                           "    ce2x      143  (attributes)",
                                           "       x       72  (signature)",
                                       });
}

TEST_CASE("list: prints each property with --property", "[list]") {
    const auto listing = GENERATE(from_range(property_listings));
    CAPTURE(listing.property);
    SampleArchive sample(AttributedArchiveArgs());

    auto result =
        Run(mpqcli_binary, {"list", "-a", "-d", sample.archive.string(), "-q", listing.property});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    RequireLines(result.stdout_output, listing.lines);
}

TEST_CASE("list: prints several properties as columns", "[list]") {
    SampleArchive sample(AttributedArchiveArgs());

    auto result =
        Run(mpqcli_binary, {"list", "-a", "-d", sample.archive.string(), "-q", "hash-index", "-q",
                            "name-hash1", "-q", "name-hash2", "-q", "name-hash3", "-q",
                            "compressed-size", "-q", "flags", "-q", "encryption-key-raw"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    RequireLines(
        result.stdout_output,
        {
            "   35 147178ed c99b9ee2 0000000000000000       16      cex eaa753f9  bytes",
            "   44 0fd58937 70ab788e 0000000000000000       35      cex 935a7772  cats.txt",
            "    0 eb30456b 48345fbb 0000000000000000       35      cex a073c614  dogs.txt",
            "   25 fd657910 4e9b98a7 0000000000000000       35     ce2x 2d2f0a94  (listfile)",
            "   14 d38437cb 07dfeaec 0000000000000000      124     ce2x 50e314af  (attributes)",
        });
}

TEST_CASE("list: prints the properties in the order given", "[list]") {
    SampleArchive sample;

    auto locale_first =
        Run(mpqcli_binary, {"list", "-q", "locale", "-q", "file-size", sample.archive.string()});
    auto size_first =
        Run(mpqcli_binary, {"list", "-q", "file-size", "-q", "locale", sample.archive.string()});

    REQUIRE(locale_first.returncode == 0);
    REQUIRE(locale_first.stderr_output.empty());
    RequireLines(locale_first.stdout_output,
                 {"enUS        8  bytes", "enUS       27  cats.txt", "enUS       27  dogs.txt"});
    REQUIRE(size_first.returncode == 0);
    REQUIRE(size_first.stderr_output.empty());
    RequireLines(size_first.stdout_output,
                 {"       8 enUS  bytes", "      27 enUS  cats.txt", "      27 enUS  dogs.txt"});
}

TEST_CASE("list: --property alone implies --detailed", "[list]") {
    const auto flag = GENERATE(as<std::string>{}, "-q", "--property");
    CAPTURE(flag);
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"list", flag, "locale", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    // A column appears without -d, while the special entries stay hidden without -a
    RequireLines(result.stdout_output, {"enUS  bytes", "enUS  cats.txt", "enUS  dogs.txt"});
}

TEST_CASE("list: shows a file stored under several locales once per locale", "[list]") {
    ManyLocalesArchive archive;

    SECTION("the name repeats without --detailed") {
        auto result = Run(mpqcli_binary, {"list", archive.archive.string()});

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stderr_output.empty());
        REQUIRE(LinesToVector(result.stdout_output) == std::vector<std::string>(4, "cats.txt"));
    }

    SECTION("each locale gets a row with --detailed") {
        auto result = Run(mpqcli_binary, {"list", "-d", archive.archive.string()});

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stderr_output.empty());
        // One row per variant, each with its own content's size; the archive was
        // created without an (attributes) block, so there is no time column
        RequireLines(result.stdout_output, {
                                               "      26 enUS                      cats.txt",
                                               "      27 041D                      cats.txt",
                                               "      33 deDE                      cats.txt",
                                               "      31 esES                      cats.txt",
                                           });
    }

    SECTION("the locale column names an LCID the locale table lacks in hexadecimal") {
        auto result = Run(mpqcli_binary,
                          {"list", "-q", "locale", "-q", "file-size", archive.archive.string()});

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stderr_output.empty());
        RequireLines(result.stdout_output, {"enUS       26  cats.txt", "041D       27  cats.txt",
                                            "deDE       33  cats.txt", "esES       31  cats.txt"});
    }
}

TEST_CASE("list: prints nested entries with the archive's backslash separator", "[list]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "root.txt", "root file");
    WriteFile(source_dir / "sub" / "nested.txt", "nested file");
    const auto archive = dir.Path() / "nested.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"list", "-d", archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    RequireLines(result.stdout_output, {"       9 enUS                      root.txt",
                                        "      11 enUS                      sub\\nested.txt"});
}

TEST_CASE("list: prints nothing for an archive with no user files", "[list]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "empty";
    fs::create_directories(source_dir);
    const auto archive = dir.Path() / "empty.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"list", archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(result.stderr_output.empty());
    // Only the empty (listfile) is there to show
    auto all = Run(mpqcli_binary, {"list", "-a", "-d", archive.string()});
    REQUIRE(all.returncode == 0);
    REQUIRE(all.stderr_output.empty());
    RequireLines(all.stdout_output, {"       0 enUS                      (listfile)"});
}

TEST_CASE("list: names entries by position without a listfile", "[list]") {
    NoListfileArchive archive;
    const std::set<std::string> positions = {"File00000000.xxx", "File00000001.xxx",
                                             "File00000002.xxx"};

    // With no names to know, --all changes nothing
    for (const auto &args : std::vector<std::vector<std::string>>{{}, {"-a"}}) {
        CAPTURE(args);
        std::vector<std::string> full = {"list"};
        full.insert(full.end(), args.begin(), args.end());
        full.push_back(archive.archive.string());

        auto result = Run(mpqcli_binary, full);

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stderr_output.empty());
        RequireLines(result.stdout_output, positions);
    }

    // The size and locale of each entry are still known, in insertion order
    auto detailed = Run(mpqcli_binary, {"list", "-ad", archive.archive.string()});

    REQUIRE(detailed.returncode == 0);
    REQUIRE(detailed.stderr_output.empty());
    RequireLines(detailed.stdout_output, {
                                             "      31 enUS                      File00000000.xxx",
                                             "      33 deDE                      File00000001.xxx",
                                             "      27 041D                      File00000002.xxx",
                                         });
}

TEST_CASE("list: names the entries a partial external listfile knows", "[list]") {
    const auto flag = GENERATE(as<std::string>{}, "-l", "--listfile");
    CAPTURE(flag);
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, "cats.txt");

    auto result = Run(mpqcli_binary, {"list", archive.archive.string(), flag, listfile.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    // The named entry gets its name back; the rest keep their positions
    RequireLines(result.stdout_output, {"File00000000.xxx", "cats.txt", "File00000002.xxx"});
    auto detailed =
        Run(mpqcli_binary, {"list", "-ad", archive.archive.string(), flag, listfile.string()});
    REQUIRE(detailed.returncode == 0);
    REQUIRE(detailed.stderr_output.empty());
    RequireLines(detailed.stdout_output, {
                                             "      31 enUS                      File00000000.xxx",
                                             "      33 deDE                      cats.txt",
                                             "      27 041D                      File00000002.xxx",
                                         });
}

TEST_CASE("list: names every entry with a complete external listfile", "[list]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, "cats.txt\ndogs.txt\ncapybaras.txt");

    auto result = Run(mpqcli_binary, {"list", archive.archive.string(), "-l", listfile.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    RequireLines(result.stdout_output, {"capybaras.txt", "cats.txt", "dogs.txt"});
    auto detailed =
        Run(mpqcli_binary, {"list", "-ad", archive.archive.string(), "-l", listfile.string()});
    REQUIRE(detailed.returncode == 0);
    REQUIRE(detailed.stderr_output.empty());
    RequireLines(detailed.stdout_output, {
                                             "      31 enUS                      capybaras.txt",
                                             "      33 deDE                      cats.txt",
                                             "      27 041D                      dogs.txt",
                                         });
}

TEST_CASE("list: ignores listfile names the archive lacks", "[list]") {
    SampleArchive sample;
    const auto listfile = sample.files.dir.Path() / "listfile.txt";
    WriteFile(listfile, "ghost.txt\ncats.txt\n");

    auto result = Run(mpqcli_binary, {"list", sample.archive.string(), "-l", listfile.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    // A name with no entry behind it is not invented
    RequireLines(result.stdout_output, user_entries);
}

TEST_CASE("list: prints the encryption keys of entries listed without a name", "[list]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "listfile.txt";
    WriteFile(listfile, "capybaras.txt\ncats.txt\ndogs.txt\n");
    const std::vector<std::string> columns = {"-q", "file-index",     "-q", "flags",
                                              "-q", "encryption-key", "-q", "encryption-key-raw"};
    std::vector<std::string> unnamed_args = {"list", archive.archive.string()};
    unnamed_args.insert(unnamed_args.end(), columns.begin(), columns.end());
    std::vector<std::string> named_args = {"list", archive.archive.string(), "-l",
                                           listfile.string()};
    named_args.insert(named_args.end(), columns.begin(), columns.end());

    auto unnamed = Run(mpqcli_binary, unnamed_args);
    auto named = Run(mpqcli_binary, named_args);

    // Every entry is encrypted with a key derived from its name. An unnamed
    // entry's key is recovered from its data, so both listings agree.
    REQUIRE(unnamed.returncode == 0);
    REQUIRE(unnamed.stderr_output.empty());
    RequireLines(unnamed.stdout_output, {
                                            "    0      cex 40526ca5 40526ca5  File00000000.xxx",
                                            "    1      cex 935a7772 935a7772  File00000001.xxx",
                                            "    2      cex a073c614 a073c614  File00000002.xxx",
                                        });
    REQUIRE(named.returncode == 0);
    REQUIRE(named.stderr_output.empty());
    RequireLines(named.stdout_output, {
                                          "    0      cex 40526ca5 40526ca5  capybaras.txt",
                                          "    1      cex 935a7772 935a7772  cats.txt",
                                          "    2      cex a073c614 a073c614  dogs.txt",
                                      });
}

TEST_CASE("list: ignores an empty listfile, with a warning", "[list]") {
    NoListfileArchive archive;
    const auto listfile = archive.dir.Path() / "empty.txt";
    WriteFile(listfile, "");
    auto without_listfile = Run(mpqcli_binary, {"list", archive.archive.string()});
    REQUIRE(without_listfile.returncode == 0);

    auto result = Run(mpqcli_binary, {"list", archive.archive.string(), "-l", listfile.string()});

    // An empty listfile names nothing, so the listing is the one without it
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output == without_listfile.stdout_output);
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        std::set<std::string>{"[!] Ignoring the listfile, which is empty: " + listfile.string()});
}

TEST_CASE("list: an archive with no entries lists nothing", "[list]") {
    const auto detailed = GENERATE(false, true);
    CAPTURE(detailed);
    TempDir dir;
    const auto source_dir = dir.Path() / "empty";
    fs::create_directories(source_dir);
    const auto archive = dir.Path() / "bare.mpq";
    // No files and no (listfile) either, so the hash table is empty
    REQUIRE(
        Run(mpqcli_binary, {"create", archive.string(), source_dir.string(), "--file-flags1", "0"})
            .returncode == 0);
    std::vector<std::string> args = {"list", "-a", archive.string()};
    if (detailed) {
        args.push_back("-d");
    }

    auto result = Run(mpqcli_binary, args);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(result.stderr_output.empty());
}

TEST_CASE("list: counts a Blizzard patch's entries and special files", "[list]") {
    const auto patch = GENERATE(from_range(patch_listings));
    CAPTURE(patch.name);
    const auto archive = IntegrationArchive(patch.name);

    auto result = Run(mpqcli_binary, {"list", archive.string()});
    auto all = Run(mpqcli_binary, {"list", "-a", archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    const auto names = LinesToVector(result.stdout_output);
    REQUIRE(names.size() == patch.user_entries);
    REQUIRE(all.returncode == 0);
    REQUIRE(all.stderr_output.empty());
    const auto all_names = LinesToVector(all.stdout_output);
    REQUIRE(all_names.size() == patch.user_entries + patch.special_entries.size());
    std::set<std::string> specials;
    for (const auto &name : all_names) {
        if (!name.empty() && name.front() == '(') {
            specials.insert(name);
        }
    }
    REQUIRE(specials == patch.special_entries);
}

TEST_CASE("list: prints the documented sizes for the weak-signed patch", "[list]") {
    const auto archive = IntegrationArchive("v1_weak_signature_patch.mpq");

    // Size and locale only: the docs page's detailed example carries file times
    // in the zone it was written in, which no other machine reproduces
    auto result = Run(mpqcli_binary, {"list", "-q", "file-size", "-q", "locale", archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    const auto lines = LinesToSet(result.stdout_output);
    REQUIRE(lines.size() == 62);
    for (const auto &line :
         {"   88604 enUS  BM_COKETENT01.BLP", "     243 enUS  Blizzard_CraftUI.xml",
          "     184 enUS  Blizzard_CraftUI.lua", "      68 enUS  realmlist.wtf"}) {
        CAPTURE(line);
        REQUIRE(lines.count(line) == 1);
    }
}
