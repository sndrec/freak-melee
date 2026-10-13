#include "gobjuserdata.h"

#include "debug.h"
#include "gobj.h"
#ifdef MELEE_NATIVE
#include "gobjplink.h"
#include "native_gobj_pool_storage.h"
#endif

void GObj_InitUserData(HSD_GObj* gobj, u8 kind,
                       void (*remove_func)(void* user_data), void* data)
{
    HSD_ASSERT(40, gobj->user_data_kind == HSD_GOBJ_USER_DATA_NONE);
    gobj->user_data_kind = kind;
    gobj->user_data = data;
    gobj->user_data_remove_func = remove_func;
}

void GObj_RemoveUserData(HSD_GObj* gobj)
{
#ifdef MELEE_NATIVE
    if (gobj->native_userdata_removing) return;
#endif
    if (gobj->user_data_kind == HSD_GOBJ_USER_DATA_NONE) {
        return;
    }

    HSD_ASSERT(99, gobj->user_data_remove_func);
#ifdef MELEE_NATIVE
    NativeMatchPoolReference reference;
    HSD_ASSERTREPORT(0, nativeMatchPool_Reference(&nativeGObjPool,gobj,&reference),
        "Invalid native userdata removal owner.\n");
    gobj->native_userdata_removing = true;
#endif
    gobj->user_data_remove_func(gobj->user_data);
#ifdef MELEE_NATIVE
    if (nativeMatchPool_Resolve(reference) != gobj) return;
#endif
    gobj->user_data_kind = HSD_GOBJ_USER_DATA_NONE;
    gobj->user_data = NULL;
#ifdef MELEE_NATIVE
    gobj->native_userdata_removing = false;
    if (gobj->native_destroy_requested) {
        gobj->native_destroy_requested = false;
        if (!gobj->native_destroying) HSD_GObjFree(gobj);
    }
#endif
}
