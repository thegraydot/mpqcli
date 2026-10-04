#include "mpq/info.h"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

#include "mpq/query.h"

namespace mpqcli {

void PrintMpqInfo(HANDLE archive, const std::optional<std::string> &info_property,
                  std::ostream &out) {
    std::map<std::string, std::function<void(bool)>> property_actions = {
        {"format-version",
         [&](bool print_name) {
             TMPQHeader header = GetFileInfo<TMPQHeader>(archive, SFileMpqHeader);
             uint16_t format_version =
                 header.wFormatVersion + 1; // StormLib counts the formats from 0
             if (print_name) {
                 out << "Format version: ";
             }
             out << format_version << std::endl;
         }},
        {"header-offset",
         [&](bool print_name) {
             int64_t header_offset = GetFileInfo<int64_t>(archive, SFileMpqHeaderOffset);
             if (print_name) {
                 out << "Header offset: ";
             }
             out << header_offset << std::endl;
         }},
        {"header-size",
         [&](bool print_name) {
             int64_t header_size = GetFileInfo<int64_t>(archive, SFileMpqHeaderSize);
             if (print_name) {
                 out << "Header size: ";
             }
             out << header_size << std::endl;
         }},
        {"archive-size",
         [&](bool print_name) {
             int64_t archive_size = GetFileInfo<int64_t>(archive, SFileMpqArchiveSize64);
             if (print_name) {
                 out << "Archive size: ";
             }
             out << archive_size << std::endl;
         }},
        {"file-count",
         [&](bool print_name) {
             int32_t number_of_files = GetFileInfo<int32_t>(archive, SFileMpqNumberOfFiles);
             if (print_name) {
                 out << "File count: ";
             }
             out << number_of_files << std::endl;
         }},
        {"max-files",
         [&](bool print_name) {
             int32_t max_files = GetFileInfo<int32_t>(archive, SFileMpqMaxFileCount);
             if (print_name) {
                 out << "Max files: ";
             }
             out << max_files << std::endl;
         }},
        {"signature-type", [&](bool print_name) {
             int32_t signature_type = GetFileInfo<int32_t>(archive, SFileMpqSignatures);
             if (print_name) {
                 out << "Signature type: ";
             }
             if (signature_type == SIGNATURE_TYPE_NONE) {
                 out << "None" << std::endl;
             } else if (signature_type == SIGNATURE_TYPE_WEAK) {
                 out << "Weak" << std::endl;
             } else if (signature_type == SIGNATURE_TYPE_STRONG) {
                 out << "Strong" << std::endl;
             }
         }}};

    if (!info_property.has_value()) {
        for (const auto &[key, action] : property_actions) {
            action(true);
        }
    } else {
        auto it = property_actions.find(info_property.value());
        if (it != property_actions.end()) {
            it->second(false);
        }
    }
}

} // namespace mpqcli
