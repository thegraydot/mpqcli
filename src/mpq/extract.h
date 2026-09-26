#ifndef MPQ_EXTRACT_H
#define MPQ_EXTRACT_H

#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

namespace mpqcli {

int ExtractFiles(HANDLE archive, const std::string &output,
                 const std::optional<std::string> &listfile_name, LCID preferred_locale,
                 std::ostream &err);
int ExtractFile(HANDLE archive, const std::string &output, const std::string &file_name,
                bool keep_folder_structure, LCID preferred_locale, std::ostream &err);

} // namespace mpqcli

#endif
