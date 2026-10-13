#include "fighter.h"
#ifdef MELEE_NATIVE
#include "native_script_fighter.h"
#endif
#include <melee/ft/native_callback_lifetime.h>
#include "ftcoll.h"
#include "ftcommon.h"
#include "kinds/ftCommon/ftCo_Bury.h"
#include "types.h"
#include <melee/mp/mplib.h>
#include <dolphin/mtx.h>

void Fighter_UpdatePosition(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;
    Vec3 windOffset;

    if (!fp->x2219_b5) {
        if (fp->x2064_ledgeCooldown) {
            fp->x2064_ledgeCooldown -= 1;
        }

        if (fp->capture_timer != 0) {
            --fp->capture_timer;
        }

        ftCo_800C0A98(gobj);

#ifdef MELEE_NATIVE
        if (nativeScriptFighter_MotionBound(gobj,NativeScriptMotion_Physics)) {
            if (!nativeScriptFighter_RunMotion(gobj,NativeScriptMotion_Physics)) return;
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        } else
#endif
        if (fp->phys_cb) {
            fp->phys_cb(gobj);
            if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
        }

        ftCommon_IntegrateMovement(fp);
        // accumulate wind hazards into the windOffset vector
        ftColl_GetWindOffsetVec(gobj,
                                /*result vec3*/ &windOffset);
    } else {
        windOffset.x = windOffset.y = windOffset.z = 0;
    }

    ftColl_80076528(gobj);

    if (fp->hitlag_cb) {
        fp->hitlag_cb(gobj);
        if (!nativeCallbackSource_Alive(gobj, callback_generation, NativeCallbackSource_Fighter)) return;
    }

    if (fp->ground_or_air == GA_Ground) {
        Vec3 difference;
        // I think this function always returns r3=1, but it contains two
        // __assert functions. But I guess these just stop or reset the game.
        // result is written to where r5 points to, which is 'difference' in
        // this case
        if (mpGetSpeed(fp->coll_data.floor.index, &fp->cur_pos, &difference)) {
            // fp->position += difference
            C_VECAdd(&fp->cur_pos, &difference, &fp->cur_pos);
        }
    }

    fp->cur_pos.x += windOffset.x;
    fp->cur_pos.y += windOffset.y;
    fp->cur_pos.z += windOffset.z; ///< @todo do the bitflag tests here tell us
                                   ///< if the player is dead?
}
