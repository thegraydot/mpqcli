#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

#include <StormLib.h>

#include "util/format.h"

using mpqcli::FileTimeToLsTime;
using mpqcli::StormErrorString;
using mpqcli::WriteBinary;

namespace {

// A Windows FILETIME counts 100-nanosecond intervals from 1601-01-01, which is this
// many seconds before the Unix epoch
constexpr int64_t intervals_per_second = 10000000;
constexpr int64_t filetime_epoch_offset = 11644473600;

// The FILETIME of a local calendar time. Going through mktime, the inverse of the
// localtime the code under test calls, keeps the expected text right in any time zone.
int64_t LocalFileTime(int year, int month, int day, int hour, int minute, int second) {
    std::tm calendar{};
    calendar.tm_year = year - 1900;
    calendar.tm_mon = month - 1;
    calendar.tm_mday = day;
    calendar.tm_hour = hour;
    calendar.tm_min = minute;
    calendar.tm_sec = second;
    calendar.tm_isdst = -1;
    const std::time_t unix_time = std::mktime(&calendar);
    return (static_cast<int64_t>(unix_time) + filetime_epoch_offset) * intervals_per_second;
}

} // namespace

TEST_CASE("FileTimeToLsTime", "[format]") {
    SECTION("prints nothing for a zero time") {
        REQUIRE(FileTimeToLsTime(0).empty());
    }

    SECTION("prints the local date and time") {
        REQUIRE(FileTimeToLsTime(LocalFileTime(2025, 7, 29, 14, 31, 5)) == "2025-07-29 14:31:05");
    }

    SECTION("pads every field to a fixed width") {
        REQUIRE(FileTimeToLsTime(LocalFileTime(2001, 2, 3, 4, 5, 6)) == "2001-02-03 04:05:06");
    }

    SECTION("drops the fraction of a second") {
        const int64_t file_time = LocalFileTime(2025, 7, 29, 14, 31, 5) + intervals_per_second - 1;
        REQUIRE(FileTimeToLsTime(file_time) == "2025-07-29 14:31:05");
    }
}

TEST_CASE("StormErrorString", "[format]") {
    SECTION("describes success") {
        REQUIRE(StormErrorString(ERROR_SUCCESS) == "Success");
    }

    SECTION("describes the system codes StormLib reports") {
        REQUIRE(StormErrorString(ERROR_FILE_NOT_FOUND) == "File not found");
        REQUIRE(StormErrorString(ERROR_DISK_FULL) == "Disk full");
        REQUIRE(StormErrorString(ERROR_BAD_FORMAT) == "Bad MPQ format");
    }

    SECTION("describes the codes StormLib defines for itself") {
        REQUIRE(StormErrorString(ERROR_FILE_CORRUPT) == "File is corrupt");
        REQUIRE(StormErrorString(ERROR_UNKNOWN_FILE_KEY) == "Unknown file encryption key");
        REQUIRE(StormErrorString(ERROR_CHECKSUM_ERROR) == "Sector checksum mismatch");
        REQUIRE(StormErrorString(ERROR_FILE_DELETED) == "File contains delete marker");
    }

    SECTION("describes a code it has no case for by the code and the platform's own words") {
#ifdef _WIN32
        // 32 is ERROR_SHARING_VIOLATION. Windows localises the text, so the test checks
        // where it came from rather than what it says: strerror(32) is "Broken pipe", and
        // "Unknown error 32" is what is left when FormatMessage fails. MSVC deprecates
        // strerror, so its text comes from strerror_s.
        const std::string prefix = "(32) ";
        const std::string text = StormErrorString(32U);
        REQUIRE(text.rfind(prefix, 0) == 0);
        const std::string words = text.substr(prefix.size());
        REQUIRE_FALSE(words.empty());
        char errno_text[256] = {};
        REQUIRE(strerror_s(errno_text, sizeof(errno_text), 32) == 0);
        REQUIRE(words != errno_text);
        REQUIRE(words != "Unknown error 32");
        REQUIRE(words.back() != '\n');
        REQUIRE(words.back() != '.');
#else
        REQUIRE(StormErrorString(EISDIR) ==
                "(" + std::to_string(EISDIR) + ") " + std::strerror(EISDIR));
#endif
    }
}

TEST_CASE("WriteBinary", "[format]") {
    std::ostringstream out;

    SECTION("passes every byte through unchanged, line endings and nulls included") {
        const std::string bytes("a\n\r\0b\x0A\x00\xFF", 8);
        WriteBinary(out, bytes.data(), static_cast<uint32_t>(bytes.size()));
        REQUIRE(out.str() == bytes);
    }

    SECTION("writes only as many bytes as size counts") {
        WriteBinary(out, "abcdef", 3U);
        REQUIRE(out.str() == "abc");
    }

    SECTION("writes nothing for a size of zero") {
        WriteBinary(out, "abc", 0U);
        REQUIRE(out.str().empty());
    }
}
