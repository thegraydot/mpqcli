#include "mpq/read.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"
#include "util/format.h"
#include "util/locales.h"

namespace mpqcli {

int HandleRead(const std::string &file, const std::string &target,
               const std::optional<std::string> &locale, std::ostream &out, std::ostream &err) {
    Archive archive = Archive::Open(target, MPQ_OPEN_READ_ONLY);

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    if (locale.has_value() && lcid == default_locale) {
        out << "[!] Warning: The locale '" << locale.value()
            << "' is unknown. Will use default locale instead." << std::endl;
    }

    uint32_t file_size;
    auto file_content = ReadFile(archive.Handle(), file.c_str(), &file_size, lcid, err);
    if (!file_content) {
        archive.Close();
        return 1;
    }

    WriteBinary(out, file_content.get(), file_size);

    archive.Close();
    return 0;
}

} // namespace mpqcli
