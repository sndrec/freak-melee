#ifndef MELEE_IT_ITYOYO_H
#define MELEE_IT_ITYOYO_H

#include <Runtime/platform.h>

#include <sysdolphin/baselib/forward.h>

typedef struct itYoyoAttributes {
    /* DAT +0x00 */ s32 link_count;
    /* DAT +0x04 */ s32 up_smash_charge_link_count;
    /* DAT +0x08 */ s32 down_smash_charge_link_count;
    /* DAT +0x0C */ float link_length;
    /* DAT +0x10 */ float minimum_link_length;
    /* DAT +0x14 */ float unused_angle_radians;
    /* DAT +0x18 */ float released_spin_rate;
    /* DAT +0x1C */ float charged_spin_rate;
    /* DAT +0x20 */ float initial_charge_spin_rate;
    /* DAT +0x24 */ float release_velocity_x;
    /* DAT +0x28 */ float horizontal_acceleration;
    /* DAT +0x2C */ float maximum_horizontal_speed;
    /* DAT +0x30 */ float release_velocity_y;
    /* DAT +0x34 */ float gravity;
    /* DAT +0x38 */ float terminal_velocity;
    /* DAT +0x3C */ float link_gravity;
    /* DAT +0x40 */ s32 up_smash_rotate_frame;
    /* DAT +0x44 */ s32 up_smash_retract_frame;
    /* DAT +0x48 */ s32 down_smash_rotate_frame;
    /* DAT +0x4C */ s32 down_smash_retract_frame;
    /* DAT +0x50 */ HSD_Joint* string_joint;
    /* DAT +0x54 */ HSD_Joint* yoyo_joint;
    /* DAT +0x58 */ HSD_MatAnimJoint* yoyo_material_animation;
} itYoyoAttributes;

#endif
