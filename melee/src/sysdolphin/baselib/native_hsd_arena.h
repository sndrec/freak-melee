#ifndef SYSDOLPHIN_NATIVE_HSD_ARENA_H
#define SYSDOLPHIN_NATIVE_HSD_ARENA_H
#include <melee/gm/native_match_arena.h>
enum { NATIVE_HSD_ARENA_BYTES = 4 * 1024 * 1024 };
extern NativeMatchArena nativeHsdArena;
bool nativeHsdArena_BeginHeap(void);
bool nativeHsdArena_ResetHistory(void);
// Called once after scene construction, before binding private history. The
// class tree and definitions must remain fixed until that history is retired.
bool nativeHsdArena_AdmitHistory(void);
#endif
