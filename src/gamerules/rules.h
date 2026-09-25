#ifndef GAMERULES_RULES_H
#define GAMERULES_RULES_H

#include <string>
#include <vector>

#include <StormLib.h>

#include "gamerules/profiles.h"
#include "gamerules/settings.h"

namespace mpqcli {

/// Compression rules and archive creation settings for one game profile
class GameRules {
private:
    GameProfile profile_;
    std::vector<CompressionRule> rules_;
    MpqCreateSettings create_settings_;

    /// Matches filename against a mask with * and ? wildcards, ignoring case
    static bool MatchFileMask(const std::string &filename, const std::string &mask);

    /// Adds a rule matched by file mask
    void AddRuleByFileMask(const std::string &file_mask, DWORD mpq_flags, DWORD compression_first,
                           DWORD compression_next = MPQ_COMPRESSION_NEXT_SAME);

    /// Adds a rule matched by file size; a size_max of UINT32_MAX means no upper limit
    void AddRuleByFileSize(DWORD size_min, DWORD size_max, DWORD mpq_flags, DWORD compression_first,
                           DWORD compression_next = MPQ_COMPRESSION_NEXT_SAME);

    /// Adds the fallback rule used when no other matches
    void AddRuleDefault(DWORD mpq_flags, DWORD compression_first,
                        DWORD compression_next = MPQ_COMPRESSION_NEXT_SAME);

    /// Initialises the rules for the selected game profile
    void InitializeRules();

    /// Converts a GameProfile to its canonical name
    static std::string ProfileToString(GameProfile profile);

public:
    /// Builds the rule set for game_profile
    explicit GameRules(GameProfile game_profile);

    /// Returns the compression settings for a file, first matching rule wins
    [[nodiscard]] CompressionSettings GetCompressionSettings(const std::string &filename,
                                                             DWORD file_size) const;

    /// Returns the MPQ creation settings
    [[nodiscard]] const MpqCreateSettings &GetCreateSettings() const { return create_settings_; }

    /// Applies user overrides on top of the profile's creation settings
    void OverrideCreateSettings(const MpqCreateSettingsOverrides &overrides);

    /// Converts a profile name or alias to a GameProfile, GENERIC when unknown
    static GameProfile StringToProfile(const std::string &profile_name);

    /// Returns the canonical profile names, for display
    static std::vector<std::string> GetCanonicalProfiles();

    /// Returns the canonical profile names as one comma-separated string
    static std::string GetAvailableProfiles();

    /// Returns the default profile, GENERIC
    static GameProfile GetDefaultProfile() { return GameProfile::GENERIC; }
};

} // namespace mpqcli

#endif
