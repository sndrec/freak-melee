#ifndef MELEE_GM_NATIVE_REPLAY_ITEM_FIELDS_H
#define MELEE_GM_NATIVE_REPLAY_ITEM_FIELDS_H
#ifdef MELEE_NATIVE
#include "native_semantic_state.h"
// Optional named values for diagnosing item digests. No gameplay writes.
// FREAK_MELEE_ITEM_FIELDS=start:end[,start:end...] selects inclusive samples.
void nativeReplayItemFields_Trace(uint64_t sample,unsigned index,const Item* item,
    const NativeSemanticObjectMap* objects,const NativeSemanticItemTrace* fields);
#endif
#endif
