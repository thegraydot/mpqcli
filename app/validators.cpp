#include "validators.h"

#include <string>

#include "gamerules/rules.h"
#include "util/locales.h"

using namespace mpqcli;

// Accepts every profile name but lists only the canonical ones on failure
extern const CLI::Validator game_profile_valid = CLI::Validator(
    [](const std::string &str) {
        if (str == "default")
            return std::string();

        const GameProfile profile = GameRules::StringToProfile(str);

        // StringToProfile returns GENERIC for an unrecognised name, so the only
        // way to tell a real "generic" from a miss is to compare the input
        if (profile == GameProfile::GENERIC && str != "generic") {
            std::string valid_profiles = "Game profile must be one of:";
            for (const auto &p : GameRules::GetCanonicalProfiles()) {
                valid_profiles += " " + p;
            }
            return valid_profiles;
        }
        return std::string();
    },
    "", "GameProfileValidator");

extern const CLI::Validator locale_valid = CLI::Validator(
    [](const std::string &str) {
        if (str == "default")
            return std::string();

        if (ParseHexLocale(str) != default_locale) {
            return std::string();
        }

        const LCID locale = LangToLocale(str);
        if (locale == 0) {
            std::string valid_locales = "Locale must be nothing, or one of:";
            for (const auto &l : GetAllLocales()) {
                valid_locales += " " + l;
            }
            return valid_locales;
        }
        return std::string();
    },
    "", "LocaleValidator");
