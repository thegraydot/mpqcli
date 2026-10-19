#ifndef SAMPLE_FILES_H
#define SAMPLE_FILES_H

#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "temp_dir.h"
#include "write_file.h"

namespace mpqcli::testing {

/// The modification time every sample file carries: 2025-07-29 14:31:00 local time
///
/// An archive's (attributes) block stores each file's time, so the archive sizes and
/// listings the tests assert only hold when the inputs always carry the same one. Local
/// time, because that is what mpqcli prints back in a detailed listing. The block itself
/// stores UTC, so its compressed size depends in principle on the time zone the tests
/// run in. For this time and these files it comes out the same at every UTC offset in
/// use, but changing either can make one zone see different archive sizes.
inline std::filesystem::file_time_type SampleFileTime() {
    std::tm calendar{};
    calendar.tm_year = 2025 - 1900;
    calendar.tm_mon = 7 - 1;
    calendar.tm_mday = 29;
    calendar.tm_hour = 14;
    calendar.tm_min = 31;
    calendar.tm_sec = 0;
    calendar.tm_isdst = -1;
    const auto system_time = std::chrono::system_clock::from_time_t(std::mktime(&calendar));
    // C++17 has no clock_cast, so the two clocks are bridged through their current
    // readings, which land a few microseconds either side of the target. Rounding to
    // the second is required, not tidying: mpqcli stores the whole seconds stat()
    // reports, so a time just short of 14:31:00 is stored as 14:30:59 and every
    // archive size shifts. Whether it falls short depends on which now() the compiler
    // happens to evaluate first. Every standard library's file clock epoch is a whole
    // number of seconds from the system clock's, so the rounded time stays exact.
    const auto system_now = std::chrono::system_clock::now();
    const auto file_now = std::filesystem::file_time_type::clock::now();
    return std::chrono::round<std::chrono::seconds>(file_now + (system_time - system_now));
}

/// The three files every archive-building test starts from, in a scratch directory
///
/// cats.txt and dogs.txt are 27 bytes with LF endings, and bytes is the eight values
/// 0x00 to 0x07, matching what the pytest suite generated.
struct SampleFiles {
    TempDir dir;
    std::filesystem::path files_dir;

    SampleFiles() : files_dir(dir.Path() / "files") {
        const auto time = SampleFileTime();
        for (const auto &[name, content] : Contents()) {
            const auto path = files_dir / name;
            WriteFile(path, content);
            std::filesystem::last_write_time(path, time);
        }
    }

    /// Name and content of each sample file
    static std::vector<std::pair<std::string, std::string>> Contents() {
        return {
            {"cats.txt", "This is a file about cats.\n"},
            {"dogs.txt", "This is a file about dogs.\n"},
            {"bytes", std::string("\x00\x01\x02\x03\x04\x05\x06\x07", 8)},
        };
    }
};

} // namespace mpqcli::testing

#endif // SAMPLE_FILES_H
