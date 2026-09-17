#include "gamerules/rules.h"

namespace mpqcli {

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

} // namespace mpqcli
