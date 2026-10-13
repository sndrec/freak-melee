#include "ftdemo.h"

#include <Runtime/platform.h>

#include <melee/pl/forward.h>

#include <math.h>

#include "fighter.h"
#ifdef MELEE_NATIVE
#include "native_fighter_pool.h"
#include "native_fighter_lifetime.h"
#endif
#include "ft_0BEC.h"
#include "ft_0BEF.h"
#include "ft_0BF0.h"
#include "ft_0C88.h"
#include "ftanim.h"
#include "ftCo_800C7070.h"
#include "ftCo_800C70D0.h"
#include "ftCo_800C7178.h"
#include "ftCo_800C7220.h"
#include "ftCo_800C739C.h"
#include "ftCo_800C7434.h"
#include "ftcoll.h"
#include "ftdata.h"
#include "ftdrawcommon.h"
#include "ftdynamics.h"
#include "ftlib.h"
#include "ftparts.h"
#include "native_fighter_runtime.h"
#include "native_callback_lifetime.h"
#include "kinds/ftCommon/ftCo_09F4.h"
#include "kinds/ftCommon/ftCo_DemoCallback0.h"
#include "types.h"
#include <melee/cm/camera.h>
#include <melee/ef/efasync.h>
#include <melee/lb/lbshadow.h>
#include <melee/pl/types.h>
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/objalloc.h>

static HSD_GObjEvent on_create_fighter[16] = {
    ftCo_800BECB0, ftCo_800BED88, ftCo_800BEF04, NULL,
    ftCo_800BEFD4, ftCo_800BF034, ftCo_800BF108, ftCo_800C6150,
    ftCo_800C7070, ftCo_800C70D0, ftCo_800C7178, ftCo_800C7220,
    ftCo_800C7294, ftCo_800C7308, ftCo_800C739C, ftCo_800C7434
};

bool ftDemo_HasResultClips(FighterKind kind, int variant)
{
    // Win1, Win2, Win3 use separate entry and waiting animations; Lose is
    // self-looping. Demo descriptor 4 is intentionally empty in retail data.
    static const u8 entries[5] = { 0, 2, 5, 0xFF, 9 };
    if ((unsigned) kind >= Ft_Kind_RuntimeCapacity ||
        (unsigned) variant >= 5 || entries[variant] == 0xFF) return false;
    ftData* data = gFtDataList[kind];
    unsigned entry = entries[variant];
    unsigned last = variant == 4 ? entry : entry + 1;
    if (data == NULL || data->x14 == NULL ||
        ftData_UnkIntPairs[kind].count <= last) return false;
    return data->x14[entry].x14 != NULL && data->x14[last].x14 != NULL;
}

static void initFighter(HSD_GObj* gobj, plAllocInfo2* alloc_info)
{
    struct plAllocInfo temp1 = { 0 };
    temp1.internal_id = alloc_info->internal_id;
    temp1.slot = alloc_info->slot;
    temp1.b0 = alloc_info->has_transformation;
    temp1.x5 = -1;
    Fighter_UnkInitLoad_80068914(gobj, &temp1);
}

Fighter_GObj* ftDemo_CreateFighter(plAllocInfo2* alloc_info)
{
    if (alloc_info == NULL || (unsigned)alloc_info->unk8 >= 16) return NULL;
#ifdef MELEE_NATIVE
    // Source packages do not carry a native custom demo selector. Common
    // results variants use their owned imported demo clips directly.
    if (nativeFighterRuntime_SourcePackage(alloc_info->internal_id) != NULL &&
        alloc_info->unk8 >= 9 &&
        ftData_UnkDemoCallbacks0[alloc_info->internal_id] == NULL) return NULL;
#endif
    Fighter_GObj* gobj = GObj_Create(HSD_GOBJ_CLASS_FIGHTER, 8, 0);
#ifdef MELEE_NATIVE
    if(gobj==NULL)return NULL;
    NativeMatchPoolReference object_reference;
    if(!nativeMatchPool_Reference(&nativeGObjPool,gobj,&object_reference)) {
        HSD_GObjFree(gobj);return NULL;
    }
#endif
    GObj_SetupGXLink(gobj, ftDrawCommon_80080E18, 5, 0);
    {
#ifdef MELEE_NATIVE
        NativeMatchPoolReference reference;
        NativeFighterPoolSlot* own=nativeMatchPool_Allocate(&nativeFighterPool,&reference);
        if(own==NULL){HSD_GObjFree(gobj);return NULL;}
        Fighter* fp=&own->actor;
        fp->dat_attrs_backup=own->attributes;
#else
        Fighter* fp = HSD_ObjAlloc(&fighter_alloc_data);
        // Demo fighters use the same native layout and callback storage as
        // gameplay fighters. Pool contents from a previous life are not state.
        *fp = (Fighter){ 0 };
        fp->dat_attrs_backup = HSD_ObjAlloc(&fighter_dat_attrs_alloc_data);
#endif
        GObj_InitUserData(gobj, 4, Fighter_Unload_8006DABC, fp);
        ftData_8008572C(alloc_info->internal_id);
        initFighter(gobj, alloc_info);
        fp->x18 = 14;
        fp->x1C_actionStateList = ftData_803C52A0;
        fp->x20_actionStateList = ftData_UnkMotionStates0[fp->kind];
        fp->x24 = fp->ft_data->x14;
        fp->x28 = fp->ft_data->x18;
        if (ftData_UnkBytePerCharacter[fp->kind] != 0xFF) {
            efAsync_LoadSync(ftData_UnkBytePerCharacter[fp->kind]);
        }
        ftData_80085820(fp->kind, fp->x619_costume_id);
        Fighter_UnkUpdateCostumeJoint_800686E4(gobj);
        {
            enum_t a, b;
            if (alloc_info->unk8 >= 9) {
                ftData_UnkDemoCallbacks0[fp->kind](alloc_info->unk8, &a, &b);
            } else if (alloc_info->unk8 >= 8) {
                b = 13;
                a = 13;
            } else if (alloc_info->unk8 >= 7) {
                b = 12;
                a = 12;
            } else if (alloc_info->unk8 >= 5) {
                a = 10;
                b = 11;
            } else {
                a = 0;
                b = 9;
            }
            ftData_80085B98(fp, a, b);
        }
        ftParts_80074E58(fp);
        ftParts_SetupParts(gobj);
        ftAnim_80070308(gobj);
        ftCo_800C884C(gobj);
        Fighter_80068E64(gobj);
        ftParts_800749CC(gobj);
        ftAnim_8007077C(gobj);
        ftCo_8009CF84(fp);
        ftAnim_8006FE48(gobj);
        Fighter_UnkUpdateVecFromBones_8006876C(fp);
        ftCo_8009F578(fp);
#ifdef MELEE_NATIVE
        // Match gameplay construction: owned model/parts are ready before Load.
        // Presentation callbacks below must see this actual attached actor.
        if(!nativeScriptFighter_Attach(gobj)) {
            if(nativeMatchPool_Resolve(object_reference)==gobj && gobj->user_data==fp)HSD_GObjFree(gobj);
            return NULL;
        }
        NativeFighterLifetime initialized;
        if(!nativeFighterLifetime_Capture(fp,&initialized))return NULL;
        if(!initialized.source_owned) {
#endif
        if (ftData_OnLoad[fp->kind] != NULL) {
            ftData_OnLoad[fp->kind](gobj);
        }
#ifdef MELEE_NATIVE
        }
        if(!nativeFighterLifetime_Current(&initialized))return NULL;
#endif
        ftColl_8007B320(gobj);
        fp->x890_cameraBox = Camera_80029020();
        lbShadow_8000ED54(&fp->x20A4, gobj->hsd_obj);
    }
    HSD_GObj_SetupProc(gobj, Fighter_8006A360, 1);
    HSD_GObj_SetupProc(gobj, Fighter_procUpdate, 4);
    HSD_GObj_SetupProc(gobj, Fighter_procMap, 5);
    HSD_GObj_SetupProc(gobj, Fighter_8006C80C, 9);
    HSD_GObj_SetupProc(gobj, Fighter_8006D9AC, 16);
#ifdef MELEE_NATIVE
    NativeFighterLifetime previous_life;
    if(!nativeFighterLifetime_Capture(gobj->user_data,&previous_life))return NULL;
#endif
    if (!Fighter_UnkProcessDeath_80068354(gobj)) return NULL;
#ifdef MELEE_NATIVE
    // The native reset intentionally changes spawn identity. Only that change
    // is accepted; a different physical object/data/program cannot be adopted.
    if(nativeMatchPool_Resolve(previous_life.reference)!=gobj || gobj->user_data!=previous_life.data)return NULL;
    NativeFighterLifetime life;
    if(!nativeFighterLifetime_Capture(gobj->user_data,&life) ||
       life.kind!=previous_life.kind || life.fighter_data!=previous_life.fighter_data ||
       life.source_owned!=previous_life.source_owned ||
       (life.source_owned && (life.program!=previous_life.program ||
        life.program_identity!=previous_life.program_identity || life.view.program!=previous_life.view.program ||
        life.view.storage!=previous_life.view.storage || life.layout_identity!=previous_life.layout_identity ||
        life.parameter_identity!=previous_life.parameter_identity)) ||
       (!life.source_owned && life.native_definition!=previous_life.native_definition))return NULL;
#endif
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter, &generation)) return NULL;
    if (on_create_fighter[alloc_info->unk8] != NULL) {
        on_create_fighter[alloc_info->unk8](gobj);
    }
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return NULL;
#ifdef MELEE_NATIVE
    if(!nativeFighterLifetime_Current(&life))return NULL;
    if(life.source_owned) {
        NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=alloc_info->unk8};
        if(nativeScriptFighter_Run(gobj,NativeScriptFighter_DemoEnter,&argument,1, NULL,NULL)!=NativeScriptVM_Ok ||
           !nativeFighterLifetime_Current(&life))return NULL;
    } else {
#endif
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(GET_FIGHTER(gobj)->kind);
    if (definition != NULL && definition->on_demo_enter != NULL) {
        definition->on_demo_enter(gobj, alloc_info->unk8);
        if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter)) return NULL;
    }
#ifdef MELEE_NATIVE
    }
    if(!nativeFighterLifetime_Current(&life))return NULL;
#endif
    ftLib_800867E8(gobj);
    return gobj;
}

void ftDemo_ObjAllocInit(void)
{
    Fighter_800679B0();
#ifndef MELEE_NATIVE
    HSD_ObjAllocInit(&fighter_x59C_alloc_data, 0xB000, 0x20);
#endif
}

void ftDemo_SetArchiveData(int pairs_idx, HSD_Archive* archive, int arr_idx)
{
    static int ints[5] = { 9, 10, 11, 14, 15 };
    ftData_UnkCountStruct* pair = &ftData_UnkIntPairs[pairs_idx];
    if (pair->data == NULL) {
        if (arr_idx >= 4) {
            pair->data = HSD_ArchiveGetPublicAddress(
                archive,
                ftDemo_GetMotionFileString(pairs_idx, ints[arr_idx - 4]));
        } else {
            pair->data = HSD_ArchiveGetPublicAddress(
                archive, ((char***) ftData_803C2468)[pairs_idx][arr_idx]);
        }
    }
}

char* ftDemo_GetMotionFileString(int cb_idx, int cb_arg)
{
    if (ftData_803C24EC[cb_idx] != NULL) {
        return ftData_803C24EC[cb_idx](cb_arg);
    } else {
        HSD_ASSERTREPORT(296, 0, "no demo vi anim! %d\n", cb_arg);
    }
}

void ftDemo_SetFacingDirection(Fighter_GObj* gobj, float facing_dir)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->facing_dir = facing_dir;
    ftPartSetRotY(fp, 0, M_PI_2 * fp->facing_dir);
}
