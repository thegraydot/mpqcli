#ifndef GAMERULES_H
#define GAMERULES_H

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <StormLib.h>

enum class GameProfile {
    GENERIC,       // Default/generic MPQ with basic compression
    DIABLO1,       // Diablo I / Hellfire (1997)
    LORDSOFMAGIC,  // Lords of Magic SE (1998)
    STARCRAFT1,    // StarCraft / Brood War (1998)
    WARCRAFT2,     // Warcraft II: Battle.net Edition (1999)
    DIABLO2,       // Diablo II / Lords of Destruction (2000)
    WARCRAFT3,     // Warcraft III / The Frozen Throne (2002)
    WARCRAFT3_MAP, // Warcraft III Map files (2002)
    WOW_1X,        // World of Warcraft 1 - Vanilla (2004)
    WOW_2X,        // World of Warcraft 2 - The Burning Crusade (2007)
    WOW_3X,        // World of Warcraft 3 - Wrath of the Lich King (2008)
    WOW_4X,        // World of Warcraft 4 - Cataclysm (2010)
    WOW_5X,        // World of Warcraft 5 - Mists of Pandaria (2012)
    STARCRAFT2,    // StarCraft II (2010)
    DIABLO3        // Diablo III (2012)
};

enum class RuleType {
    FILE_MASK, // Rule based on file pattern (e.g., "*.wav")
    FILE_SIZE, // Rule based on file size range
    DEFAULT    // Default rule (fallback)
};

// Structure representing a single compression rule
struct CompressionRule {
    RuleType type;
    std::string file_mask;   // For FILE_MASK rules (e.g., "*.wav", "UI\\*.blp")
    DWORD size_min;          // For FILE_SIZE rules
    DWORD size_max;          // For FILE_SIZE rules
    DWORD mpq_flags;         // MPQ file flags (compression, encryption, etc.)
    DWORD compression_first; // Compression for first sector
    DWORD compression_next;  // Compression for subsequent sectors

    CompressionRule(std::string mask, const DWORD flags, const DWORD comp_first,
                    const DWORD comp_next = MPQ_COMPRESSION_NEXT_SAME)
        : type(RuleType::FILE_MASK), file_mask(std::move(mask)), size_min(0), size_max(0),
          mpq_flags(flags), compression_first(comp_first), compression_next(comp_next) {}

    CompressionRule(const DWORD min_size, const DWORD max_size, const DWORD flags,
                    const DWORD comp_first, const DWORD comp_next = MPQ_COMPRESSION_NEXT_SAME)
        : type(RuleType::FILE_SIZE), file_mask(""), size_min(min_size), size_max(max_size),
          mpq_flags(flags), compression_first(comp_first), compression_next(comp_next) {}

    CompressionRule(const DWORD flags, const DWORD comp_first,
                    const DWORD comp_next = MPQ_COMPRESSION_NEXT_SAME)
        : type(RuleType::DEFAULT), file_mask(""), size_min(0), size_max(0), mpq_flags(flags),
          compression_first(comp_first), compression_next(comp_next) {}
};

// Structure to hold compression settings for a file
struct CompressionSettings {
    DWORD mpq_flags;
    DWORD compression_first;
    DWORD compression_next;
};

// Structure to hold optional override settings for adding files
struct CompressionSettingsOverrides {
    std::optional<DWORD> flags;
    std::optional<DWORD> compression;
    std::optional<DWORD> compression_next;
};

// Structure to hold MPQ archive creation settings
struct MpqCreateSettings {
    DWORD mpq_version;    // MPQ format version (1, 2, 3, or 4)
    DWORD stream_flags;   // Stream flags (e.g., STREAM_PROVIDER_FLAT)
    DWORD file_flags1;    // File flags for (listfile)
    DWORD file_flags2;    // File flags for (attributes)
    DWORD file_flags3;    // File flags for (signature)
    DWORD attr_flags;     // Attribute flags (CRC32, FILETIME, MD5, etc.)
    DWORD sector_size;    // Sector size (typically 0x1000 or 0x4000)
    DWORD raw_chunk_size; // Raw chunk size (for MPQ v4, typically 0x4000)

    // Constructor with defaults
    MpqCreateSettings()
        : mpq_version(MPQ_FORMAT_VERSION_1),
          stream_flags(STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE),
          file_flags1(MPQ_FILE_DEFAULT_INTERNAL), file_flags2(0), file_flags3(0), attr_flags(0),
          sector_size(0x1000), raw_chunk_size(0) {}
};

// Structure to hold optional override settings for MPQ archive creation
struct MpqCreateSettingsOverrides {
    std::optional<DWORD> mpq_version;
    std::optional<DWORD> stream_flags;
    std::optional<DWORD> file_flags1;
    std::optional<DWORD> file_flags2;
    std::optional<DWORD> file_flags3;
    std::optional<DWORD> attr_flags;
    std::optional<DWORD> sector_size;
    std::optional<DWORD> raw_chunk_size;
};

// Game rules class that manages compression rules for different games
class GameRules {
private:
    GameProfile profile_;
    std::vector<CompressionRule> rules_;
    MpqCreateSettings create_settings_;

    // Helper function to match file mask pattern
    static bool MatchFileMask(const std::string &filename, const std::string &mask);

    // Add rule by file mask
    void AddRuleByFileMask(const std::string &file_mask, DWORD mpq_flags, DWORD compression_first,
                           DWORD compression_next = MPQ_COMPRESSION_NEXT_SAME);

    // Add rule by file size
    void AddRuleByFileSize(DWORD size_min, DWORD size_max, DWORD mpq_flags, DWORD compression_first,
                           DWORD compression_next = MPQ_COMPRESSION_NEXT_SAME);

    // Add default rule
    void AddRuleDefault(DWORD mpq_flags, DWORD compression_first,
                        DWORD compression_next = MPQ_COMPRESSION_NEXT_SAME);

    // Initialize rules for the selected game profile
    void InitializeRules();

    // Convert GameProfile enum to string
    static std::string ProfileToString(GameProfile profile);

public:
    // Constructor
    explicit GameRules(GameProfile game_profile);

    // Get compression settings for a specific file
    [[nodiscard]] CompressionSettings GetCompressionSettings(const std::string &filename,
                                                             DWORD file_size) const;

    // Get MPQ creation settings
    [[nodiscard]] const MpqCreateSettings &GetCreateSettings() const { return create_settings_; }

    // Override MPQ creation settings
    void OverrideCreateSettings(const MpqCreateSettingsOverrides &overrides);

    // Convert string to GameProfile enum
    static GameProfile StringToProfile(const std::string &profile_name);

    // Get list of canonical game profile names (for display purposes)
    static std::vector<std::string> GetCanonicalProfiles();

    // Get available profiles as a comma-separated string
    static std::string GetAvailableProfiles();

    // Get default game profile (GENERIC)
    static GameProfile GetDefaultProfile() { return GameProfile::GENERIC; }
};

#endif // GAMERULES_H
