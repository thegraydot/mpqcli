#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>

#include "commands/list.h"
#include "created_archive.h"
#include "write_file.h"

namespace fs = std::filesystem;

using mpqcli::List;
using mpqcli::ListOptions;
using mpqcli::testing::CreatedArchive;
using mpqcli::testing::CreateWithoutListfile;
using mpqcli::testing::WriteFile;

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

    SECTION("prints nothing for an archive with no entries") {
        const fs::path empty_dir = created.files.dir.Path() / "empty";
        fs::create_directories(empty_dir);
        const fs::path bare = created.files.dir.Path() / "bare.mpq";
        CreateWithoutListfile(bare, empty_dir);
        ListOptions options;
        options.archive = bare;
        options.all = true;
        List(options, out, err);
        REQUIRE(out.str().empty());
        REQUIRE(err.str().empty());
    }

    SECTION("ignores an empty listfile, with a warning") {
        const fs::path unnamed = created.files.dir.Path() / "unnamed.mpq";
        CreateWithoutListfile(unnamed, created.files.files_dir);
        const fs::path listfile = created.files.dir.Path() / "empty.txt";
        WriteFile(listfile, "");
        ListOptions options;
        options.archive = unnamed;
        std::ostringstream without_listfile;
        List(options, without_listfile, err);
        REQUIRE_FALSE(without_listfile.str().empty());

        options.listfile = listfile;
        List(options, out, err);
        REQUIRE(out.str() == without_listfile.str());
        REQUIRE(err.str() ==
                "[!] Ignoring the listfile, which is empty: " + listfile.string() + "\n");
    }
}
