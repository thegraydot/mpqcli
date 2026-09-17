#ifndef GAMERULES_RULES_H
#define GAMERULES_RULES_H

#include <string>
#include <vector>

#include <StormLib.h>

#include "gamerules/profiles.h"
#include "gamerules/settings.h"

namespace mpqcli {

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

} // namespace mpqcli

#endif
