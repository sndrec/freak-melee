#ifndef SYSDOLPHIN_SFXCOMMANDS_H
#define SYSDOLPHIN_SFXCOMMANDS_H

#include <Runtime/platform.h>

typedef struct HSD_SfxCommands {
    u32* words;
    u32* banks;
    u32* streams;
    u32 bank_count;
    u32 stream_count;
} HSD_SfxCommands;

// Validate a word-aligned SEM file, then convert its words in place from BE.
// The caller retains storage for the lifetime of all playing command streams.
bool HSD_SfxCommandsDecode(HSD_SfxCommands* commands, void* data, size_t size);

#endif
