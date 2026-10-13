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
    if (opcode == 59) return "Timed hitbox";
    static const char* control[] = {
        "End script", "Wait frames", "Wait until frame", "Begin loop",
        "Repeat loop", "Call subroutine", "Return from subroutine", "Go to",
        "Wait for animation end", "Background flash"
    };
    if (opcode < sizeof(control) / sizeof(control[0])) return control[opcode];
    static const char* names[] = {
        "Spawn effect",
        "Create hitbox",
        "Set hitbox damage",
        "Set hitbox size",
        "Set hitbox flags",
        "Clear hitbox",
        "Clear all hitboxes",
        "Play sound",
        "Play smash sound",
        "Set command variable",
        "Throw release / reverse facing",
        "Set throw flag 1",
        "Set throw flag 2",
        "Allow interrupt",
        "Set neutral cue",
        "Set grounded / airborne",
        "Set body collision",
        "Set global hurt state",
        "Set bone hurt state",
        "Jab combo window",
        "Rapid jab window",
        "Set visibility group",
        "Restore visibility groups",
        "Hide visibility groups",
        "Define throw values",
        "Held item visibility",
        "Attached article visibility",
        "Fighter visibility",
        "Play random sound",
        "Play stage sound",
        "Set texture animation",
        "Set part animation",
        "Animate held article",
        "Start rumble",
        "Stop rumble",
        "Animate beam sword",
        "Start color animation",
        "Reset color animation",
        "Hide shadow",
        "Set attack trail",
        "Toggle bone dynamics",
        "Add self damage",
        "Ground alignment flags",
        "Copy hat visibility",
        "Footstep sound / effect",
        "Landing sound / effect",
        "Smash charge window",
        "Refraction / vibration",
        "Wind effect",
    };
    if (opcode >= 10 && opcode < 10 + sizeof(names) / sizeof(names[0])) return names[opcode - 10];
    return "Unknown command " + std::to_string(opcode);
}

const char* command_description(unsigned opcode)
{
    switch (opcode) {
    case 0: return "Stops this action's event script.";
    case 1: return "Waits this many frames before running the following events.";
    case 2: return "Waits until the specified animation frame.";
    case 3: return "Starts a repeated section of events.";
    case 4: return "Repeats the current loop until its count is exhausted.";
    case 5: return "Runs another section of events, then returns here.";
    case 6: return "Returns to the event following a subroutine call.";
    case 7: return "Continues running events at another script location.";
    case 8: return "Pauses event execution until the animation finishes.";
    case 9: return "Starts a background flash with the chosen effect and priority.";
    case 10: return "Spawns a visual effect attached to a bone.";
    case 11: return "Creates an attack hitbox with damage, knockback, size and attachment settings.";
    case 59: return "Creates a hitbox for the specified number of animation frames.";
    case 12: return "Changes the damage of an existing hitbox.";
    case 13: return "Changes the size of an existing hitbox.";
    case 14: return "Changes one of the collision flags of an existing hitbox.";
    case 15: return "Disables one attack hitbox.";
    case 16: return "Disables all attack hitboxes.";
    case 17: return "Plays a sound with the specified volume and panning.";
    case 18: return "Plays this fighter's smash attack sound cue.";
    case 19: return "Assigns a value to one of four action command variables.";
    case 20: return "Signals a throw release or a facing reversal to the action's behavior.";
    case 21: case 22: return "Signals a throw timing flag to the action's behavior.";
    case 23: return "Opens the action's interrupt window.";
    case 24: return "Signals the action's neutral timing cue.";
    case 25: return "Switches to grounded or airborne movement and updates available jumps.";
    case 26: return "Changes the fighter's body collision state.";
    case 27: return "Changes the hurt state of all hurtboxes.";
    case 28: return "Changes the hurt state of a selected bone.";
    case 29: return "Opens the jab combo timing window.";
    case 30: return "Changes the rapid jab input window.";
    case 31: return "Chooses the visible mesh variant of a fighter visibility group.";
    case 32: return "Restores the previous mesh variants in all visibility groups.";
    case 33: return "Hides all visibility-group mesh variants.";
    case 34: return "Defines a throw record's damage, knockback and sound. Release throw applies the throw later.";
    case 35: return "Changes the visibility of a held item and runs its visibility callbacks.";
    case 36: return "Changes the attached article visibility flag.";
    case 37: return "Changes the fighter visibility flag.";
    case 38: return "Picks and plays a sound from a list of random choices.";
    case 39: return "Plays a sound with stage-dependent settings.";
    case 40: return "Sets the animation frame of one or two texture groups.";
    case 41: return "Starts the selected part animation at the chosen frame.";
    case 42: return "Selects a held article animation and its playback timing.";
    case 43: return "Starts a controller rumble pattern for the specified duration.";
    case 44: return "Stops a controller rumble pattern.";
    case 45: return "Changes the held Beam Sword's animation or visible variant.";
    case 46: return "Starts a fighter color animation.";
    case 47: return "Resets a fighter color animation.";
    case 48: return "Changes whether the fighter's shadow is hidden.";
    case 49: return "Sets the attack trail sample count and chooses a fighter or held-sword trail.";
    case 50: return "Switches a bone between animation and dynamic movement at the current frame.";
    case 51: return "Adds damage to the fighter.";
    case 52: return "Sets foot and body alignment flags for standing on sloped ground.";
    case 53: return "Changes whether a copy-ability hat is drawn.";
    case 54: return "Plays a surface-dependent footstep sound and effect.";
    case 55: return "Plays a surface-dependent sound and a visual effect.";
    case 56: return "Starts a smash charge window with a charge limit, damage growth and color animation.";
    case 57: return "Sets fighter refraction and the vibration-frame counter.";
    case 58: return "Starts a wind effect using the chosen bone, duration, position and force.";
    default: return "This command's behavior has not yet been identified.";
    }
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
