#include <algorithm>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "read_file.h"
#include "subprocess_helper.h"
#include "temp_dir.h"
#include "test_environment.h"

namespace fs = std::filesystem;

using namespace mpqcli::testing;
using Catch::Matchers::ContainsSubstring;

namespace {

// The binary every case spawns: a reference into the singleton, so it is the
// baked path itself rather than a copy that could drift from it
const fs::path &mpqcli_binary = TestEnvironment::Instance().binary_path;

// A shell the completion subcommand supports, and the script it embeds
struct Shell {
    std::string name;
    std::string script;
};

const std::vector<Shell> shells = {
    {"bash", "mpqcli.bash"},
    {"zsh", "mpqcli.zsh"},
    {"fish", "mpqcli.fish"},
    {"powershell", "mpqcli.ps1"},
};

// The scripts live in the source tree beside test/, reached through the baked path
fs::path ScriptPath(const Shell &shell) {
    return TestEnvironment::Instance().test_dir.parent_path() / "completion" / shell.script;
}

// The scripts are committed with LF endings while stdout is text mode on
// Windows, so both sides drop carriage returns before comparing
std::string WithoutCarriageReturns(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

std::string ScriptOutput(const std::string &shell) {
    auto result = Run(mpqcli_binary, {"completion", shell});
    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    return result.stdout_output;
}

// True when token stands on its own in text, not run into a neighbouring name
// character: "--compression" inside "--compression-next" does not count, and
// nor does "info" inside "_mpqcli_info"
bool HasToken(const std::string &text, const std::string &token) {
    const auto is_name_char = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '_';
    };
    for (auto pos = text.find(token); pos != std::string::npos; pos = text.find(token, pos + 1)) {
        const bool clean_before = pos == 0 || !is_name_char(text[pos - 1]);
        const auto end = pos + token.size();
        const bool clean_after = end == text.size() || !is_name_char(text[end]);
        if (clean_before && clean_after) {
            return true;
        }
    }
    return false;
}

// How a shell's script spells an option: fish declares "-l name" or "-s n"
// rather than writing the option itself
std::string OptionToken(const Shell &shell, const std::string &option) {
    if (shell.name != "fish") {
        return option;
    }
    return option.rfind("--", 0) == 0 ? "-l " + option.substr(2) : "-s " + option.substr(1);
}

// True when line opens the part of shell's script that completes subcommand
bool OpensSection(const Shell &shell, const std::string &line, const std::string &subcommand) {
    const auto start = line.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return false;
    }
    const auto text = line.substr(start);
    if (shell.name == "bash") {
        // The subcommand's case label
        return text == subcommand + ")";
    }
    if (shell.name == "zsh") {
        // The function that calls _arguments for the subcommand
        return text.rfind("_mpqcli_" + subcommand + "()", 0) == 0;
    }
    if (shell.name == "fish") {
        // Each complete command conditioned on the subcommand
        return HasToken(text, "__fish_seen_subcommand_from " + subcommand);
    }
    // The subcommand's table in $optionSpec, not its description in $subcommands
    return text.rfind("'" + subcommand + "'", 0) == 0 && text.find("@{") != std::string::npos;
}

// The part of script that completes subcommand: each line that opens it, with
// the lines below it indented deeper, such as a case body or a fish
// continuation. Fish opens one such block per complete command, the other
// shells a single block.
std::string SubcommandSection(const Shell &shell, const std::string &script,
                              const std::string &subcommand) {
    std::istringstream lines(WithoutCarriageReturns(script));
    std::string section;
    std::string line;
    bool inside = false;
    std::size_t opener_indent = 0;
    while (std::getline(lines, line)) {
        const auto indent = line.find_first_not_of(" \t");
        if (inside && indent != std::string::npos && indent <= opener_indent) {
            inside = false;
        }
        if (!inside && OpensSection(shell, line, subcommand)) {
            inside = true;
            opener_indent = indent;
        }
        if (inside) {
            section += line + "\n";
        }
    }
    return section;
}

// The subcommands the binary's own help lists, so a subcommand added to
// main.cpp is demanded of every script without this file changing
std::vector<std::string> Subcommands() {
    auto result = Run(mpqcli_binary, {"--help"});
    REQUIRE(result.returncode == 0);
    std::vector<std::string> names;
    std::istringstream lines(result.stdout_output);
    std::string line;
    bool in_subcommands = false;
    while (std::getline(lines, line)) {
        if (line.rfind("SUBCOMMANDS:", 0) == 0) {
            in_subcommands = true;
            continue;
        }
        std::istringstream fields(line);
        std::string name;
        if (in_subcommands && fields >> name) {
            names.push_back(name);
        }
    }
    REQUIRE_FALSE(names.empty());
    return names;
}

// Every capture of pattern's first group in a subcommand's help
std::set<std::string> HelpMatches(const std::string &subcommand, const std::regex &pattern) {
    auto result = Run(mpqcli_binary, {subcommand, "--help"});
    REQUIRE(result.returncode == 0);
    std::set<std::string> matches;
    std::sregex_iterator match(result.stdout_output.begin(), result.stdout_output.end(), pattern);
    const std::sregex_iterator end;
    for (; match != end; ++match) {
        matches.insert((*match)[1].str());
    }
    return matches;
}

// The long options a subcommand's help lists, without --help itself
std::set<std::string> LongOptions(const std::string &subcommand) {
    static const std::regex option_pattern("(--[a-z][a-z0-9-]*)");
    auto options = HelpMatches(subcommand, option_pattern);
    options.erase("--help");
    return options;
}

// The short options a subcommand's help lists, without -h itself. CLI11
// writes each before its long name, as "-q,     --property".
std::set<std::string> ShortOptions(const std::string &subcommand) {
    static const std::regex option_pattern("(?:^|\\s)(-[a-zA-Z]),");
    auto options = HelpMatches(subcommand, option_pattern);
    options.erase("-h");
    return options;
}

// The values a validator lists after prefix on one line: space-separated after
// "one of: ", comma-separated inside braces after "not in {"
std::set<std::string> ValuesAfter(const std::string &text, const std::string &prefix) {
    const auto start = text.find(prefix);
    REQUIRE(start != std::string::npos);
    auto rest = text.substr(start + prefix.size());
    rest = rest.substr(0, rest.find_first_of("}\r\n"));
    std::replace(rest.begin(), rest.end(), ',', ' ');
    std::istringstream fields(rest);
    return std::set<std::string>(std::istream_iterator<std::string>(fields),
                                 std::istream_iterator<std::string>());
}

// A named value set the CLI accepts, read back from the binary's own rejection
// message so the scripts are held to what the binary actually takes
struct ValueSet {
    std::string what;
    std::set<std::string> values;
};

std::vector<ValueSet> ValueSets() {
    // Any existing file, and for create a new archive name, so the value
    // validator is what fails and not an archive or files check
    const auto existing = ScriptPath(shells.front()).string();
    const TempDir dir;
    const auto new_archive = (dir.Path() / "new.mpq").string();
    const auto rejection = [&](const std::vector<std::string> &args) {
        auto result = Run(mpqcli_binary, args);
        // 105 is CLI11's ValidationError
        REQUIRE(result.returncode == 105);
        return result.stderr_output;
    };
    return {
        {"locales",
         ValuesAfter(rejection({"create", "--locale", "nope", new_archive, existing}), "one of: ")},
        {"game profiles",
         ValuesAfter(rejection({"create", "-g", "nope", new_archive, existing}), "one of: ")},
        {"info properties", ValuesAfter(rejection({"info", "-q", "nope", existing}), "not in {")},
        {"list properties", ValuesAfter(rejection({"list", "-q", "nope", existing}), "not in {")},
    };
}

} // namespace

TEST_CASE("completion: prints its help with --help", "[completion]") {
    auto result = Run(mpqcli_binary, {"completion", "--help"});

    REQUIRE(result.returncode == 0);
    REQUIRE(result.stderr_output.empty());
    for (const auto &shell : shells) {
        CAPTURE(shell.name);
        REQUIRE_THAT(result.stdout_output, ContainsSubstring(shell.name));
    }
}

TEST_CASE("completion: fails without a shell", "[completion]") {
    auto result = Run(mpqcli_binary, {"completion"});

    // 106 is CLI11's RequiredError: each shell is a subcommand of completion,
    // which requires one
    REQUIRE(result.returncode == 106);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("A subcommand is required"));
}

TEST_CASE("completion: rejects an unknown shell", "[completion]") {
    auto result = Run(mpqcli_binary, {"completion", "nushell"});

    // 106 is CLI11's RequiredError: no subcommand matches the name, and CLI11
    // reports the missing subcommand before the unexpected argument
    REQUIRE(result.returncode == 106);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("A subcommand is required"));
}

TEST_CASE("completion: rejects an argument after the shell", "[completion]") {
    auto result = Run(mpqcli_binary, {"completion", "bash", "extra"});

    // 109 is CLI11's ExtrasError
    REQUIRE(result.returncode == 109);
    REQUIRE(result.stdout_output.empty());
    REQUIRE_THAT(result.stderr_output, ContainsSubstring("not expected: extra"));
}

TEST_CASE("completion: prints each shell's script byte for byte", "[completion]") {
    const auto shell = GENERATE(from_range(shells));
    CAPTURE(shell.name);
    const auto expected = ReadFile(ScriptPath(shell));
    REQUIRE_FALSE(expected.empty());

    const auto output = ScriptOutput(shell.name);

    // The embedded copy is the committed script, whole: a truncated raw string
    // literal or a stale configure would show up here
    REQUIRE(WithoutCarriageReturns(output) == WithoutCarriageReturns(expected));
}

TEST_CASE("completion: each script registers itself for its shell", "[completion]") {
    // The line each shell needs to pick the script up at all
    const auto [shell, registration] = GENERATE(table<std::string, std::string>({
        {"bash", "complete -F _mpqcli mpqcli"},
        {"zsh", "#compdef mpqcli"},
        {"fish", "complete -c mpqcli"},
        {"powershell", "Register-ArgumentCompleter -Native -CommandName 'mpqcli'"},
    }));
    CAPTURE(shell);

    const auto output = ScriptOutput(shell);

    REQUIRE_THAT(output, ContainsSubstring(registration));
    if (shell == "zsh") {
        // zsh autoloads a completion only when #compdef is the first line
        REQUIRE(output.rfind(registration, 0) == 0);
    }
}

TEST_CASE("completion: every script covers every subcommand and option", "[completion]") {
    const auto shell = GENERATE(from_range(shells));
    CAPTURE(shell.name);

    const auto script = ScriptOutput(shell.name);

    // A subcommand or option wired into the CLI but into only some of the
    // hand-written scripts, or under the wrong subcommand, is the failure this
    // guards against
    // Options come from scraping the help text, so an empty scrape is checked rather than
    // skipped: after a change to CLI11's layout it would otherwise check nothing and pass
    const std::set<std::string> without_options = {"about", "completion", "version"};
    for (const auto &subcommand : Subcommands()) {
        CAPTURE(subcommand);
        REQUIRE(HasToken(script, subcommand));
        auto options = LongOptions(subcommand);
        const auto short_options = ShortOptions(subcommand);
        if (without_options.count(subcommand) != 0) {
            REQUIRE(options.empty());
            REQUIRE(short_options.empty());
            continue;
        }
        REQUIRE_FALSE(options.empty());
        REQUIRE_FALSE(short_options.empty());
        options.insert(short_options.begin(), short_options.end());
        const auto section = SubcommandSection(shell, script, subcommand);
        REQUIRE_FALSE(section.empty());
        for (const auto &option : options) {
            CAPTURE(option);
            REQUIRE(HasToken(section, OptionToken(shell, option)));
        }
    }
}

TEST_CASE("completion: no script offers a subcommand a short option it rejects", "[completion]") {
    // Short options a script could plausibly offer the subcommand: -p is
    // create's and add's --path, -o is extract's --output, and -f and -k are
    // tar's file and keep flags
    const auto [subcommand, option] = GENERATE(table<std::string, std::string>({
        {"info", "-p"},
        {"list", "-p"},
        {"verify", "-p"},
        {"create", "-o"},
        {"extract", "-f"},
        {"extract", "-k"},
    }));
    CAPTURE(subcommand, option);
    // The subcommand's own help agrees it takes no such option. The scrape must find its
    // other short options, or the check that follows would pass on an empty set.
    const auto short_options = ShortOptions(subcommand);
    REQUIRE_FALSE(short_options.empty());
    REQUIRE(short_options.count(option) == 0);

    for (const auto &shell : shells) {
        CAPTURE(shell.name);
        const auto section = SubcommandSection(shell, ScriptOutput(shell.name), subcommand);
        REQUIRE_FALSE(section.empty());
        REQUIRE_FALSE(HasToken(section, OptionToken(shell, option)));
    }
}

TEST_CASE("completion: every script offers every locale, game profile and property",
          "[completion]") {
    const auto shell = GENERATE(from_range(shells));
    CAPTURE(shell.name);

    const auto script = ScriptOutput(shell.name);

    for (const auto &set : ValueSets()) {
        CAPTURE(set.what);
        REQUIRE_FALSE(set.values.empty());
        for (const auto &value : set.values) {
            CAPTURE(value);
            REQUIRE(HasToken(script, value));
        }
    }
}
