#ifndef MPQ_H
#define MPQ_H

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include <StormLib.h>

#include "gamerules.h"

namespace fs = std::filesystem;

bool OpenMpqArchive(const std::string &filename, HANDLE *archive, int32_t flags);
bool CloseMpqArchive(HANDLE archive);
bool SignMpqArchive(HANDLE archive);
int ExtractFiles(HANDLE archive, const std::string &output,
                 const std::optional<std::string> &listfile_name, LCID preferred_locale);
int ExtractFile(HANDLE archive, const std::string &output, const std::string &file_name,
                bool keep_folder_structure, LCID preferred_locale);
HANDLE CreateMpqArchive(const std::string &output_archive_name, uint32_t file_count,
                        const GameRules &game_rules);
int AddFiles(HANDLE archive, const std::string &input_path, const std::string &path_prefix,
             LCID locale, const GameRules &game_rules,
             const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
             bool overwrite = false, bool update = false);
int AddFile(HANDLE archive, const fs::path &local_file, const std::string &archive_file_path,
            LCID locale, const GameRules &game_rules,
            const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
            bool overwrite = false);
int RemoveFile(HANDLE archive, const std::string &archive_file_path, LCID locale);
int ListFiles(HANDLE archive, const std::optional<std::string> &listfile_name, bool list_all,
              bool list_detailed, const std::vector<std::string> &properties);
std::unique_ptr<char[]> ReadFile(HANDLE archive, const char *file_name, unsigned int *file_size,
                                 LCID preferred_locale);
void PrintMpqInfo(HANDLE archive, const std::optional<std::string> &info_property);
uint32_t VerifyMpqArchive(HANDLE archive);
int CompactMpqArchive(HANDLE archive, const std::optional<std::string> &listfile_name);
int32_t PrintMpqSignature(HANDLE archive, const std::string &target);

template <typename T> T GetFileInfo(HANDLE file, SFileInfoClass info_class) {
    T value{};
    if (!SFileGetFileInfo(file, info_class, &value, sizeof(T), nullptr)) {
        return T{};
    }
    return value;
}

#endif
