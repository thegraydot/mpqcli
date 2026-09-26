#include "commands/rename.h"

#include <ostream>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/rename.h"
#include "util/locales.h"

namespace mpqcli {

bool Rename(const RenameOptions &options, std::ostream &err) {
    Archive archive = Archive::Open(options.archive, 0);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;
    const int result = RenameFile(archive.Handle(), options.old_file, options.new_file, lcid, err);
    archive.Close();
    return result == 0;
}

} // namespace mpqcli
