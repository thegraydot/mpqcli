#include "mpq/read.h"

#include <memory>
#include <ostream>

#include <StormLib.h>

#include "mpq/query.h"
#include "util/locales.h"

namespace mpqcli {

std::unique_ptr<char[]> ReadArchivedFile(HANDLE archive, const char *file_name,
                                         unsigned int *file_size, LCID preferred_locale,
                                         std::ostream &err) {
    SFileSetLocale(preferred_locale);
    if (!FileExistsInArchiveForLocale(archive, file_name, preferred_locale) &&
        !FileExistsInArchiveForLocale(archive, file_name, default_locale)) {
        err << "[!] Failed: File doesn't exist"
            << PrettyPrintLocale(preferred_locale, " for locale ", true) << ": " << file_name
            << std::endl;
        return nullptr;
    }

    HANDLE file;
    if (!SFileOpenFileEx(archive, file_name, SFILE_OPEN_FROM_MPQ, &file)) {
        err << "[!] Failed: File cannot be opened: " << file_name << std::endl;
        return nullptr;
    }

    *file_size = SFileGetFileSize(file, nullptr);
    if (*file_size == SFILE_INVALID_SIZE) {
        err << "[!] Failed: Invalid file size for: " << file_name << std::endl;
        SFileCloseFile(file);
        return nullptr;
    }

    auto file_content = std::make_unique<char[]>(*file_size);
    DWORD bytes_read;
    if (!SFileReadFile(file, file_content.get(), *file_size, &bytes_read, nullptr)) {
        err << "[!] Failed: Cannot read file contents for: " << file_name << std::endl;
        SFileCloseFile(file);
        return nullptr;
    }

    SFileCloseFile(file);
    return file_content;
}

} // namespace mpqcli
