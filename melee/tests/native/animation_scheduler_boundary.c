#include <sysdolphin/baselib/gobj.h>
// Isolated animation/heap tests run outside the GObj scheduler. Ownership,
// decoding and pool lifetime checks still use the production implementations.
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
HSD_GObj* HSD_GObj_804D7818;
struct _unk_gobj_struct HSD_GObj_DelayedProcInfo;
