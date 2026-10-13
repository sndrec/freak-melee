#include "native_fighter_runtime.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include "native_script_fighter.h"
#include "ft_0892.h"

#include "fighter.h"
#include "inlines.h"
#include "kinds/ftFox/ftfoxappeals.h"
#include <melee/it/it_26B1.h>
#include <melee/pl/plattack.h>
#include <melee/pl/pltrick.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>

void lbBgFlash_80020E38(HSD_JObj*, Vec3*, f32, f32, f32);
void lbBgFlash_80021410(IKState*);

/// Local struct with bitfields - reversed order for MWCC big-endian
typedef struct {
    s32 x0;
    s32 x4;
    float x8;
    s32 xC;
    u8 x10_b7 : 1; // bit 7 (MSB)
    u8 x10_b6 : 1;
    u8 x10_b5 : 1;
    u8 x10_b4 : 1;
    u8 x10_b3 : 1;
    u8 x10_b2 : 1;
    u8 x10_b1 : 1;
    u8 x10_b0 : 1; // bit 0 (LSB)
    u8 x11_b7 : 1;
    u8 x11_b6 : 1;
    u8 x11_b5 : 1;
    u8 x11_b4 : 1;
    u8 x11_b3 : 1;
    u8 x11_b2 : 1;
    u8 x11_b1 : 1;
    u8 x11_b0 : 1;
} UnkStruct89768;

void ft_800892D4(Fighter* arg0)
{
    union Struct2070 spC;

    spC.x2070_int = 0;
    arg0->dmg.x18d4 = *(UnkPlBonusBits*) &spC;
    arg0->x2070.x2070_int = *(s32*) &arg0->dmg.x18d4;
    *(s32*) &arg0->x2074.x2074_vec.x = 0;
    *(s32*) &arg0->x2074.x2074_vec.y = 0;
    *(f32*) &arg0->x2074.x207C.x = 0.0f;
    arg0->x2074.x207C.y = 6;
    arg0->x2074.x2084_b0 = 0;
    arg0->x2074.x2085_b3 = 0;
    arg0->x2074.x2084_b1 = 0;
    arg0->x2074.x2084_b2 = 0;
    arg0->x2074.x2084_b3 = 0;
    arg0->x2074.x2084_b4 = 0;
    arg0->x2074.x2084_b5 = 0;
    arg0->x2074.x2084_b6 = 0;
    arg0->x2074.x2084_b7 = 0;
    arg0->x2074.x2085_b0 = 0;
    arg0->x2074.x2085_b1 = 0;
    arg0->x2074.x2085_b2 = 0;
    arg0->x2074.x2085_b4 = 0;
    arg0->dmg.x18d8.x0 = 0;
    arg0->dmg.x18d8.x4 = 0;
    arg0->dmg.x18d8.kb_applied1 = 0.0f;
    arg0->dmg.x18d8.xC = 6;
    arg0->dmg.x18d8.x10_b0 = 0;
    arg0->dmg.x18d8.x11_b3 = 0;
    arg0->dmg.x18d8.x10_b1 = 0;
    arg0->dmg.x18d8.x10_b2 = 0;
    arg0->dmg.x18d8.x10_b3 = 0;
    arg0->dmg.x18d8.x10_b4 = 0;
    arg0->dmg.x18d8.x10_b5 = 0;
    arg0->dmg.x18d8.x10_b6 = 0;
    arg0->dmg.x18d8.x10_b7 = 0;
    arg0->dmg.x18d8.x11_b0 = 0;
    arg0->dmg.x18d8.x11_b1 = 0;
    arg0->dmg.x18d8.x11_b2 = 0;
    arg0->dmg.x18d8.x11_b4 = 0;
    arg0->dmg.x18ec_instancehitby = 0;
    arg0->x2074.x2088 = 0;
}

void ft_80089460(Fighter* fp)
{
    *(s32*) &fp->x2074.x2074_vec.x = 0;
    *(s32*) &fp->x2074.x2074_vec.y = 0;
    *(f32*) &fp->x2074.x207C.x = 0.0f;
    fp->x2074.x207C.y = 6;
    fp->x2074.x2084_b0 = 0;
    fp->x2074.x2085_b3 = 0;
    fp->x2074.x2084_b1 = 0;
    fp->x2074.x2084_b2 = 0;
    fp->x2074.x2084_b3 = 0;
    fp->x2074.x2084_b4 = 0;
    fp->x2074.x2084_b5 = 0;
    fp->x2074.x2084_b6 = 0;
    fp->x2074.x2084_b7 = 0;
    fp->x2074.x2085_b0 = 0;
    fp->x2074.x2085_b1 = 0;
    fp->x2074.x2085_b2 = 0;
    fp->x2074.x2085_b4 = 0;
    *(s32*) &fp->x2074.x2074_vec.x = (s32) fp->dmg.x1830_percent;
    if (fp->is_metal) {
        fp->x2074.x2084_b1 = 1;
    }
    if (fp->x197C != NULL) {
        fp->x2074.x2084_b2 = 1;
    }
    if (fp->x221D_b6) {
        fp->x2074.x2084_b3 = 1;
    }
    if (fp->x2226_b4) {
        fp->x2074.x2084_b4 = 1;
    }
    if (fp->x2220_b5) {
        fp->x2074.x2084_b5 = 1;
    }
    if (fp->x2220_b6) {
        fp->x2074.x2084_b6 = 1;
    }
}

// Motion tables use the console word convention independently of native byte
// order and C bitfield allocation. Decode its semantic fields explicitly.
static union Struct2070 decode_action_metadata(uint32_t metadata)
{
    union Struct2070 value = { 0 };
    value.x2070 = (s8)(metadata >> 24);
    value.x2071_b0_3 = (metadata >> 20) & 15;
    value.x2071_b4 = (metadata >> 19) & 1;
    value.x2071_b5 = (metadata >> 18) & 1;
    value.x2071_b6 = (metadata >> 17) & 1;
    value.x2071_b7 = (metadata >> 16) & 1;
    value.x2072_b0 = (metadata >> 15) & 1;
    value.x2072_b1 = (metadata >> 14) & 1;
    value.x2072_b2 = (metadata >> 13) & 1;
    value.count_x1A4 = (metadata >> 12) & 1;
    value.count_thrown_items = (metadata >> 11) & 1;
    value.count_aerials = (metadata >> 10) & 1;
    value.count_x1A0 = (metadata >> 9) & 1;
    value.count_specials = (metadata >> 8) & 1;
    value.x2073 = metadata & 255;
    return value;
}

// arg1 is canonical console metadata, not a native Struct2070 memory image.
void ft_800895E0(Fighter* fp, int arg1)
{
    union Struct2070 val = decode_action_metadata((uint32_t)arg1);
    FighterKind decision_kind = fp->kind;
#ifdef MELEE_NATIVE
    decision_kind = fp->behavior_kind;
#endif
    if (val.x2073 == 0 || val.x2073 != fp->x2070.x2073) {
        fp->x2074.x2088 = plAttack_80037B08();
    }
#ifdef MELEE_NATIVE
    const NativeFighterDefinition* authored = nativeFighterRuntime_Definition(fp->kind);
    if (nativeScriptFighter_IsAttached(fp->gobj)) {
        // An omitted query preserves the incoming descriptor. The script's
        // explicit or implicit return owns classification for an attached actor.
        if (nativeScriptFighter_HasHook(fp->gobj,NativeScriptFighter_ActionMetadata)) {
            NativeFighterLifetime retained;
            NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=arg1},output;
            if (!nativeFighterLifetime_Capture(fp,&retained) ||
                nativeScriptFighter_Run(fp->gobj,NativeScriptFighter_ActionMetadata,
                                       &input,1,NULL,&output)!=NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&retained)) return;
            val=decode_action_metadata((uint32_t)output.as.i64);
        }
    } else if (authored != NULL && authored->on_action_metadata != NULL) {
        Fighter_GObj* source = fp->gobj;
        const uint32_t generation = (uint32_t)fp->x8_spawnNum;
        int32_t metadata = authored->on_action_metadata(source, (int32_t)arg1);
        if (!nativeCallbackSource_Alive(source, generation, NativeCallbackSource_Fighter))
            return;
        val = decode_action_metadata((uint32_t)metadata);
    } else
#endif
    if (decision_kind == Ft_Kind_Luigi && val.x2073 == 0x71) {
        val = decode_action_metadata(UINT32_C(0x240063));
    }
    if (val.x2073 == 0x62) {
        if (fp->item_gobj != NULL && it_8026B6C8(fp->item_gobj) != 0) {
            val = decode_action_metadata(UINT32_C(0x44003D));
        }
    }
    fp->x2070.x2070_int = val.x2070_int;
    ft_80089768(&fp->x2074.x2074_vec);
    *(s32*) &fp->x2074.x2074_vec.x = (s32) fp->dmg.x1830_percent;
    if (fp->is_metal) {
        fp->x2074.x2084_b1 = 1;
    }
    if (fp->x197C != NULL) {
        fp->x2074.x2084_b2 = 1;
    }
    if (fp->x221D_b6) {
        fp->x2074.x2084_b3 = 1;
    }
    if (fp->x2226_b4) {
        fp->x2074.x2084_b4 = 1;
    }
    if (fp->x2220_b5) {
        fp->x2074.x2084_b5 = 1;
    }
    if (fp->x2220_b6) {
        fp->x2074.x2084_b6 = 1;
    }
}

void ft_80089768(Vec2* ptr)
{
    UnkStruct89768* s = (UnkStruct89768*) ptr;
    s->x0 = 0;
    s->x4 = 0;
    s->x8 = 0.0f;
    s->xC = 6;
    s->x10_b7 = false;
    s->x11_b4 = false;
    s->x10_b6 = false;
    s->x10_b5 = false;
    s->x10_b4 = false;
    s->x10_b3 = false;
    s->x10_b2 = false;
    s->x10_b1 = false;
    s->x10_b0 = false;
    s->x11_b7 = false;
    s->x11_b6 = false;
    s->x11_b5 = false;
    s->x11_b3 = false;
}

void ft_80089824(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    union Struct2070 val = fp->x2070;
    fp->x2070.x2070_int = val.x2070_int;
    ft_80089460(fp);
    fp->x2074.x2088 = plAttack_80037B08();
    pl_80037C60(gobj, 0);
}

union Struct2070* ft_80089884(Fighter_GObj* gobj)
{
    return &GET_FIGHTER(gobj)->x2070;
}

UnkPlBonusBits* ft_80089890(Fighter_GObj* gobj)
{
    return &GET_FIGHTER(gobj)->dmg.x18d4;
}

u16 ft_8008989C(Fighter_GObj* gobj)
{
    return GET_FIGHTER(gobj)->dmg.x18ec_instancehitby;
}

struct Struct2074* ft_800898A8(Fighter_GObj* gobj)
{
    return &GET_FIGHTER(gobj)->x2074;
}

ft_800898B4_t* ft_800898B4(Fighter_GObj* gobj)
{
    return &GET_FIGHTER(gobj)->dmg.x18d8;
}

bool ft_800898C0(HSD_GObj* gobj)
{
    Fighter* ft = GET_FIGHTER(gobj);
    if (ft->x2070.x2073 == 113 || ft->x2070.x2073 == 114 || ft->x2070.x2073 == 99)
        return true;
    NativeFighterLifetime lifetime;
    bool retained = nativeFighterLifetime_Capture(ft, &lifetime);
    if (!retained && nativeScriptFighter_Program(ft->kind)) return false;
    if (retained && lifetime.source_owned) {
        NativeScriptValue result;
        return nativeScriptFighter_Run(gobj, NativeScriptFighter_ActionStatus,
                   NULL, 0, NULL, &result) == NativeScriptVM_Ok &&
            result.type == NativeScriptType_Bool && nativeFighterLifetime_Current(&lifetime) && result.as.u64 != 0;
    }
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(ft->kind);
    if (definition != NULL && definition->on_action_status != NULL) {
        bool active = definition->on_action_status(gobj);
        return (!retained || nativeFighterLifetime_Current(&lifetime)) && active;
    }
    return ftFx_AppealS_CheckIfUsed(ft);
}

s32 ft_80089914(HSD_GObj* gobj, int msid)
{
    Fighter* fp = gobj->user_data;
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fp->kind);
    NativeFighterLifetime lifetime;
    bool retained = nativeFighterLifetime_Capture(fp, &lifetime);
    if (!retained && nativeScriptFighter_Program(fp->kind)) return false;
    if (nativeScriptFighter_HasHook(gobj, NativeScriptFighter_MotionIsAngled)) {
        NativeScriptValue argument = {.type=NativeScriptType_I32,.as.i64=msid}, result;
        if (!retained || nativeScriptFighter_Run(gobj, NativeScriptFighter_MotionIsAngled,
                &argument, 1, NULL, &result) != NativeScriptVM_Ok ||
            result.type != NativeScriptType_Bool || !nativeFighterLifetime_Current(&lifetime)) return false;
        return result.as.u64 != 0;
    }
    if (definition != NULL && definition->on_motion_is_angled != NULL)
    {
        bool result = definition->on_motion_is_angled(gobj, msid);
        return (!retained || nativeFighterLifetime_Current(&lifetime)) && result;
    }

    if (msid < ftCo_MS_AttackS3Hi || msid > ftCo_MS_AttackS4LwS) {
        return false;
    }

    if (fp->kind == Ft_Kind_Fox) {
        if ((u32) (msid - ftCo_MS_AttackS3LwS) <= 2) {
            return false;
        }
    }

    if (fp->kind == Ft_Kind_Falco) {
        if ((u32) (msid - ftCo_MS_AttackS3LwS) <= 2) {
            return false;
        }
    }

    if (fp->kind == Ft_Kind_Mewtwo) {
        if (msid == ftCo_MS_AttackS3S) {
            return false;
        }
    }

    return true;
}
