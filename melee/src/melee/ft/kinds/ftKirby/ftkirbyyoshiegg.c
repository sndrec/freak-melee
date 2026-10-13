#include "ftkirbyyoshiegg.h"

#include "types.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>

void ftKb_SpecialNYs_EnclosureSettings(Fighter_GObj* gobj,
                                      NativeCaptureEnclosureSettings* settings)
{
    Fighter* fp = GET_FIGHTER(gobj);
    struct ftKb_DatAttrs* own = fp->dat_attrs;
    struct ftKb_DatAttrs* shared = gFtDataList[Ft_Kind_Kirby]->ext_attr;
    *settings = (NativeCaptureEnclosureSettings) {
        .launch_velocity = { -fp->facing_dir * own->specialn_ys_initial_horizontal_momentum,
                            own->specialn_ys_initial_vertical_momentum, 0 },
        .escape_velocity = { shared->specialn_ys_horizontal_velocity_on_breakout,
                            shared->specialn_ys_vertical_velocity_on_breakout, 0 },
        .damage_behavior = own->specialn_ys_damage_multiplier,
        .shrink_fraction = own->specialn_ys_unk,
        .shrink_frames = own->specialn_ys_growth_time,
        .lifetime = own->specialn_ys_base_duration,
        .damage_time_scale = shared->specialn_ys_unk3 / shared->specialn_ys_damage_multiplier,
        .timer_decay = shared->specialn_ys_egg_breakout_resistance,
        .mash_reduction = shared->specialn_ys_frames_reduced_per_input,
        .mash_pause_frames = shared->specialn_ys_unk1,
        .mash_animation_rate = shared->specialn_ys_unk2,
        .escape_invincibility_frames = shared->specialn_ys_iframes_on_release,
    };
}
