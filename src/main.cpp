#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>

#include "commands.h"
#include "completion.h"
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
    std::string base_target;                        // all subcommands
    std::string base_file;                          // extract, read
    std::optional<std::string> base_locale;         // add, create, extract, read, remove
    std::optional<std::string> base_path;           // add, create
    std::optional<std::string> base_output;         // create, extract
    std::optional<std::string> base_listfile_name;   // extract, list, compact
    std::optional<std::string> base_game_profile;    // add, create
    int64_t file_flags = -1;                      // add, create
    int64_t file_compression = -1;                // add, create
    int64_t file_compression_next = -1;            // add, create
    // clang-format on
    // CLI: info
    std::optional<std::string> info_property;
    // CLI: add
    bool add_overwrite = false;
    bool add_update = false;
    std::vector<std::string> add_files;
    // CLI: remove
    std::vector<std::string> remove_files;
    // CLI: extract
    bool extract_keep_folder_structure = false;
    // CLI: create
    bool create_sign_archive = false;
    int32_t create_mpq_version = -1;
    int64_t create_stream_flags = -1;
    int64_t create_sector_size = -1;
    int64_t create_raw_chunk_size = -1;
    int64_t create_file_flags1 = -1;
    int64_t create_file_flags2 = -1;
    int64_t create_file_flags3 = -1;
    int64_t create_attr_flags = -1;
    // CLI: list
    bool list_detailed = false;
    bool list_all = false;
    std::vector<std::string> list_properties;
    // CLI: verify
    bool verify_print_signature = false;

    // clang-format off - preserve vertical alignment of string set initialisers
    std::set<std::string> valid_info_properties = {
        "format-version",
        "header-offset",
        "header-size",
        "archive-size",
        "file-count",
        "max-files",
        "signature-type",
    };
    std::set<std::string> valid_file_list_properties = {
        "hash-index",
        "name-hash1",
        "name-hash2",
        "name-hash3",
        "locale",
        "file-index",
        "byte-offset",
        "file-time",
        "file-size",
        "compressed-size",
        "flags",
        "encryption-key",
        "encryption-key-raw",
    };
    // clang-format on

    // Subcommand: Version
    CLI::App *version = app.add_subcommand("version", "Prints program version");

    // Subcommand: About
    CLI::App *about = app.add_subcommand("about", "Prints program information");

    // Subcommand: Info
    CLI::App *info = app.add_subcommand("info", "Prints info about an MPQ archive");
    info->add_option("target", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    info->add_option("-p,--property", info_property, "Prints only a specific property value")
        ->check(CLI::IsMember(valid_info_properties));

    // Subcommand: Create
    CLI::App *create =
        app.add_subcommand("create", "Create an MPQ archive from target file or directory");
    create->add_option("target", base_target, "Directory or file to put in MPQ archive")
        ->required()
        ->check(CLI::ExistingPath);
    create->add_option("-p,--path", base_path,
                       "Archive path for a single file, or prefix for a directory");
    create->add_option("-o,--output", base_output, "Output MPQ archive");
    create->add_flag("-s,--sign", create_sign_archive, "Sign the MPQ archive (default false)");
    create->add_option("--locale", base_locale, "Locale to use for added files")
        ->check(locale_valid);
    create
        ->add_option("-g,--game", base_game_profile,
                     "Game profile for MPQ creation. Valid options:\n" +
                         GameRules::GetAvailableProfiles())
        ->check(game_profile_valid);
    // MPQ creation settings overrides
    create->add_option("--version", create_mpq_version, "Override the MPQ archive version")
        ->check(CLI::Range(1, 4))
        ->group("Game setting overrides");
    create->add_option("--stream-flags", create_stream_flags, "Override stream flags")
        ->group("Game setting overrides");
    create->add_option("--sector-size", create_sector_size, "Override sector size")
        ->group("Game setting overrides");
    create
        ->add_option("--raw-chunk-size", create_raw_chunk_size,
                     "Override raw chunk size for MPQ v4")
        ->group("Game setting overrides");
    create->add_option("--file-flags1", create_file_flags1, "Override file flags for (listfile)")
        ->group("Game setting overrides");
    create->add_option("--file-flags2", create_file_flags2, "Override file flags for (attributes)")
        ->group("Game setting overrides");
    create->add_option("--file-flags3", create_file_flags3, "Override file flags for (signature)")
        ->group("Game setting overrides");
    create
        ->add_option("--attr-flags", create_attr_flags,
                     "Override attribute flags (CRC32, FILETIME, MD5)")
        ->group("Game setting overrides");
    // Compression settings overrides for files being added
    create->add_option("--flags", file_flags, "Override MPQ file flags for added files")
        ->group("Game setting overrides");
    create
        ->add_option("--compression", file_compression,
                     "Override compression for first sector of added files")
        ->group("Game setting overrides");
    create
        ->add_option("--compression-next", file_compression_next,
                     "Override compression for subsequent sectors of added files")
        ->group("Game setting overrides");

    // Subcommand: Add
    CLI::App *add = app.add_subcommand("add", "Add files to an existing MPQ archive");
    add->add_option("archive", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    add->add_option("files", add_files,
                    "Files or directories to add; pass - to read paths from stdin")
        ->required()
        ->expected(-1);
    add->add_option("-p,--path", base_path,
                    "Archive path for a single file, or prefix for a directory");
    add->add_flag("-w,--overwrite", add_overwrite,
                  "Overwrite file if it already is in MPQ archive");
    add->add_flag("-u,--update", add_update,
                  "Skip files whose archived size matches the on-disk size (directory add only)");
    add->add_option("--locale", base_locale, "Locale to use for added file")->check(locale_valid);
    add->add_option("-g,--game", base_game_profile,
                    "Game profile for compression rules. Valid options:\n" +
                        GameRules::GetAvailableProfiles())
        ->check(game_profile_valid);
    // Compression settings overrides
    add->add_option("--flags", file_flags, "Override MPQ file flags")
        ->group("Game setting overrides");
    add->add_option("--compression", file_compression, "Override compression for first sector")
        ->group("Game setting overrides");
    add->add_option("--compression-next", file_compression_next,
                    "Override compression for subsequent sectors")
        ->group("Game setting overrides");

    // Subcommand: Remove
    CLI::App *remove = app.add_subcommand("remove", "Remove files from an existing MPQ archive");
    remove->add_option("archive", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    remove
        ->add_option("files", remove_files,
                     "Archive paths of files to remove; pass - to read paths from stdin")
        ->required()
        ->expected(-1);
    remove->add_option("--locale", base_locale, "Locale of file to remove")->check(locale_valid);

    // Subcommand: List
    CLI::App *list = app.add_subcommand("list", "List files from the MPQ archive");
    list->add_option("target", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    list->add_option("-l,--listfile", base_listfile_name, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);
    list->add_flag("-d,--detailed", list_detailed,
                   "File listing with additional columns (default false)");
    list->add_flag("-a,--all", list_all, "File listing including hidden files (default false)");
    list->add_option("-p,--property", list_properties, "Prints only specific property values")
        ->check(CLI::IsMember(valid_file_list_properties));

    // Subcommand: Extract
    CLI::App *extract = app.add_subcommand("extract", "Extract files from the MPQ archive");
    extract->add_option("target", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    extract->add_option("-o,--output", base_output, "Output directory");
    extract->add_option("-f,--file", base_file, "Target file to extract");
    extract->add_flag("-k,--keep", extract_keep_folder_structure,
                      "Keep folder structure (default false)");
    extract
        ->add_option("-l,--listfile", base_listfile_name, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);
    extract->add_option("--locale", base_locale, "Preferred locale for extracted file");

    // Subcommand: Read
    CLI::App *read = app.add_subcommand("read", "Read a file from an MPQ archive");
    read->add_option("file", base_file, "File to read")->required();
    read->add_option("target", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    read->add_option("--locale", base_locale, "Preferred locale for read file");

    // Subcommand: Verify
    CLI::App *verify = app.add_subcommand("verify", "Verify the MPQ archive");
    verify->add_option("target", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    verify->add_flag("-p,--print", verify_print_signature, "Print the digital signature (in hex)");

    // Subcommand: Compact
    CLI::App *compact = app.add_subcommand("compact", "Compact the MPQ archive");
    compact->add_option("target", base_target, "Target MPQ archive")
        ->required()
        ->check(CLI::ExistingFile);
    compact
        ->add_option("-l,--listfile", base_listfile_name, "File listing content of an MPQ archive")
        ->check(CLI::ExistingFile);

    // Subcommand: Completion
    CLI::App *completion = app.add_subcommand("completion", "Generate shell completion script");
    CLI::App *completion_bash =
        completion->add_subcommand("bash", "Generate bash completion script");
    CLI::App *completion_zsh = completion->add_subcommand("zsh", "Generate zsh completion script");
    CLI::App *completion_ps =
        completion->add_subcommand("powershell", "Generate PowerShell completion script");
    CLI::App *completion_fish =
        completion->add_subcommand("fish", "Generate fish completion script");

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
        return HandleInfo(base_target, info_property);
    }

    if (app.got_subcommand(create)) {
        return HandleCreate(base_target, base_path, base_output, create_sign_archive, base_locale,
                            base_game_profile, create_mpq_version, create_stream_flags,
                            create_sector_size, create_raw_chunk_size, create_file_flags1,
                            create_file_flags2, create_file_flags3, create_attr_flags, file_flags,
                            file_compression, file_compression_next);
    }

    if (app.got_subcommand(add)) {
        std::vector<std::string> resolved_add_files;
        for (const auto &f : add_files) {
            if (f == "-") {
                std::string line;
                while (std::getline(std::cin, line)) {
                    if (!line.empty())
                        resolved_add_files.push_back(line);
                }
            } else {
                resolved_add_files.push_back(f);
            }
        }
        return HandleAdd(resolved_add_files, base_target, base_path, add_overwrite, add_update,
                         base_locale, base_game_profile, file_flags, file_compression,
                         file_compression_next);
    }

    if (app.got_subcommand(remove)) {
        std::vector<std::string> resolved_remove_files;
        for (const auto &f : remove_files) {
            if (f == "-") {
                std::string line;
                while (std::getline(std::cin, line)) {
                    if (!line.empty())
                        resolved_remove_files.push_back(line);
                }
            } else {
                resolved_remove_files.push_back(f);
            }
        }
        return HandleRemove(resolved_remove_files, base_target, base_locale);
    }

    if (app.got_subcommand(list)) {
        return HandleList(base_target, base_listfile_name, list_all, list_detailed,
                          list_properties);
    }

    if (app.got_subcommand(extract)) {
        std::optional<std::string> extract_file =
            base_file.empty() ? std::nullopt : std::make_optional(base_file);
        return HandleExtract(base_target, base_output, extract_file, extract_keep_folder_structure,
                             base_listfile_name, base_locale);
    }

    if (app.got_subcommand(read)) {
        return HandleRead(base_file, base_target, base_locale);
    }

    if (app.got_subcommand(verify)) {
        return HandleVerify(base_target, verify_print_signature);
    }

    if (app.got_subcommand(compact)) {
        return HandleCompact(base_target, base_listfile_name);
    }

    if (app.got_subcommand(completion)) {
        if (completion->got_subcommand(completion_bash)) {
            HandleCompletionBash();
            return 0;
        }
        if (completion->got_subcommand(completion_zsh)) {
            HandleCompletionZsh();
            return 0;
        }
        if (completion->got_subcommand(completion_ps)) {
            HandleCompletionPs();
            return 0;
        }
        if (completion->got_subcommand(completion_fish)) {
            HandleCompletionFish();
            return 0;
        }
        std::cerr << completion->help();
        return 1;
    }

    return 0;
}
