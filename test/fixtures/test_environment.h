#ifndef TEST_ENVIRONMENT_H
#define TEST_ENVIRONMENT_H

#include <filesystem>

namespace mpqcli::testing {

/// Singleton that exposes the paths CMake baked into the test binary at configure time
///
/// The one fixture that is not function-scoped: these values are constant for the whole
/// run and cannot vary per test.
struct TestEnvironment {
    static const TestEnvironment &Instance() {
        static TestEnvironment env;
        return env;
    }

    /// The mpqcli binary the functional tests spawn
    const std::filesystem::path binary_path{MPQCLI_BINARY_PATH};

    /// The test/ source directory, for inputs committed under test/data/
    const std::filesystem::path test_dir{MPQCLI_TEST_DIR};

private:
    TestEnvironment() = default;
};

} // namespace mpqcli::testing

#endif // TEST_ENVIRONMENT_H
