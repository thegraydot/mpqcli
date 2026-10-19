#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <set>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "archive_helper.h"
#include "compressible_text.h"
#include "read_file.h"
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

// The entry names an archive made from SampleFiles holds
const std::set<std::string> sample_entries = {"bytes", "cats.txt", "dogs.txt"};

// TMPQHeader field offsets, from StormLib. wSectorSize holds an exponent, the
// sector being 512 << value bytes; dwRawChunkSize exists in v4 headers only.
constexpr std::streamoff header_sector_size_offset = 14;
constexpr std::streamoff header_raw_chunk_size_offset = 108;

// Reads a little-endian field of the MPQ header, which starts at byte 0 of an
// archive mpqcli creates
std::uint64_t HeaderField(const fs::path &archive, std::streamoff offset, int width) {
    std::ifstream in(archive, std::ios::binary);
    REQUIRE(in.is_open());
    in.seekg(offset);
    std::uint64_t value = 0;
    for (int i = 0; i < width; ++i) {
        const int byte = in.get();
        REQUIRE(byte != std::char_traits<char>::eof());
        value |= static_cast<std::uint64_t>(byte) << (8 * i);
    }
    return value;
}

// The line create prints on stderr before it adds anything. The archive path
// it names is absolute, so archive must be the path the binary was given,
// resolved against the same working directory.
std::string ProfileLine(const fs::path &archive, const std::string &profile = "default") {
    return "[*] Game profile: " + profile + ", Output file: " + fs::absolute(archive).string();
}

// Everything create prints on stderr when it stores each entry without a locale
std::set<std::string> ProgressLines(const fs::path &archive, const std::set<std::string> &entries,
                                    const std::string &profile = "default") {
    std::set<std::string> lines = {ProfileLine(archive, profile)};
    for (const auto &entry : entries) {
        lines.insert("[+] Adding file: " + entry);
    }
    return lines;
}

// The names directly inside a directory, as the pytest suite compared them
std::set<std::string> DirectoryEntries(const fs::path &dir) {
    std::set<std::string> names;
    for (const auto &entry : fs::directory_iterator(dir)) {
        names.insert(entry.path().filename().string());
    }
    return names;
}

} // namespace

TEST_CASE("create: prints its help with --help", "[create]") {
    auto result = Run(mpqcli_binary, {"create", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("create [OPTIONS] archive files..."));
    REQUIRE_THAT(result.stdout_output, ContainsSubstring("--sector-size"));
}

TEST_CASE("create: fails when its input does not exist", "[create]") {
    TempDir dir;
    const auto archive = dir.Path() / "x.mpq";
    const auto missing = dir.Path() / "missing";

    auto result = Run(mpqcli_binary, {"create", archive.string(), missing.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Path does not exist: " + missing.string()});
    REQUIRE(DirectoryEntries(dir.Path()).empty());
}

TEST_CASE("create: creates nothing when one of its inputs does not exist", "[create]") {
    TempDir dir;
    const auto input = dir.Path() / "a.txt";
    WriteFile(input, "A file.");
    const auto archive = dir.Path() / "missing.mpq";
    const auto typo = dir.Path() / "typo";

    auto result = Run(mpqcli_binary, {"create", archive.string(), input.string(), typo.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Path does not exist: " + typo.string()});
    REQUIRE(DirectoryEntries(dir.Path()) == std::set<std::string>{"a.txt"});
}

#ifndef _WIN32
// A FIFO exists but is neither a file nor a directory, which no other case can
// reach. Windows has no mkfifo, so the case is POSIX only.
TEST_CASE("create: rejects an input that is neither file nor directory", "[create]") {
    TempDir dir;
    const auto fifo = dir.Path() / "pipe";
    REQUIRE(mkfifo(fifo.c_str(), 0600) == 0);
    const auto archive = dir.Path() / "pipe.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), fifo.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] Not a file or directory: " + fifo.string()});
    REQUIRE(DirectoryEntries(dir.Path()) == std::set<std::string>{"pipe"});
}
#endif

TEST_CASE("create: requires at least one input after the archive", "[create]") {
    TempDir dir;
    WriteFile(dir.Path() / "files" / "a.txt", "A file.");
    RunOptions options;
    options.working_dir = dir.Path();

    auto result = Run(mpqcli_binary, {"create", "files"}, options);

    // 106 is CLI11's RequiredError: the directory fills the archive positional,
    // which leaves the files positional empty
    REQUIRE(result.returncode == 106);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("files is required"));
    REQUIRE(DirectoryEntries(dir.Path()) == std::set<std::string>{"files"});
}

TEST_CASE("create: rejects --output as an unknown option", "[create]") {
    const auto flag = GENERATE(as<std::string>{}, "-o", "--output");
    CAPTURE(flag);
    TempDir dir;
    WriteFile(dir.Path() / "files" / "a.txt", "A file.");
    RunOptions options;
    options.working_dir = dir.Path();

    auto result = Run(mpqcli_binary, {"create", flag, "out.mpq", "files"}, options);

    // 109 is CLI11's ExtrasError: create has no such option
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: " + flag));
    REQUIRE(DirectoryEntries(dir.Path()) == std::set<std::string>{"files"});
}

TEST_CASE("create: rejects --version as an unknown option", "[create]") {
    TempDir dir;
    WriteFile(dir.Path() / "files" / "a.txt", "A file.");
    RunOptions options;
    options.working_dir = dir.Path();

    auto result = Run(mpqcli_binary, {"create", "--version", "1", "files"}, options);

    // 109 is CLI11's ExtrasError: the format version is set with --format-version
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("The following argument was not expected: --version"));
    REQUIRE(DirectoryEntries(dir.Path()) == std::set<std::string>{"files"});
}

TEST_CASE("create: fails when the archive's directory does not exist", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "missing" / "dir" / "out.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to create MPQ archive"));
    // The game profile line, then the failure
    REQUIRE(LinesToVector(result.stderr_output).size() == 2);
    REQUIRE(DirectoryEntries(files.dir.Path()) == std::set<std::string>{"files"});
}

TEST_CASE("create: refuses to overwrite an existing file", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "existing.mpq";
    WriteFile(archive, "Not an archive");

    auto result = Run(mpqcli_binary, {"create", archive.string(), files.files_dir.string()});

    // 105 is CLI11's ValidationError: the CLI::NonexistentPath check rejects
    // the archive before the command runs
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("Path already exists: " + archive.string()));
    REQUIRE(ReadFile(archive) == "Not an archive");
}

TEST_CASE("create: from a single file", "[create]") {
    TempDir dir;
    const auto source = dir.Path() / "test.txt";
    WriteFile(source, "This is a test file for MPQ addition.");
    const auto archive = dir.Path() / "test.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), source.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"test.txt"}));
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  test.txt"});
}

TEST_CASE("create: from a single file with --path", "[create]") {
    TempDir dir;
    const auto source = dir.Path() / "test.txt";
    WriteFile(source, "This is a test file for MPQ addition.");
    const auto archive = dir.Path() / "test.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), source.string(), "--path",
                                      "messages\\important.txt"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            ProgressLines(archive, {"messages\\important.txt"}));
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  messages\\important.txt"});
}

TEST_CASE("create: from a directory with --path prefix", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "files2";
    WriteFile(source_dir / "important.txt", "This is a test file for MPQ addition.");
    const auto archive = dir.Path() / "files_test.mpq";

    auto result =
        Run(mpqcli_binary, {"create", archive.string(), source_dir.string(), "--path", "messages"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            ProgressLines(archive, {"messages\\important.txt"}));
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  messages\\important.txt"});
}

TEST_CASE("create: from several inputs", "[create]") {
    TempDir dir;
    const auto input_dir = dir.Path() / "textures";
    WriteFile(input_dir / "bear" / "bear.blp", "Bear.");
    const auto input_file = dir.Path() / "readme.txt";
    WriteFile(input_file, "Read me.");
    const auto archive = dir.Path() / "several.mpq";

    auto result =
        Run(mpqcli_binary, {"create", archive.string(), input_file.string(), input_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            ProgressLines(archive, {"readme.txt", "bear\\bear.blp"}));
    REQUIRE(ArchiveEntries(archive) ==
            std::set<std::string>{"enUS  readme.txt", "enUS  bear\\bear.blp"});
}

TEST_CASE("create: --path is a prefix for several inputs", "[create]") {
    TempDir dir;
    const auto file_a = dir.Path() / "a.txt";
    const auto file_b = dir.Path() / "b.txt";
    WriteFile(file_a, "A.");
    WriteFile(file_b, "B.");
    const auto input_dir = dir.Path() / "textures";
    WriteFile(input_dir / "bear" / "bear.blp", "Bear.");
    const auto archive = dir.Path() / "prefixed.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), file_a.string(), file_b.string(),
                                      input_dir.string(), "--path", "data"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            ProgressLines(archive, {"data\\a.txt", "data\\b.txt", "data\\bear\\bear.blp"}));
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  data\\a.txt",
                                                             "enUS  data\\b.txt",
                                                             "enUS  data\\bear\\bear.blp"});
}

TEST_CASE("create: creates nothing when two inputs hold the same name", "[create]") {
    TempDir dir;
    WriteFile(dir.Path() / "first" / "a.txt", "First.");
    WriteFile(dir.Path() / "second" / "a.txt", "Second.");
    const auto archive = dir.Path() / "duplicate.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), (dir.Path() / "first").string(),
                                      (dir.Path() / "second").string()});

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("[!] 1 file(s) were not added because an earlier input added "
                                   "a file by the same name"));
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("no archive was created"));
    // The game profile line, the first a.txt added, the second skipped, the count of
    // skips, and the summary
    REQUIRE(LinesToVector(result.stderr_output).size() == 5);
    REQUIRE(DirectoryEntries(dir.Path()) == std::set<std::string>{"first", "second"});
}

TEST_CASE("create: reads its input paths from stdin with -", "[create]") {
    TempDir dir;
    const auto file_a = dir.Path() / "a.txt";
    const auto file_b = dir.Path() / "b.txt";
    WriteFile(file_a, "A.");
    WriteFile(file_b, "B.");
    const auto archive = dir.Path() / "stdin.mpq";
    RunOptions options;
    options.stdin_input = file_a.string() + "\n" + file_b.string() + "\n";

    auto result = Run(mpqcli_binary, {"create", archive.string(), "-"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"a.txt", "b.txt"}));
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  a.txt", "enUS  b.txt"});
}

TEST_CASE("create: creates nothing from an empty stdin", "[create]") {
    TempDir dir;
    const auto archive = dir.Path() / "empty.mpq";
    RunOptions options;
    options.stdin_input = "";

    auto result = Run(mpqcli_binary, {"create", archive.string(), "-"}, options);

    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            std::set<std::string>{"[!] No files or directories to add"});
    REQUIRE(DirectoryEntries(dir.Path()).empty());
}

TEST_CASE("create: sizes the archive for paths from stdin", "[create]") {
    // Enough files that the count matters: an archive sized for none of them,
    // and grown as they are added, reports the same max-files as one made from
    // a single file
    TempDir dir;
    const auto input_dir = dir.Path() / "files";
    std::string paths;
    for (int i = 0; i < 30; ++i) {
        const auto path = input_dir / ("file" + std::to_string(i) + ".txt");
        WriteFile(path, "File " + std::to_string(i) + ".");
        paths += path.string() + "\n";
    }
    const auto create_and_read_max_files = [&](const std::string &name, const std::string &input,
                                               const RunOptions &options) {
        const auto archive = dir.Path() / name;
        auto result = Run(mpqcli_binary, {"create", archive.string(), input}, options);
        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        return std::stoul(InfoProperty(archive, "max-files"));
    };
    RunOptions from_stdin_options;
    from_stdin_options.stdin_input = paths;

    const auto from_stdin = create_and_read_max_files("stdin.mpq", "-", from_stdin_options);
    const auto from_directory = create_and_read_max_files("directory.mpq", input_dir.string(), {});
    const auto from_one_file =
        create_and_read_max_files("one.mpq", (input_dir / "file0.txt").string(), {});

    CAPTURE(from_stdin, from_directory, from_one_file);
    REQUIRE(from_stdin == from_directory);
    REQUIRE(from_stdin > from_one_file);
}

#ifndef _WIN32
// Windows has no permission bit that stops the owner reading a file, so the
// case is POSIX only
TEST_CASE("create: creates nothing when a file cannot be read", "[create]") {
    if (geteuid() == 0) {
        SKIP("root reads a file whatever its permissions");
    }
    TempDir dir;
    const auto input_dir = dir.Path() / "mod";
    WriteFile(input_dir / "a.txt", "A file.");
    const auto locked = input_dir / "locked.txt";
    WriteFile(locked, "Locked.");
    const auto other = dir.Path() / "b.txt";
    WriteFile(other, "B file.");
    const auto archive = dir.Path() / "unreadable.mpq";
    fs::permissions(locked, fs::perms::none, fs::perm_options::replace);

    auto result =
        Run(mpqcli_binary, {"create", archive.string(), other.string(), input_dir.string()});

    fs::permissions(locked, fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace);
    REQUIRE(result.returncode == 1);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to add: locked.txt"));
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("no archive was created"));
    // The game profile line, three files added, the one failure, and the summary
    REQUIRE(LinesToVector(result.stderr_output).size() == 6);
    // Neither the archive nor the .partial it was written to is left behind
    REQUIRE(DirectoryEntries(dir.Path()) == std::set<std::string>{"b.txt", "mod"});
}
#endif

TEST_CASE("create: skips the archive inside its own input directory", "[create]") {
    TempDir dir;
    const auto input_dir = dir.Path() / "mod";
    WriteFile(input_dir / "a.txt", "A file.");
    RunOptions options;
    options.working_dir = input_dir;

    auto result = Run(mpqcli_binary, {"create", "out.mpq", "."}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[*] Skipping the archive itself"));
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("[+] Adding file: a.txt"));
    // The game profile line, the skip and the one file added
    REQUIRE(LinesToVector(result.stderr_output).size() == 3);
    REQUIRE(DirectoryEntries(input_dir) == std::set<std::string>{"a.txt", "out.mpq"});
    REQUIRE(ArchiveEntries(input_dir / "out.mpq") == std::set<std::string>{"enUS  a.txt"});
}

TEST_CASE("create: keeps folder structure with a trailing slash on the input", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "root.txt", "root file");
    WriteFile(source_dir / "sub" / "nested.txt", "nested file");
    const auto archive = dir.Path() / "output.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), source_dir.string() + "/"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            ProgressLines(archive, {"root.txt", "sub\\nested.txt"}));
    REQUIRE(ArchiveEntries(archive) ==
            std::set<std::string>{"enUS  root.txt", "enUS  sub\\nested.txt"});
}

TEST_CASE("create: keeps folder structure with a dot-relative input", "[create]") {
    TempDir dir;
    WriteFile(dir.Path() / "src" / "sub" / "nested.txt", "nested file");
    const auto archive = dir.Path() / "output.mpq";
    RunOptions options;
    options.working_dir = dir.Path();

    auto result = Run(mpqcli_binary, {"create", archive.string(), "./src"}, options);

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"sub\\nested.txt"}));
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  sub\\nested.txt"});
}

#ifndef _WIN32
// Creating a symlink needs privileges on Windows, so this case is POSIX only
TEST_CASE("create: skips a dangling symlink", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "real.txt", "real file");
    fs::create_symlink(dir.Path() / "missing.txt", source_dir / "dangling.txt");
    const auto archive = dir.Path() / "output.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), source_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"real.txt"}));
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{"enUS  real.txt"});
}
#endif

TEST_CASE("create: from an empty directory", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "empty";
    fs::create_directories(source_dir);
    const auto archive = dir.Path() / "empty.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), source_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {}));
    REQUIRE(ArchiveEntries(archive).empty());
    REQUIRE(InfoProperty(archive, "file-count") == "1"); // The (listfile) alone
    // The smallest max file count create gives an archive
    REQUIRE(InfoProperty(archive, "max-files") == "64");
}

TEST_CASE("create: skips special files found in the source directory", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "readme.txt", "hello");
    for (const auto &name : {"(listfile)", "(signature)", "(attributes)"}) {
        WriteFile(source_dir / name, "fake content");
    }
    const auto archive = dir.Path() / "output.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), source_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    auto expected_progress = ProgressLines(archive, {"readme.txt"});
    for (const auto &name : {"(listfile)", "(signature)", "(attributes)"}) {
        expected_progress.insert(std::string("[*] Skipping special MPQ file: ") + name);
    }
    REQUIRE(LinesToSet(result.stderr_output) == expected_progress);
    // The default profile writes a (listfile) of its own and nothing else; a
    // planted special file that was ingested would show up beside it
    REQUIRE(ArchiveEntries(archive, true) ==
            std::set<std::string>{"enUS  readme.txt", "enUS  (listfile)"});
    REQUIRE(InfoProperty(archive, "file-count") == "2");
    REQUIRE(InfoProperty(archive, "signature-type") == "None");
}

TEST_CASE("create: rejects an unknown locale", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "new_mpq.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), files.files_dir.string(),
                                      "--locale", "illegal_locale"});

    // 105 is CLI11's ValidationError, raised by the locale validator
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("Locale must be nothing, or one of"));
    REQUIRE_FALSE(fs::exists(archive));
}

TEST_CASE("create: stores files under the given locale", "[create]") {
    const auto flag = GENERATE(as<std::string>{}, "-e", "--locale");
    // A language code, and a locale given as a hexadecimal LCID
    const auto locale = GENERATE(as<std::string>{}, "esES", "041D");
    CAPTURE(flag, locale);
    SampleFiles files;
    const auto archive = files.dir.Path() / "mpq_with_one_locale.mpq";

    auto result =
        Run(mpqcli_binary, {"create", archive.string(), files.files_dir.string(), flag, locale});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    std::set<std::string> expected_progress = {ProfileLine(archive)};
    for (const auto &entry : sample_entries) {
        expected_progress.insert("[+] Adding file for locale " + locale + ": " + entry);
    }
    REQUIRE(LinesToSet(result.stderr_output) == expected_progress);
    REQUIRE(ArchiveEntries(archive) == std::set<std::string>{locale + "  bytes",
                                                             locale + "  cats.txt",
                                                             locale + "  dogs.txt"});
}

TEST_CASE("create: rejects an unknown game profile", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "mpq_with_invalid_profile.mpq";

    auto result = Run(mpqcli_binary, {"create", "-g", "invalid_profile", archive.string(),
                                      files.files_dir.string()});

    // 105 is CLI11's ValidationError, raised by the game profile validator
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("Game profile must be one of"));
    REQUIRE_FALSE(fs::exists(archive));
}

TEST_CASE("create: applies the game profile's version and compression", "[create]") {
    // Profile, the MPQ version it selects, and the exact flags a .txt file gets. Every
    // entry ends in x, for a file that exists.
    const auto [profile, expected_version, expected_flags] =
        GENERATE(table<std::string, std::string, std::string>({
            {"generic", "1", "cex"},     // Compressed, encrypted
            {"diablo1", "1", "iex"},     // Imploded, encrypted
            {"starcraft1", "1", "ce2x"}, // Compressed, encrypted, key v2
            {"warcraft3", "1", "ce2x"},  // Compressed, encrypted, key v2
            {"diablo2", "1", "cx"},      // Compressed; .txt files are not encrypted
            {"wow1", "1", "cx"},         // Compressed
            {"wow2", "2", "crx"},        // Compressed, sector CRC
            {"starcraft2", "2", "cux"},  // Compressed, small files as a single unit
            {"diablo3", "4", "cux"},     // Compressed, small files as a single unit
        }));
    CAPTURE(profile);
    SampleFiles files;
    const auto archive = files.dir.Path() / ("mpq_with_profile_" + profile + ".mpq");

    auto result =
        Run(mpqcli_binary, {"create", "-g", profile, archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries, profile));
    REQUIRE(InfoProperty(archive, "format-version") == expected_version);
    REQUIRE(EntryProperty(archive, "cats.txt", "flags") == expected_flags);
}

TEST_CASE("create: accepts every game profile", "[create]") {
    const auto [profile, expected_version] = GENERATE(table<std::string, std::string>({
        {"generic", "1"},
        {"diablo1", "1"},
        {"lordsofmagic", "1"},
        {"starcraft1", "1"},
        {"warcraft2", "1"},
        {"diablo2", "1"},
        {"warcraft3", "1"},
        {"warcraft3-map", "1"},
        {"wow1", "1"},
        {"wow2", "2"},
        {"wow3", "2"},
        {"wow4", "4"},
        {"wow5", "4"},
        {"starcraft2", "2"},
        {"diablo3", "4"},
    }));
    CAPTURE(profile);
    SampleFiles files;
    const auto archive = files.dir.Path() / ("mpq_all_profiles_" + profile + ".mpq");

    auto result =
        Run(mpqcli_binary, {"create", "-g", profile, archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries, profile));
    REQUIRE(InfoProperty(archive, "format-version") == expected_version);
}

TEST_CASE("create: marks only zero-size files for deletion", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "deletion_marker_test";
    WriteFile(source_dir / "zero_size.txt", "");
    WriteFile(source_dir / "nonzero_size.txt", "Hello");
    REQUIRE(fs::file_size(source_dir / "zero_size.txt") == 0);
    REQUIRE(fs::file_size(source_dir / "nonzero_size.txt") == 5);
    const auto archive = dir.Path() / "test_deletion_marker.mpq";

    // The starcraft2 profile has the deletion-marker rule for size 0
    auto result =
        Run(mpqcli_binary, {"create", "-g", "starcraft2", archive.string(), source_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) ==
            ProgressLines(archive, {"zero_size.txt", "nonzero_size.txt"}, "starcraft2"));
    const auto zero_size_flags = EntryProperty(archive, "zero_size.txt", "flags");
    const auto nonzero_size_flags = EntryProperty(archive, "nonzero_size.txt", "flags");
    CAPTURE(zero_size_flags, nonzero_size_flags);
    REQUIRE(zero_size_flags.find('d') != std::string::npos);
    REQUIRE(nonzero_size_flags.find('d') == std::string::npos);
    REQUIRE(nonzero_size_flags.find('c') != std::string::npos);
}

TEST_CASE("create: writes each MPQ format version", "[create]") {
    const auto version = GENERATE(as<std::string>{}, "1", "2", "3", "4");
    CAPTURE(version);
    SampleFiles files;
    const auto archive = files.dir.Path() / "files.mpq";

    auto result = Run(mpqcli_binary, {"create", "--format-version", version, archive.string(),
                                      files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    REQUIRE(InfoProperty(archive, "format-version") == version);
}

TEST_CASE("create: --format-version overrides the game profile's version", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "override.mpq";

    // wow2 selects version 2 on its own; see the game profile case
    auto result = Run(mpqcli_binary, {"create", "-g", "wow2", "--format-version", "1",
                                      archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries, "wow2"));
    REQUIRE(InfoProperty(archive, "format-version") == "1");
}

TEST_CASE("create: rejects a format version outside 1 to 4", "[create]") {
    const auto version = GENERATE(as<std::string>{}, "0", "5");
    CAPTURE(version);
    SampleFiles files;
    const auto archive = files.dir.Path() / "bad_version.mpq";

    auto result = Run(mpqcli_binary, {"create", "--format-version", version, archive.string(),
                                      files.files_dir.string()});

    // 105 is CLI11's ValidationError, raised by the CLI::Range check
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("not in range [1 - 4]"));
    REQUIRE_FALSE(fs::exists(archive));
}

TEST_CASE("create: --sector-size sets the header's sector exponent", "[create]") {
    // Sector size in bytes, and the exponent the header stores for it
    const auto [sector_size, exponent] = GENERATE(table<std::string, std::uint64_t>({
        {"1024", 1},
        {"4096", 3},
        {"8192", 4},
        {"65536", 7},
    }));
    CAPTURE(sector_size);
    SampleFiles files;
    const auto archive = files.dir.Path() / "sector.mpq";

    auto result = Run(mpqcli_binary, {"create", "--sector-size", sector_size, archive.string(),
                                      files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    REQUIRE(HeaderField(archive, header_sector_size_offset, 2) == exponent);
    REQUIRE(ArchiveEntries(archive).size() == 3);
}

TEST_CASE("create: --sector-size rejects a size StormLib cannot write or read back", "[create]") {
    // 512 is written as a sector shift of 0, which StormLib refuses to open
    const auto sector_size = GENERATE(as<std::string>{}, "0", "512", "3000");
    CAPTURE(sector_size);
    SampleFiles files;
    const auto archive = files.dir.Path() / "sector.mpq";

    auto result = Run(mpqcli_binary, {"create", "--sector-size", sector_size, archive.string(),
                                      files.files_dir.string()});

    // 105 is CLI11's ValidationError, from sector_size_valid
    REQUIRE(result.returncode == 105);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output,
                 ContainsSubstring("Sector size must be a power of two of at least 1024"));
    REQUIRE_FALSE(fs::exists(archive));
}

TEST_CASE("create: --raw-chunk-size sets the v4 header's raw chunk size", "[create]") {
    const auto chunk_size = GENERATE(as<std::string>{}, "4096", "16384");
    CAPTURE(chunk_size);
    SampleFiles files;
    const auto archive = files.dir.Path() / "chunk.mpq";

    auto result = Run(mpqcli_binary, {"create", "--format-version", "4", "--raw-chunk-size",
                                      chunk_size, archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    REQUIRE(HeaderField(archive, header_raw_chunk_size_offset, 4) == std::stoull(chunk_size));
    REQUIRE(ArchiveEntries(archive).size() == 3);
}

TEST_CASE("create: --stream-flags reaches the archive stream", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "stream.mpq";

    SECTION("a write-share stream creates a listable archive") {
        auto result = Run(mpqcli_binary, {"create", "--stream-flags", "512", archive.string(),
                                          files.files_dir.string()});

        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
        REQUIRE(ArchiveEntries(archive).size() == 3);
    }

    SECTION("a read-only stream cannot be created into") {
        auto result = Run(mpqcli_binary, {"create", "--stream-flags", "256", archive.string(),
                                          files.files_dir.string()});

        REQUIRE(result.returncode == 1);
        REQUIRE(result.stdout_output.empty());
        REQUIRE_THAT(result.stderr_output, ContainsSubstring("[!] Failed to create MPQ archive"));
        REQUIRE(LinesToVector(result.stderr_output).size() == 2);
        // Neither the archive nor the .partial it was written to is left behind
        REQUIRE(DirectoryEntries(files.dir.Path()) == std::set<std::string>{"files"});
    }
}

TEST_CASE("create: --file-flags1 0 writes no (listfile)", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "no_listfile.mpq";

    auto result = Run(mpqcli_binary,
                      {"create", "--file-flags1", "0", archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    // Without a listfile the entries have no names to list, only positions
    REQUIRE(ArchiveEntries(archive, true) == std::set<std::string>{"enUS  File00000000.xxx",
                                                                   "enUS  File00000001.xxx",
                                                                   "enUS  File00000002.xxx"});
}

TEST_CASE("create: --file-flags2 and --attr-flags store file times", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "attributes.mpq";

    // 4294967295 is MPQ_FILE_DEFAULT_INTERNAL, StormLib's default flags for an
    // internal file; 15 is every attribute: CRC32, FILETIME, MD5 and PATCH_BIT
    auto result = Run(mpqcli_binary, {"create", "--file-flags2", "4294967295", "--attr-flags", "15",
                                      archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    auto listing = Run(mpqcli_binary, {"list", "-a", "-d", "-q", "file-time", archive.string()});
    REQUIRE(listing.returncode == 0);
    REQUIRE(LinesToSet(listing.stdout_output) ==
            std::set<std::string>{"2025-07-29 14:31:00  bytes", "2025-07-29 14:31:00  cats.txt",
                                  "2025-07-29 14:31:00  dogs.txt",
                                  "                     (attributes)",
                                  "                     (listfile)"});
}

TEST_CASE("create: --file-flags3 reserves a (signature) entry that is signed on close",
          "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "reserved_signature.mpq";

    auto result = Run(mpqcli_binary, {"create", "--file-flags3", "4294967295", archive.string(),
                                      files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    REQUIRE(ArchiveEntries(archive, true).count("enUS  (signature)") == 1);
    REQUIRE(InfoProperty(archive, "signature-type") == "Weak");
    REQUIRE(Run(mpqcli_binary, {"verify", archive.string()}).returncode == 0);
}

TEST_CASE("create: --flags sets the added files' flags", "[create]") {
    // Flag value, then the letters the flags column must and must not carry
    const auto [flags, present, absent] = GENERATE(table<std::string, std::string, std::string>({
        {"0", "", "ce"},     // Stored raw: neither compressed nor encrypted
        {"512", "c", "e"},   // MPQ_FILE_COMPRESS
        {"66048", "ce", ""}, // MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED
    }));
    CAPTURE(flags);
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "text.txt", CompressibleText(12000));
    const auto archive = dir.Path() / "flags.mpq";

    auto result =
        Run(mpqcli_binary, {"create", "--flags", flags, archive.string(), source_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"text.txt"}));
    const auto stored = EntryProperty(archive, "text.txt", "flags");
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

TEST_CASE("create: --flags 0 stores files uncompressed", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "text.txt", CompressibleText(12000));
    const auto archive = dir.Path() / "raw.mpq";

    auto result =
        Run(mpqcli_binary, {"create", "--flags", "0", archive.string(), source_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"text.txt"}));
    REQUIRE(EntryNumber(archive, "text.txt", "compressed-size") ==
            EntryNumber(archive, "text.txt", "file-size"));
}

TEST_CASE("create: --compression selects the algorithm for added files", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    WriteFile(source_dir / "text.txt", CompressibleText(12000));
    // Same algorithm for every sector, so only --compression is under test
    const auto create_with = [&](const std::string &compression) {
        const auto archive = dir.Path() / ("compression_" + compression + ".mpq");
        auto result =
            Run(mpqcli_binary, {"create", "--compression", compression, "--compression-next",
                                compression, archive.string(), source_dir.string()});
        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"text.txt"}));
        return archive;
    };

    const auto zlib = create_with("2");   // MPQ_COMPRESSION_ZLIB
    const auto bzip2 = create_with("16"); // MPQ_COMPRESSION_BZIP2

    const auto raw_size = EntryNumber(zlib, "text.txt", "file-size");
    const auto zlib_size = EntryNumber(zlib, "text.txt", "compressed-size");
    const auto bzip2_size = EntryNumber(bzip2, "text.txt", "compressed-size");
    CAPTURE(raw_size, zlib_size, bzip2_size);
    REQUIRE(zlib_size < raw_size);
    REQUIRE(bzip2_size < raw_size);
    REQUIRE(zlib_size != bzip2_size);
}

TEST_CASE("create: --compression-next selects the algorithm after the first sector", "[create]") {
    TempDir dir;
    const auto source_dir = dir.Path() / "src";
    // Well over one sector, so there are subsequent sectors for the option to reach
    WriteFile(source_dir / "text.txt", CompressibleText(12000));
    const auto create_with = [&](const std::string &compression_next) {
        const auto archive = dir.Path() / ("next_" + compression_next + ".mpq");
        auto result = Run(mpqcli_binary, {"create", "--compression", "2", "--compression-next",
                                          compression_next, archive.string(), source_dir.string()});
        REQUIRE(result.returncode == 0);
        REQUIRE(result.stdout_output.empty());
        REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, {"text.txt"}));
        return archive;
    };

    const auto all_zlib = create_with("2");
    const auto then_bzip2 = create_with("16");

    const auto raw_size = EntryNumber(all_zlib, "text.txt", "file-size");
    const auto all_zlib_size = EntryNumber(all_zlib, "text.txt", "compressed-size");
    const auto then_bzip2_size = EntryNumber(then_bzip2, "text.txt", "compressed-size");
    CAPTURE(raw_size, all_zlib_size, then_bzip2_size);
    REQUIRE(all_zlib_size < raw_size);
    REQUIRE(then_bzip2_size < raw_size);
    REQUIRE(all_zlib_size != then_bzip2_size);
}

TEST_CASE("create: signs the archive with --sign", "[create]") {
    const auto flag = GENERATE(as<std::string>{}, "-s", "--sign");
    CAPTURE(flag);
    SampleFiles files;
    const auto archive = files.dir.Path() / "mpq_with_weak_signature.mpq";

    auto result = Run(mpqcli_binary, {"create", flag, archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    REQUIRE(InfoProperty(archive, "signature-type") == "Weak");
    auto verify = Run(mpqcli_binary, {"verify", archive.string()});
    REQUIRE(verify.returncode == 0);
    REQUIRE(verify.stdout_output.empty());
    REQUIRE(LinesToSet(verify.stderr_output) == std::set<std::string>{"[*] Verify success"});
}

TEST_CASE("create: leaves the archive unsigned without --sign", "[create]") {
    SampleFiles files;
    const auto archive = files.dir.Path() / "mpq_no_sign.mpq";

    auto result = Run(mpqcli_binary, {"create", archive.string(), files.files_dir.string()});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stdout_output.empty());
    REQUIRE(LinesToSet(result.stderr_output) == ProgressLines(archive, sample_entries));
    REQUIRE(InfoProperty(archive, "signature-type") == "None");
    REQUIRE(ArchiveEntries(archive, true).count("enUS  (signature)") == 0);
}
