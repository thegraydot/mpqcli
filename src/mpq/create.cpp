#include "mpq/create.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

#include <StormLib.h>

#include "gamerules/rules.h"
#include "util/format.h"

namespace fs = std::filesystem;

HANDLE CreateMpqArchive(const std::string &output_archive_name, const uint32_t file_count,
                        const GameRules &game_rules) {
    // Check if file already exists
    std::error_code ec;
    if (fs::exists(output_archive_name, ec)) {
        std::cerr << "[!] File already exists: " << output_archive_name << " Exiting..."
                  << std::endl;
        return nullptr;
    }

    HANDLE archive;

    // Use game-specific create settings
    const MpqCreateSettings &settings = game_rules.GetCreateSettings();

    SFILE_CREATE_MPQ create_info = {};
    // All logic for defaults and dependencies is handled in GameRules::OverrideCreateSettings
    create_info.cbSize = sizeof(SFILE_CREATE_MPQ);
    create_info.dwMpqVersion = settings.mpq_version;
    create_info.dwStreamFlags = settings.stream_flags;
    create_info.dwFileFlags1 = settings.file_flags1;
    create_info.dwFileFlags2 = settings.file_flags2;
    create_info.dwFileFlags3 = settings.file_flags3;
    create_info.dwAttrFlags = settings.attr_flags;
    create_info.dwSectorSize = settings.sector_size;
    create_info.dwRawChunkSize = settings.raw_chunk_size;
    create_info.dwMaxFileCount = file_count;

    const bool result = SFileCreateArchive2(output_archive_name.c_str(), &create_info, &archive);

    if (!result) {
        const auto error = SErrGetLastError();
        std::cerr << "[!] Failed to create MPQ archive: " << output_archive_name << ": (" << error
                  << ") " << StormErrorString(error) << std::endl;
        return nullptr;
    }

    return archive;
}
