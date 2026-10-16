#include <algorithm>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "compressible_text.h"
#include "integration_data.h"
#include "many_locales_archive.h"
#include "one_locale_archive.h"
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

// The failure read prints when neither the asked-for locale nor the default
// one holds the entry; the locale named is the one asked for
std::string MissingUnderLocale(const std::string &locale, const std::string &name) {
    return "[!] Failed: File doesn't exist for locale " + locale + ": " + name;
}

// A locale request as the arguments that make it, with the locale name the
// failure message then carries: none given means the default, enUS
struct LocaleRequest {
    std::vector<std::string> args;
    std::string named;
};

const std::vector<LocaleRequest> locale_requests = {
    {{}, "enUS"},
    {{"--locale", "deDE"}, "deDE"},
};

// The sample files, whose bytes read must hand back unchanged
const std::vector<std::pair<std::string, std::string>> sample_contents = SampleFiles::Contents();

// The docs page's example: patch.cmd from the weak-signed Blizzard patch, which
// is 522 bytes of CRLF text
const std::vector<std::string> documented_patch_cmd = {
    "* set the product patch name",
    "PatchVersion This patch upgrades World of Warcraft from version 1.10.0.5195 to version "
    "1.10.1.5230.",
    "* make sure that we don't patch version 1.10.1.5230 or greater",
    "FileVersionEqualTo    \"$(InstallPath)\\WoW.exe\"    1.10.0.5195   "
    "0xffff.0xffff.0xffff.0xffff",
    "* ProductVersionLessThan \"$(InstallPath)\\WoW.exe\"    0.0.0.256  0.0.0.0xffff",
    "Language enGB",
    "SetLauncher \"$(InstallPath)\\WoW.exe\"",
    "* SetUninstall $(WinDir)\\WoWUnin.dat",
    "Self \"World of Warcraft\"",
    "WoWPatchIndex 2",
    "PatchSize 1992294400",
};

} // namespace

TEST_CASE("read: prints its help with --help", "[read]") {
    auto result = Run(mpqcli_binary, {"read", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("read [OPTIONS] archive file"));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--locale"));
}

TEST_CASE("read: every subcommand with a locale offers it as -e", "[read]") {
    // A short flag means one thing across the binary
    const auto subcommand =
        GENERATE(as<std::string>{}, "add", "create", "extract", "read", "remove", "rename");
    CAPTURE(subcommand);

    auto result = Run(mpqcli_binary, {subcommand, "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    const std::regex locale_option(R"(^\s+-e,\s+--locale\b)");
    const auto lines = LinesToVector(result.stdout_output);
    REQUIRE(std::any_of(lines.begin(), lines.end(), [&](const std::string &line) {
        return std::regex_search(line, locale_option);
    }));
}

TEST_CASE("read: archive does not exist", "[read]") {
    auto result = Run(mpqcli_binary, {"read", "no-such-archive.mpq", "cats.txt"});

    // 105 is CLI11's ValidationError, from the CLI::ExistingFile check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("File does not exist: no-such-archive.mpq"));
}

TEST_CASE("read: fails when the archive is not an MPQ", "[read]") {
    SampleFiles files;

    auto result = Run(mpqcli_binary, {"read", (files.files_dir / "cats.txt").string(), "cats.txt"});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output).size() == 1);
    REQUIRE_THAT(result.stderr_output, ContainsSubstring(open_failure));
}

TEST_CASE("read: rejects extra arguments", "[read]") {
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"read", sample.archive.string(), "cats.txt", "extra"});

    // 109 is CLI11's ExtrasError
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: extra"));
}

TEST_CASE("read: fails when the archive has no such entry", "[read]") {
    const auto request = GENERATE(from_range(locale_requests));
    CAPTURE(request.args);
    SampleArchive sample;
    std::vector<std::string> args = {"read", sample.archive.string(), "does-not-exist.txt"};
    args.insert(args.end(), request.args.begin(), request.args.end());

    auto result = Run(mpqcli_binary, args);

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    // The message names the locale that was looked for, the default when none was given
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{MissingUnderLocale(request.named, "does-not-exist.txt")});
}

TEST_CASE("read: writes an entry's bytes to stdout unchanged", "[read]") {
    const auto [name, content] = GENERATE(from_range(sample_contents));
    CAPTURE(name);
    SampleArchive sample(AttributedArchiveArgs());

    auto result = Run(mpqcli_binary, {"read", sample.archive.string(), name});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    // Byte for byte: the text keeps its own line ending and gains none, and the
    // binary entry's NUL and control bytes come through untouched
    REQUIRE(result.stdout_output == content);
}

TEST_CASE("read: reads the special entries by name", "[read]") {
    std::vector<std::string> args = AttributedArchiveArgs();
    args.push_back("-s");
    SampleArchive sample(args);

    SECTION("the (listfile) names every user entry") {
        auto result = Run(mpqcli_binary, {"read", sample.archive.string(), "(listfile)"});

        REQUIRE(result.returncode == 0);
        REQUIRE(LinesToSet(result.stdout_output) ==
                std::set<std::string>{"bytes", "cats.txt", "dogs.txt"});
    }

    SECTION("the (attributes) block and the (signature) come back whole") {
        // The sizes list reports for the same archive: 177 bytes of attributes
        // for six entries, and a 64-byte weak signature with its 8-byte header
        auto attributes = Run(mpqcli_binary, {"read", sample.archive.string(), "(attributes)"});
        auto signature = Run(mpqcli_binary, {"read", sample.archive.string(), "(signature)"});

        REQUIRE(attributes.returncode == 0);
        REQUIRE(attributes.stdout_output.size() == 177);
        REQUIRE(signature.returncode == 0);
        REQUIRE(signature.stdout_output.size() == 72);
    }
}

TEST_CASE("read: matches the entry name without regard to case", "[read]") {
    const auto name = GENERATE(as<std::string>{}, "CATS.TXT", "Cats.Txt");
    CAPTURE(name);
    SampleArchive sample;

    auto result = Run(mpqcli_binary, {"read", sample.archive.string(), name});

    // MPQ name hashes are computed over the upper-cased name
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output == "This is a file about cats.\n");
}

TEST_CASE("read: takes a nested entry by its backslash path", "[read]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "sub" / "nested.txt", "nested file");
    const auto archive = dir.Path() / "nested.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"read", archive.string(), "sub\\nested.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output == "nested file");
}

TEST_CASE("read: hands back a file larger than one sector intact", "[read]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    // About 68 KB, well over the 4 KB sector, so the read spans many sectors
    const auto text = CompressibleText(12000);
    WriteFile(source_dir / "text.txt", text);
    const auto archive = dir.Path() / "big.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"read", archive.string(), "text.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.size() == text.size());
    REQUIRE(result.stdout_output == text);
}

TEST_CASE("read: prints nothing for an empty entry", "[read]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "zero.txt", "");
    const auto archive = dir.Path() / "zero.mpq";
    REQUIRE(Run(mpqcli_binary, {"create", archive.string(), source_dir.string()}).returncode == 0);

    auto result = Run(mpqcli_binary, {"read", archive.string(), "zero.txt"});

    // An empty entry is a success with no bytes, not a failure
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(result.stderr_output.empty());
}

TEST_CASE("read: the variant under the given locale", "[read]") {
    // The locale as given, and the variant it selects: a language code, the
    // hexadecimal LCID the locale table does not name, and the hexadecimal
    // LCID of one it does (0407 is deDE)
    const auto [locale, variant] = GENERATE(table<std::string, std::string>({
        {"deDE", "deDE"},
        {"esES", "esES"},
        {"041D", "041D"},
        {"0407", "deDE"},
    }));
    const auto flag = GENERATE(as<std::string>{}, "-e", "--locale");
    CAPTURE(locale, flag);
    ManyLocalesArchive locales;

    auto result = Run(mpqcli_binary, {"read", locales.archive.string(), "cats.txt", flag, locale});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE(result.stdout_output == CatsContent(variant));
}

TEST_CASE("read: takes the default locale when none is given", "[read]") {
    ManyLocalesArchive locales;

    auto result = Run(mpqcli_binary, {"read", locales.archive.string(), "cats.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output == CatsContent(""));
}

TEST_CASE("read: rejects an unknown locale", "[read]") {
    ManyLocalesArchive locales;

    auto result = Run(mpqcli_binary,
                      {"read", locales.archive.string(), "cats.txt", "--locale", "nosuchlocale"});

    // 105 is CLI11's ValidationError, from the locale validator
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("--locale: Locale must be nothing, or one of:"));
}

TEST_CASE("read: falls back to the default locale when the file has no such variant", "[read]") {
    ManyLocalesArchive locales;

    auto result =
        Run(mpqcli_binary, {"read", locales.archive.string(), "cats.txt", "--locale", "ptPT"});

    // Silently: a known locale the entry lacks is not worth a warning
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE(result.stdout_output == CatsContent(""));
}

TEST_CASE("read: fails when neither the given nor the default locale has the file", "[read]") {
    const auto request = GENERATE(from_range(locale_requests));
    CAPTURE(request.args);
    OneLocaleArchive archive;
    std::vector<std::string> args = {"read", archive.archive.string(), "cats.txt"};
    args.insert(args.end(), request.args.begin(), request.args.end());

    auto result = Run(mpqcli_binary, args);

    // The entry exists under esES alone, so there is nothing to fall back to
    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{MissingUnderLocale(request.named, "cats.txt")});
}

TEST_CASE("read: the only variant when its locale is given", "[read]") {
    OneLocaleArchive archive;

    auto result = Run(mpqcli_binary, {"read", archive.archive.string(), "cats.txt", "--locale",
                                      OneLocaleArchive::locale});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output == CatsContent(OneLocaleArchive::locale));
}

TEST_CASE("read: prints the documented patch.cmd from the weak-signed patch", "[read]") {
    const auto archive = IntegrationArchive("v1_weak_signature_patch.mpq");

    auto result = Run(mpqcli_binary, {"read", archive.string(), "patch.cmd"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE(result.stdout_output.size() == 522);
    // The lines the docs page shows; the file's own CRLF endings are dropped by
    // the split on both sides
    REQUIRE(LinesToVector(result.stdout_output) == documented_patch_cmd);
}

TEST_CASE("read: rejects the file name in the archive's place", "[read]") {
    ManyLocalesArchive locales;
    TempDir working_dir;
    RunOptions options;
    options.working_dir = working_dir.Path();

    auto result = Run(mpqcli_binary, {"read", "cats.txt", locales.archive.string()}, options);

    // The file name is taken as the archive, and the working directory has no
    // file by that name
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("archive: File does not exist: cats.txt"));
}
