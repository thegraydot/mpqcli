#include "mpq/read.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"
#include "util/format.h"
#include "util/locales.h"

namespace mpqcli {

int HandleRead(const std::string &file, const std::string &target,
               const std::optional<std::string> &locale) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, MPQ_OPEN_READ_ONLY)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    if (locale.has_value() && lcid == default_locale) {
        std::cout << "[!] Warning: The locale '" << locale.value()
                  << "' is unknown. Will use default locale instead." << std::endl;
    }

    uint32_t file_size;
    auto file_content = ReadFile(archive, file.c_str(), &file_size, lcid);
    if (!file_content) {
        CloseMpqArchive(archive);
        return 1;
    }

    PrintAsBinary(file_content.get(), file_size);

    CloseMpqArchive(archive);
    return 0;
}

} // namespace mpqcli
