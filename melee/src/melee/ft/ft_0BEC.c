#include "ft_0BEC.h"

#include <Runtime/platform.h>

#include "fighter.h"
#include "forward.h"
#include "ftparts.h"
#include "kinds/ftCommon/forward.h"
#include "kinds/ftFox/types.h"
#include "kinds/ftMario/ftmariospecialn.h"
#include "types.h"
#ifdef MELEE_NATIVE
#include "native_fighter_runtime.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#endif
#include <dolphin/mtx.h>
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/itdrmariopill.h>
#include <melee/it/kinds/itfoxblaster.h>
#include <melee/it/kinds/itnessbat.h>

static inline void setupInitialState(Fighter* fp)
{
    fp->x2219_b2 = true;
    fp->x2219_b1 = true;
    fp->x2228_b1 = true;
    fp->item_gobj = NULL;
    fp->x1984_heldItemSpec = NULL;
}

void ftCo_800BECB0(Fighter_GObj* gobj)
{
    int unused[2];
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime life;
    if (!nativeFighterLifetime_Capture(fp,&life)) return;
    const NativeFighterDefinition* authored=life.native_definition;
#endif
    Fighter_ChangeMotionState(gobj, ftCo_MS_DeadDown, Ft_MF_None, 0.f, 1.f,
                              0.f, NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&life)) return;
#endif
    setupInitialState(fp);
#ifdef MELEE_NATIVE
    if (life.source_owned) {
        if (nativeScriptFighter_Run(gobj,NativeScriptFighter_PresentationItem,NULL,0, NULL,NULL)!=NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&life)) return;
        return;
    }
    if (authored != NULL && authored->on_presentation_item != NULL) {
        authored->on_presentation_item(gobj);
        return;
    }
#endif
    switch (fp->kind) {
    case Ft_Kind_Fox: {
        ftFox_DatAttrs* da = fp->dat_attrs;
        fp->item_gobj =
            it_802AE994(gobj, ftParts_GetBoneIndex(fp, FtPart_RThumbNb),
                        da->x20_FOX_BLASTER_GUN_ITKIND);
        it_8026BAE8(fp->item_gobj,
                    fp->x34_scale.y * fp->co_attrs.model_scaling);
    }
    default:
        break;
    }
}

void ftCo_800BED84(Fighter_GObj* gobj) {}

static inline void FoxHelper(Fighter_GObj* gobj, Fighter* fp)
{
    ftFox_DatAttrs* da = fp->dat_attrs;
    fp->item_gobj =
        it_802AE994(gobj, ftParts_GetBoneIndex(fp, FtPart_RThumbNb),
                    da->x20_FOX_BLASTER_GUN_ITKIND);
    it_8026BAE8(fp->item_gobj, fp->x34_scale.y * fp->co_attrs.model_scaling);
}

static inline void DocHelper(Fighter_GObj* gobj, Fighter* fp)
{
    ftCo_DatAttrs* co;
    fp->item_gobj = itDrMarioPill_802C09C4(
        gobj, &fp->cur_pos, ftMr_SpecialN_VitaminRandom(gobj),
        It_Kind_DrMario_Vitamin, 1, 0, fp->facing_dir);
    co = &fp->co_attrs;
    it_8026BAE8(fp->item_gobj,
                0.71428f * (fp->x34_scale.y * co->model_scaling));
    fp->x1984_heldItemSpec = itDrMarioPill_802C09C4(
        gobj, &fp->cur_pos, ftMr_SpecialN_VitaminRandom(gobj),
        It_Kind_DrMario_Vitamin, 0, 0, fp->facing_dir);
    it_8026BAE8(fp->x1984_heldItemSpec,
                0.71428f * (fp->x34_scale.y * co->model_scaling));
}

void ftCo_800BED88(Fighter_GObj* gobj)
{
    int unused[2];
    Fighter* fp = gobj->user_data;
#ifdef MELEE_NATIVE
    NativeFighterLifetime life;
    if (!nativeFighterLifetime_Capture(fp,&life)) return;
    const NativeFighterDefinition* authored=life.native_definition;
#endif
    Fighter_ChangeMotionState(gobj, ftCo_MS_DeadRight, Ft_MF_None, 0.f, 1.f,
                              0.f, NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&life)) return;
#endif
    setupInitialState(fp);

#ifdef MELEE_NATIVE
    if (life.source_owned) {
        if (nativeScriptFighter_Run(gobj,NativeScriptFighter_PresentationItem,NULL,0, NULL,NULL)!=NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&life)) return;
        return;
    }
    if (authored != NULL && authored->on_presentation_item != NULL) {
        authored->on_presentation_item(gobj);
        return;
    }
#endif
    switch (fp->kind) {
    case Ft_Kind_Fox: {
        FoxHelper(gobj, fp);
        break;
    }
    case Ft_Kind_DrMario: {
        DocHelper(gobj, fp);
        break;
    }
    default:
        break;
    }
}

void ftCo_800BEF00(Fighter_GObj* gobj) {}

void ftCo_800BEF04(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime life;
    if (!nativeFighterLifetime_Capture(fp,&life)) return;
    const NativeFighterDefinition* authored=life.native_definition;
#endif
    Fighter_ChangeMotionState(gobj, ftCo_MS_DeadUpStarIce, Ft_MF_None, 0.f,
                              1.f, 0.f, NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&life)) return;
#endif
    setupInitialState(fp);
#ifdef MELEE_NATIVE
    if (life.source_owned) {
        if (nativeScriptFighter_Run(gobj,NativeScriptFighter_PresentationItem,NULL,0, NULL,NULL)!=NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&life)) return;
        return;
    }
    if (authored != NULL && authored->on_presentation_item != NULL) {
        authored->on_presentation_item(gobj);
        return;
    }
#endif
    switch (fp->kind) {
    case Ft_Kind_Ness: {
        fp->item_gobj =
            it_802AD590(gobj, ftParts_GetBoneIndex(fp, FtPart_RThumbNb));
        it_8026BAE8(fp->item_gobj,
                    0.8f * (fp->x34_scale.y * fp->co_attrs.model_scaling));
    }
    default:
        break;
    }
}

void ftCo_800BEFD0(Fighter_GObj* gobj) {}
