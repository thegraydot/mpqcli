#include "commands/add.h"

#include <atomic>
#include <ostream>
#include <string>

#include <StormLib.h>

#include "gamerules/rules.h"
#include "mpq/add.h"
#include "mpq/archive.h"
#include "util/locales.h"

namespace mpqcli {

bool Add(const AddOptions &options, std::ostream &err, const std::atomic<bool> &cancelled) {
    Archive archive = Archive::Open(options.archive, 0);

    LCID lcid = options.locale.has_value() ? LangToLocale(options.locale.value()) : default_locale;

    GameProfile profile;
    if (options.game_profile.has_value()) {
        profile = GameRules::StringToProfile(options.game_profile.value());
        err << "[*] Using game profile: " << options.game_profile.value() << std::endl;
    } else {
        profile = GameRules::GetDefaultProfile();
    }
    GameRules game_rules(profile);

    int files_skipped = 0;
    const int result =
        AddInputs(archive.Handle(), options.files, options.path, lcid, game_rules, err, cancelled,
                  options.compression_overrides, options.overwrite, options.update, &files_skipped);

    // Skipping pre-existing files is the default, so point at the flags that change it
    // rather than letting the run look like it silently did nothing.
    if (!options.overwrite && !options.update && files_skipped > 0) {
        err << "[*] " << files_skipped
            << " file(s) already in the archive were skipped. Use --overwrite to replace "
               "them, or --update to replace only the ones that changed."
            << std::endl;
    }

    archive.Close();
    return result == 0;
}

} // namespace mpqcli
