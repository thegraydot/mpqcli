#include "mpq/verify.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include <StormLib.h>

#include "errors.h"
#include "mpq/query.h"
#include "mpq/read.h"
#include "util/format.h"
#include "util/locales.h"

namespace fs = std::filesystem;

namespace mpqcli {

uint32_t VerifyMpqArchive(HANDLE archive) {
    return SFileVerifyArchive(archive);
}

void PrintMpqSignature(HANDLE archive, const fs::path &target, std::ostream &out,
                       std::ostream &err) {
    int32_t signature_type = GetFileInfo<int32_t>(archive, SFileMpqSignatures);

    if (signature_type == SIGNATURE_TYPE_NONE) {
        return;
    } else if (signature_type == SIGNATURE_TYPE_WEAK) {
        const char *file_name = "(signature)";
        uint32_t file_size;
        auto file_content = ReadArchivedFile(archive, file_name, &file_size, default_locale, err);

        if (!file_content) {
            throw ArchiveError("Failed to read weak signature file.");
        }

        WriteBinary(out, file_content.get(), file_size);

    } else if (signature_type == SIGNATURE_TYPE_STRONG) {
        // StormLib does not expose the strong signature via SFileGetFileInfo into a
        // growable buffer (SFileMpqStrongSignature's advertised size always exceeds
        // sizeof(std::vector<char>)), so read it directly from the archive file instead.
        int64_t archive_size = GetFileInfo<int64_t>(archive, SFileMpqArchiveSize64);
        int64_t archive_offset = GetFileInfo<int64_t>(archive, SFileMpqHeaderOffset);

        std::error_code ec;
        const auto file_size = static_cast<int64_t>(fs::file_size(target, ec));
        if (ec) {
            throw FileError("Failed to read archive size", target, ec);
        }
        // fs::file_size returns uintmax_t, so without the cast the subtraction runs
        // unsigned and a legitimately negative length reaches the check below only
        // through a conversion C++17 leaves implementation-defined
        const int64_t signature_length = file_size - archive_offset - archive_size;

        if (signature_length <= 0) {
            throw ArchiveError("Invalid signature length: " + std::to_string(signature_length));
        }

        std::vector<char> signature_content(static_cast<size_t>(signature_length));
        std::ifstream file_mpq(target, std::ios::binary);
        file_mpq.seekg(archive_offset + archive_size, std::ios::beg);
        file_mpq.read(signature_content.data(),
                      static_cast<std::streamsize>(signature_content.size()));
        if (!file_mpq) {
            throw ArchiveError("Failed to read strong signature: " + target.string());
        }

        WriteBinary(out, signature_content.data(), static_cast<uint32_t>(signature_content.size()));
    }
}

} // namespace mpqcli
