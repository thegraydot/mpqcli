#include "mpq/remove.h"

#include <ostream>
#include <string>

#include <StormLib.h>

#include "mpq/query.h"
#include "util/locales.h"

namespace mpqcli {

int RemoveFile(HANDLE archive, const std::string &archive_file_path, LCID locale,
               std::ostream &err) {
    SFileSetLocale(locale);
    err << "[-] Removing file" << PrettyPrintLocale(locale, " for locale ") << ": "
        << archive_file_path << std::endl;

    if (!FileExistsInArchiveForLocale(archive, archive_file_path, locale)) {
        err << "[!] Failed: File doesn't exist" << PrettyPrintLocale(locale, " for locale ", true)
            << ": " << archive_file_path << std::endl;
        return 1;
    }

    if (!SFileRemoveFile(archive, archive_file_path.c_str(), 0)) {
        err << "[!] Failed: File cannot be removed"
            << PrettyPrintLocale(locale, " for locale ", true) << ": " << archive_file_path
            << std::endl;
        return 1;
    }

    return 0;
}

} // namespace mpqcli
