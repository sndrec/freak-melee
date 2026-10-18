#ifndef MELEE_FT_NATIVE_FIGHTER_RIG_EDIT_H
#define MELEE_FT_NATIVE_FIGHTER_RIG_EDIT_H
#include "native_fighter_project.h"
typedef enum NativeFighterRigEditKind {
    NativeRigEdit_Add, NativeRigEdit_Reparent, NativeRigEdit_Remove
} NativeFighterRigEditKind;
typedef struct NativeFighterRigEdit {
    NativeFighterRigEditKind kind;
    int32_t bone_id, parent_id;
    char name[64]; // Required for Add; a unique printable bone name.
} NativeFighterRigEdit;
#ifdef __cplusplus
extern "C" {
#endif
// Complete in-memory authoring transaction. Only authored extra bones can be
// moved/removed. Add appends a child at its parent's origin. Reparent preserves
// world rest transforms; Remove reparents children in place to its parent and
// rejects live references to the removed bone. Required base helpers stay put.
// All four core assets and facts change together after admission. Failure keeps
// the project and result ID untouched. No files or runtime scene are changed.
bool nativeFighterRig_Edit(NativeFighterProject*, const NativeFighterRigEdit*,
                           int32_t* result_id, char* error, size_t error_size);
#ifdef __cplusplus
}
#endif
#endif
