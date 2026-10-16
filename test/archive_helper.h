#ifndef ARCHIVE_HELPER_H
#define ARCHIVE_HELPER_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>

namespace mpqcli::testing {

// Queries every subcommand's tests use to read an archive back through the
// binary. Each spawns list, info or read and asserts that the query itself
// succeeded, so a test that reads a wrong value fails on the value rather than
// on a parse of empty output. They belong to the functional layer alone:
// asserting through the binary is the point, and nothing here links the core.

/// Lists the archive as "<locale>  <name>" lines
///
/// @param archive The archive to list.
/// @param all_files Include the special entries such as (listfile).
/// @param listfile An external listfile, for an archive without one of its own.
/// @return One line per entry, order-independent.
std::set<std::string> ArchiveEntries(const std::filesystem::path &archive, bool all_files = false,
                                     const std::optional<std::filesystem::path> &listfile = {});

/// Reads one info property back from the archive, as the bare value
std::string InfoProperty(const std::filesystem::path &archive, const std::string &property);

/// Returns one list property for one entry
///
/// Reads a "list -d -q <property>" listing, whose lines are "<value>  <name>", and fails
/// the test when no entry has that name.
std::string EntryProperty(const std::filesystem::path &archive, const std::string &name,
                          const std::string &property);

/// EntryProperty parsed as a number, for sizes and counts
std::uint64_t EntryNumber(const std::filesystem::path &archive, const std::string &name,
                          const std::string &property);

/// Reads one entry back through the read subcommand, as the set of its non-empty lines
std::set<std::string> ReadEntry(const std::filesystem::path &archive, const std::string &name,
                                const std::optional<std::string> &locale = std::nullopt);

} // namespace mpqcli::testing

#endif // ARCHIVE_HELPER_H
