#ifndef INTEGRATION_DATA_H
#define INTEGRATION_DATA_H

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

namespace mpqcli::testing {

/// Returns the directory holding the real-world archives, or an empty optional
///
/// Checks in order:
///   1. The MPQCLI_INTEGRATION_DATA compile-time define, which CMake bakes in from the
///      cache variable of the same name (by default test/data/integration/)
///   2. The MPQCLI_INTEGRATION_DATA environment variable at runtime
///
/// @return The directory, or nullopt if neither source names one that exists.
inline std::optional<std::filesystem::path> IntegrationDataPath() {
#ifdef MPQCLI_INTEGRATION_DATA
    {
        std::filesystem::path path{MPQCLI_INTEGRATION_DATA};
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
#endif
    if (const char *env = std::getenv("MPQCLI_INTEGRATION_DATA")) {
        std::filesystem::path path{env};
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return std::nullopt;
}

/// Returns one real-world archive from the integration data, skipping the test if absent
///
/// The two Blizzard patches are the only archives carrying a genuine Blizzard signature,
/// one weak and one strong, since StormLib can only sign weakly. They are not committed,
/// so a checkout without them still runs green. SKIP() leaves the test case, so the
/// return after it is never reached.
inline std::filesystem::path IntegrationArchive(const std::string &name) {
    const auto data = IntegrationDataPath();
    if (!data || !std::filesystem::exists(*data / name)) {
        SKIP("integration archive " << name
                                    << " is absent; set MPQCLI_INTEGRATION_DATA or run make "
                                       "fetch_integration_data");
    }
    return *data / name;
}

} // namespace mpqcli::testing

#endif // INTEGRATION_DATA_H
