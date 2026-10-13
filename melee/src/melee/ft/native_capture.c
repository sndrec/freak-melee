#include "native_capture.h"
#include "native_capture_enclosure.h"
#include "types.h"
#include "fighter.h"
#include "ftcharacter_assets.h"
#include "ftcoll.h"
#include "ftcommon.h"
#include "ftparts.h"
#include "native_fighter_capture.h"
#include "native_fighter_capture_rig.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftCo_Throw.h"
#include "kinds/ftCommon/ftCo_YoshiEgg.h"

bool nativeCapture_ReleaseIntoEnclosure(Fighter_GObj* holder)
{
    NativeFighterCapturePair pair;
    if (!nativeFighterCapture_Retain(holder, &pair)) return false;
    if (!pair.victim.object) return true;
    Fighter* fighter = pair.holder.data;
    Fighter_GObj* victim = pair.victim.object;
    if (fighter->ft_data == NULL || fighter->ft_data->x48_items == NULL) return false;
    NativeCaptureEnclosureSettings settings = fighter->capture_enclosure_settings;
    if ((unsigned)settings.model_slot >= FT_CHARACTER_ITEM_CAPACITY) return false;
    HSD_Joint* model = fighter->ft_data->x48_items[settings.model_slot];
    if (model == NULL) return false;
    // The native throw-damage routine samples TransN2 and can detach XRotN.
    // Check those source-owned rig mappings before it clears the grab links.
    HSD_JObj* joint;
    if (!nativeFighterCapture_Joint(&pair.holder, FtPart_TransN2, &joint) ||
        !nativeFighterCapture_Joint(&pair.victim, FtPart_TransN2, &joint) ||
        !nativeFighterCapture_Joint(&pair.victim, FtPart_TransN, &joint)) return false;
    const NativeFighterLifetime* detached = fighter->x221B_b7 ? &pair.holder : &pair.victim;
    if (detached->data->x2226_b2 &&
        !nativeFighterCapture_Joint(detached, FtPart_XRotN, &joint)) return false;
    settings.timer_decay = fighter->capture_settings.timer_decay;
    settings.mash_reduction = fighter->capture_settings.mash_reduction;
    settings.mash_pause_frames = fighter->capture_settings.mash_pause_frames;
    settings.mash_animation_rate = fighter->capture_settings.mash_animation_rate;
    // Damage release clears reciprocal capture links. Cache every input first,
    // including model/facing, so a later holder edit cannot retune this victim.
    float facing = fighter->facing_dir;
    ftCo_800DE2CC(holder, victim);
    if (!nativeFighterCapture_Current(&pair, false)) return false;
    if (!ftCo_CaptureEnclosure_Enter(victim, holder, ftCo_MS_YoshiEgg,
                                    &settings, model, facing)) return false;
    return nativeFighterCapture_Current(&pair, false);
}

// Configure a caught fighter's engine hurt capsule and visual tracking. The
// holder script supplies its radius and decides when this policy is requested.
void nativeCapture_EncloseCaught(Fighter_GObj* holder, float radius)
{
    Fighter* holder_fighter = GET_FIGHTER(holder);
    if (holder_fighter->victim_gobj == NULL) return;
    Fighter_GObj* caught_gobj = holder_fighter->victim_gobj;
    Fighter* caught = GET_FIGHTER(caught_gobj);
    ftHurtboxInit capsule;
    caught->invisible = true;
    caught->accessory1_cb = ftCo_800DB464;
    ftColl_8007B0C0(caught_gobj, HurtCapsule_Intangible);
    capsule.bone_idx = ftParts_GetBoneIndex(caught, FtPart_XRotN);
    capsule.height = HurtHeight_Mid;
    capsule.is_grabbable = false;
    capsule.a_offset.x = capsule.a_offset.y = capsule.a_offset.z = 0.0f;
    capsule.b_offset.x = capsule.b_offset.y = capsule.b_offset.z = 0.0f;
    capsule.scale = radius / ftCommon_GetModelScale(caught);
    ftColl_HurtboxInit(caught, caught->hurt_capsules, &capsule);
}
