#ifndef MELEE_GM_NATIVE_CONTENT_STORE_INTERNAL_H
#define MELEE_GM_NATIVE_CONTENT_STORE_INTERNAL_H

#include "native_content_store.h"

#ifdef MELEE_NATIVE
// Shared content hash implementation; builders and the cache use the same bytes.
bool nativeContent_Hash(const void* package, size_t size, NativeContentId* id);
#endif

#endif
