#include "mpq/info.h"

#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>

#include <StormLib.h>

#include "mpq/query.h"
#include "util/format.h"

namespace mpqcli {

void PrintMpqInfo(HANDLE archive, const std::optional<std::string> &info_property) {
    // Map of property names to their corresponding actions
    std::map<std::string, std::function<void(bool)>> property_actions = {
        {"format-version",
         [&](bool print_name) {
             TMPQHeader header = GetFileInfo<TMPQHeader>(archive, SFileMpqHeader);
             uint16_t format_version =
                 header.wFormatVersion + 1; // Add +1 because StormLib starts at 0
             if (print_name) {
                 std::cout << "Format version: ";
             }
             std::cout << format_version << std::endl;
         }},
        {"header-offset",
         [&](bool print_name) {
             int64_t header_offset = GetFileInfo<int64_t>(archive, SFileMpqHeaderOffset);
             if (print_name) {
                 std::cout << "Header offset: ";
             }
             std::cout << header_offset << std::endl;
         }},
        {"header-size",
         [&](bool print_name) {
             int64_t header_size = GetFileInfo<int64_t>(archive, SFileMpqHeaderSize);
             if (print_name) {
                 std::cout << "Header size: ";
             }
             std::cout << header_size << std::endl;
         }},
        {"archive-size",
         [&](bool print_name) {
             int64_t archive_size = GetFileInfo<int64_t>(archive, SFileMpqArchiveSize64);
             if (print_name) {
                 std::cout << "Archive size: ";
             }
             std::cout << archive_size << std::endl;
         }},
        {"file-count",
         [&](bool print_name) {
             int32_t number_of_files = GetFileInfo<int32_t>(archive, SFileMpqNumberOfFiles);
             if (print_name) {
                 std::cout << "File count: ";
             }
             std::cout << number_of_files << std::endl;
         }},
        {"max-files",
         [&](bool print_name) {
             int32_t max_files = GetFileInfo<int32_t>(archive, SFileMpqMaxFileCount);
             if (print_name) {
                 std::cout << "Max files: ";
             }
             std::cout << max_files << std::endl;
         }},
        {"signature-type", [&](bool print_name) {
             int32_t signature_type = GetFileInfo<int32_t>(archive, SFileMpqSignatures);
             if (print_name) {
                 std::cout << "Signature type: ";
             }
             if (signature_type == SIGNATURE_TYPE_NONE) {
                 std::cout << "None" << std::endl;
             } else if (signature_type == SIGNATURE_TYPE_WEAK) {
                 std::cout << "Weak" << std::endl;
             } else if (signature_type == SIGNATURE_TYPE_STRONG) {
                 std::cout << "Strong" << std::endl;
             }
         }}};

    // If infoProperty is not set, print all properties with their names (key)
    // Otherwise, print only the specified property value
    if (!info_property.has_value()) {
        for (const auto &[key, action] : property_actions) {
            action(true); // Print property name and value
        }
    } else {
        auto it = property_actions.find(info_property.value());
        if (it != property_actions.end()) {
            it->second(false); // Print only the value
        }
    }
}

} // namespace mpqcli
