#include "mpq/remove.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <StormLib.h>

#include "cli/commands.h"
#include "mpq/archive.h"
#include "util/locales.h"

namespace mpqcli {

int HandleRemove(const std::vector<std::string> &files, const std::string &target,
                 const std::optional<std::string> &locale) {
    HANDLE archive;
    if (!OpenMpqArchive(target, &archive, 0)) {
        return 1;
    }

    LCID lcid = locale.has_value() ? LangToLocale(locale.value()) : default_locale;
    std::unordered_set<std::string> seen;
    int overall_result = 0;
    for (const auto &f : files) {
        if (!seen.insert(f).second) {
            continue;
        }
        int result = RemoveFile(archive, f, lcid);
        if (result != 0) {
            overall_result = result;
        }
    }
    CloseMpqArchive(archive);
    return overall_result;
}

} // namespace mpqcli
