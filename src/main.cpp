#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "gamerules.h"
#include "validators.h"

int main(int argc, char **argv) {
    CLI::App app{
        "A command line tool to create, add, remove, list, extract, read, and verify MPQ archives "
        "using the StormLib library"};

    app.require_subcommand(1);

    // CLI: base
    // These are reused in multiple subcommands
    // clang-format off
    std::string baseTarget;                        // all subcommands
    std::string baseFile;                          // extract, read
    std::optional<std::string> baseLocale;         // add, create, extract, read, remove
    std::optional<std::string> basePath;           // add, create
    std::optional<std::string> baseOutput;         // create, extract
    std::optional<std::string> baseListfileName;   // extract, list, compact
    std::optional<std::string> baseGameProfile;    // add, create
    int64_t fileDwFlags = -1;                      // add, create
    int64_t fileDwCompression = -1;                // add, create
    int64_t fileDwCompressionNext = -1;            // add, create
    // clang-format on
    // CLI: info
    std::optional<std::string> infoProperty;
    // CLI: add
    bool addOverwrite = false;
    bool addUpdate = false;
    std::vector<std::string> addFiles;
    // CLI: remove
    std::vector<std::string> removeFiles;
    // CLI: extract
    bool extractKeepFolderStructure = false;
    // CLI: create
    bool createSignArchive = false;
    int32_t createMpqVersion = -1;
    int64_t createStreamFlags = -1;
    int64_t createSectorSize = -1;
    int64_t createRawChunkSize = -1;
    int64_t createFileFlags1 = -1;
    int64_t createFileFlags2 = -1;
    int64_t createFileFlags3 = -1;
    int64_t createAttrFlags = -1;
    // CLI: list
    bool listDetailed = false;
    bool listAll = false;
    std::vector<std::string> listProperties;
    // CLI: verify
    bool verifyPrintSignature = false;

    // clang-format off -- preserve vertical alignment of string set initialisers
    std::set<std::string> validInfoProperties = {
        "format-version", "header-offset", "header-size",    "archive-size",
        "file-count",     "max-files",     "signature-type",
    };
    std::set<std::string> validFileListProperties = {
        "hash-index", "name-hash1",     "name-hash2",         "name-hash3", "locale",
        "file-index", "byte-offset",    "file-time",          "file-size",  "compressed-size",
        "flags",      "encryption-key", "encryption-key-raw",
    };
    // clang-format on

    // Subcommand: Version
    CLI::App *version = app.add_subcommand("version", "Prints program version");

    // Subcommand: About
    CLI::App *about = app.add_subcommand("about", "Prints program information");

    // Subcommand: Info
    CLI::App *info = app.add_subcommand("info", "Prints info about an MPQ archive");
    info->add_option("target", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    info->add_option("-p,--property", infoProperty, "Prints only a specific property value")
        ->check(CLI::IsMember(validInfoProperties));

    // Subcommand: Create
    CLI::App *create =
        app.add_subcommand("create", "Create an MPQ archive from target file or directory");
    create->add_option("target", baseTarget, "Directory or file to put in MPQ archive")
        ->required()
        ->check(CLI::ExistingPath);
    create->add_option("-p,--path", basePath,
                       "Archive path for a single file, or prefix for a directory");
    create->add_option("-o,--output", baseOutput, "Output MPQ archive");
    create->add_flag("-s,--sign", createSignArchive, "Sign the MPQ archive (default false)");
    create->add_option("--locale", baseLocale, "Locale to use for added files")->check(LocaleValid);
    create
        ->add_option(
            "-g,--game", baseGameProfile,
            "Game profile for MPQ creation. Valid options:\n" + GameRules::GetAvailableProfiles())
        ->check(GameProfileValid);
    // MPQ creation settings overrides
    create->add_option("--version", createMpqVersion, "Override the MPQ archive version")
        ->check(CLI::Range(1, 4))
        ->group("Game setting overrides");
    create->add_option("--stream-flags", createStreamFlags, "Override stream flags")
        ->group("Game setting overrides");
    create->add_option("--sector-size", createSectorSize, "Override sector size")
        ->group("Game setting overrides");
    create->add_option("--raw-chunk-size", createRawChunkSize, "Override raw chunk size for MPQ v4")
        ->group("Game setting overrides");
    create->add_option("--file-flags1", createFileFlags1, "Override file flags for (listfile)")
        ->group("Game setting overrides");
    create->add_option("--file-flags2", createFileFlags2, "Override file flags for (attributes)")
        ->group("Game setting overrides");
    create->add_option("--file-flags3", createFileFlags3, "Override file flags for (signature)")
        ->group("Game setting overrides");
    create
        ->add_option("--attr-flags", createAttrFlags,
                     "Override attribute flags (CRC32, FILETIME, MD5)")
        ->group("Game setting overrides");
    // Compression settings overrides for files being added
    create->add_option("--flags", fileDwFlags, "Override MPQ file flags for added files")
        ->group("Game setting overrides");
    create
        ->add_option("--compression", fileDwCompression,
                     "Override compression for first sector of added files")
        ->group("Game setting overrides");
    create
        ->add_option("--compression-next", fileDwCompressionNext,
                     "Override compression for subsequent sectors of added files")
        ->group("Game setting overrides");

    // Subcommand: Add
    CLI::App *add = app.add_subcommand("add", "Add files to an existing MPQ archive");
    add->add_option("archive", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    add->add_option("files", addFiles,
                    "Files or directories to add; pass - to read paths from stdin")
        ->required()
        ->expected(-1);
    add->add_option("-p,--path", basePath,
                    "Archive path for a single file, or prefix for a directory");
    add->add_flag("-w,--overwrite", addOverwrite, "Overwrite file if it already is in MPQ archive");
    add->add_flag("-u,--update", addUpdate,
                  "Skip files whose archived size matches the on-disk size (directory add only)");
    add->add_option("--locale", baseLocale, "Locale to use for added file")->check(LocaleValid);
    add->add_option("-g,--game", baseGameProfile,
                    "Game profile for compression rules. Valid options:\n" +
                        GameRules::GetAvailableProfiles())
        ->check(GameProfileValid);
    // Compression settings overrides
    add->add_option("--flags", fileDwFlags, "Override MPQ file flags")
        ->group("Game setting overrides");
    add->add_option("--compression", fileDwCompression, "Override compression for first sector")
        ->group("Game setting overrides");
    add->add_option("--compression-next", fileDwCompressionNext,
                    "Override compression for subsequent sectors")
        ->group("Game setting overrides");

    // Subcommand: Remove
    CLI::App *remove = app.add_subcommand("remove", "Remove files from an existing MPQ archive");
    remove->add_option("archive", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    remove
        ->add_option("files", removeFiles,
                     "Archive paths of files to remove; pass - to read paths from stdin")
        ->required()
        ->expected(-1);
    remove->add_option("--locale", baseLocale, "Locale of file to remove")->check(LocaleValid);

    // Subcommand: List
    CLI::App *list = app.add_subcommand("list", "List files from the MPQ archive");
    list->add_option("target", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    list->add_option("-l,--listfile", baseListfileName, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);
    list->add_flag("-d,--detailed", listDetailed,
                   "File listing with additional columns (default false)");
    list->add_flag("-a,--all", listAll, "File listing including hidden files (default false)");
    list->add_option("-p,--property", listProperties, "Prints only specific property values")
        ->check(CLI::IsMember(validFileListProperties));

    // Subcommand: Extract
    CLI::App *extract = app.add_subcommand("extract", "Extract files from the MPQ archive");
    extract->add_option("target", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    extract->add_option("-o,--output", baseOutput, "Output directory");
    extract->add_option("-f,--file", baseFile, "Target file to extract");
    extract->add_flag("-k,--keep", extractKeepFolderStructure,
                      "Keep folder structure (default false)");
    extract->add_option("-l,--listfile", baseListfileName, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);
    extract->add_option("--locale", baseLocale, "Preferred locale for extracted file");

    // Subcommand: Read
    CLI::App *read = app.add_subcommand("read", "Read a file from an MPQ archive");
    read->add_option("file", baseFile, "File to read")->required();
    read->add_option("target", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    read->add_option("--locale", baseLocale, "Preferred locale for read file");

    // Subcommand: Verify
    CLI::App *verify = app.add_subcommand("verify", "Verify the MPQ archive");
    verify->add_option("target", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    verify->add_flag("-p,--print", verifyPrintSignature, "Print the digital signature (in hex)");

    // Subcommand: compact
    CLI::App *compact = app.add_subcommand("compact", "Compact the MPQ archive");
    compact->add_option("target", baseTarget, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    compact->add_option("-l,--listfile", baseListfileName, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // If we get a "subcommand required" error, print help message
        if (e.get_exit_code() == static_cast<int>(CLI::ExitCodes::RequiredError)) {
            std::cout << app.help() << std::endl;
            return 0;
        }
        return app.exit(e);
    }

    if (app.got_subcommand(version)) {
        return HandleVersion();
    }

    if (app.got_subcommand(about)) {
        return HandleAbout();
    }

    if (app.got_subcommand(info)) {
        return HandleInfo(baseTarget, infoProperty);
    }

    if (app.got_subcommand(create)) {
        return HandleCreate(baseTarget, basePath, baseOutput, createSignArchive, baseLocale,
                            baseGameProfile, createMpqVersion, createStreamFlags, createSectorSize,
                            createRawChunkSize, createFileFlags1, createFileFlags2,
                            createFileFlags3, createAttrFlags, fileDwFlags, fileDwCompression,
                            fileDwCompressionNext);
    }

    if (app.got_subcommand(add)) {
        std::vector<std::string> resolvedAddFiles;
        for (const auto &f : addFiles) {
            if (f == "-") {
                std::string line;
                while (std::getline(std::cin, line)) {
                    if (!line.empty()) resolvedAddFiles.push_back(line);
                }
            } else {
                resolvedAddFiles.push_back(f);
            }
        }
        return HandleAdd(resolvedAddFiles, baseTarget, basePath, addOverwrite, addUpdate,
                         baseLocale, baseGameProfile, fileDwFlags, fileDwCompression,
                         fileDwCompressionNext);
    }

    if (app.got_subcommand(remove)) {
        std::vector<std::string> resolvedRemoveFiles;
        for (const auto &f : removeFiles) {
            if (f == "-") {
                std::string line;
                while (std::getline(std::cin, line)) {
                    if (!line.empty()) resolvedRemoveFiles.push_back(line);
                }
            } else {
                resolvedRemoveFiles.push_back(f);
            }
        }
        return HandleRemove(resolvedRemoveFiles, baseTarget, baseLocale);
    }

    if (app.got_subcommand(list)) {
        return HandleList(baseTarget, baseListfileName, listAll, listDetailed, listProperties);
    }

    if (app.got_subcommand(extract)) {
        std::optional<std::string> extractFile =
            baseFile.empty() ? std::nullopt : std::make_optional(baseFile);
        return HandleExtract(baseTarget, baseOutput, extractFile, extractKeepFolderStructure,
                             baseListfileName, baseLocale);
    }

    if (app.got_subcommand(read)) {
        return HandleRead(baseFile, baseTarget, baseLocale);
    }

    if (app.got_subcommand(verify)) {
        return HandleVerify(baseTarget, verifyPrintSignature);
    }

    if (app.got_subcommand(compact)) {
        return HandleCompact(baseTarget, baseListfileName);
    }

    return 0;
}
