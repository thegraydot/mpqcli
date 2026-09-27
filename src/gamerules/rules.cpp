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

    // A mask and a name may each use either separator
    std::replace(lower_filename.begin(), lower_filename.end(), '\\', '/');
    std::replace(lower_mask.begin(), lower_mask.end(), '\\', '/');

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
    for (const auto &rule : rules_) {
        switch (rule.type) {
        case RuleType::FILE_MASK:
            if (MatchFileMask(filename, rule.file_mask)) {
                return {rule.mpq_flags, rule.compression_first, rule.compression_next};
            }
            break;

        case RuleType::FILE_SIZE: {
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

    // Reached only by a profile that declares no DEFAULT rule
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
        user_set_file_flags2 = true;
    }

    if (overrides.file_flags3.has_value()) {
        create_settings_.file_flags3 = overrides.file_flags3.value();
    }

    if (overrides.attr_flags.has_value()) {
        create_settings_.attr_flags = overrides.attr_flags.value();
    }

    // Then the adjustments the overrides imply, only where the user left the
    // dependent value alone

    // StormLib (SFileCreateArchive.cpp) writes the (attributes) file only when both
    // file_flags2 and attr_flags are non-zero, so attributes asked for without a
    // storage flag would silently never be written. An explicit zero from the user
    // is left alone.
    if (!user_set_file_flags2 && create_settings_.file_flags2 == 0 &&
        create_settings_.attr_flags != 0) {
        create_settings_.file_flags2 = MPQ_FILE_DEFAULT_INTERNAL;
    }
}

} // namespace mpqcli
