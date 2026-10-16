#include <catch2/catch_test_macros.hpp>
#include <set>
#include <sstream>
#include <string>

#include "commands/list.h"
#include "created_archive.h"

using mpqcli::List;
using mpqcli::ListOptions;
using mpqcli::testing::CreatedArchive;

TEST_CASE("List", "[list]") {
    CreatedArchive created;
    std::ostringstream out;
    std::ostringstream err;

    SECTION("prints each file's name on a line of its own") {
        ListOptions options;
        options.archive = created.archive;
        List(options, out, err);

        std::set<std::string> lines;
        std::istringstream in(out.str());
        for (std::string line; std::getline(in, line);) {
            lines.insert(line);
        }
        const std::set<std::string> expected = {"bytes", "cats.txt", "dogs.txt"};
        REQUIRE(lines == expected);
    }
}
