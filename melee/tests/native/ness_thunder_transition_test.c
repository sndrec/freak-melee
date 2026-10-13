// Mechanically extracted unchanged/current common entries; no engine/game.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
static Fighter fighter;
static HSD_GObj object;
static bool live;
static unsigned invalidate;
static unsigned trace[32], count, checks;
static void event(unsigned value)
{
    if (count == 32) {
        abort();
    }
    trace[count++] = value;
}
bool nativeCallbackSource_Snapshot(HSD_GObj* g, uint32_t cls,
                                   uint32_t* generation)
{
    if (g != &object || cls != NativeCallbackSource_Fighter || !live) {
        return false;
    }
    *generation = fighter.x8_spawnNum;
    return true;
}
bool nativeCallbackSource_Alive(HSD_GObj* g, uint32_t generation, uint32_t cls)
{
    return g == &object && cls == NativeCallbackSource_Fighter && live &&
           generation == fighter.x8_spawnNum;
}
void Fighter_ChangeMotionState(Fighter_GObj* g, FtMotionId motion,
                               MotionFlags flags, float start, float rate,
                               float blend, HSD_GObj* other)
{
    (void) g;
    (void) flags;
    (void) rate;
    (void) other;
    event(1);
    fighter.motion_id = motion;
    fighter.cur_anim_frame = start;
    fighter.frame_speed_mul = blend;
    if (invalidate) {
        live = invalidate == 2;
        fighter.x8_spawnNum++;
    }
}
void ftCo_80090780(Fighter_GObj* g)
{
    (void) g;
    event(2);
}
void ftCommon_8007D60C(Fighter* f)
{
    event(3);
    f->ground_or_air = GA_Air;
    f->x1968_jumpsUsed = f->co_attrs.max_jumps;
}
void ftCommon_UseAllJumps(Fighter* f)
{
    event(4);
    f->x1968_jumpsUsed = f->co_attrs.max_jumps;
}
void un_80322598(int id, float y)
{
    (void) id;
    (void) y;
    event(5);
}
void ftCommon_8007D7FC(Fighter* f)
{
    event(6);
    f->ground_or_air = GA_Ground;
}
static bool ftCo_80097570(Fighter_GObj* g)
{
    (void) g;
    event(7);
    return true;
}
static void ftCo_800978D4_inline(Fighter_GObj* g, float* param)
{
    (void) g;
    *param = 0;
    event(8);
}
void ftCommon_8007E2F4(Fighter* f, s16 value)
{
    (void) f;
    (void) value;
    event(9);
}
void ftCommon_SetGroundedKnockbackIfLanded(Fighter* f)
{
    (void) f;
    event(10);
}
static void ftCo_80097AF4(Fighter_GObj* g)
{
    (void) g;
    event(11);
    if (invalidate) {
        live = invalidate == 2;
        fighter.x8_spawnNum++;
    }
}
#include "common_transition_current.generated.h"
#include "common_transition_original.generated.h"
static void reset(bool air, bool alternative, unsigned remove, bool bag)
{
    memset(&fighter, 0, sizeof(fighter));
    object.user_data = &fighter;
    fighter.x8_spawnNum = 0xf0000023u;
    fighter.co_attrs.air_drift_max = 2.3f;
    fighter.co_attrs.max_jumps = 5;
    fighter.ground_or_air = air ? GA_Air : GA_Ground;
    fighter.x2224_b2 = alternative;
    fighter.is_sandbag = bag;
    fighter.mv.co.downspot.x4 = 77;
    live = true;
    invalidate = remove;
    count = 0;
}
static void require(bool condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "common transition check %u failed\n", checks);
        exit(1);
    }
}
int main(void)
{
    for (unsigned air = 0; air < 2; air++) {
        for (unsigned alt = 0; alt < 2; alt++) {
            for (unsigned blended = 0; blended < 2; blended++) {
                reset(air, alt, false, false);
                if (blended) {
                    reference_freefall_blend(&object, 1, 0, 1, .7f, 12, .25f);
                } else {
                    reference_freefall(&object, 1, 0, true, .7f, 12);
                }
                Fighter expected = fighter;
                unsigned expected_count = count, saved[32];
                memcpy(saved, trace, sizeof(saved));
                reset(air, alt, false, false);
                if (blended) {
                    ftCo_800969D8(&object, 1, 0, 1, .7f, 12, .25f);
                } else {
                    ftCo_80096900(&object, 1, 0, true, .7f, 12);
                }
                require(!memcmp(&expected, &fighter, sizeof(fighter)));
                require(count == expected_count &&
                        !memcmp(trace, saved, count * sizeof(*trace)));
            }
        }
    }
    for (unsigned air = 0; air < 2; air++) {
        for (unsigned bag = 0; bag < 2; bag++) {
            reset(air, false, false, bag);
            reference_down_bound(&object);
            Fighter expected = fighter;
            unsigned expected_count = count, saved[32];
            memcpy(saved, trace, sizeof(saved));
            reset(air, false, false, bag);
            ftCo_80097D40(&object);
            require(!memcmp(&expected, &fighter, sizeof(fighter)));
            require(count == expected_count &&
                    !memcmp(trace, saved, count * sizeof(*trace)));
        }
    }
    for (unsigned air = 0; air < 2; air++) {
        for (unsigned bag = 0; bag < 2; bag++) {
            for (unsigned removal = 1; removal < 3; removal++) {
                reset(air, false, removal, bag);
                ftCo_80097D40(&object);
                require(fighter.mv.co.downspot.x4 == 77);
                require(fighter.x67C == 0 && fighter.x67D == 0);
                require(count == (bag ? 1 : air ? 3 : 2));
            }
        }
    }
    for (unsigned removal = 1; removal < 3; removal++) {
        reset(false, false, removal, false);
        fighter.mv.co.fallspecial.x0 = 99;
        ftCo_800969D8(&object, 1, 0, 1, .7f, 12, .25f);
        require(fighter.mv.co.fallspecial.x0 == 99);
        require(count == 1);
    }
    reset(false, false, false, false);
    live = false;
    ftCo_80096900(&object, 1, 0, true, .7f, 12);
    require(count == 0);
    printf("Common Thunder transition entries: %u checks PASS\n", checks);
    return 0;
}
