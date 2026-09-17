#ifndef GAMERULES_PROFILES_H
#define GAMERULES_PROFILES_H

namespace mpqcli {

enum class GameProfile {
    GENERIC,       // Default/generic MPQ with basic compression
    DIABLO1,       // Diablo I / Hellfire (1997)
    LORDSOFMAGIC,  // Lords of Magic SE (1998)
    STARCRAFT1,    // StarCraft / Brood War (1998)
    WARCRAFT2,     // Warcraft II: Battle.net Edition (1999)
    DIABLO2,       // Diablo II / Lords of Destruction (2000)
    WARCRAFT3,     // Warcraft III / The Frozen Throne (2002)
    WARCRAFT3_MAP, // Warcraft III Map files (2002)
    WOW_1X,        // World of Warcraft 1 - Vanilla (2004)
    WOW_2X,        // World of Warcraft 2 - The Burning Crusade (2007)
    WOW_3X,        // World of Warcraft 3 - Wrath of the Lich King (2008)
    WOW_4X,        // World of Warcraft 4 - Cataclysm (2010)
    WOW_5X,        // World of Warcraft 5 - Mists of Pandaria (2012)
    STARCRAFT2,    // StarCraft II (2010)
    DIABLO3        // Diablo III (2012)
};

} // namespace mpqcli

#endif
