#include "object.h"
#include "aobj.h"
#include "displayfunc.h"
#include "fobj.h"
#include "id.h"
#include "list.h"
#include "mtx.h"
#include "robj.h"
#include "shadow.h"
#include "state.h"
#include "tev.h"
#ifdef MELEE_NATIVE
#include "native_animation_pool.h"
#endif

void HSD_ObjInit(void)
{
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0, nativeAnimationPool_BeginHeap(),
        "Cannot initialize animation ownership during an active callback.\n");
#endif
    HSD_ListInitAllocData();
#ifndef MELEE_NATIVE
    HSD_AObjInitAllocData();
    HSD_FObjInitAllocData();
#endif
    HSD_IDInitAllocData();
    HSD_VecInitAllocData();
    HSD_MtxInitAllocData();
    HSD_RObjInitAllocData();
    HSD_RenderInitAllocData();
    HSD_ShadowInitAllocData();
    HSD_ZListInitAllocData();
}

HSD_ClassInfo hsdObj = { ObjInfoInit };

void ObjInfoInit(void)
{
    hsdInitClassInfo(&hsdObj, &hsdClass, "sysdolphin_base_library", "hsd_obj",
                     sizeof(HSD_ObjInfo), sizeof(HSD_Obj));
}
