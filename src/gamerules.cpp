#include "gamerules.h"

#include <algorithm>
#include <cctype>
#include <map>

#include <CLI/CLI.hpp>

// Constructor
GameRules::GameRules(GameProfile game_profile) : profile_(game_profile) {
    InitializeRules();
}

// Helper function to convert string to lowercase
static std::string ToLower(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Helper function to match wildcards (* and ?)
bool GameRules::MatchFileMask(const std::string &filename, const std::string &mask) {
    // Convert both to lowercase for case-insensitive matching
    std::string lower_filename = ToLower(filename);
    std::string lower_mask = ToLower(mask);

    // Replace backslashes with forward slashes for consistent path handling
    std::replace(lower_filename.begin(), lower_filename.end(), '\\', '/');
    std::replace(lower_mask.begin(), lower_mask.end(), '\\', '/');

    // Simple wildcard matching
    size_t mask_pos = 0;
    size_t file_pos = 0;
    size_t star_pos = std::string::npos;
    size_t match_pos = 0;

    while (file_pos < lower_filename.length()) {
        if (mask_pos < lower_mask.length() &&
            (lower_mask[mask_pos] == '?' || lower_mask[mask_pos] == lower_filename[file_pos])) {
            mask_pos++;
            file_pos++;
        } else if (mask_pos < lower_mask.length() && lower_mask[mask_pos] == '*') {
            star_pos = mask_pos;
            match_pos = file_pos;
            mask_pos++;
        } else if (star_pos != std::string::npos) {
            mask_pos = star_pos + 1;
            match_pos++;
            file_pos = match_pos;
        } else {
            return false;
        }
    }

    while (mask_pos < lower_mask.length() && lower_mask[mask_pos] == '*') {
        mask_pos++;
    }

    return mask_pos == lower_mask.length();
}

void GameRules::AddRuleByFileMask(const std::string &file_mask, DWORD mpq_flags,
                                  DWORD compression_first, DWORD compression_next) {
    rules_.emplace_back(file_mask, mpq_flags, compression_first, compression_next);
}

// Use UINT32_MAX for sizeMax to indicate "no upper limit"
// Examples:
//   AddRuleByFileSize(0, 0, ...)                - Match files with exactly 0 bytes
//   AddRuleByFileSize(0, 0x4000, ...)           - Match files from 0 to 16KB
//   AddRuleByFileSize(0x4000, UINT32_MAX, ...)  - Match files from 16KB onwards
void GameRules::AddRuleByFileSize(DWORD size_min, DWORD size_max, DWORD mpq_flags,
                                  DWORD compression_first, DWORD compression_next) {
    rules_.emplace_back(size_min, size_max, mpq_flags, compression_first, compression_next);
}

void GameRules::AddRuleDefault(DWORD mpq_flags, DWORD compression_first, DWORD compression_next) {
    rules_.emplace_back(mpq_flags, compression_first, compression_next);
}

// Get compression settings for a specific file
CompressionSettings GameRules::GetCompressionSettings(const std::string &filename,
                                                      const DWORD file_size) const {
    // Iterate through rules in order (first match wins)
    for (const auto &rule : rules_) {
        switch (rule.type) {
        case RuleType::FILE_MASK:
            if (MatchFileMask(filename, rule.file_mask)) {
                return {rule.mpq_flags, rule.compression_first, rule.compression_next};
            }
            break;

        case RuleType::FILE_SIZE: {
            // Use UINT32_MAX to indicate "no upper limit"
            bool has_upper_limit = (rule.size_max != UINT32_MAX);
            bool in_range =
                file_size >= rule.size_min && (!has_upper_limit || file_size <= rule.size_max);

            if (in_range) {
                return {rule.mpq_flags, rule.compression_first, rule.compression_next};
            }
            break;
        }

        case RuleType::DEFAULT:
            return {rule.mpq_flags, rule.compression_first, rule.compression_next};
        }
    }

    // Fallback if no rules match (shouldn't happen if DEFAULT rule is present)
    return {MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED, MPQ_COMPRESSION_PKWARE,
            MPQ_COMPRESSION_NEXT_SAME};
}

// Override MPQ creation settings with user-provided values
void GameRules::OverrideCreateSettings(const MpqCreateSettingsOverrides &overrides) {
    // Track whether user explicitly set fileFlags2 (needed for automatic adjustment logic)
    bool user_set_file_flags2 = false;

    // Step 1: Apply user overrides
    // User-provided values always take priority, even if they might be incorrect.
    // We only apply override if the optional has a value (i.e., user specified it)

    if (overrides.mpq_version.has_value()) {
        create_settings_.mpq_version = overrides.mpq_version.value();
    }

    if (overrides.stream_flags.has_value()) {
        create_settings_.stream_flags = overrides.stream_flags.value();
    }

    if (overrides.sector_size.has_value()) {
        create_settings_.sector_size = overrides.sector_size.value();
    }

    if (overrides.raw_chunk_size.has_value()) {
        create_settings_.raw_chunk_size = overrides.raw_chunk_size.value();
    }

    if (overrides.file_flags1.has_value()) {
        create_settings_.file_flags1 = overrides.file_flags1.value();
    }

    if (overrides.file_flags2.has_value()) {
        create_settings_.file_flags2 = overrides.file_flags2.value();
        user_set_file_flags2 = true; // User explicitly set this value
    }

    if (overrides.file_flags3.has_value()) {
        create_settings_.file_flags3 = overrides.file_flags3.value();
    }

    if (overrides.attr_flags.has_value()) {
        create_settings_.attr_flags = overrides.attr_flags.value();
    }

    // Step 2: Apply automatic adjustments based on dependencies
    // These only apply if the user hasn't explicitly overridden the values

    // fileFlags2 controls the (attributes) file, which is only meaningful when
    // attrFlags is also set. According to StormLib's SFileCreateArchive.cpp:
    // - The (attributes) file is created only when BOTH fileFlags2 AND attrFlags are non-zero
    // - If attrFlags is set but fileFlags2 is still 0 (not overridden by user or profile),
    //   we should set fileFlags2 to MPQ_FILE_DEFAULT_INTERNAL to enable the attributes file

    if (!user_set_file_flags2 && create_settings_.file_flags2 == 0 &&
        create_settings_.attr_flags != 0) {
        // User wants attributes (attrFlags is set) but hasn't specified how to store
        // the (attributes) file itself. Use the default internal file flags.
        create_settings_.file_flags2 = MPQ_FILE_DEFAULT_INTERNAL;
    }

    // Note: If user explicitly sets fileFlags2 to 0 via override, we respect that choice
    // even if attrFlags is non-zero.
}

// Get the profile name map (single source of truth for all valid profile names)
static const std::map<std::string, GameProfile> &GetProfileMap() {
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

// Convert string to GameProfile enum
GameProfile GameRules::StringToProfile(const std::string &profile_name) {
    const auto &profile_map = GetProfileMap();
    std::string lower = ToLower(profile_name);
    auto it = profile_map.find(lower);
    if (it != profile_map.end()) {
        return it->second;
    }
    return GameProfile::GENERIC;
}

// Convert GameProfile enum to string
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

// Get list of canonical game profile names (for display purposes)
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

// Get available profiles as a comma-separated string
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

// Validator for CLI11 - accepts all profile names but only displays canonical ones
extern const CLI::Validator game_profile_valid = CLI::Validator(
    [](const std::string &str) {
        if (str == "default")
            return std::string();

        // Try to convert the string to a profile
        GameProfile profile = GameRules::StringToProfile(str);

        // If it's GENERIC and the input wasn't "generic", it means the profile wasn't found
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

// Initialize rules for the selected game profile
void GameRules::InitializeRules() {
    rules_.clear();

    switch (profile_) {
    case GameProfile::DIABLO1:
    case GameProfile::LORDSOFMAGIC:
        // File rules when adding files to archive:
        AddRuleByFileMask("*.wav", MPQ_FILE_ENCRYPTED, 0x00, 0x00);
        AddRuleByFileMask("*.smk", 0x00000000, 0x00, 0x00);
        AddRuleByFileMask("*.bik", 0x00000000, 0x00, 0x00);
        AddRuleByFileMask("*.mpq", MPQ_FILE_ENCRYPTED, 0x00, 0x00);
        AddRuleByFileMask("game", MPQ_FILE_IMPLODE, 0x00, 0x00);
        AddRuleByFileMask("hero", MPQ_FILE_IMPLODE, 0x00, 0x00);
        AddRuleDefault(MPQ_FILE_IMPLODE | MPQ_FILE_ENCRYPTED, MPQ_COMPRESSION_PKWARE);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_1;
        create_settings_.sector_size = 0x1000;
        break;

    case GameProfile::WARCRAFT2:
    case GameProfile::STARCRAFT1:
        // File rules when adding files to archive:
        AddRuleByFileMask("*.wav", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_PKWARE,
                          MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_STEREO);
        AddRuleByFileMask("*.smk", 0x00000000, 0x00, 0x00);
        AddRuleByFileMask("*.bik", 0x00000000, 0x00, 0x00);
        AddRuleByFileMask("*.mpq", 0x00000000, 0x00, 0x00);
        AddRuleDefault(MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                       MPQ_COMPRESSION_PKWARE);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_1;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS | MPQ_FILE_SECTOR_CRC;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS | MPQ_FILE_SECTOR_CRC;
        create_settings_.sector_size = 0x1000;
        break;

    case GameProfile::DIABLO2:
        // File rules when adding files to archive:
        AddRuleByFileMask("*.wav", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_PKWARE,
                          MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_STEREO);
        AddRuleByFileMask("*.d2", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.txt", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.dc6", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.tbl", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.map", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.key", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.dat", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.ds1", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.dcc", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.cof", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.dt1", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.pl2", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.dn1", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleByFileMask("*.ico", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_PKWARE);
        AddRuleDefault(MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                       MPQ_COMPRESSION_PKWARE);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_1;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.sector_size = 0x1000;
        break;

    case GameProfile::WARCRAFT3:
        // File rules when adding files to archive:
        AddRuleByFileMask("Abilities\\*.wav", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB,
                          MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO);
        AddRuleByFileMask("Buildings\\*.wav", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB,
                          MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO);
        AddRuleByFileMask("*.wav", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_ZLIB,
                          MPQ_COMPRESSION_HUFFMANN | MPQ_COMPRESSION_ADPCM_MONO);

        AddRuleByFileMask("ReplaceableTextures\\WorldEditUI\\*.blp", MPQ_FILE_COMPRESS,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("ReplaceableTextures\\Selection\\*.blp", MPQ_FILE_COMPRESS,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("ReplaceableTextures\\Shadows\\*.blp", MPQ_FILE_COMPRESS,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("UI\\Glues\\Loading\\Backgrounds\\*.blp", 0, 0);
        AddRuleByFileMask("UI\\Glues\\Loading\\Multiplayer\\*.blp", 0, 0);
        AddRuleByFileMask("UI\\*.blp", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.blp", 0, 0);

        AddRuleByFileMask("Maps\\Campaign\\*.w3m", 0, 0);
        AddRuleByFileMask("*.w3m", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_PKWARE);

        AddRuleByFileMask("*.toc", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.ifl", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.mdx", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.tga", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.slk", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.ai", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.j", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.txt", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.fdf", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.pld", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.mid", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.dls", MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.mpq", 0, 0);
        AddRuleByFileMask("*.mp3", 0, 0);

        AddRuleDefault(MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED | MPQ_FILE_KEY_V2,
                       MPQ_COMPRESSION_PKWARE);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_1;
        create_settings_.sector_size = 0x1000;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.attr_flags = MPQ_ATTRIBUTE_FILETIME | MPQ_ATTRIBUTE_CRC32;
        break;

    case GameProfile::WARCRAFT3_MAP: // Warcraft III Map files
        // File rules when adding files to archive:
        AddRuleDefault(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_1;
        create_settings_.sector_size = 0x1000;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.attr_flags = MPQ_ATTRIBUTE_FILETIME | MPQ_ATTRIBUTE_CRC32;
        break;

    case GameProfile::WOW_1X:
        // File rules when adding files to archive:
        AddRuleByFileMask("*.mp3", 0, 0);
        AddRuleDefault(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_1;
        create_settings_.sector_size = 0x1000;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.attr_flags =
            MPQ_ATTRIBUTE_FILETIME | MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_MD5;
        break;

    case GameProfile::WOW_2X:
    case GameProfile::WOW_3X:
        // File rules when adding files to archive:
        AddRuleByFileMask("*.mp3", 0, 0);
        AddRuleDefault(MPQ_FILE_COMPRESS | MPQ_FILE_SECTOR_CRC, MPQ_COMPRESSION_ZLIB);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_2;
        create_settings_.sector_size = 0x1000;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.attr_flags =
            MPQ_ATTRIBUTE_FILETIME | MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_MD5;
        break;

    case GameProfile::WOW_4X:
    case GameProfile::WOW_5X:
        // File rules when adding files to archive:
        AddRuleByFileSize(0, 0, MPQ_FILE_DELETE_MARKER, 0);
        AddRuleByFileMask("*.mp3", 0, 0);
        AddRuleByFileMask("*.ogg", 0, 0);
        AddRuleByFileMask("*.ogv", 0, 0);
        AddRuleByFileSize(0, 0x4000, MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleDefault(MPQ_FILE_COMPRESS | MPQ_FILE_SECTOR_CRC, MPQ_COMPRESSION_ZLIB);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_4;
        create_settings_.raw_chunk_size = 0x4000;
        create_settings_.sector_size = 0x4000;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.attr_flags = MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_MD5;
        break;

    case GameProfile::STARCRAFT2:
        // File rules when adding files to archive:
        AddRuleByFileSize(0, 0, MPQ_FILE_DELETE_MARKER, 0);
        AddRuleByFileMask("*.mp3", 0, 0);
        AddRuleByFileMask("*.ogg", 0, 0);
        AddRuleByFileMask("*.ogv", 0, 0);
        AddRuleByFileSize(0, 0x4000, MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleByFileMask("*.wav", MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);
        AddRuleDefault(MPQ_FILE_COMPRESS | MPQ_FILE_SECTOR_CRC, MPQ_COMPRESSION_ZLIB);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_2;
        create_settings_.sector_size = 0x4000;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.attr_flags = MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_MD5;
        break;

    case GameProfile::DIABLO3:
        // File rules when adding files to archive:
        AddRuleByFileSize(0, 0, MPQ_FILE_DELETE_MARKER, 0);
        AddRuleByFileMask("*.mp3", 0, 0);
        AddRuleByFileMask("*.ogg", 0, 0);
        AddRuleByFileMask("*.ogv", 0, 0);
        AddRuleByFileSize(0, 0x4000, MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT,
                          MPQ_COMPRESSION_ZLIB);
        AddRuleDefault(MPQ_FILE_COMPRESS, MPQ_COMPRESSION_ZLIB);

        // Settings for archive creation:
        create_settings_.mpq_version = MPQ_FORMAT_VERSION_4;
        create_settings_.raw_chunk_size = 0x4000;
        create_settings_.sector_size = 0x4000;
        create_settings_.file_flags1 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.file_flags2 = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS;
        create_settings_.attr_flags = MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_MD5;
        break;

    case GameProfile::GENERIC:
    default:
        // File rules when adding files to archive:
        AddRuleDefault(MPQ_FILE_COMPRESS | MPQ_FILE_ENCRYPTED, MPQ_COMPRESSION_PKWARE);

        // For settings for archive creation, use defaults from MpqCreateSettings constructor
        break;
    }
}
