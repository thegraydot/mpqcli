#include "mpq/rename.h"

#include <iostream>
#include <string>

#include <StormLib.h>

#include "mpq/query.h"
#include "util/format.h"
#include "util/locales.h"
#include "util/path.h"

namespace mpqcli {

int RenameFile(HANDLE archive, const std::string &old_archive_file_path,
               const std::string &new_archive_file_path, LCID locale) {
    SFileSetLocale(locale);
    const std::string new_stored_path = WindowsifyFilePath(new_archive_file_path);
    std::cout << "[~] Renaming file" << PrettyPrintLocale(locale, " for locale ") << ": "
              << old_archive_file_path << " -> " << new_stored_path << std::endl;

    if (!FileExistsInArchiveForLocale(archive, old_archive_file_path, locale)) {
        std::cerr << "[!] Failed: File doesn't exist"
                  << PrettyPrintLocale(locale, " for locale ", true) << ": "
                  << old_archive_file_path << std::endl;
        return 1;
    }

    if (!SFileRenameFile(archive, old_archive_file_path.c_str(), new_stored_path.c_str())) {
        std::cerr << "[!] Failed: File cannot be renamed"
                  << PrettyPrintLocale(locale, " for locale ", true) << ": "
                  << old_archive_file_path << " -> " << new_stored_path << std::endl;
        return 1;
    }

    return 0;
}

} // namespace mpqcli
