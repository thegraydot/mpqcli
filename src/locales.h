#ifndef LOCALES_H
#define LOCALES_H

#include <filesystem>
#include <string>
#include <vector>

#include <StormLib.h>

inline constexpr LCID default_locale = 0;

std::string LocaleToLang(uint16_t locale);
LCID LangToLocale(const std::string &lang);
LCID ParseHexLocale(const std::string &str);
std::vector<std::string> GetAllLocales();
std::string PrettyPrintLocale(LCID locale, const std::string &prefix = "",
                              bool always_print = false);

#endif // LOCALES_H
