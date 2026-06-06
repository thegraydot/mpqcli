#ifndef MPQ_H
#define MPQ_H

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include <StormLib.h>

#include "gamerules.h"

namespace fs = std::filesystem;

bool OpenMpqArchive(const std::string &filename, HANDLE *hArchive, int32_t flags);
bool CloseMpqArchive(HANDLE hArchive);
bool SignMpqArchive(HANDLE hArchive);
int ExtractFiles(HANDLE hArchive, const std::string &output,
                 const std::optional<std::string> &listfileName, LCID preferredLocale);
int ExtractFile(HANDLE hArchive, const std::string &output, const std::string &fileName,
                bool keepFolderStructure, LCID preferredLocale);
HANDLE CreateMpqArchive(const std::string &outputArchiveName, uint32_t fileCount,
                        const GameRules &gameRules);
int AddFiles(HANDLE hArchive, const std::string &inputPath, const std::string &pathPrefix,
             LCID locale, const GameRules &gameRules,
             const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
             bool overwrite = false, bool update = false);
int AddFile(HANDLE hArchive, const fs::path &localFile, const std::string &archiveFilePath,
            LCID locale, const GameRules &gameRules,
            const CompressionSettingsOverrides &overrides = CompressionSettingsOverrides(),
            bool overwrite = false);
int RemoveFile(HANDLE hArchive, const std::string &archiveFilePath, LCID locale);
int ListFiles(HANDLE hArchive, const std::optional<std::string> &listfileName, bool listAll,
              bool listDetailed, const std::vector<std::string> &properties);
std::unique_ptr<char[]> ReadFile(HANDLE hArchive, const char *szFileName, unsigned int *fileSize,
                                 LCID preferredLocale);
void PrintMpqInfo(HANDLE hArchive, const std::optional<std::string> &infoProperty);
uint32_t VerifyMpqArchive(HANDLE hArchive);
int CompactMpqArchive(HANDLE hArchive, const std::optional<std::string> &listfileName);
int32_t PrintMpqSignature(HANDLE hArchive, const std::string &target);

template <typename T>
T GetFileInfo(HANDLE hFile, SFileInfoClass infoClass) {
    T value{};
    if (!SFileGetFileInfo(hFile, infoClass, &value, sizeof(T), nullptr)) {
        return T{};
    }
    return value;
}

#endif
