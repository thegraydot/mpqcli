#include "gamerules/rules.h"

#include <algorithm>
#include <string>

#include "util/string.h"

// Constructor
GameRules::GameRules(GameProfile game_profile) : profile_(game_profile) {
    InitializeRules();
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
