#include "commands/remove.h"

#include <ostream>
#include <string>
#include <unordered_set>

#include <StormLib.h>

#include "mpq/archive.h"
#include "mpq/remove.h"
#include "util/locales.h"

namespace mpqcli {

bool Remove(const RemoveOptions &options, std::ostream &err) {
    Archive archive = Archive::Open(options.archive, 0);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;
    std::unordered_set<std::string> seen;
    bool all_removed = true;
    for (const auto &f : options.files) {
        if (!seen.insert(f).second) {
            continue;
        }
        if (RemoveFile(archive.Handle(), f, lcid, err) != 0) {
            all_removed = false;
        }
    }
    archive.Close();
    return all_removed;
}

} // namespace mpqcli
