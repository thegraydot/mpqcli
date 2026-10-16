#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <vector>

#include <StormLib.h>

#include "mpq/flags.h"

using mpqcli::GetFlagString;

namespace {

struct FlagLetter {
    uint32_t flag;
    char letter;
};

// The flags that each own a bit and a letter, in the order the letters are printed
const std::vector<FlagLetter> flag_letters = {
    {MPQ_FILE_IMPLODE, 'i'},       {MPQ_FILE_COMPRESS, 'c'},   {MPQ_FILE_ENCRYPTED, 'e'},
    {MPQ_FILE_KEY_V2, '2'},        {MPQ_FILE_PATCH_FILE, 'p'}, {MPQ_FILE_SINGLE_UNIT, 'u'},
    {MPQ_FILE_DELETE_MARKER, 'd'}, {MPQ_FILE_SECTOR_CRC, 'r'}, {MPQ_FILE_SIGNATURE, 's'},
    {MPQ_FILE_EXISTS, 'x'},
};

} // namespace

TEST_CASE("GetFlagString", "[flags]") {
    SECTION("is empty for no flags") {
        REQUIRE(GetFlagString(0U).empty());
    }

    SECTION("prints each flag's letter") {
        for (const auto &[flag, letter] : flag_letters) {
            CAPTURE(flag, letter);
            REQUIRE(GetFlagString(flag).find(letter) != std::string::npos);
        }
    }

    SECTION("prints exactly one letter for a single flag") {
        for (const auto &[flag, letter] : flag_letters) {
            CAPTURE(flag, letter);
            REQUIRE(GetFlagString(flag) == std::string(1, letter));
        }
    }

    SECTION("prints one letter per flag set, and nothing for the masks") {
        uint32_t all_flags = 0U;
        for (const auto &entry : flag_letters) {
            all_flags |= entry.flag;
        }
        REQUIRE(GetFlagString(all_flags) == "ice2pudrsx");
    }

    SECTION("prints no other flag's letter") {
        for (const auto &[flag, letter] : flag_letters) {
            const std::string flags = GetFlagString(flag);
            for (const auto &other : flag_letters) {
                if (other.flag == flag) {
                    continue;
                }
                CAPTURE(flag, other.letter);
                REQUIRE(flags.find(other.letter) == std::string::npos);
            }
        }
    }

    SECTION("prints the letters in a fixed order whatever the flags") {
        uint32_t all_flags = 0U;
        for (const auto &entry : flag_letters) {
            all_flags |= entry.flag;
        }
        const std::string flags = GetFlagString(all_flags);
        std::string::size_type previous = 0U;
        for (const auto &[flag, letter] : flag_letters) {
            CAPTURE(letter);
            const std::string::size_type position = flags.find(letter);
            REQUIRE(position != std::string::npos);
            REQUIRE(position >= previous);
            previous = position;
        }
    }
}
