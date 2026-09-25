#include "mpq/verify.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "mpq/query.h"
#include "mpq/read.h"
#include "util/format.h"
#include "util/locales.h"

namespace fs = std::filesystem;

namespace mpqcli {

uint32_t VerifyMpqArchive(HANDLE archive) {
    return SFileVerifyArchive(archive);
}

int32_t PrintMpqSignature(HANDLE archive, const std::string &target) {
    // Determine if we have a strong or weak digital signature
    int32_t signature_type = GetFileInfo<int32_t>(archive, SFileMpqSignatures);

    std::vector<char> signature_content;

    if (signature_type == SIGNATURE_TYPE_NONE) {
        return 1;
    } else if (signature_type == SIGNATURE_TYPE_WEAK) {
        const char *file_name = "(signature)";
        uint32_t file_size;
        auto file_content = ReadFile(archive, file_name, &file_size, default_locale);

        if (!file_content) {
            std::cerr << "[!] Failed to read weak signature file." << std::endl;
            return -1;
        }
        signature_content.resize(file_size);
        std::copy(file_content.get(), file_content.get() + file_size, signature_content.begin());

        PrintAsBinary(file_content.get(), file_size);

    } else if (signature_type == SIGNATURE_TYPE_STRONG) {
        // StormLib does not expose the strong signature via SFileGetFileInfo into a
        // growable buffer (SFileMpqStrongSignature's advertised size always exceeds
        // sizeof(std::vector<char>)), so read it directly from the archive file instead.
        int64_t archive_size = GetFileInfo<int64_t>(archive, SFileMpqArchiveSize64);
        int64_t archive_offset = GetFileInfo<int64_t>(archive, SFileMpqHeaderOffset);

        const fs::path archive_path(target);
        std::error_code ec;
        const auto file_size = static_cast<int64_t>(fs::file_size(archive_path, ec));
        if (ec) {
            std::cerr << "[!] Failed to read archive size: (" << ec.value() << ") " << ec.message()
                      << ": " << target << std::endl;
            return -1;
        }
        // fs::file_size returns uintmax_t, so without the cast the subtraction runs
        // unsigned and a legitimately negative length reaches the check below only
        // through a conversion C++17 leaves implementation-defined
        const int64_t signature_length = file_size - archive_offset - archive_size;

        if (signature_length <= 0) {
            std::cerr << "[!] Invalid signature length: " << signature_length << std::endl;
            return -1;
        }

        std::ifstream file_mpq(archive_path, std::ios::binary);
        file_mpq.seekg(archive_offset + archive_size, std::ios::beg);
        signature_content.resize(static_cast<size_t>(signature_length));
        file_mpq.read(signature_content.data(),
                      static_cast<std::streamsize>(signature_content.size()));
        file_mpq.close();

        PrintAsBinary(signature_content.data(), static_cast<uint32_t>(signature_content.size()));
    }

    return 0;
}

} // namespace mpqcli
