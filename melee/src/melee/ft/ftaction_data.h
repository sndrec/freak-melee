#ifndef MELEE_FT_FTACTION_DATA_H
#define MELEE_FT_FTACTION_DATA_H

#include "ftwaitanim.h"

#include <sysdolphin/baselib/dat_import.h>

struct Fighter_WaitAnimData;
union CmdUnion;

typedef struct FtActionDataStats {
    union CmdUnion* commands;
    u32 source_begin;
    u32 word_count;
    u32 instruction_count;
    u32 entry_count;
} FtActionDataStats;

bool ftActionData_Import(HSD_DatImport* import, u32 wait_source,
                         u32 wait_count, struct Fighter_WaitAnimData* table,
                         FtActionDataStats* stats);

// Width of one original big-endian subaction command, in 32-bit words.
// Source bit widths, in most-significant-bit order. A zero count denotes a
// complete 32-bit payload word. Opcode occupies the first field of word zero.
unsigned ftActionData_SourceFieldWidths(unsigned opcode, unsigned word, u8 widths[8]);
u32 ftActionData_SourceCommandWidth(u32 source_word);

// The retail hitbox handler reads bit 19 (least-significant-bit numbering)
// from the following command word, regardless of that command's opcode.
bool ftActionData_NextHitboxFlag(const union CmdUnion* command);

#endif
