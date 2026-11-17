#pragma once
#include <dolphin/types.h>
#ifdef __cplusplus
extern "C" {
#endif
// Source bit widths run from most-significant bit to least-significant bit.
// A zero count denotes one complete payload word. Word zero begins with
// opcode.
unsigned ftActionData_SourceFieldWidths(unsigned opcode, unsigned word,
                                        u8 widths[8]);
u32 ftActionData_SourceCommandWidth(u32 source_word);
u32 ftActionData_SourceFieldMaximum(unsigned opcode, unsigned field, unsigned width);
#ifdef __cplusplus
}
#endif
