#include "validators.h"

#include <cstdint>
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

        // The same resolution every command uses, a name in any case or else four hex digits
        if (LangToLocale(str) == default_locale) {
            std::string valid_locales = "Locale must be nothing, or one of:";
            for (const auto &l : GetAllLocales()) {
                valid_locales += " " + l;
            }
            return valid_locales;
        }
        return std::string();
    },
    "", "LocaleValidator");

// StormLib's create stores a 512-byte sector as a shift of 0, which its own open
// rejects as a bad format, so 1024 is the smallest size that can be read back. A
// size that is not a power of two would otherwise fail inside StormLib with no more
// than "Invalid parameter".
extern const CLI::Validator sector_size_valid = CLI::Validator(
    [](const std::string &str) {
        // Parsed the way CLI11 parses the option itself, so hexadecimal input works
        std::uint32_t size = 0;
        if (!CLI::detail::lexical_cast(str, size) || size < 1024 || (size & (size - 1)) != 0) {
            return std::string("Sector size must be a power of two of at least 1024");
        }
        return std::string();
    },
    "", "SectorSizeValidator");
