#ifndef COMMANDS_H
#define COMMANDS_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

int HandleVersion();
int HandleAbout();
int HandleInfo(const std::string &target, const std::optional<std::string> &property);
int HandleCreate(const std::string &target, const std::optional<std::string> &path,
                 const std::optional<std::string> &output, bool signArchive,
                 const std::optional<std::string> &locale,
                 const std::optional<std::string> &gameProfile, int32_t mpqVersion,
                 int64_t streamFlags, int64_t sectorSize, int64_t rawChunkSize, int64_t fileFlags1,
                 int64_t fileFlags2, int64_t fileFlags3, int64_t attrFlags, int64_t fileDwFlags,
                 int64_t fileDwCompression, int64_t fileDwCompressionNext);
int HandleAdd(const std::vector<std::string> &files, const std::string &target,
              const std::optional<std::string> &path, bool overwrite, bool update,
              const std::optional<std::string> &locale,
              const std::optional<std::string> &gameProfile, int64_t fileDwFlags,
              int64_t fileDwCompression, int64_t fileDwCompressionNext);
int HandleRemove(const std::vector<std::string> &files, const std::string &target,
                 const std::optional<std::string> &locale);
int HandleList(const std::string &target, const std::optional<std::string> &listfileName,
               bool listAll, bool listDetailed, const std::vector<std::string> &properties);
int HandleExtract(const std::string &target, const std::optional<std::string> &output,
                  const std::optional<std::string> &file, bool keepFolderStructure,
                  const std::optional<std::string> &listfileName,
                  const std::optional<std::string> &locale);
int HandleRead(const std::string &file, const std::string &target,
               const std::optional<std::string> &locale);
int HandleVerify(const std::string &target, bool printSignature);
int HandleCompact(const std::string &target, const std::optional<std::string> &listfileName);

#endif  // COMMANDS_H
