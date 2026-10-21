#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include <StormLib.h>

#include "gamerules/profiles.h"
#include "gamerules/rules.h"
#include "gamerules/settings.h"

using mpqcli::CompressionSettings;
using mpqcli::GameProfile;
using mpqcli::GameRules;
using mpqcli::MpqCreateSettings;
using mpqcli::MpqCreateSettingsOverrides;

namespace {

// The three fields as one hex string, so a failed comparison shows all of them
std::string Describe(DWORD flags, DWORD compression_first,
                     DWORD compression_next = MPQ_COMPRESSION_NEXT_SAME) {
    std::ostringstream text;
    text << std::hex << std::setfill('0') << "flags 0x" << std::setw(8) << flags << ", first 0x"
         << std::setw(8) << compression_first << ", next 0x" << std::setw(8) << compression_next;
    return text.str();
}

std::string Describe(const CompressionSettings &settings) {
    return Describe(settings.mpq_flags, settings.compression_first, settings.compression_next);
}

} // namespace

TEST_CASE("GameRules::GetCompressionSettings", "[rules]") {
    SECTION("compresses and encrypts every file under the generic profile") {
        const GameRules rules(GameProfile::GENERIC);
        const std::string expected =
            Describe(MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED, MPQ_COMPRESSION_PKWARE);
        REQUIRE(Describe(rules.GetCompressionSettings("cats.txt", 27U)) == expected);
        REQUIRE(Describe(rules.GetCompressionSettings("sound\\a.wav", 0x10000U)) == expected);
    }

    SECTION("matches a mask by extension") {
        const GameRules rules(GameProfile::DIABLO2);
        REQUIRE(Describe(rules.GetCompressionSettings("data\\global\\excel\\armor.txt", 100U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE));
    }

    SECTION("matches a mask whatever the case of name or mask") {
        const GameRules rules(GameProfile::DIABLO2);
        REQUIRE(Describe(rules.GetCompressionSettings("DATA\\ARMOR.TXT", 100U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE));

        const GameRules warcraft3(GameProfile::WARCRAFT3);
        REQUIRE(Describe(warcraft3.GetCompressionSettings("abilities\\a.WAV", 100U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB,
                         MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO));
    }

    SECTION("matches a mask written with either separator") {
        const GameRules rules(GameProfile::WARCRAFT3);
        const std::string expected =
            Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB,
                     MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO);
        REQUIRE(Describe(rules.GetCompressionSettings("Abilities\\a.wav", 100U)) == expected);
        REQUIRE(Describe(rules.GetCompressionSettings("Abilities/a.wav", 100U)) == expected);
    }

    SECTION("lets a wildcard span directory separators") {
        const GameRules rules(GameProfile::WARCRAFT3);
        REQUIRE(Describe(rules.GetCompressionSettings("Abilities\\Spells\\Human\\a.wav", 100U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB,
                         MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO));
    }

    SECTION("matches a mask with no wildcard against the whole name only") {
        const GameRules rules(GameProfile::DIABLO1);
        const std::string exact = Describe(MPQ_FILE_IMPLODE, 0U, 0U);
        const std::string fallback =
            Describe(MPQ_FILE_IMPLODE | MPQ_FILE_ENCRYPTED, MPQ_COMPRESSION_PKWARE);
        REQUIRE(Describe(rules.GetCompressionSettings("game", 100U)) == exact);
        REQUIRE(Describe(rules.GetCompressionSettings("GAME", 100U)) == exact);
        REQUIRE(Describe(rules.GetCompressionSettings("game.txt", 100U)) == fallback);
        REQUIRE(Describe(rules.GetCompressionSettings("endgame", 100U)) == fallback);
    }

    SECTION("applies the first matching mask") {
        const GameRules rules(GameProfile::WARCRAFT3);
        REQUIRE(Describe(rules.GetCompressionSettings("Abilities\\a.wav", 100U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB,
                         MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO));
        REQUIRE(Describe(rules.GetCompressionSettings("Sound\\a.wav", 100U)) ==
                Describe(MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                         MPQ_COMPRESSION_ZLIB,
                         MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO));

        REQUIRE(Describe(rules.GetCompressionSettings("UI\\Glues\\Loading\\Backgrounds\\a.blp",
                                                      100U)) == Describe(0U, 0U));
        REQUIRE(Describe(rules.GetCompressionSettings("UI\\Widgets\\a.blp", 100U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB));
        REQUIRE(Describe(rules.GetCompressionSettings("Textures\\a.blp", 100U)) ==
                Describe(0U, 0U));
    }

    SECTION("applies a size rule ahead of a later mask") {
        const GameRules rules(GameProfile::STARCRAFT2);
        REQUIRE(Describe(rules.GetCompressionSettings("a.wav", 100U)) ==
                Describe(MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT, MPQ_COMPRESSION_ZLIB));
        REQUIRE(Describe(rules.GetCompressionSettings("a.wav", 0x5000U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB));
    }

    SECTION("applies a mask ahead of a later size rule") {
        const GameRules rules(GameProfile::WOW_4X);
        REQUIRE(Describe(rules.GetCompressionSettings("a.mp3", 100U)) == Describe(0U, 0U));
    }

    SECTION("includes both bounds of a size rule") {
        const GameRules rules(GameProfile::WOW_4X);
        const std::string single_unit =
            Describe(MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT, MPQ_COMPRESSION_ZLIB);
        const std::string fallback =
            Describe(MPQ_FILE_COMPRESS | MPQ_FILE_SECTOR_CRC, MPQ_COMPRESSION_ZLIB);
        REQUIRE(Describe(rules.GetCompressionSettings("a.dat", 1U)) == single_unit);
        REQUIRE(Describe(rules.GetCompressionSettings("a.dat", 0x4000U)) == single_unit);
        REQUIRE(Describe(rules.GetCompressionSettings("a.dat", 0x4001U)) == fallback);
        REQUIRE(Describe(rules.GetCompressionSettings("a.dat", UINT32_MAX)) == fallback);
    }

    SECTION("falls back to the profile's default rule") {
        const GameRules rules(GameProfile::WARCRAFT3_MAP);
        REQUIRE(Describe(rules.GetCompressionSettings("war3map.j", 100U)) ==
                Describe(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB));
    }
}

TEST_CASE("GameRules::GetCreateSettings", "[rules]") {
    SECTION("uses the defaults for the generic profile") {
        const MpqCreateSettings defaults;
        const GameRules rules(GameProfile::GENERIC);
        const MpqCreateSettings &settings = rules.GetCreateSettings();
        REQUIRE(settings.mpq_version == defaults.mpq_version);
        REQUIRE(settings.stream_flags == defaults.stream_flags);
        REQUIRE(settings.file_flags1 == defaults.file_flags1);
        REQUIRE(settings.file_flags2 == defaults.file_flags2);
        REQUIRE(settings.file_flags3 == defaults.file_flags3);
        REQUIRE(settings.attr_flags == defaults.attr_flags);
        REQUIRE(settings.sector_size == defaults.sector_size);
        REQUIRE(settings.raw_chunk_size == defaults.raw_chunk_size);
    }

    SECTION("writes a version 1 archive with attributes for Warcraft III") {
        const GameRules rules(GameProfile::WARCRAFT3);
        const MpqCreateSettings &settings = rules.GetCreateSettings();
        REQUIRE(settings.mpq_version == static_cast<DWORD>(MPQ_FORMAT_VERSION_1));
        REQUIRE(settings.file_flags1 == (MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS));
        REQUIRE(settings.file_flags2 == (MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS));
        REQUIRE(settings.attr_flags == (MPQ_ATTRIBUTE_FILETIME | MPQ_ATTRIBUTE_CRC32));
        REQUIRE(settings.sector_size == 0x1000U);
        REQUIRE(settings.raw_chunk_size == 0U);
    }

    SECTION("writes a version 4 archive with raw chunks for Cataclysm") {
        const GameRules rules(GameProfile::WOW_4X);
        const MpqCreateSettings &settings = rules.GetCreateSettings();
        REQUIRE(settings.mpq_version == static_cast<DWORD>(MPQ_FORMAT_VERSION_4));
        REQUIRE(settings.attr_flags == (MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_MD5));
        REQUIRE(settings.sector_size == 0x4000U);
        REQUIRE(settings.raw_chunk_size == 0x4000U);
    }
}

TEST_CASE("GameRules::OverrideCreateSettings", "[rules]") {
    SECTION("leaves the profile's settings alone when nothing is overridden") {
        GameRules rules(GameProfile::WOW_4X);
        const MpqCreateSettings before = rules.GetCreateSettings();
        rules.OverrideCreateSettings(MpqCreateSettingsOverrides{});
        const MpqCreateSettings &after = rules.GetCreateSettings();
        REQUIRE(after.mpq_version == before.mpq_version);
        REQUIRE(after.stream_flags == before.stream_flags);
        REQUIRE(after.file_flags1 == before.file_flags1);
        REQUIRE(after.file_flags2 == before.file_flags2);
        REQUIRE(after.file_flags3 == before.file_flags3);
        REQUIRE(after.attr_flags == before.attr_flags);
        REQUIRE(after.sector_size == before.sector_size);
        REQUIRE(after.raw_chunk_size == before.raw_chunk_size);
    }

    SECTION("replaces each field that is overridden") {
        GameRules rules(GameProfile::GENERIC);
        MpqCreateSettingsOverrides overrides;
        overrides.mpq_version = 2U;
        overrides.stream_flags = 3U;
        overrides.file_flags1 = 4U;
        overrides.file_flags2 = 5U;
        overrides.file_flags3 = 6U;
        overrides.attr_flags = 7U;
        overrides.sector_size = 0x8000U;
        overrides.raw_chunk_size = 0x9000U;
        rules.OverrideCreateSettings(overrides);

        const MpqCreateSettings &settings = rules.GetCreateSettings();
        REQUIRE(settings.mpq_version == 2U);
        REQUIRE(settings.stream_flags == 3U);
        REQUIRE(settings.file_flags1 == 4U);
        REQUIRE(settings.file_flags2 == 5U);
        REQUIRE(settings.file_flags3 == 6U);
        REQUIRE(settings.attr_flags == 7U);
        REQUIRE(settings.sector_size == 0x8000U);
        REQUIRE(settings.raw_chunk_size == 0x9000U);
    }

    SECTION("gives attributes asked for without a storage flag the default one") {
        GameRules rules(GameProfile::GENERIC);
        MpqCreateSettingsOverrides overrides;
        overrides.attr_flags = MPQ_ATTRIBUTE_CRC32;
        rules.OverrideCreateSettings(overrides);
        REQUIRE(rules.GetCreateSettings().file_flags2 == MPQ_FILE_DEFAULT_INTERNAL);
    }

    SECTION("keeps a storage flag of zero the user gave explicitly") {
        GameRules rules(GameProfile::GENERIC);
        MpqCreateSettingsOverrides overrides;
        overrides.attr_flags = MPQ_ATTRIBUTE_CRC32;
        overrides.file_flags2 = 0U;
        rules.OverrideCreateSettings(overrides);
        REQUIRE(rules.GetCreateSettings().file_flags2 == 0U);
    }

    SECTION("keeps the storage flag a profile already sets") {
        GameRules rules(GameProfile::WARCRAFT3);
        MpqCreateSettingsOverrides overrides;
        overrides.attr_flags = MPQ_ATTRIBUTE_MD5;
        rules.OverrideCreateSettings(overrides);
        REQUIRE(rules.GetCreateSettings().file_flags2 == (MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS));
    }

    SECTION("adds no storage flag when no attributes are asked for") {
        GameRules rules(GameProfile::GENERIC);
        MpqCreateSettingsOverrides overrides;
        overrides.sector_size = 0x4000U;
        rules.OverrideCreateSettings(overrides);
        REQUIRE(rules.GetCreateSettings().file_flags2 == 0U);
    }
}

TEST_CASE("GameRules::GetDefaultProfile", "[rules]") {
    SECTION("is the generic profile") {
        REQUIRE(GameRules::GetDefaultProfile() == GameProfile::GENERIC);
    }
}
