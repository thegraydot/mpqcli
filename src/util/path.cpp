#include "util/path.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace mpqcli {

std::string NormalizeFilePath(const fs::path &path) {
    std::string file_path = path.u8string();
#ifndef _WIN32
    std::replace(file_path.begin(), file_path.end(), '\\', '/');
    return file_path;
#else
    return file_path;
#endif
}

std::string WindowsifyFilePath(const fs::path &path) {
    std::string file_path = path.u8string();
    std::replace(file_path.begin(), file_path.end(), '/', '\\');
    return file_path;
}

std::vector<fs::path> ListFilesRecursive(const fs::path &directory, std::error_code &ec) {
    std::vector<fs::path> files;
    fs::recursive_directory_iterator it(directory, ec);
    while (!ec && it != fs::recursive_directory_iterator()) {
        // A dangling symlink reports not_found rather than a hard error, and is
        // skipped like any other non-regular entry
        std::error_code status_ec;
        const fs::file_status status = it->status(status_ec);
        if (fs::is_regular_file(status)) {
            files.push_back(it->path());
        } else if (status_ec && status.type() != fs::file_type::not_found) {
            ec = status_ec;
            break;
        }
        it.increment(ec);
    }
    if (ec) {
        return {};
    }
    std::sort(files.begin(), files.end());
    return files;
}

// Returns the file's last-modification time as a Windows FILETIME value
// (100-nanosecond intervals since 1601-01-01 UTC).  Returns 0 on error.
uint64_t LocalFileTimestamp(const fs::path &path) {
#ifdef _WIN32
    // _wstat64 handles paths with non-ASCII characters, which the narrow
    // stat() would mangle on Windows.
    struct _stat64 st {};
    if (_wstat64(path.wstring().c_str(), &st) != 0) {
        return 0;
    }
#else
    struct stat st {};
    if (stat(path.string().c_str(), &st) != 0) {
        return 0;
    }
#endif
    constexpr int64_t epoch_diff = 11644473600LL;
    return static_cast<uint64_t>((static_cast<int64_t>(st.st_mtime) + epoch_diff) * 10000000LL);
}

bool IsWithinDirectory(const fs::path &base, const fs::path &path) {
    return std::mismatch(base.begin(), base.end(), path.begin(), path.end()).first == base.end();
}

std::string ResolveArchiveName(const std::string &f, const std::optional<std::string> &path,
                               const bool treat_as_directory) {
    fs::path file_path = path.value_or(fs::path(f).filename().u8string());
    if (treat_as_directory) {
        const std::string filename = fs::path(f).filename().u8string();
        file_path = path.value_or("") / fs::path(filename);
    }
    return WindowsifyFilePath(file_path);
}

} // namespace mpqcli
