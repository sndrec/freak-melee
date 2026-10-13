#ifndef MELEE_GM_NATIVE_REPLAY_RESOURCES_H
#define MELEE_GM_NATIVE_REPLAY_RESOURCES_H

#include <melee/it/forward.h>
#include <stdint.h>

// Read-only diagnostics for separately allocated article nodes. Stock tether
// links and owned resource arrays are printed in the same attachment-tip order.
void nativeReplayResources_Trace(uint64_t sample, unsigned item_index,
                                 Item_GObj* object);

#endif
