#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>

#include <StormLib.h>

#include "errors.h"

namespace fs = std::filesystem;

using Catch::Matchers::Message;
using mpqcli::ArchiveCreateError;
using mpqcli::ArchiveError;
using mpqcli::ArchiveOpenError;
using mpqcli::Error;
using mpqcli::FileError;
using mpqcli::Interrupted;
using mpqcli::StormError;
using mpqcli::ThrowIfCancelled;

namespace {

constexpr uint32_t corrupt = ERROR_FILE_CORRUPT;

} // namespace

TEST_CASE("Error", "[errors]") {
    SECTION("carries its message as a std::runtime_error") {
        STATIC_REQUIRE(std::is_base_of_v<std::runtime_error, Error>);
        REQUIRE(std::string(Error("Something failed").what()) == "Something failed");
    }

    SECTION("is the base of every exception mpqcli throws") {
        STATIC_REQUIRE(std::is_base_of_v<Error, Interrupted>);
        STATIC_REQUIRE(std::is_base_of_v<Error, FileError>);
        STATIC_REQUIRE(std::is_base_of_v<Error, ArchiveError>);
        STATIC_REQUIRE(std::is_base_of_v<ArchiveError, StormError>);
        STATIC_REQUIRE(std::is_base_of_v<StormError, ArchiveOpenError>);
        STATIC_REQUIRE(std::is_base_of_v<StormError, ArchiveCreateError>);
    }
}

TEST_CASE("Interrupted", "[errors]") {
    SECTION("says the command was interrupted") {
        REQUIRE(std::string(Interrupted().what()) == "Interrupted");
    }
}

TEST_CASE("ThrowIfCancelled", "[errors]") {
    SECTION("returns while the flag is clear") {
        const std::atomic<bool> cancelled{false};
        REQUIRE_NOTHROW(ThrowIfCancelled(cancelled));
    }

    SECTION("throws Interrupted once the flag is set") {
        const std::atomic<bool> cancelled{true};
        REQUIRE_THROWS_MATCHES(ThrowIfCancelled(cancelled), Interrupted, Message("Interrupted"));
    }
}

TEST_CASE("FileError", "[errors]") {
    const fs::path path = fs::path("dir") / "a.txt";

    SECTION("ends its message with the code, the code's text and the path") {
        const std::error_code ec = std::make_error_code(std::errc::no_such_file_or_directory);
        const FileError error("Failed to open", path, ec);
        REQUIRE(std::string(error.what()) == "Failed to open: (" + std::to_string(ec.value()) +
                                                 ") " + ec.message() + ": " + path.string());
        REQUIRE(error.Path() == path);
        REQUIRE(error.ErrorCode() == ec);
    }

    SECTION("ends its message with the path alone when there is no code") {
        const FileError error("Not a file or directory", path);
        REQUIRE(std::string(error.what()) == "Not a file or directory: " + path.string());
        REQUIRE(error.Path() == path);
        REQUIRE_FALSE(error.ErrorCode());
    }
}

TEST_CASE("ArchiveError", "[errors]") {
    SECTION("carries its message unchanged") {
        REQUIRE(std::string(ArchiveError("Bad archive").what()) == "Bad archive");
    }
}

TEST_CASE("StormError", "[errors]") {
    SECTION("ends its message with the error's text") {
        const StormError error("Failed to read", corrupt);
        REQUIRE(std::string(error.what()) == "Failed to read: File is corrupt");
        REQUIRE(error.ErrorCode() == corrupt);
    }
}

TEST_CASE("ArchiveOpenError", "[errors]") {
    SECTION("names the archive ahead of the error's text") {
        const fs::path path = fs::path("dir") / "a.mpq";
        const ArchiveOpenError error(path, corrupt);
        REQUIRE(std::string(error.what()) ==
                "Failed to open MPQ archive: " + path.string() + ": File is corrupt");
        REQUIRE(error.Path() == path);
        REQUIRE(error.ErrorCode() == corrupt);
    }
}

TEST_CASE("ArchiveCreateError", "[errors]") {
    SECTION("names the archive ahead of the error's text") {
        const fs::path path = fs::path("dir") / "a.mpq";
        const ArchiveCreateError error(path, corrupt);
        REQUIRE(std::string(error.what()) ==
                "Failed to create MPQ archive: " + path.string() + ": File is corrupt");
        REQUIRE(error.Path() == path);
        REQUIRE(error.ErrorCode() == corrupt);
    }
}
