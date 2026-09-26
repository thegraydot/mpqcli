#include "mpq/rename.h"

#include <optional>
#include <ostream>
#include <string>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"
#include "util/locales.h"

namespace mpqcli {

int HandleRename(const std::string &old_file, const std::string &new_file,
                 const std::string &target, const std::optional<std::string> &locale,
                 std::ostream &out, std::ostream &err) {
    Archive archive = Archive::Open(target, 0);

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    const int result = RenameFile(archive.Handle(), old_file, new_file, lcid, out, err);
    archive.Close();
    return result;
}

} // namespace mpqcli
