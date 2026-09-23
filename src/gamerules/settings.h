#ifndef GAMERULES_SETTINGS_H
#define GAMERULES_SETTINGS_H

#include <optional>
#include <string>
#include <utility>

#include <StormLib.h>

namespace mpqcli {

/// How a compression rule selects the files it applies to
enum class RuleType {
    FILE_MASK, // Rule based on file pattern (e.g., "*.wav")
    FILE_SIZE, // Rule based on file size range
    DEFAULT    // Default rule (fallback)
};

/// A single compression rule
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

/// The compression settings applied to one file
struct CompressionSettings {
    DWORD mpq_flags;
    DWORD compression_first;
    DWORD compression_next;
};

/// Optional overrides for the settings applied to added files
struct CompressionSettingsOverrides {
    std::optional<DWORD> flags;
    std::optional<DWORD> compression;
    std::optional<DWORD> compression_next;
};

/// MPQ archive creation settings
struct MpqCreateSettings {
    DWORD mpq_version;    // MPQ format version (1, 2, 3, or 4)
    DWORD stream_flags;   // Stream flags (e.g., STREAM_PROVIDER_FLAT)
    DWORD file_flags1;    // File flags for (listfile)
    DWORD file_flags2;    // File flags for (attributes)
    DWORD file_flags3;    // File flags for (signature)
    DWORD attr_flags;     // Attribute flags (CRC32, FILETIME, MD5, etc.)
    DWORD sector_size;    // Sector size (typically 0x1000 or 0x4000)
    DWORD raw_chunk_size; // Raw chunk size (for MPQ v4, typically 0x4000)

    /// Defaults for a version 1 archive with a flat file stream
    MpqCreateSettings()
        : mpq_version(MPQ_FORMAT_VERSION_1),
          stream_flags(STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE),
          file_flags1(MPQ_FILE_DEFAULT_INTERNAL), file_flags2(0), file_flags3(0), attr_flags(0),
          sector_size(0x1000), raw_chunk_size(0) {}
};

/// Optional overrides for MPQ archive creation settings
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

} // namespace mpqcli

#endif
