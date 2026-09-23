#include "gamerules/rules.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#include "util/string.h"

namespace mpqcli {

GameRules::GameRules(GameProfile game_profile) : profile_(game_profile) {
    InitializeRules();
}

bool GameRules::MatchFileMask(const std::string &filename, const std::string &mask) {
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

void GameRules::AddRuleByFileSize(DWORD size_min, DWORD size_max, DWORD mpq_flags,
                                  DWORD compression_first, DWORD compression_next) {
    rules_.emplace_back(size_min, size_max, mpq_flags, compression_first, compression_next);
}

void GameRules::AddRuleDefault(DWORD mpq_flags, DWORD compression_first, DWORD compression_next) {
    rules_.emplace_back(mpq_flags, compression_first, compression_next);
}

CompressionSettings GameRules::GetCompressionSettings(const std::string &filename,
                                                      const DWORD file_size) const {
    // First matching rule wins
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

void GameRules::OverrideCreateSettings(const MpqCreateSettingsOverrides &overrides) {
    // Whether the user set file_flags2 themselves decides the adjustment below
    bool user_set_file_flags2 = false;

    // User overrides first: a value the user gave always wins, even one that
    // looks wrong

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

    // Then the adjustments the overrides imply, only where the user left the
    // dependent value alone

    // file_flags2 controls the (attributes) file, which is only meaningful when
    // attr_flags is also set. According to StormLib's SFileCreateArchive.cpp:
    // - The (attributes) file is created only when BOTH file_flags2 AND attr_flags are non-zero
    // - If attr_flags is set but file_flags2 is still 0 (not overridden by user or profile),
    //   we should set file_flags2 to MPQ_FILE_DEFAULT_INTERNAL to enable the attributes file

    if (!user_set_file_flags2 && create_settings_.file_flags2 == 0 &&
        create_settings_.attr_flags != 0) {
        // User wants attributes (attr_flags is set) but hasn't specified how to store
        // the (attributes) file itself. Use the default internal file flags.
        create_settings_.file_flags2 = MPQ_FILE_DEFAULT_INTERNAL;
    }

    // Note: If user explicitly sets file_flags2 to 0 via override, we respect that choice
    // even if attr_flags is non-zero.
}

} // namespace mpqcli
