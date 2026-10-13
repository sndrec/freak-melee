#ifndef MELEE_FT_CHARA_FTCOMMON_INLINES_H
#define MELEE_FT_CHARA_FTCOMMON_INLINES_H

#include <Runtime/platform.h>

#include <sysdolphin/baselib/forward.h>

#include <placeholder.h>

#include <melee/ef/efasync.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.h>
#include <melee/ft/kinds/ftCommon/ftCo_Thrown.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>

static inline void ftCo_SpawnEf(Fighter_GObj* gobj, HSD_JObj* joint, u32 arg2,
                                enum_t arg3)
{
    Fighter* fp = gobj->user_data;
    FORCE_PAD_STACK_8;
    efAsync_Spawn((HSD_GObj*) gobj, &fp->x60C, arg2, arg3, joint);
}

static inline void ftCo_Barrel_ApplyKnockback(Fighter* fp, float kb_angle,
                                              HitCapsule* hit)
{
    if (kb_angle < 0) {
        kb_angle += 360;
    }
    kb_angle = fp->facing_dir < 0.0f ? kb_angle : 180.0f - kb_angle;
    fp->dmg.kb_applied = ftColl_80079EA8(fp, hit, hit->unk_count);
    fp->dmg.x1848_kb_angle = kb_angle;
    fp->dmg.facing_dir_1 = fp->facing_dir;
    fp->dmg.x184c_damaged_hurtbox = 0;
    fp->dmg.x1854_collpos = fp->cur_pos;
    fp->dmg.x1860_element = hit->element;
}

static inline void ftCo_ItemParasol_Phys(Fighter_GObj* gobj)
{
    ftCo_ItemParasolOpen_Phys(gobj);
}

static inline void ftCommon_GroundToAirStateChange(Fighter_GObj* gobj,
                                                   Fighter* fp,
                                                   FtMotionId msid,
                                                   MotionFlags flags)
{
    ftCommon_8007D5D4(fp);
    Fighter_ChangeMotionState(gobj, msid, flags, fp->cur_anim_frame, 1.0f,
                              0.0f, NULL);
}

static inline void ftCommon_AirToGroundStateChange(Fighter_GObj* gobj,
                                                   Fighter* fp,
                                                   FtMotionId msid,
                                                   MotionFlags flags)
{
    ftCommon_8007D7FC(fp);
    Fighter_ChangeMotionState(gobj, msid, flags, fp->cur_anim_frame, 1.0f,
                              0.0f, NULL);
}

#endif
