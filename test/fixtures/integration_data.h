#ifndef INTEGRATION_DATA_H
#define INTEGRATION_DATA_H

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mpqcli::testing {

/// Returns the path to one real-world archive, or an empty optional
///
/// Looks for name in each place in turn:
///   1. The directory the MPQCLI_INTEGRATION_DATA compile-time define names, which CMake
///      bakes in from the cache variable of the same name (by default test/data/integration/)
///   2. The directory the MPQCLI_INTEGRATION_DATA environment variable names at runtime
///
/// The file is looked for in each, rather than the first directory that exists being taken,
/// so a default directory that exists without the archives does not hide the variable.
///
/// @param name The archive's file name.
/// @return The archive's path, or nullopt if neither place holds it.
inline std::optional<std::filesystem::path> IntegrationDataPath(const std::string &name) {
    std::vector<std::filesystem::path> directories;
#ifdef MPQCLI_INTEGRATION_DATA
    directories.emplace_back(MPQCLI_INTEGRATION_DATA);
#endif
    if (const char *env = std::getenv("MPQCLI_INTEGRATION_DATA")) {
        directories.emplace_back(env);
    }
    for (const auto &directory : directories) {
        const auto path = directory / name;
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return std::nullopt;
}

/// Returns real-world archives from the integration data by name, skipping the test once if
/// any is absent
///
/// The two Blizzard patches are the only archives carrying a genuine Blizzard signature, one
/// weak and one strong, since StormLib can only sign weakly. They are not committed, so a
/// checkout without them still runs green. Every name is resolved before the test asserts
/// anything, so it runs in full or not at all, never as a smaller check. SKIP() leaves the
/// test case, so the return after it is never reached.
inline std::map<std::string, std::filesystem::path>
IntegrationArchives(const std::vector<std::string> &names) {
    std::map<std::string, std::filesystem::path> archives;
    std::string missing;
    for (const auto &name : names) {
        if (const auto path = IntegrationDataPath(name)) {
            archives.emplace(name, *path);
        } else {
            missing += (missing.empty() ? "" : ", ") + name;
        }
    }
    if (!missing.empty()) {
        SKIP("integration archives absent: " << missing
                                             << "; set MPQCLI_INTEGRATION_DATA or run make "
                                                "fetch_integration_data");
    }
    return archives;
}

/// Returns one real-world archive from the integration data, skipping the test if absent
inline std::filesystem::path IntegrationArchive(const std::string &name) {
    return IntegrationArchives({name}).at(name);
}

} // namespace mpqcli::testing

#endif // INTEGRATION_DATA_H
