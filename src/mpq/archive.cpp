#include "mpq/archive.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

#include <StormLib.h>

#include "errors.h"
#include "gamerules/rules.h"
#include "gamerules/settings.h"

namespace fs = std::filesystem;

namespace mpqcli {

Archive Archive::Open(const fs::path &path, const DWORD flags) {
    HANDLE handle = nullptr;
    if (!SFileOpenArchive(path.string().c_str(), 0, flags, &handle)) {
        throw ArchiveOpenError(path, SErrGetLastError());
    }
    return Archive(handle);
}

Archive Archive::Create(const fs::path &path, const uint32_t file_count,
                        const GameRules &game_rules) {
    std::error_code ec;
    if (fs::exists(path, ec)) {
        throw ArchiveError("File already exists: " + path.string());
    }

    const MpqCreateSettings &settings = game_rules.GetCreateSettings();

    SFILE_CREATE_MPQ create_info = {};
    create_info.cbSize = sizeof(SFILE_CREATE_MPQ);
    create_info.dwMpqVersion = settings.mpq_version;
    create_info.dwStreamFlags = settings.stream_flags;
    create_info.dwFileFlags1 = settings.file_flags1;
    create_info.dwFileFlags2 = settings.file_flags2;
    create_info.dwFileFlags3 = settings.file_flags3;
    create_info.dwAttrFlags = settings.attr_flags;
    create_info.dwSectorSize = settings.sector_size;
    create_info.dwRawChunkSize = settings.raw_chunk_size;
    create_info.dwMaxFileCount = file_count;

    HANDLE handle = nullptr;
    if (!SFileCreateArchive2(path.string().c_str(), &create_info, &handle)) {
        throw ArchiveCreateError(path, SErrGetLastError());
    }
    return Archive(handle);
}

Archive::Archive(Archive &&other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
}

Archive &Archive::operator=(Archive &&other) noexcept {
    if (this != &other) {
        if (handle_ != nullptr) {
            SFileCloseArchive(handle_);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

Archive::~Archive() {
    if (handle_ != nullptr) {
        SFileCloseArchive(handle_);
    }
}

void Archive::Sign() {
    if (!SFileSignArchive(handle_, SIGNATURE_TYPE_WEAK)) {
        throw StormError("Failed to sign MPQ archive", SErrGetLastError());
    }
}

void Archive::Close() {
    if (!SFileCloseArchive(handle_)) {
        // StormLib frees the handle even when its flush fails, so drop it before
        // the throw or the destructor would close it a second time
        const auto error = SErrGetLastError();
        handle_ = nullptr;
        throw StormError("Failed to close MPQ archive", error);
    }
    handle_ = nullptr;
}

} // namespace mpqcli
