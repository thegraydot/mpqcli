#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "util/locales.h"

using mpqcli::default_locale;
using mpqcli::GetAllLocales;
using mpqcli::LangToLocale;
using mpqcli::LocaleToLang;
using mpqcli::ParseHexLocale;
using mpqcli::PrettyPrintLocale;

TEST_CASE("LocaleToLang", "[locales]") {
    SECTION("names a known locale") {
        REQUIRE(LocaleToLang(0x407U) == "deDE");
        REQUIRE(LocaleToLang(0x409U) == "enUS");
        REQUIRE(LocaleToLang(0x809U) == "enGB");
        REQUIRE(LocaleToLang(0x80AU) == "esMX");
    }

    SECTION("names the default locale enUS") {
        REQUIRE(LocaleToLang(default_locale) == "enUS");
    }

    SECTION("formats an unknown locale as four uppercase hex digits") {
        REQUIRE(LocaleToLang(0x41DU) == "041D");
        REQUIRE(LocaleToLang(0x1U) == "0001");
    }

    SECTION("keeps every digit of a locale wider than four") {
        REQUIRE(LocaleToLang(0x12345U) == "12345");
    }
}

TEST_CASE("LangToLocale", "[locales]") {
    SECTION("resolves a known language name") {
        REQUIRE(LangToLocale("deDE") == 0x407U);
        REQUIRE(LangToLocale("enGB") == 0x809U);
        REQUIRE(LangToLocale("zhTW") == 0x404U);
    }

    SECTION("resolves a name whatever its case") {
        REQUIRE(LangToLocale("dede") == 0x407U);
        REQUIRE(LangToLocale("DEDE") == 0x407U);
        REQUIRE(LangToLocale("ENgb") == 0x809U);
    }

    SECTION("resolves enUS to its own locale rather than the default") {
        REQUIRE(LangToLocale("enUS") == 0x409U);
    }

    SECTION("accepts a four-digit hex locale") {
        REQUIRE(LangToLocale("041D") == 0x41DU);
    }

    SECTION("returns the default for anything else") {
        REQUIRE(LangToLocale("xxXX") == default_locale);
        REQUIRE(LangToLocale("German") == default_locale);
        REQUIRE(LangToLocale("") == default_locale);
    }
}

TEST_CASE("ParseHexLocale", "[locales]") {
    SECTION("parses four hex digits") {
        REQUIRE(ParseHexLocale("0407") == 0x407U);
        REQUIRE(ParseHexLocale("FFFF") == 0xFFFFU);
    }

    SECTION("accepts lowercase digits") {
        REQUIRE(ParseHexLocale("041d") == 0x41DU);
    }

    SECTION("returns the default for the wrong length") {
        REQUIRE(ParseHexLocale("407") == default_locale);
        REQUIRE(ParseHexLocale("04070") == default_locale);
        REQUIRE(ParseHexLocale("") == default_locale);
    }

    SECTION("returns the default for a character that is not a hex digit") {
        REQUIRE(ParseHexLocale("04G7") == default_locale);
        REQUIRE(ParseHexLocale("0x07") == default_locale);
        REQUIRE(ParseHexLocale(" 407") == default_locale);
        REQUIRE(ParseHexLocale("-407") == default_locale);
    }
}

TEST_CASE("GetAllLocales", "[locales]") {
    SECTION("lists every language once, sorted") {
        const std::vector<std::string> expected = {
            "csCZ", "deDE", "enGB", "enUS", "esES", "esMX", "frFR", "itIT", "jaJP",
            "koKR", "nlNL", "plPL", "ptBR", "ptPT", "ruRU", "zhCN", "zhTW",
        };
        REQUIRE(GetAllLocales() == expected);
    }

    SECTION("lists only names that resolve back to themselves") {
        for (const auto &lang : GetAllLocales()) {
            CAPTURE(lang);
            REQUIRE(LocaleToLang(LangToLocale(lang)) == lang);
        }
    }
}

TEST_CASE("PrettyPrintLocale", "[locales]") {
    SECTION("prints nothing for the default locale") {
        REQUIRE(PrettyPrintLocale(default_locale, " for locale ").empty());
    }

    SECTION("prints the default locale when always_print is set") {
        REQUIRE(PrettyPrintLocale(default_locale, " for locale ", true) == " for locale enUS");
    }

    SECTION("prints the prefix and name of any other locale") {
        REQUIRE(PrettyPrintLocale(0x407U, " for locale ") == " for locale deDE");
        REQUIRE(PrettyPrintLocale(0x407U) == "deDE");
    }

    SECTION("prints an unknown locale as hex") {
        REQUIRE(PrettyPrintLocale(0x41DU) == "041D");
    }
}
