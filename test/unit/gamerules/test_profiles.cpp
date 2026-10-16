#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "gamerules/profiles.h"
#include "gamerules/rules.h"

using mpqcli::GameProfile;
using mpqcli::GameRules;

TEST_CASE("GameRules::StringToProfile", "[profiles]") {
    SECTION("resolves each canonical name") {
        const std::vector<std::pair<std::string, GameProfile>> names = {
            {"generic", GameProfile::GENERIC},
            {"diablo1", GameProfile::DIABLO1},
            {"lordsofmagic", GameProfile::LORDSOFMAGIC},
            {"starcraft1", GameProfile::STARCRAFT1},
            {"warcraft2", GameProfile::WARCRAFT2},
            {"diablo2", GameProfile::DIABLO2},
            {"warcraft3", GameProfile::WARCRAFT3},
            {"warcraft3-map", GameProfile::WARCRAFT3_MAP},
            {"wow-vanilla", GameProfile::WOW_1X},
            {"wow-tbc", GameProfile::WOW_2X},
            {"wow-wotlk", GameProfile::WOW_3X},
            {"wow-cataclysm", GameProfile::WOW_4X},
            {"wow-mop", GameProfile::WOW_5X},
            {"starcraft2", GameProfile::STARCRAFT2},
            {"diablo3", GameProfile::DIABLO3},
        };
        for (const auto &[name, profile] : names) {
            CAPTURE(name);
            REQUIRE(GameRules::StringToProfile(name) == profile);
        }
    }

    SECTION("resolves each alias") {
        const std::vector<std::pair<std::string, GameProfile>> aliases = {
            {"diablo", GameProfile::DIABLO1},
            {"d1", GameProfile::DIABLO1},
            {"lomse", GameProfile::LORDSOFMAGIC},
            {"starcraft", GameProfile::STARCRAFT1},
            {"sc", GameProfile::STARCRAFT1},
            {"sc1", GameProfile::STARCRAFT1},
            {"wc2", GameProfile::WARCRAFT2},
            {"war2", GameProfile::WARCRAFT2},
            {"d2", GameProfile::DIABLO2},
            {"wc3", GameProfile::WARCRAFT3},
            {"war3", GameProfile::WARCRAFT3},
            {"wc3-map", GameProfile::WARCRAFT3_MAP},
            {"war3-map", GameProfile::WARCRAFT3_MAP},
            {"wow1", GameProfile::WOW_1X},
            {"wow2", GameProfile::WOW_2X},
            {"wow3", GameProfile::WOW_3X},
            {"wow4", GameProfile::WOW_4X},
            {"wow5", GameProfile::WOW_5X},
            {"sc2", GameProfile::STARCRAFT2},
            {"d3", GameProfile::DIABLO3},
        };
        for (const auto &[alias, profile] : aliases) {
            CAPTURE(alias);
            REQUIRE(GameRules::StringToProfile(alias) == profile);
        }
    }

    SECTION("ignores case") {
        REQUIRE(GameRules::StringToProfile("WarCraft3") == GameProfile::WARCRAFT3);
        REQUIRE(GameRules::StringToProfile("WOW-TBC") == GameProfile::WOW_2X);
        REQUIRE(GameRules::StringToProfile("D2") == GameProfile::DIABLO2);
    }

    SECTION("falls back to the generic profile for an unknown name") {
        REQUIRE(GameRules::StringToProfile("quake") == GameProfile::GENERIC);
        REQUIRE(GameRules::StringToProfile("warcraft 3") == GameProfile::GENERIC);
        REQUIRE(GameRules::StringToProfile("") == GameProfile::GENERIC);
    }
}

TEST_CASE("GameRules::GetCanonicalProfiles", "[profiles]") {
    SECTION("lists one name per profile, in display order") {
        const std::vector<std::string> expected = {
            "generic",   "diablo1",       "lordsofmagic",  "starcraft1",  "warcraft2",
            "diablo2",   "warcraft3",     "warcraft3-map", "wow-vanilla", "wow-tbc",
            "wow-wotlk", "wow-cataclysm", "wow-mop",       "starcraft2",  "diablo3",
        };
        REQUIRE(GameRules::GetCanonicalProfiles() == expected);
    }

    SECTION("lists names that each resolve to a different profile") {
        const auto names = GameRules::GetCanonicalProfiles();
        std::set<GameProfile> profiles;
        for (const auto &name : names) {
            profiles.insert(GameRules::StringToProfile(name));
        }
        REQUIRE(profiles.size() == names.size());
    }
}

TEST_CASE("GameRules::GetAvailableProfiles", "[profiles]") {
    SECTION("joins the canonical names with a comma and a space") {
        const auto names = GameRules::GetCanonicalProfiles();
        std::string expected = names.front();
        for (std::size_t i = 1; i < names.size(); ++i) {
            expected += ", " + names[i];
        }
        REQUIRE(GameRules::GetAvailableProfiles() == expected);
    }

    SECTION("starts and ends with a name, not a separator") {
        const std::string profiles = GameRules::GetAvailableProfiles();
        REQUIRE(profiles.rfind("generic, ", 0) == 0U);
        REQUIRE(profiles.size() > 9U);
        REQUIRE(profiles.substr(profiles.size() - 9U) == ", diablo3");
    }
}
