#include "ft_0BF0.h"

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
#include <melee/it/kinds/itdrmariopill.h>
#include <melee/it/kinds/itfoxblaster.h>
#include <melee/it/kinds/itnessbat.h>

void ftCo_800BF034(Fighter_GObj* gobj)
{
    u8 _[8];
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime life;
    if (!nativeFighterLifetime_Capture(fp,&life)) return;
    const NativeFighterDefinition* authored=life.native_definition;
#endif
    Fighter_ChangeMotionState(gobj, ftCo_MS_DeadUpFallHitCameraIce, Ft_MF_None,
                              0, 1, 0, NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&life)) return;
#endif
    fp->x2219_b2 = true;
    fp->x2219_b1 = true;
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
        return;
    }
    case Ft_Kind_Ness: {
        fp->item_gobj =
            it_802AD590(gobj, ftParts_GetBoneIndex(fp, FtPart_RThumbNb));
        return;
    }
    default:
        break;
    }
}

void ftCo_800BF108(Fighter_GObj* gobj)
{
    u8 _[8] = { 0 };
    Fighter* fp = GET_FIGHTER(gobj);
#ifdef MELEE_NATIVE
    NativeFighterLifetime life;
    if (!nativeFighterLifetime_Capture(fp,&life)) return;
    const NativeFighterDefinition* authored=life.native_definition;
#endif
    Fighter_ChangeMotionState(gobj, ftCo_MS_Sleep, Ft_MF_None, 0, 1, 0, NULL);
#ifdef MELEE_NATIVE
    if (!nativeFighterLifetime_Current(&life)) return;
#endif
    fp->x2219_b2 = true;
    fp->x2219_b1 = true;
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
        return;
    }
    case Ft_Kind_Ness: {
        fp->item_gobj =
            it_802AD590(gobj, ftParts_GetBoneIndex(fp, FtPart_RThumbNb));
        return;
    }
    case Ft_Kind_DrMario: {
        fp->item_gobj = itDrMarioPill_802C09C4(
            gobj, &fp->cur_pos, ftMr_SpecialN_VitaminRandom(gobj),
            It_Kind_DrMario_Vitamin, 2,
            ftParts_GetBoneIndex(fp, FtPart_RThumbNb), fp->facing_dir);
        return;
    }
    default:
        break;
    }
}

bool ftCo_800BF228(Fighter_GObj* gobj)
{
    if (gobj != NULL) {
        Fighter* fp = GET_FIGHTER(gobj);
        if (fp != NULL && (fp->motion_id == ftCo_MS_DeadUpFallHitCameraIce ||
                           fp->motion_id == ftCo_MS_Sleep))
        {
            return true;
        }
    }
    return false;
}
