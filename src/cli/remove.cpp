#include "mpq/remove.h"

#include <optional>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"
#include "util/locales.h"

namespace mpqcli {

int HandleRemove(const std::vector<std::string> &files, const std::string &target,
                 const std::optional<std::string> &locale, std::ostream &out, std::ostream &err) {
    Archive archive = Archive::Open(target, 0);

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    std::unordered_set<std::string> seen;
    int overall_result = 0;
    for (const auto &f : files) {
        if (!seen.insert(f).second) {
            continue;
        }
        int result = RemoveFile(archive.Handle(), f, lcid, out, err);
        if (result != 0) {
            overall_result = result;
        }
    }
    archive.Close();
    return overall_result;
}

} // namespace mpqcli
