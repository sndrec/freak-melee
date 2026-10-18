#ifndef MELEE_FT_NATIVE_COSTUME_RUNTIME_H
#define MELEE_FT_NATIVE_COSTUME_RUNTIME_H

#ifdef MELEE_NATIVE

#include <melee/ft/forward.h>
#include <melee/gm/native_content_store.h>

#include <stdbool.h>
#include <stdint.h>

// Load an installed cosmetic at a scene transition. NULL root uses the normal
// per-installation collection. The package remains immutable while selected.
bool nativeCostumeRuntime_LoadInstalled(unsigned port,
                                        const NativeContentId* id,
                                        uint8_t character,
                                        uint8_t base_costume,
                                        const char* root);
bool nativeCostumeRuntime_LoadCached(unsigned port,
                                     const NativeContentId* id,
                                     uint8_t character,
                                     uint8_t base_costume,
                                     const char* root);
bool nativeCostumeRuntime_LoadCustom(unsigned port,
                                     const NativeContentId* id,
                                     const NativeContentId* fighter_id,
                                     FighterKind fighter_kind,
                                     const char* root,
                                     bool require_installed);
const UnkCostumeStruct* nativeCostumeRuntime_ForFighter(const Fighter* fp);
// Immutable selected-skin configuration, or the ordinary asset index for stock.
uint8_t nativeCostumeRuntime_ModelVariant(Fighter* fp);
void nativeCostumeRuntime_Clear(void);
void nativeCostumeRuntime_Inspect(void);

#endif
#endif
