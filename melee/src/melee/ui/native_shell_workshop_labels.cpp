#include "native_shell_workshop_labels.h"

#ifdef MELEE_NATIVE

#include <cstring>

namespace WorkshopLabels {

const char* event(unsigned kind)
{
    static const char* labels[] = {
        "Sync timer", "Async timer", "Create hitbox", "Set hitbox damage",
        "Set hitbox scale", "Set hitbox flag", "Set command variable",
        "Body collision", "Global hurt state", "Bone hurt state",
        "Sound effect", "Spawn effect", "Define throw values", "Allow interrupt",
        "Neutral cue", "Clear hitbox", "Clear all hitboxes",
        "Release throw", "Reverse facing", "Set grounded / airborne"
    };
    return kind < sizeof(labels) / sizeof(labels[0]) ? labels[kind]
                                                    : "Original command";
}

std::string command(unsigned opcode)
{
    static const char* control[] = {
        "End script", "Wait frames", "Wait until frame", "Begin loop",
        "Repeat loop", "Call subroutine", "Return from subroutine", "Go to",
        "Wait for animation end", "Command 9"
    };
    if (opcode < sizeof(control) / sizeof(control[0])) return control[opcode];
    return "Opcode " + std::to_string(opcode);
}

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
        { "AttackHi3", "Up tilt" }, { "AttackLw3", "Down tilt" },
        { "AttackS4", "Forward smash" }, { "AttackHi4", "Up smash" },
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
