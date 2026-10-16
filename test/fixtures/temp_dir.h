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
/// which would collide across processes. The destructor swallows the removal error:
/// throwing out of it would mask whichever assertion actually failed.
class TempDir {
public:
    TempDir() {
        std::random_device device;
        std::ostringstream name;
        name << "mpqcli_test_" << std::hex << device() << device();
        path_ = std::filesystem::temp_directory_path() / name.str();
        std::filesystem::create_directories(path_);
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
