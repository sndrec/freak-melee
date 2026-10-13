#ifndef MELEE_GM_NATIVE_ROLLBACK_STATE_SECTION_H
#define MELEE_GM_NATIVE_ROLLBACK_STATE_SECTION_H

// Forced into game/HSD translation units by the native build. Keeping mutable
// decomp state in one owned PE section lets rollback register it without
// sweeping unrelated Aurora, CRT, or operating-system globals.
#if defined(MELEE_NATIVE) && defined(MELEE_ROLLBACK_AUDIO_STATE)
#pragma clang section data = ".maudio$m"
#elif defined(MELEE_NATIVE) && !defined(MELEE_ROLLBACK_HOST_STATE)
#pragma clang section data = ".mstate$m"
#endif

#endif
