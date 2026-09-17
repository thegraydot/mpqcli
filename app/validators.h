#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <CLI/CLI.hpp>

#include "gamerules.h"
#include "locales.h"

// Defined in gamerules.cpp
extern const CLI::Validator game_profile_valid;

// Inline locale validator
inline const auto locale_valid = CLI::Validator(
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

#endif // VALIDATORS_H
