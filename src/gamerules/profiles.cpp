#include <map>
#include <string>
#include <vector>

#include "gamerules/rules.h"
#include "util/string.h"

namespace mpqcli {

namespace {

// The single source of truth for valid profile names and their aliases
const std::map<std::string, GameProfile> &GetProfileMap() {
    static const std::map<std::string, GameProfile> profile_map = {
        {"generic", GameProfile::GENERIC},
        {"diablo1", GameProfile::DIABLO1},
        {"diablo", GameProfile::DIABLO1},
        {"d1", GameProfile::DIABLO1},
        {"lordsofmagic", GameProfile::LORDSOFMAGIC},
        {"lomse", GameProfile::LORDSOFMAGIC},
        {"starcraft", GameProfile::STARCRAFT1},
        {"starcraft1", GameProfile::STARCRAFT1},
        {"sc", GameProfile::STARCRAFT1},
        {"sc1", GameProfile::STARCRAFT1},
        {"warcraft2", GameProfile::WARCRAFT2},
        {"wc2", GameProfile::WARCRAFT2},
        {"war2", GameProfile::WARCRAFT2},
        {"diablo2", GameProfile::DIABLO2},
        {"d2", GameProfile::DIABLO2},
        {"warcraft3", GameProfile::WARCRAFT3},
        {"wc3", GameProfile::WARCRAFT3},
        {"war3", GameProfile::WARCRAFT3},
        {"warcraft3-map", GameProfile::WARCRAFT3_MAP},
        {"wc3-map", GameProfile::WARCRAFT3_MAP},
        {"war3-map", GameProfile::WARCRAFT3_MAP},
        {"wow1", GameProfile::WOW_1X},
        {"wow-vanilla", GameProfile::WOW_1X},
        {"wow2", GameProfile::WOW_2X},
        {"wow-tbc", GameProfile::WOW_2X},
        {"wow3", GameProfile::WOW_3X},
        {"wow-wotlk", GameProfile::WOW_3X},
        {"wow4", GameProfile::WOW_4X},
        {"wow-cataclysm", GameProfile::WOW_4X},
        {"wow5", GameProfile::WOW_5X},
        {"wow-mop", GameProfile::WOW_5X},
        {"starcraft2", GameProfile::STARCRAFT2},
        {"sc2", GameProfile::STARCRAFT2},
        {"diablo3", GameProfile::DIABLO3},
        {"d3", GameProfile::DIABLO3}};
    return profile_map;
}

} // namespace

GameProfile GameRules::StringToProfile(const std::string &profile_name) {
    const auto &profile_map = GetProfileMap();
    std::string lower = ToLower(profile_name);
    auto it = profile_map.find(lower);
    if (it != profile_map.end()) {
        return it->second;
    }
    return GameProfile::GENERIC;
}

std::string GameRules::ProfileToString(GameProfile profile) {
    switch (profile) {
    case GameProfile::GENERIC:
        return "generic";
    case GameProfile::DIABLO1:
        return "diablo1";
    case GameProfile::LORDSOFMAGIC:
        return "lordsofmagic";
    case GameProfile::WARCRAFT2:
        return "warcraft2";
    case GameProfile::STARCRAFT1:
        return "starcraft1";
    case GameProfile::DIABLO2:
        return "diablo2";
    case GameProfile::WARCRAFT3:
        return "warcraft3";
    case GameProfile::WARCRAFT3_MAP:
        return "warcraft3-map";
    case GameProfile::WOW_1X:
        return "wow-vanilla";
    case GameProfile::WOW_2X:
        return "wow-tbc";
    case GameProfile::WOW_3X:
        return "wow-wotlk";
    case GameProfile::WOW_4X:
        return "wow-cataclysm";
    case GameProfile::WOW_5X:
        return "wow-mop";
    case GameProfile::STARCRAFT2:
        return "starcraft2";
    case GameProfile::DIABLO3:
        return "diablo3";
    default:
        return "generic";
    }
}

std::vector<std::string> GameRules::GetCanonicalProfiles() {
    static const std::vector<GameProfile> all_profiles = {
        GameProfile::GENERIC,    GameProfile::DIABLO1,       GameProfile::LORDSOFMAGIC,
        GameProfile::STARCRAFT1, GameProfile::WARCRAFT2,     GameProfile::DIABLO2,
        GameProfile::WARCRAFT3,  GameProfile::WARCRAFT3_MAP, GameProfile::WOW_1X,
        GameProfile::WOW_2X,     GameProfile::WOW_3X,        GameProfile::WOW_4X,
        GameProfile::WOW_5X,     GameProfile::STARCRAFT2,    GameProfile::DIABLO3,
    };

    std::vector<std::string> profiles;
    profiles.reserve(all_profiles.size());
    for (const auto &p : all_profiles) {
        profiles.push_back(ProfileToString(p));
    }
    return profiles;
}

std::string GameRules::GetAvailableProfiles() {
    auto profiles = GetCanonicalProfiles();
    std::string result;

    for (size_t i = 0; i < profiles.size(); ++i) {
        result += profiles[i];
        if (i < profiles.size() - 1) {
            result += ", ";
        }
    }

    return result;
}

} // namespace mpqcli
