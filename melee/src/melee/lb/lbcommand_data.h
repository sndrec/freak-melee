#ifndef MELEE_LB_LBCOMMAND_DATA_H
#define MELEE_LB_LBCOMMAND_DATA_H

#include <sysdolphin/baselib/dat_import.h>

// The loader records a command's width at its first word, this marker at its
// operands, and zero everywhere else.
enum { LB_COMMAND_OPERAND = 255 };

typedef struct LbCommandEntry {
    u32 source;
    const void* owner;
} LbCommandEntry;

typedef bool (*LbCommandOperandCheck)(HSD_DatImport*, const void* owner,
                                      u32 offset, u32 raw);

bool lbCommandData_ValidateFlow(HSD_DatImport* import, const u8* widths,
                                u32 word_count, const LbCommandEntry* entries,
                                u32 entry_count, bool animation_wait_yields,
                                LbCommandOperandCheck check);

#endif
