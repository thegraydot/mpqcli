#ifndef CLI_COMMANDS_H
#define CLI_COMMANDS_H

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace mpqcli {

int HandleInfo(const std::string &target, const std::optional<std::string> &property,
               std::ostream &out);
int HandleCreate(const std::string &target, const std::optional<std::string> &path,
                 const std::optional<std::string> &output, bool sign_archive,
                 const std::optional<std::string> &locale,
                 const std::optional<std::string> &game_profile, int32_t mpq_version,
                 int64_t stream_flags, int64_t sector_size, int64_t raw_chunk_size,
                 int64_t file_flags1, int64_t file_flags2, int64_t file_flags3, int64_t attr_flags,
                 int64_t file_flags, int64_t file_compression, int64_t file_compression_next,
                 std::ostream &out, std::ostream &err);
int HandleAdd(const std::vector<std::string> &files, const std::string &target,
              const std::optional<std::string> &path, bool overwrite, bool update,
              const std::optional<std::string> &locale,
              const std::optional<std::string> &game_profile, int64_t file_flags,
              int64_t file_compression, int64_t file_compression_next, std::ostream &out,
              std::ostream &err);
int HandleRemove(const std::vector<std::string> &files, const std::string &target,
                 const std::optional<std::string> &locale, std::ostream &out, std::ostream &err);
int HandleRename(const std::string &old_file, const std::string &new_file,
                 const std::string &target, const std::optional<std::string> &locale,
                 std::ostream &out, std::ostream &err);
int HandleList(const std::string &target, const std::optional<std::string> &listfile_name,
               bool list_all, bool list_detailed, const std::vector<std::string> &properties,
               std::ostream &out, std::ostream &err);
int HandleExtract(const std::string &target, const std::optional<std::string> &output,
                  const std::optional<std::string> &file, bool keep_folder_structure,
                  const std::optional<std::string> &listfile_name,
                  const std::optional<std::string> &locale, std::ostream &out, std::ostream &err);
int HandleRead(const std::string &file, const std::string &target,
               const std::optional<std::string> &locale, std::ostream &out, std::ostream &err);
int HandleVerify(const std::string &target, bool print_signature, std::ostream &out,
                 std::ostream &err);
int HandleCompact(const std::string &target, const std::optional<std::string> &listfile_name,
                  std::ostream &out);

} // namespace mpqcli

#endif
