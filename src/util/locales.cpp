#include "util/locales.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace mpqcli {

namespace {
// Files with the same name can coexist under different locales; the identifiers
// are Windows LCIDs, listed in MS-LCID:
// https://winprotocoldoc.z19.web.core.windows.net/MS-LCID/%5bMS-LCID%5d.pdf

const std::map<LCID, std::string> locale_to_lang_map = {
    {0x000, "enUS"}, // Default - English (US)
    {0x404, "zhTW"}, // Chinese (Taiwan)
    {0x405, "csCZ"}, // Czech
    {0x407, "deDE"}, // German
    {0x409, "enUS"}, // English (US)
    {0x40a, "esES"}, // Spanish (Spain)
    {0x40c, "frFR"}, // French
    {0x410, "itIT"}, // Italian
    {0x411, "jaJP"}, // Japanese
    {0x412, "koKR"}, // Korean
    {0x413, "nlNL"}, // Dutch
    {0x415, "plPL"}, // Polish
    {0x416, "ptBR"}, // Portuguese (Brazil)
    {0x419, "ruRU"}, // Russian
    {0x804, "zhCN"}, // Chinese (Simplified)
    {0x809, "enGB"}, // English (UK)
    {0x80A, "esMX"}, // Spanish (Mexico)
    {0x816, "ptPT"}, // Portuguese (Portugal)
};

const std::map<std::string, LCID> lang_to_locale_map = []() {
    std::map<std::string, LCID> reverse_map;
    for (const auto &[locale, lang] : locale_to_lang_map) {
        if (locale != default_locale) { // 0x000 prints as enUS but enUS resolves to 0x409
            reverse_map[lang] = locale;
        }
    }
    return reverse_map;
}();

std::string FormatLocaleAsHex(const LCID locale) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << locale;
    const std::string hex_str = ss.str();
    // Pad to four digits, guarding the subtraction: an LCID wider than 0xFFFF
    // formats to more than four characters and would underflow the length
    if (hex_str.length() >= 4) {
        return hex_str;
    }
    return std::string(4 - hex_str.length(), '0') + hex_str;
}
} // namespace

LCID ParseHexLocale(const std::string &str) {
    if (str.length() != 4) {
        return default_locale;
    }

    for (char c : str) {
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            return default_locale;
        }
    }

    std::stringstream ss;
    ss << std::hex << str;
    LCID locale;
    ss >> locale;
    return locale;
}

std::string LocaleToLang(const LCID locale) {
    auto it = locale_to_lang_map.find(locale);
    return it != locale_to_lang_map.end() ? it->second : FormatLocaleAsHex(locale);
}

LCID LangToLocale(const std::string &lang) {
    auto it = lang_to_locale_map.find(lang);
    if (it != lang_to_locale_map.end()) {
        return it->second;
    }

    LCID hex_locale = ParseHexLocale(lang);
    if (hex_locale != default_locale) {
        return hex_locale;
    }

    return default_locale;
}

std::vector<std::string> GetAllLocales() {
    std::vector<std::string> locales;
    for (const auto &[locale, lang] : locale_to_lang_map) {
        if (locale != default_locale) { // enUS would otherwise be listed twice
            locales.push_back(lang);
        }
    }
    std::sort(locales.begin(), locales.end());
    return locales;
}

std::string PrettyPrintLocale(const LCID locale, const std::string &prefix, bool always_print) {
    if (locale == default_locale && !always_print) {
        return "";
    }
    const auto lang = LocaleToLang(locale);
    return prefix + lang;
}

} // namespace mpqcli
