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
                 const std::optional<std::string> &output, bool sign_archive,
                 const std::optional<std::string> &locale,
                 const std::optional<std::string> &game_profile, int32_t mpq_version,
                 int64_t stream_flags, int64_t sector_size, int64_t raw_chunk_size,
                 int64_t file_flags1, int64_t file_flags2, int64_t file_flags3, int64_t attr_flags,
                 int64_t file_flags, int64_t file_compression, int64_t file_compression_next);
int HandleAdd(const std::vector<std::string> &files, const std::string &target,
              const std::optional<std::string> &path, bool overwrite, bool update,
              const std::optional<std::string> &locale,
              const std::optional<std::string> &game_profile, int64_t file_flags,
              int64_t file_compression, int64_t file_compression_next);
int HandleRemove(const std::vector<std::string> &files, const std::string &target,
                 const std::optional<std::string> &locale);
int HandleList(const std::string &target, const std::optional<std::string> &listfile_name,
               bool list_all, bool list_detailed, const std::vector<std::string> &properties);
int HandleExtract(const std::string &target, const std::optional<std::string> &output,
                  const std::optional<std::string> &file, bool keep_folder_structure,
                  const std::optional<std::string> &listfile_name,
                  const std::optional<std::string> &locale);
int HandleRead(const std::string &file, const std::string &target,
               const std::optional<std::string> &locale);
int HandleVerify(const std::string &target, bool print_signature);
int HandleCompact(const std::string &target, const std::optional<std::string> &listfile_name);

#endif // COMMANDS_H
