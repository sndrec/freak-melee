#ifndef MELEE_IT_ITCOMMAND_DATA_H
#define MELEE_IT_ITCOMMAND_DATA_H

#include <sysdolphin/baselib/dat_import.h>
#include "forward.h"

union CmdUnion;

typedef struct ItScriptEntry {
    union CmdUnion** destination;
    u32 source;
    // Imported immutable owner; model topology is available before collection.
    const Article* article;
} ItScriptEntry;

typedef struct ItScriptCollect {
    ItScriptEntry* entries;
    u32 count;
    u32 capacity;
} ItScriptCollect;

typedef struct ItCommandData {
    union CmdUnion* commands;
    u32 source_begin;
    u32 word_count;
    u32 entry_count;
    u32 instruction_count;
} ItCommandData;

bool itCommandData_Import(HSD_DatImport* import,
                          const ItScriptCollect* scripts,
                          ItCommandData* output);

#endif
