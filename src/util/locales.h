#ifndef UTIL_LOCALES_H
#define UTIL_LOCALES_H

#include <string>
#include <vector>

#include <StormLib.h>

namespace mpqcli {

inline constexpr LCID default_locale = 0;

std::string LocaleToLang(LCID locale);
LCID LangToLocale(const std::string &lang);
/// Parses a four-digit hexadecimal locale, returning default_locale for anything else
LCID ParseHexLocale(const std::string &str);
std::vector<std::string> GetAllLocales();
std::string PrettyPrintLocale(LCID locale, const std::string &prefix = "",
                              bool always_print = false);

} // namespace mpqcli

#endif
