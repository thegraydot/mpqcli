#include <algorithm>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "many_locales_archive.h"
#include "no_listfile_archive.h"
#include "one_locale_archive.h"
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

const std::string failed_all = "[!] Failed to extract all files.";

// The names directly inside a directory, as the pytest suite compared them
std::set<std::string> DirectoryEntries(const fs::path &dir) {
    std::set<std::string> names;
    for (const auto &entry : fs::directory_iterator(dir)) {
        names.insert(entry.path().filename().string());
    }
    return names;
}

// The stderr lines extract prints for a set of entry names
std::set<std::string> ExtractedLines(const std::set<std::string> &names) {
    std::set<std::string> lines;
    for (const auto &name : names) {
        lines.insert("[*] Extracted: " + name);
    }
    return lines;
}

// The lines of output that start with prefix, for output that also carries
// lines the test does not pin
std::set<std::string> LinesStartingWith(const std::string &output, const std::string &prefix) {
    std::set<std::string> lines;
    for (const auto &line : LinesToSet(output)) {
        if (line.rfind(prefix, 0) == 0) {
            lines.insert(line);
        }
    }
    return lines;
}

// A nested entry's path as extract prints it: the archive's backslash turned
// into the platform's own separator
std::string Native(const std::string &archive_path) {
    std::string native = archive_path;
    for (auto &c : native) {
        if (c == '\\') {
            c = fs::path::preferred_separator;
        }
    }
    return native;
}

// Stores a second copy of cats.txt in the sample archive as texts\cats.txt
void AddNestedCats(const SampleArchive &sample) {
    REQUIRE(Run(mpqcli_binary,
                {"add", sample.archive.string(), (sample.files.files_dir / "cats.txt").string(),
                 "--path", "texts\\cats.txt"})
                .returncode == 0);
}

// An archive whose only entries are safe.txt and a copy of it stored under a
// path that climbs two directories above wherever it is extracted
fs::path TraversalArchive(const TempDir &dir) {
    const auto source_dir = dir.Path() / "traversal_files";
    const auto safe = source_dir / "safe.txt";
    WriteFile(safe, "This is a safe file.\n");
    const auto archive = dir.Path() / "mpq_with_path_traversal.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);
    REQUIRE(Run(mpqcli_binary, {"add", archive.string(), safe.string(), "-p", "../../sneaky.txt"})
                .returncode == 0);
    return archive;
}

} // namespace

TEST_CASE("extract: prints its help with --help", "[extract]") {
    auto result = Run(mpqcli_binary, {"extract", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("extract [OPTIONS] archive [files...]"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--flat"));
}

TEST_CASE("extract: archive does not exist", "[extract]") {
    auto result = Run(mpqcli_binary, {"extract", "no-such-archive.mpq"});

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("File does not exist: no-such-archive.mpq"));
}

TEST_CASE("extract: listfile does not exist", "[extract]") {
    SampleArchive sample;

    auto result =
        Run(mpqcli_binary, {"extract", sample.archive.string(), "--listfile", "no-such-list.txt"});

    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("--listfile: File does not exist: no-such-list.txt"));
}

TEST_CASE("extract: fails when the archive is not an MPQ", "[extract]") {
    SampleFiles files;

    auto result = Run(mpqcli_binary, {"extract", (files.files_dir / "cats.txt").string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to open MPQ archive"));
}

TEST_CASE("extract: all files to a directory named after the archive", "[extract]") {
    SampleArchive sample(AttributedArchiveArgs(), "archive.mpq");
    const auto output_dir = sample.files.dir.Path() / "archive";
    const std::set<std::string> expected = {"cats.txt", "dogs.txt", "bytes", "(listfile)",
                                            "(attributes)"};

    auto result = Run(mpqcli_binary, {"extract", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ExtractedLines(expected));
    REQUIRE(fs::is_directory(output_dir));
    REQUIRE(DirectoryEntries(output_dir) == expected);
    REQUIRE(ReadFile(output_dir / "cats.txt") == "This is a file about cats.\n");
    REQUIRE(ReadFile(output_dir / "dogs.txt") == "This is a file about dogs.\n");
    REQUIRE(fs::file_size(output_dir / "cats.txt") == 27);
    REQUIRE(fs::file_size(output_dir / "bytes") == 8);
}

TEST_CASE("extract: all files to --output", "[extract]") {
    const auto flag = GENERATE(as<std::string>{}, "-o", "--output");
    CAPTURE(flag);
    SampleArchive sample(AttributedArchiveArgs());
    const auto output_dir = sample.files.dir.Path() / "extracted";
    const std::set<std::string> expected = {"cats.txt", "dogs.txt", "bytes", "(listfile)",
                                            "(attributes)"};

    auto result =
        Run(mpqcli_binary, {"extract", flag, output_dir.string(), sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ExtractedLines(expected));
    REQUIRE(fs::is_directory(output_dir));
    REQUIRE(DirectoryEntries(output_dir) == expected);
    REQUIRE(ReadFile(output_dir / "cats.txt") == "This is a file about cats.\n");
    REQUIRE(ReadFile(output_dir / "dogs.txt") == "This is a file about dogs.\n");
}

TEST_CASE("extract: --output with a trailing separator", "[extract]") {
    SampleArchive sample;
    const auto output_dir = sample.files.dir.Path() / "extracted";
    const std::set<std::string> expected = {"cats.txt", "dogs.txt", "bytes", "(listfile)"};

    auto result =
        Run(mpqcli_binary, {"extract", "-o", output_dir.string() + "/", sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ExtractedLines(expected));
    REQUIRE(DirectoryEntries(output_dir) == expected);
}

TEST_CASE("extract: fails when --output is an existing file", "[extract]") {
    SampleArchive sample;
    const auto not_a_dir = sample.files.files_dir / "cats.txt";

    auto result =
        Run(mpqcli_binary, {"extract", "-o", not_a_dir.string(), sample.archive.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to create output directory"));
}

TEST_CASE("extract: fails when the archive has no extension", "[extract]") {
    // The default output directory is the archive's stem, which is then the
    // archive itself
    SampleArchive sample({}, "archive");

    auto result = Run(mpqcli_binary, {"extract", sample.archive.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to create output directory"));
}

TEST_CASE("extract: overwrites files from a previous extraction", "[extract]") {
    SampleArchive sample;
    const auto output_dir = sample.files.dir.Path() / "extracted";
    const std::vector<std::string> args = {"extract", "-o", output_dir.string(),
                                           sample.archive.string()};
    REQUIRE(Run(mpqcli_binary, args).returncode == 0);
    WriteFile(output_dir / "cats.txt", "stale");

    auto result = Run(mpqcli_binary, args);

    // No complaint about the files already there: the second run reports each
    // one as the first did
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            ExtractedLines({"cats.txt", "dogs.txt", "bytes", "(listfile)"}));
    REQUIRE(ReadFile(output_dir / "cats.txt") == "This is a file about cats.\n");
}

TEST_CASE("extract: a single named file", "[extract]") {
    SampleArchive sample(AttributedArchiveArgs());
    const auto output_dir = sample.files.dir.Path() / "extracted_file";

    auto result = Run(mpqcli_binary,
                      {"extract", "-o", output_dir.string(), sample.archive.string(), "cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[*] Extracted: cats.txt"});
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"cats.txt"});
    REQUIRE(ReadFile(output_dir / "cats.txt") == "This is a file about cats.\n");
    REQUIRE(fs::file_size(output_dir / "cats.txt") == 27);
}

TEST_CASE("extract: several named files", "[extract]") {
    SampleArchive sample;
    const auto output_dir = sample.files.dir.Path() / "extracted";

    auto result = Run(mpqcli_binary, {"extract", "-o", output_dir.string(), sample.archive.string(),
                                      "cats.txt", "dogs.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ExtractedLines({"cats.txt", "dogs.txt"}));
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"cats.txt", "dogs.txt"});
    REQUIRE(ReadFile(output_dir / "dogs.txt") == "This is a file about dogs.\n");
}

TEST_CASE("extract: reads the file names from stdin with -", "[extract]") {
    // A list saved with Windows line endings names the same files
    const auto newline = GENERATE(as<std::string>{}, "\n", "\r\n");
    CAPTURE(newline.size());
    SampleArchive sample;
    const auto output_dir = sample.files.dir.Path() / "extracted";
    RunOptions options;
    options.stdin_input = "cats.txt" + newline + "dogs.txt" + newline;

    auto result =
        Run(mpqcli_binary, {"extract", "-o", output_dir.string(), sample.archive.string(), "-"},
            options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ExtractedLines({"cats.txt", "dogs.txt"}));
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"cats.txt", "dogs.txt"});
}

TEST_CASE("extract: an empty stdin extracts nothing", "[extract]") {
    SampleArchive sample;
    const auto output_dir = sample.files.dir.Path() / "extracted";
    RunOptions options;
    options.stdin_input = "";

    auto result =
        Run(mpqcli_binary, {"extract", "-o", output_dir.string(), sample.archive.string(), "-"},
            options);

    // Nothing rather than everything, so a pipeline whose filter matched no
    // names cannot unpack the whole archive
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[*] No files to extract"});
    REQUIRE_FALSE(fs::exists(output_dir));
}

TEST_CASE("extract: a named file that does not exist in the archive", "[extract]") {
    SampleArchive sample;
    const auto output_dir = sample.files.dir.Path() / "extracted_file";

    auto result = Run(mpqcli_binary,
                      {"extract", "-o", output_dir.string(), sample.archive.string(), "nope.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Failed: File doesn't exist for locale enUS: nope.txt", "",
                                  failed_all});
    REQUIRE(DirectoryEntries(output_dir).empty());
}

TEST_CASE("extract: a named nested file keeps its folder", "[extract]") {
    SampleArchive sample;
    AddNestedCats(sample);
    const auto output_dir = sample.files.dir.Path() / "extracted_nested";

    auto result = Run(mpqcli_binary, {"extract", "-o", output_dir.string(), sample.archive.string(),
                                      "texts\\cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Extracted: " + Native("texts\\cats.txt")});
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"texts"});
    REQUIRE(ReadFile(output_dir / "texts" / "cats.txt") == "This is a file about cats.\n");
}

TEST_CASE("extract: a named nested file drops its folder with --flat", "[extract]") {
    const auto flag = GENERATE(as<std::string>{}, "-j", "--flat");
    CAPTURE(flag);
    SampleArchive sample;
    AddNestedCats(sample);
    const auto output_dir = sample.files.dir.Path() / "extracted_nested";

    auto result = Run(mpqcli_binary, {"extract", flag, "-o", output_dir.string(),
                                      sample.archive.string(), "texts\\cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[*] Extracted: cats.txt"});
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"cats.txt"});
    REQUIRE(ReadFile(output_dir / "cats.txt") == "This is a file about cats.\n");
}

TEST_CASE("extract: all files keep their folder structure", "[extract]") {
    SampleArchive sample;
    AddNestedCats(sample);
    const auto output_dir = sample.files.dir.Path() / "extracted";

    auto result =
        Run(mpqcli_binary, {"extract", "-o", output_dir.string(), sample.archive.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(
        LinesToSet(result.stderr_output) ==
        ExtractedLines({"cats.txt", "dogs.txt", "bytes", "(listfile)", Native("texts\\cats.txt")}));
    REQUIRE(DirectoryEntries(output_dir) ==
            std::set<std::string>{"cats.txt", "dogs.txt", "bytes", "(listfile)", "texts"});
    REQUIRE(ReadFile(output_dir / "texts" / "cats.txt") == "This is a file about cats.\n");
}

TEST_CASE("extract: --flat extracts nothing when two names land on the same file", "[extract]") {
    TempDir dir;
    const auto input_dir = dir.Path() / "files";
    WriteFile(input_dir / "a" / "x.txt", "A");
    WriteFile(input_dir / "b" / "x.txt", "B");
    const auto archive = dir.Path() / "collision.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), input_dir.string()}).returncode == 0);
    const auto output_dir = dir.Path() / "extracted";

    auto result =
        Run(mpqcli_binary, {"extract", "--flat", "-o", output_dir.string(), archive.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    // Which of the two the message names first follows the archive's own order
    const std::set<std::string> either_order = {
        "[!] --flat would extract a\\x.txt and b\\x.txt to the same file",
        "[!] --flat would extract b\\x.txt and a\\x.txt to the same file",
    };
    const auto stderr_lines = LinesToVector(result.stderr_output);
    REQUIRE(stderr_lines.size() == 1);
    REQUIRE(either_order.count(stderr_lines[0]) == 1);
    REQUIRE_FALSE(fs::exists(output_dir));
}

TEST_CASE("extract: --flat extracts one name stored under two locales", "[extract]") {
    TempDir dir;
    const auto input_dir = dir.Path() / "files";
    WriteFile(input_dir / "cats.txt", "Cats in the default locale.");
    const auto german_file = dir.Path() / "cats.txt";
    WriteFile(german_file, "Cats in German.");
    const auto archive = dir.Path() / "locales.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), input_dir.string()}).returncode == 0);
    REQUIRE(Run(mpqcli_binary, {"add", archive.string(), german_file.string(), "--locale", "deDE"})
                .returncode == 0);
    const auto output_dir = dir.Path() / "extracted";

    auto result =
        Run(mpqcli_binary, {"extract", "--flat", "-o", output_dir.string(), archive.string()});

    // cats.txt is listed once per locale, and both land on the same file without
    // counting as a collision
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    const auto stderr_lines = LinesToVector(result.stderr_output);
    REQUIRE(stderr_lines.size() == 3);
    REQUIRE(std::count(stderr_lines.begin(), stderr_lines.end(), "[*] Extracted: cats.txt") == 2);
    REQUIRE(LinesToSet(result.stderr_output) == ExtractedLines({"cats.txt", "(listfile)"}));
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"cats.txt", "(listfile)"});
}

TEST_CASE("extract: --listfile cannot be combined with named files", "[extract]") {
    // A name on the command line, or - with the name on stdin
    const auto named = GENERATE(as<std::string>{}, "cats.txt", "-");
    CAPTURE(named);
    SampleArchive sample;
    const auto listfile = sample.files.dir.Path() / "listfile.txt";
    WriteFile(listfile, "cats.txt\n");
    const auto output_dir = sample.files.dir.Path() / "extracted";
    RunOptions options;
    options.stdin_input = "cats.txt\n";

    auto result = Run(mpqcli_binary,
                      {"extract", "-o", output_dir.string(), "-l", listfile.string(),
                       sample.archive.string(), named},
                      options);

    // 108 is CLI11's ExcludesError
    REQUIRE(result.returncode == 108);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("files excludes --listfile"));
    REQUIRE_FALSE(fs::exists(output_dir));
}

TEST_CASE("extract: has no -f option", "[extract]") {
    SampleArchive sample;
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result =
        Run(mpqcli_binary, {"extract", "-f", "cats.txt", sample.archive.string()}, options);

    // The file name after -f is taken as the archive, and the working directory
    // has no file by that name
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("archive: File does not exist: cats.txt"));
    REQUIRE(DirectoryEntries(working_dir.Path()).empty());
}

TEST_CASE("extract: has no -k option", "[extract]") {
    SampleArchive sample;
    const auto output_dir = sample.files.dir.Path() / "extracted";

    auto result =
        Run(mpqcli_binary, {"extract", "-k", "-o", output_dir.string(), sample.archive.string()});

    // 109 is CLI11's ExtrasError
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: -k"));
    REQUIRE_FALSE(fs::exists(output_dir));
}

TEST_CASE("extract: a single file under the given locale", "[extract]") {
    const auto flag = GENERATE(as<std::string>{}, "-e", "--locale");
    CAPTURE(flag);
    ManyLocalesArchive locales;
    const auto output_dir = locales.dir.Path() / "extracted_file";

    auto result = Run(mpqcli_binary, {"extract", "-o", output_dir.string(),
                                      locales.archive.string(), "cats.txt", flag, "esES"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[*] Extracted: cats.txt"});
    REQUIRE(ReadFile(output_dir / "cats.txt") == CatsContent("esES"));
}

TEST_CASE("extract: a single file takes the default locale when none is given", "[extract]") {
    ManyLocalesArchive locales;
    const auto output_dir = locales.dir.Path() / "extracted_file";

    auto result = Run(mpqcli_binary,
                      {"extract", "-o", output_dir.string(), locales.archive.string(), "cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[*] Extracted: cats.txt"});
    REQUIRE(ReadFile(output_dir / "cats.txt") == CatsContent(""));
}

TEST_CASE("extract: rejects an unknown locale", "[extract]") {
    ManyLocalesArchive locales;
    const auto output_dir = locales.dir.Path() / "extracted_file";

    auto result =
        Run(mpqcli_binary, {"extract", "-o", output_dir.string(), locales.archive.string(),
                            "cats.txt", "--locale", "nosuchlocale"});

    // 105 is CLI11's ValidationError, from the locale validator
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("--locale: Locale must be nothing, or one of:"));
    REQUIRE_FALSE(fs::exists(output_dir));
}

TEST_CASE("extract: falls back to the default locale when the file has no such variant",
          "[extract]") {
    ManyLocalesArchive locales;
    const auto output_dir = locales.dir.Path() / "extracted_file";

    auto result = Run(mpqcli_binary, {"extract", "-o", output_dir.string(),
                                      locales.archive.string(), "cats.txt", "--locale", "ptPT"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == std::set<std::string>{"[*] Extracted: cats.txt"});
    REQUIRE(ReadFile(output_dir / "cats.txt") == CatsContent(""));
}

TEST_CASE("extract: fails when neither the given nor the default locale has the file",
          "[extract]") {
    // No --locale asks for enUS; a wrong one asks for that locale. Neither is
    // stored, and there is no default variant to fall back to
    const auto [locale, reported] = GENERATE(table<std::string, std::string>({
        {"", "enUS"},
        {"deDE", "deDE"},
    }));
    CAPTURE(reported);
    OneLocaleArchive one;
    const auto output_dir = one.dir.Path() / "extracted_file";
    std::vector<std::string> args = {"extract", "-o", output_dir.string(), one.archive.string(),
                                     "cats.txt"};
    if (!locale.empty()) {
        args.insert(args.end(), {"--locale", locale});
    }

    auto result = Run(mpqcli_binary, args);

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Failed: File doesn't exist for locale " + reported +
                                      ": cats.txt",
                                  "", failed_all});
    REQUIRE(DirectoryEntries(output_dir).empty());
}

TEST_CASE("extract: all files from an archive without a listfile", "[extract]") {
    // External listfile content (none when empty), the locale asked for (the
    // default when empty), and the names that come out. Entries under another
    // locale fail, so every row exits 1; unnamed entries land as File0000000N.xxx
    const auto [listfile_content, locale, expected] =
        GENERATE(table<std::string, std::string, std::set<std::string>>({
            {"", "", {"File00000000.xxx"}},
            {"", "041D", {"File00000000.xxx", "File00000002.xxx"}},
            {"capybaras.txt\n", "", {"capybaras.txt"}},
            {"cats.txt\n", "deDE", {"File00000000.xxx", "cats.txt"}},
            {"cats.txt\ndogs.txt\ncapybaras.txt\n", "", {"capybaras.txt"}},
            {"cats.txt\ndogs.txt\ncapybaras.txt\n", "041D", {"dogs.txt", "capybaras.txt"}},
        }));
    CAPTURE(listfile_content, locale);
    NoListfileArchive archive;
    const auto output_dir = archive.dir.Path() / "extracted";
    std::vector<std::string> args = {"extract", "-o", output_dir.string()};
    if (!listfile_content.empty()) {
        const auto listfile = archive.dir.Path() / "listfile.txt";
        WriteFile(listfile, listfile_content);
        args.insert(args.end(), {"-l", listfile.string()});
    }
    args.push_back(archive.archive.string());
    if (!locale.empty()) {
        args.insert(args.end(), {"--locale", locale});
    }

    auto result = Run(mpqcli_binary, args);

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesStartingWith(result.stderr_output, "[*] Extracted: ") == ExtractedLines(expected));
    REQUIRE_THAT(result.stderr_output, ContainsSubstring(failed_all));
    REQUIRE(DirectoryEntries(output_dir) == expected);
    // Whatever the name it came out under, the content is the stored one
    const std::map<std::string, std::string> contents = {
        {"File00000000.xxx", "capybaras.txt"},
        {"File00000001.xxx", "cats.txt"},
        {"File00000002.xxx", "dogs.txt"},
    };
    for (const auto &name : expected) {
        const auto stored = contents.count(name) != 0 ? contents.at(name) : name;
        for (const auto &entry : NoListfileArchive::Entries()) {
            if (entry.name == stored) {
                CAPTURE(name);
                REQUIRE(ReadFile(output_dir / name) == entry.content);
            }
        }
    }
}

TEST_CASE("extract: a single unnamed file from an archive without a listfile", "[extract]") {
    NoListfileArchive archive;
    const auto output_dir = archive.dir.Path() / "extracted_file";

    auto result = Run(mpqcli_binary, {"extract", "-o", output_dir.string(),
                                      archive.archive.string(), "File00000000.xxx"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[*] Extracted: File00000000.xxx"});
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"File00000000.xxx"});
    REQUIRE(ReadFile(output_dir / "File00000000.xxx") == NoListfileArchive::Entries()[0].content);
}

TEST_CASE("extract: blocks an entry whose path climbs out of the output directory", "[extract]") {
    TempDir dir;
    const auto archive = TraversalArchive(dir);
    // Two levels down, so the climb stays inside the scratch directory and the
    // escape target can be checked without touching anything shared
    const auto output_dir = dir.Path() / "nested" / "deeper" / "out";
    fs::create_directories(output_dir.parent_path());
    const auto escape_target = dir.Path() / "nested" / "sneaky.txt";

    auto result = Run(mpqcli_binary, {"extract", "-o", output_dir.string(), archive.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    auto expected_stderr = ExtractedLines({"safe.txt", "(listfile)"});
    expected_stderr.insert(
        {"[!] Blocked: path traversal attempt detected: " + Native("..\\..\\sneaky.txt"), "",
         failed_all});
    REQUIRE(LinesToSet(result.stderr_output) == expected_stderr);
    REQUIRE_FALSE(fs::exists(escape_target));
    REQUIRE(DirectoryEntries(output_dir) == std::set<std::string>{"safe.txt", "(listfile)"});
}

#ifndef _WIN32
// Creating a symlink needs privileges on Windows, so this case is POSIX only
TEST_CASE("extract: blocks an entry that escapes through a symlink in the output directory",
          "[extract]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    const auto safe = source_dir / "safe.txt";
    WriteFile(safe, "This is a safe file.\n");
    const auto archive = dir.Path() / "mpq_with_symlink_escape.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);
    REQUIRE(Run(mpqcli_binary, {"add", archive.string(), safe.string(), "-p", "escape\\evil.txt"})
                .returncode == 0);
    // The entry's path is lexically fine; only resolving the symlink shows it
    // leaves the output directory
    const auto outside = dir.Path() / "outside";
    const auto output_dir = dir.Path() / "out";
    fs::create_directories(outside);
    fs::create_directories(output_dir);
    fs::create_directory_symlink(outside, output_dir / "escape");

    auto result = Run(mpqcli_binary, {"extract", "-o", output_dir.string(), archive.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    auto expected_stderr = ExtractedLines({"safe.txt", "(listfile)"});
    expected_stderr.insert(
        {"[!] Blocked: path traversal attempt detected: " + Native("escape\\evil.txt"), "",
         failed_all});
    REQUIRE(LinesToSet(result.stderr_output) == expected_stderr);
    REQUIRE(DirectoryEntries(outside).empty());
}
#endif
