#ifndef TEMP_DIR_H
#define TEMP_DIR_H

#include <cstdint>
#include <filesystem>
#include <random>
#include <sstream>
#include <system_error>

namespace mpqcli::testing {

/// A uniquely named scratch directory, removed with everything in it when the fixture dies
///
/// Every test that writes anything writes here. ctest runs test cases as separate
/// processes in parallel, so the name carries 64 random bits rather than a counter,
/// which would collide across processes, and a name already taken is drawn again. The
/// destructor swallows the removal error: throwing out of it would mask whichever
/// assertion actually failed.
class TempDir {
public:
    TempDir() {
        std::random_device device;
        const std::filesystem::path base = std::filesystem::temp_directory_path();
        // create_directory says whether it made the directory, where create_directories
        // succeeds on one that exists, and two tests would quietly share it
        do {
            const std::uint64_t bits = (static_cast<std::uint64_t>(device()) << 32) | device();
            std::ostringstream name;
            name << "mpqcli_test_" << std::hex << bits;
            path_ = base / name.str();
        } while (!std::filesystem::create_directory(path_));
    }

    TempDir(const TempDir &) = delete;
    TempDir &operator=(const TempDir &) = delete;

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    const std::filesystem::path &Path() const { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace mpqcli::testing

#endif // TEMP_DIR_H
