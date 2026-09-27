#ifndef ERRORS_H
#define ERRORS_H

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "util/format.h"

namespace mpqcli {

/// Base for every exception thrown by mpqcli
class Error : public std::runtime_error {
public:
    explicit Error(const std::string &message) : std::runtime_error(message) {}
};

/// Thrown when the cancellation flag was set part-way through a command
class Interrupted : public Error {
public:
    Interrupted() : Error("Interrupted") {}
};

/// Throws Interrupted once cancelled is set; polled once per item at the top of a long loop
inline void ThrowIfCancelled(const std::atomic<bool> &cancelled) {
    if (cancelled.load(std::memory_order_relaxed)) {
        throw Interrupted{};
    }
}

/// Thrown when a filesystem operation fails; the message ends with the code, its text and the path
class FileError : public Error {
public:
    FileError(const std::string &what_failed, std::filesystem::path path,
              std::error_code error_code)
        : Error(what_failed + ": (" + std::to_string(error_code.value()) + ") " +
                error_code.message() + ": " + path.string()),
          path_(std::move(path)), error_code_(error_code) {}

    /// For a failure with no code to report, such as a path of the wrong kind
    FileError(const std::string &what_failed, std::filesystem::path path)
        : Error(what_failed + ": " + path.string()), path_(std::move(path)) {}

    const std::filesystem::path &Path() const { return path_; }
    const std::error_code &ErrorCode() const { return error_code_; }

private:
    std::filesystem::path path_;
    std::error_code error_code_;
};

/// Base for failures reading or writing an archive
class ArchiveError : public Error {
public:
    explicit ArchiveError(const std::string &message) : Error(message) {}
};

/// Thrown when a StormLib call fails; the message ends with the code and its text
class StormError : public ArchiveError {
public:
    StormError(const std::string &what_failed, uint32_t error_code)
        : ArchiveError(what_failed + ": (" + std::to_string(error_code) + ") " +
                       StormErrorString(error_code)),
          error_code_(error_code) {}

    uint32_t ErrorCode() const { return error_code_; }

private:
    uint32_t error_code_;
};

/// Thrown when an archive cannot be opened
class ArchiveOpenError : public StormError {
public:
    ArchiveOpenError(std::filesystem::path path, uint32_t error_code)
        : StormError("Failed to open MPQ archive: " + path.string(), error_code),
          path_(std::move(path)) {}

    const std::filesystem::path &Path() const { return path_; }

private:
    std::filesystem::path path_;
};

/// Thrown when an archive cannot be created
class ArchiveCreateError : public StormError {
public:
    ArchiveCreateError(std::filesystem::path path, uint32_t error_code)
        : StormError("Failed to create MPQ archive: " + path.string(), error_code),
          path_(std::move(path)) {}

    const std::filesystem::path &Path() const { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace mpqcli

#endif
