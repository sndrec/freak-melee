#include "ftswing.h"

#include "fighter.h"
#include "forward.h"
#include "ft_081B.h"
#include "ft_0CD1.h"
#include "ftlipstickswing.h"
#include "ftstarrodswing.h"
#include "inlines.h"
#include "kinds/ftCommon/ftCo_FallSpecial.h"
#include "kinds/ftCommon/ftpickupitem.h"
#include "types.h"
#include <sysdolphin/baselib/debug.h>
#ifdef MELEE_NATIVE
#include "native_fighter_runtime.h"
#include "native_fighter_lifetime.h"
#include "native_script_bindings.h"
#include "native_source_package.h"
#endif

int lbl_803C6D70[6][5] = {
    { 120, 121, 122, -1, 123 }, { 124, 125, 126, -1, 127 },
    { 128, 129, 130, -1, 131 }, { 132, 133, 134, -1, 135 },
    { 136, 137, 138, -1, 139 }, { 140, 141, 142, -1, 143 }
};

void ftCo_FallAerial_Coll(Fighter_GObj* gobj)
{
    ft_800831CC(gobj, ftCo_80096CC8, ft_80082B1C);
}

int fn_800CCEC4(int x);
int fn_800CCEC4(int x)
{
    switch (x) {
    case 12:
        return 0;
    case 11:
        return 1;
    case 13:
        return 2;
    case 24:
        return 3;
    case 22:
        return 4;
    case 23:
        return 5;
    default:
        HSD_ASSERTREPORT(82, 0, "ellegal swing item!!!\n");
    }
}

int get_anim_id(Fighter* fp, int swing_type, int arg1); // local-only inline
int get_anim_id(Fighter* fp, int swing_type, int arg1)
{
#ifdef MELEE_NATIVE
    if (swing_type<0 || swing_type>=6 || arg1<0 || arg1>=5) return -1;
    if (nativeScriptFighter_IsAttached(fp->gobj)) {
        const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(fp->kind);
        if (!source || !source->assets.common_motions || !source->assets.motions ||
            fp->x1C_actionStateList!=source->assets.common_motions ||
            fp->x20_actionStateList!=source->assets.motions) return -1;
    }
#endif
    if (arg1 != 3) {
        int motion=lbl_803C6D70[swing_type][arg1];
#ifdef MELEE_NATIVE
        if (nativeScriptFighter_IsAttached(fp->gobj)) {
            NativeScriptRuntimeContext context={.source=fp->gobj,
                .source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum};
            const NativeScriptMoveLayout* layout=nativeScriptBindings_SourceFighterLayout(&context,NULL);
            if (!layout || motion<0 || !nativeScriptMoves_Contains(layout,(uint32_t)motion)) return -1;
        }
#endif
        return motion;
    } else {
#ifdef MELEE_NATIVE
        if (nativeScriptFighter_IsAttached(fp->gobj)) {
            NativeFighterLifetime life;
            if (!nativeFighterLifetime_Capture(fp,&life) ||
                !nativeScriptFighter_HasHook(fp->gobj,NativeScriptFighter_ItemSwingMotion)) return -1;
            NativeScriptRuntimeContext context={.source=fp->gobj,
                .source_class=NativeCallbackSource_Fighter,.generation=life.generation,
                .scene_epoch=life.reference.lifetime};
            const NativeScriptMoveLayout* layout=nativeScriptBindings_SourceFighterLayout(&context,NULL);
            if (layout!=&life.program->bank->motion_layout) return -1;
            NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=swing_type},
                                      {.type=NativeScriptType_I32,.as.i64=arg1}},result;
            if (nativeScriptFighter_Run(fp->gobj,NativeScriptFighter_ItemSwingMotion,
                    args,2,NULL,&result)!=NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&life) ||
                nativeScriptBindings_SourceFighterLayout(&context,NULL)!=layout) return -1;
            int motion=(int32_t)result.as.i64;
            return motion>=0 && nativeScriptMoves_Contains(layout,(uint32_t)motion)?motion:-1;
        }
        const NativeFighterDefinition* definition =
            nativeFighterRuntime_Definition(fp->kind);
        if (definition != NULL && definition->on_item_swing_motion != NULL) {
            int motion = definition->on_item_swing_motion(fp->gobj, swing_type,
                                                         arg1);
            if (motion >= definition->motion_state_first &&
                motion - definition->motion_state_first <
                    definition->motion_state_count)
                return motion;
            OSReport("don't have swing42 motion!!!\n");
            HSD_ASSERT(97, 0);
        }
#endif
        switch (fp->kind) {
        case Ft_Kind_Captain:
            return swing_type + 341;
        default:
            OSReport("don't have swing42 motion!!!\n");
            HSD_ASSERT(97, 0);
        }
    }
}

void ftCo_Attack_800CCF58(Fighter_GObj* gobj, enum_t arg1)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int swing_type;
    int anim_id;
    float f;
    switch (arg1) {
    case 0:
    case 1:
        RETURN_IF(ftpickupitem_80094790(gobj));
        break;
    }
    HSD_ASSERT(124, fp->item_gobj != NULL);
    swing_type = fn_800CCEC4(itGetKind(fp->item_gobj));
    anim_id = get_anim_id(fp, swing_type, arg1);
#ifdef MELEE_NATIVE
    // A retired query source or an absent owned swing must not reach an entry.
    if (anim_id<0) return;
#endif
    f = Fighter_804D654C[swing_type][arg1];
    switch (swing_type) {
    case 0:
        ftCo_800CD350(gobj, anim_id, arg1, f);
        break;
    case 1:
        ftCo_800CD418(gobj, anim_id, arg1, f);
        break;
    case 2:
        ftCo_800CD4E0(gobj, anim_id, arg1, f);
        break;
    case 3:
        ftCo_800CD5A8(gobj, anim_id, arg1, f);
        break;
    case 4:
        ftCo_800CD82C(gobj, anim_id, arg1, f);
        break;
    case 5:
        ftCo_800CDAB4(gobj, anim_id, arg1, f);
        break;
    }
}
