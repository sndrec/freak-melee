#ifndef MELEE_GM_NATIVE_TEXTURE_IDENTITY_SNAPSHOT_H
#define MELEE_GM_NATIVE_TEXTURE_IDENTITY_SNAPSHOT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Aurora process-lifetime identity state, independent of texture image payloads.
// This private snapshot is used with resident engine code at an idle boundary.
typedef struct NativeTextureIdentitySnapshot {
    uint32_t texture,palette;
} NativeTextureIdentitySnapshot;
bool nativeTextureIdentity_Save(void*,size_t capacity,size_t* written);
bool nativeTextureIdentity_Validate(const void*,size_t bytes);
bool nativeTextureIdentity_Load(const void*,size_t bytes);
#endif
