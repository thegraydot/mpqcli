#include "mpq/rename.h"

#include <optional>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"
#include "util/locales.h"

int HandleRename(const std::string &old_file, const std::string &new_file,
                 const std::string &target, const std::optional<std::string> &locale) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, 0)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    int result = RenameFile(archive, old_file, new_file, lcid);
    CloseMpqArchive(archive);
    return result;
}
