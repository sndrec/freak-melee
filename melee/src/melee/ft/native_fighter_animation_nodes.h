#ifndef MELEE_FT_NATIVE_FIGHTER_ANIMATION_NODES_H
#define MELEE_FT_NATIVE_FIGHTER_ANIMATION_NODES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Authoring transition: rewrite every figatree root in one animation DAT.
// new_to_old maps destination animation nodes to original animation nodes;
// UINT16_MAX inserts an unanimated node. A source node may appear at most once.
// Omitted source nodes lose their tracks. The caller resolves costume/reserved
// part mappings before this operation; these are animation-node indices.
// Compressed streams, track formats, frame counts, flags and root aliases are
// retained. Output owns a malloc allocation; pass NULL/0 output fields. Failure
// leaves input and output unchanged. This does not publish a partial rig edit.
bool nativeFighterAnimation_RemapNodes(const void* source, size_t source_size,
    unsigned old_count, const uint16_t* new_to_old, unsigned new_count,
    void** output, size_t* output_size, char* error, size_t error_size);

#ifdef __cplusplus
}
#endif
#endif
