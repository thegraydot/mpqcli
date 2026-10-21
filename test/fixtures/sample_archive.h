#ifndef SAMPLE_ARCHIVE_H
#define SAMPLE_ARCHIVE_H

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "sample_files.h"
#include "subprocess_helper.h"
#include "test_environment.h"

namespace mpqcli::testing {

/// An archive built by running create over SampleFiles, one per test
///
/// The extra arguments go to create as given, so a test picks the game profile or
/// setting overrides its archive needs. Building it through the binary rather than by
/// writing bytes keeps the layer honest: every archive a test reads is one mpqcli made.
/// A failed create throws, since that is a broken environment rather than a result.
struct SampleArchive {
    SampleFiles files;
    std::filesystem::path archive;

    /// @param create_args Extra arguments for create, such as a game profile.
    /// @param archive_name The archive's file name. The default output directory of an
    ///        extract is the archive's stem, so a test extracting beside the archive
    ///        picks a name whose stem is not the sample files' own directory.
    explicit SampleArchive(const std::vector<std::string> &create_args = {},
                           const std::string &archive_name = "files.mpq")
        : archive(files.dir.Path() / archive_name) {
        std::vector<std::string> args = {"create", archive.string()};
        args.insert(args.end(), create_args.begin(), create_args.end());
        args.push_back(files.files_dir.string());
        const auto result = Run(TestEnvironment::Instance().binary_path, args);
        if (result.returncode != 0) {
            throw std::runtime_error("SampleArchive: create exited " +
                                     std::to_string(result.returncode) + ": " +
                                     result.stderr_output);
        }
    }
};

/// Create arguments for an archive carrying a (listfile) and a full (attributes) block
///
/// The recipe the pytest suite built its shared archives with, so the sizes, offsets
/// and keys that info, list and extract assert on are the ones it saw. 4294967295 is
/// MPQ_FILE_DEFAULT_INTERNAL, 15 is every attribute, and 66048 compresses and encrypts.
inline std::vector<std::string> AttributedArchiveArgs(const std::string &version = "1") {
    return {"--format-version",
            version,
            "--file-flags1",
            "4294967295",
            "--file-flags2",
            "4294967295",
            "--file-flags3",
            "0",
            "--attr-flags",
            "15",
            "--flags",
            "66048",
            "--compression",
            "2",
            "--compression-next",
            "4294967295"};
}

} // namespace mpqcli::testing

#endif // SAMPLE_ARCHIVE_H
