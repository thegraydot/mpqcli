#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/list.h"
#include "raw_archive.h"
#include "temp_dir.h"

using mpqcli::Archive;
using mpqcli::ListFiles;
using mpqcli::testing::TempDir;
using mpqcli::testing::WriteRawArchive;

TEST_CASE("ListFiles", "[list]") {
    TempDir dir;
    std::ostringstream out;
    std::ostringstream err;

    SECTION("lists every locale of a name with more than 32 of them") {
        // The default locale and 33 others, none of them in the locale table, so each
        // prints as its own four hexadecimal digits
        std::vector<std::pair<std::string, LCID>> entries = {{"a.txt", 0}};
        std::set<std::string> expected = {"enUS  a.txt"};
        for (LCID locale = 1; locale <= 33; locale++) {
            entries.emplace_back("a.txt", locale);
            char hex[5];
            std::snprintf(hex, sizeof(hex), "%04X", static_cast<unsigned>(locale));
            expected.insert(std::string(hex) + "  a.txt");
        }
        const auto path = dir.Path() / "locales.mpq";
        WriteRawArchive(path, entries);
        const auto archive = Archive::Open(path, MPQ_OPEN_READ_ONLY);

        ListFiles(archive.Handle(), std::nullopt, false, true, {"locale"}, out, err);

        std::vector<std::string> lines;
        std::istringstream in(out.str());
        for (std::string line; std::getline(in, line);) {
            lines.push_back(line);
        }
        CAPTURE(lines);
        REQUIRE(lines.size() == 34);
        REQUIRE(std::set<std::string>(lines.begin(), lines.end()) == expected);
        REQUIRE(err.str().empty());
    }
}
