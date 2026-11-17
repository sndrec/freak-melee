#include "native_shell_workshop_labels.h"

#ifdef MELEE_NATIVE

#include <cstring>

namespace WorkshopLabels {

const char* channel(unsigned channel)
{
    switch (channel) {
    case 1: return "Rotate X";
    case 2: return "Rotate Y";
    case 3: return "Rotate Z";
    case 4: return "Path";
    case 5: return "Position X";
    case 6: return "Position Y";
    case 7: return "Position Z";
    case 8: return "Scale X";
    case 9: return "Scale Y";
    case 10: return "Scale Z";
    default: return "Other channel";
    }
}

const char* action(const char* source)
{
    static const struct { const char* source; const char* label; } names[] = {
        { "Attack11", "Jab 1" }, { "Attack12", "Jab 2" },
        { "Attack100", "Rapid jab" }, { "AttackS3", "Forward tilt" },
        { "AttackS3S", "Forward tilt" }, { "AttackS3Hi", "Forward tilt (up)" },
        { "AttackS3Lw", "Forward tilt (down)" },
        { "AttackHi3", "Up tilt" }, { "AttackLw3", "Down tilt" },
        { "AttackS4", "Forward smash" }, { "AttackHi4", "Up smash" },
        { "AttackS4S", "Forward smash" }, { "AttackS4Hi", "Forward smash (up)" },
        { "AttackS4Lw", "Forward smash (down)" },
        { "AttackLw4", "Down smash" }, { "AttackAirN", "Neutral air" },
        { "AttackAirF", "Forward air" }, { "AttackAirB", "Back air" },
        { "AttackAirHi", "Up air" }, { "AttackAirLw", "Down air" },
        { "SpecialN", "Neutral special" }, { "SpecialS", "Side special" },
        { "SpecialHi", "Up special" }, { "SpecialLw", "Down special" },
        { "Walk", "Walk" }, { "Run", "Run" },
        { "Wait", "Idle" }, { "Fall", "Fall" },
    };
    for (const auto& name : names)
        if (std::strcmp(source, name.source) == 0) return name.label;
    return source;
}

unsigned group(const char* name)
{
    if (std::strncmp(name, "AttackAir", 9) == 0) return 1;
    if (std::strncmp(name, "Attack", 6) == 0 ||
        std::strncmp(name, "Catch", 5) == 0 ||
        std::strncmp(name, "Throw", 5) == 0) return 0;
    if (std::strncmp(name, "Special", 7) == 0) return 2;
    if (std::strncmp(name, "Walk", 4) == 0 ||
        std::strncmp(name, "Run", 3) == 0 ||
        std::strncmp(name, "Jump", 4) == 0 ||
        std::strncmp(name, "Fall", 4) == 0 ||
        std::strncmp(name, "Guard", 5) == 0) return 3;
    return 4;
}

} // namespace WorkshopLabels

#endif
