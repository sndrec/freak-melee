#ifndef MELEE_FT_NATIVE_FIGHTER_MODEL_GRAPH_H
#define MELEE_FT_NATIVE_FIGHTER_MODEL_GRAPH_H

#include <melee/ft/forward.h>
#include <sysdolphin/baselib/forward.h>
#include <stdbool.h>

// Scene setup: fill an owned omitted part in both model poses. Descriptor
// selection and insertion metadata come from the current package's assets.
HSD_Joint* nativeFighterModelGraph_JointAt(HSD_Joint* root, unsigned ordinal);
bool nativeFighterModelGraph_Install(Fighter_GObj*, int graph_slot, int omission);

#endif
