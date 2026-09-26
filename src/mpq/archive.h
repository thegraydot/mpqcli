#ifndef MPQ_ARCHIVE_H
#define MPQ_ARCHIVE_H

#include <cstdint>
#include <filesystem>

#include <StormLib.h>

#include "gamerules/rules.h"

namespace mpqcli {

/// An open MPQ archive, closed on destruction unless Close was called first
class Archive {
public:
    /// Opens an existing archive with StormLib's MPQ_OPEN_* flags
    ///
    /// @throws ArchiveOpenError if StormLib cannot open it
    static Archive Open(const std::filesystem::path &path, DWORD flags);

    /// Creates an archive sized for file_count files with the profile's settings
    ///
    /// @throws ArchiveError if a file already exists at path
    /// @throws ArchiveCreateError if StormLib cannot create it
    static Archive Create(const std::filesystem::path &path, uint32_t file_count,
                          const GameRules &game_rules);

    Archive(const Archive &) = delete;
    Archive &operator=(const Archive &) = delete;
    Archive(Archive &&other) noexcept;
    Archive &operator=(Archive &&other) noexcept;
    ~Archive();

    /// Signs the archive with the Blizzard weak signature
    ///
    /// @throws StormError if signing fails
    void Sign();

    /// Closes the archive and writes its tables; the destructor closes silently instead
    ///
    /// @throws StormError if the close fails, leaving the archive possibly incomplete
    void Close();

    /// The StormLib handle, for the calls this class does not wrap
    HANDLE Handle() const { return handle_; }

private:
    explicit Archive(HANDLE handle) : handle_(handle) {}

    HANDLE handle_;
};

} // namespace mpqcli

#endif
