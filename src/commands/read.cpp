#include "commands/read.h"

#include <cstdint>
#include <ostream>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/read.h"
#include "util/format.h"
#include "util/locales.h"

namespace mpqcli {

bool Read(const ReadOptions &options, std::ostream &out, std::ostream &err) {
    Archive archive = Archive::Open(options.target, MPQ_OPEN_READ_ONLY);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;
    if (options.locale.has_value() && lcid == default_locale) {
        err << "[!] The locale '" << options.locale.value()
            << "' is unknown. Will use default locale instead." << std::endl;
    }

    uint32_t file_size;
    auto file_content =
        ReadArchivedFile(archive.Handle(), options.file.c_str(), &file_size, lcid, err);
    if (!file_content) {
        archive.Close();
        return false;
    }

    WriteBinary(out, file_content.get(), file_size);

    archive.Close();
    return true;
}

} // namespace mpqcli
