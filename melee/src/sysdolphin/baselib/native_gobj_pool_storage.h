#ifndef SYSDOLPHIN_NATIVE_GOBJ_POOL_STORAGE_H
#define SYSDOLPHIN_NATIVE_GOBJ_POOL_STORAGE_H
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/gobj.h>

enum { NATIVE_GOBJ_CAPACITY = 1024, NATIVE_GOBJ_PROC_CAPACITY = 4096 };
// Actual pool ownership, not mirrors of obsolete ObjAlloc descriptors.
extern NativeMatchPool nativeGObjPool;
extern NativeMatchPool nativeGObjProcPool;
bool nativeGObjPool_BeginScene(const HSD_GObjLibInitDataType*);
// Logical retirement only, after callbacks and before pointed heap reclamation.
// Does not run destructors. Backing remains reserved for the process.
bool nativeGObjPool_RetireScene(void);
#endif
#endif
