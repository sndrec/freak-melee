#ifndef MELEE_IT_NATIVE_ITEM_MODEL_ADMISSION_H
#define MELEE_IT_NATIVE_ITEM_MODEL_ADMISSION_H
#include <sysdolphin/baselib/dat_import.h>
#include "forward.h"
enum { NATIVE_ITEM_MODEL_NODE_LIMIT=4096 };
// Transition-only proof of the actual HSD load/ID-resolution/native traversal.
// The caller supplies the imported descriptor owner for precise diagnostics.
// This counts populated traversal slots, not the model descriptor's flag.
bool nativeItemModel_Admit(HSD_DatImport*,const ItemModelDesc*,u32 source_offset,u32* populated);
#endif
