#include "archive_helper.h"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "subprocess_helper.h"
#include "test_environment.h"

namespace fs = std::filesystem;

namespace mpqcli::testing {

std::set<std::string> ArchiveEntries(const fs::path &archive, bool all_files,
                                     const std::optional<fs::path> &listfile) {
    std::vector<std::string> args = {"list", archive.string(), "-d", "-q", "locale"};
    if (all_files) {
        args.push_back("-a");
    }
    if (listfile.has_value()) {
        args.push_back("-l");
        args.push_back(listfile->string());
    }
    auto result = Run(TestEnvironment::Instance().binary_path, args);
    REQUIRE(result.returncode == 0);
    return LinesToSet(result.stdout_output);
}

std::string InfoProperty(const fs::path &archive, const std::string &property) {
    auto result =
        Run(TestEnvironment::Instance().binary_path, {"info", "-q", property, archive.string()});
    REQUIRE(result.returncode == 0);
    return TrimTrailingNewlines(result.stdout_output);
}

std::string EntryProperty(const fs::path &archive, const std::string &name,
                          const std::string &property) {
    auto result = Run(TestEnvironment::Instance().binary_path,
                      {"list", archive.string(), "-d", "-q", property});
    REQUIRE(result.returncode == 0);
    // The name is matched at the end of the line, after the two spaces list puts before
    // it, rather than split off at whitespace: a file time holds a space, and so may a name
    const std::string name_field = "  " + name;
    for (const auto &line : LinesToSet(result.stdout_output)) {
        if (line.size() >= name_field.size() &&
            line.compare(line.size() - name_field.size(), name_field.size(), name_field) == 0) {
            std::string value = line.substr(0, line.size() - name_field.size());
            value.erase(0, value.find_first_not_of(' '));
            return value;
        }
    }
    FAIL("no entry named " << name << " in " << archive);
}

std::uint64_t EntryNumber(const fs::path &archive, const std::string &name,
                          const std::string &property) {
    return std::stoull(EntryProperty(archive, name, property));
}

std::set<std::string> ReadEntry(const fs::path &archive, const std::string &name,
                                const std::optional<std::string> &locale) {
    std::vector<std::string> args = {"read", archive.string(), name};
    if (locale.has_value()) {
        args.push_back("--locale");
        args.push_back(*locale);
    }
    auto result = Run(TestEnvironment::Instance().binary_path, args);
    REQUIRE(result.returncode == 0);
    return LinesToSet(result.stdout_output, true);
}

} // namespace mpqcli::testing
