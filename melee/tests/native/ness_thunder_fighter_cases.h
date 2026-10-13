// Additional original fighter bodies link unchanged from the reference SDK.
// This header keeps the article fixture's shared data/engine-boundary mocks in
// one place, while separating the nine-motion comparison matrix from articles.
#include <stdarg.h>

#include <melee/ef/efsync.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_DownBound.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/lb/lb_00B0.h>
static ftNessAttributes fighter_tuning;
static bool frames_remaining, floor_contact, ground_contact, cliff_contact;
static MotionFlags last_flags[2];
static bool compare_wall_slide_rounding;
static float maximum_wall_slide_difference;
static unsigned wall_slide_rounding_cases;
static float parameter_float(const NativeFighterParameters* p,
                             const char* name)
{
    for (unsigned i = 0; i < p->count; i++) {
        if (!strcmp(p->fields[i].name, name)) {
            float v;
            memcpy(&v, &p->fields[i].bits, 4);
            return v;
        }
    }
    unexpected();
    return 0;
}
static int parameter_int(const NativeFighterParameters* p, const char* name)
{
    for (unsigned i = 0; i < p->count; i++) {
        if (!strcmp(p->fields[i].name, name)) {
            return (int) p->fields[i].bits;
        }
    }
    unexpected();
    return 0;
}
void Fighter_ChangeMotionState(Fighter_GObj* g, FtMotionId motion,
                               MotionFlags flags, float start, float rate,
                               float blend, HSD_GObj* other)
{
    Fighter* f = g->user_data;
    (void) blend;
    (void) other;
    f->motion_id = motion;
    f->cur_anim_frame = start;
    f->frame_speed_mul = rate;
    last_flags[lane] = flags;
    record(20, motion, flags, start, rate, blend, 0);
}
void ftAnim_8006EBA4(Fighter_GObj* g)
{
    (void) g;
    record(21, 0, 0, 0, 0, 0, 0);
}
bool ftAnim_IsFramesRemaining(Fighter_GObj* g)
{
    (void) g;
    return frames_remaining;
}
void lb_8000B1CC(HSD_JObj* j, Vec3* offset, Vec3* out)
{
    (void) j;
    (void) offset;
    *out = (Vec3) { 7, 9, 1 };
    record(22, 0, 0, 0, 0, 0, 0);
}
void ftCommon_UseAllJumps(Fighter* f)
{
    f->x1968_jumpsUsed = f->co_attrs.max_jumps;
}
void ftCommon_8007D60C(Fighter* f)
{
    f->ground_or_air = GA_Air;
    f->gr_vel = 0;
    f->x74_self_accel.y = 0;
    f->x1968_jumpsUsed = f->co_attrs.max_jumps;
    f->ecb_lock = 5;
    f->coll_data.x130_flags |= CollData_X130_Locked;
    record(23, 0, 0, 0, 0, 0, 0);
}
void ftCommon_8007D7FC(Fighter* f)
{
    f->ground_or_air = GA_Ground;
    record(24, 0, 0, 0, 0, 0, 0);
}
void ftCommon_SetSelfMovementFromGroundedMovement(Fighter_GObj* g)
{
    Fighter* f = g->user_data;
    f->self_vel.x = f->gr_vel;
    record(25, 0, 0, 0, 0, 0, 0);
}
void ft_80084F3C(Fighter_GObj* g)
{
    ((Fighter*) g->user_data)->gr_vel *= .9f;
    record(26, 0, 0, 0, 0, 0, 0);
}
void ftCommon_Fall(Fighter* f, float gravity, float terminal)
{
    f->self_vel.y -= gravity;
    if (f->self_vel.y < -terminal) {
        f->self_vel.y = -terminal;
    }
    record(27, 0, 0, gravity, terminal, 0, 0);
}
void ftCommon_CalcSelfAccel_Deaccel(Fighter* f, float friction)
{
    f->self_vel.x *= 1 - friction;
    record(28, 0, 0, friction, 0, 0, 0);
}
void ftCommon_ClampSelfVelX(Fighter* f, float limit)
{
    if (f->self_vel.x > limit) {
        f->self_vel.x = limit;
    }
    if (f->self_vel.x < -limit) {
        f->self_vel.x = -limit;
    }
    record(29, 0, 0, limit, 0, 0, 0);
}
bool ft_80082708(Fighter_GObj* g)
{
    (void) g;
    record(30, 0, 0, 0, 0, 0, 0);
    return ground_contact;
}
GroundOrAir ft_80081D0C(Fighter_GObj* g)
{
    (void) g;
    record(31, 0, 0, 0, 0, 0, 0);
    return floor_contact;
}
bool ft_CheckGroundAndLedge(Fighter_GObj* g, int direction)
{
    (void) g;
    record(32, direction, 0, 0, 0, 0, 0);
    return floor_contact;
}
bool ftCliffCommon_80081298(Fighter_GObj* g)
{
    (void) g;
    record(33, 0, 0, 0, 0, 0, 0);
    return cliff_contact;
}
void ftCliffCommon_80081370(Fighter_GObj* g)
{
    ((Fighter*) g->user_data)->motion_id = 252;
    record(34, 0, 0, 0, 0, 0, 0);
}
void ftCo_Fall_Enter(Fighter_GObj* g)
{
    ((Fighter*) g->user_data)->motion_id = 29;
    record(35, 0, 0, 0, 0, 0, 0);
}
void ftCo_800969D8(Fighter_GObj* g, int a, int b, int interrupt,
                   float mobility, float lag, float blend)
{
    ((Fighter*) g->user_data)->motion_id = 35;
    record(36, a, b, interrupt, mobility, lag, blend);
}
void ftCo_80097D40(Fighter_GObj* g)
{
    ((Fighter*) g->user_data)->motion_id = 183;
    record(37, 0, 0, 0, 0, 0, 0);
}
void ft_8008A2BC(Fighter_GObj* g)
{
    ((Fighter*) g->user_data)->motion_id = 14;
    record(38, 0, 0, 0, 0, 0, 0);
}
EF_SpawnResult efSync_Spawn(s32 id, HSD_GObj* g, ...)
{
    va_list args;
    va_start(args, g);
    if (id == 1030) {
        Vec3* position = va_arg(args, Vec3*);
        float* angle = va_arg(args, float*);
        record(39, id, 0, position->x, position->y, position->z, *angle);
    } else {
        HSD_JObj* joint = va_arg(args, HSD_JObj*);
        record(39, id, joint == bones[lane][4].joint, 0, 0, 0, 0);
    }
    va_end(args);
    return (EF_SpawnResult) { 0 };
}
static void effect_on_part(Fighter_GObj* g, s32 id, s32 part)
{
    efSync_Spawn(id, g, ((Fighter*) g->user_data)->parts[part].joint);
}
static bool owned(Fighter_GObj* g, s32 id)
{
    return id == (s32) items[1][0].x1C && items[1][0].owner == g;
}
// Full Ness OnDamage has unported Yoyo/bat dependencies; this boundary is not
// installed/called by these phase checks (only callback pointer presence).
void ftNs_Init_OnDamage(HSD_GObj* g)
{
    (void) g;
    unexpected();
}
static void configure_fighter_host(NativeFighterHost* h)
{
    h->change_motion_state = Fighter_ChangeMotionState;
    h->start_animation = ftAnim_8006EBA4;
    h->animation_frames_remaining = ftAnim_IsFramesRemaining;
    h->joint_position = lb_8000B1CC;
    h->force_air_and_use_jumps = ftCommon_8007D60C;
    h->air_to_ground = ftCommon_8007D7FC;
    h->ground_to_self_movement = ftCommon_SetSelfMovementFromGroundedMovement;
    h->ground_friction = ft_80084F3C;
    h->fall = ftCommon_Fall;
    h->air_decel = ftCommon_CalcSelfAccel_Deaccel;
    h->clamp_self_vel_x = ftCommon_ClampSelfVelX;
    h->ground_collision = ft_80082708;
    h->air_collision = ft_80081D0C;
    h->check_ground_and_ledge = ft_CheckGroundAndLedge;
    h->check_cliff_catch = ftCliffCommon_80081298;
    h->enter_cliff_catch = ftCliffCommon_80081370;
    h->enter_fall = ftCo_Fall_Enter;
    h->finish_ground_action = ft_8008A2BC;
    h->enter_freefall_blend = ftCo_800969D8;
    h->enter_down_bound = ftCo_80097D40;
    h->spawn_effect = efSync_Spawn;
    h->spawn_effect_on_part = effect_on_part;
    h->owned_article_exists = owned;
    h->vector_rotate_unit_axis = lbVector_RotateAboutUnitAxis;
    h->sqrt_float = sqrtf;
}
static void setup_fighter(int motion, float facing,
                          const NativeFighterParameters* params)
{
    reset(motion, facing);
    memset(&fighter_tuning, 0, sizeof(fighter_tuning));
    fighter_tuning.x40_PK_THUNDER_LOOP1 =
        parameter_int(params, "thunder_loop1");
    fighter_tuning.x44_PK_THUNDER_LOOP2 =
        parameter_int(params, "thunder_loop2");
    fighter_tuning.x48_PK_THUNDER_GRAVITY_DELAY =
        parameter_int(params, "thunder_gravity_delay");
    fighter_tuning.x50_PK_THUNDER_FALL_ACCEL =
        parameter_float(params, "thunder_fall_acceleration");
    fighter_tuning.x54_PK_THUNDER_2_MOMENTUM =
        parameter_float(params, "thunder_launch_speed");
    fighter_tuning.x5C_PK_THUNDER_2_DECELERATION_RATE =
        parameter_float(params, "thunder_launch_deceleration");
    fighter_tuning.x60_PK_THUNDER_2_KNOCKDOWN_ANGLE =
        parameter_float(params, "thunder_knockdown_degrees");
    fighter_tuning.x64_PK_THUNDER_2_WALLHUG_ANGLE =
        parameter_float(params, "thunder_wallhug_degrees");
    fighter_tuning.x6C_PK_THUNDER_2_FREEFALL_ANIM_BLEND =
        parameter_float(params, "thunder_freefall_blend");
    fighter_tuning.x70_PK_THUNDER_2_LANDING_LAG =
        parameter_float(params, "thunder_landing_lag");
    for (unsigned l = 0; l < 2; l++) {
        Fighter* f = &fighters[l];
        f->dat_attrs = &fighter_tuning;
        f->facing_dir = facing;
        f->x34_scale = (Vec3) { 1.2f, 1.4f, 1.6f };
        f->ground_or_air = motion >= 362 ? GA_Air : GA_Ground;
        f->co_attrs.max_jumps = 2;
        f->co_attrs.gravity = .12f;
        f->co_attrs.terminal_velocity = 2;
        f->co_attrs.aerial_friction = .02f;
        f->co_attrs.air_drift_max = .9f;
        f->self_vel = (Vec3) { 2.4f, 3.8f, .4f };
        f->gr_vel = 1.7f;
        f->cur_anim_frame = 8;
        f->coll_data.floor.normal = (Vec3) { 0, 1, 0 };
        f->coll_data.ceiling.normal = (Vec3) { 0, -1, 0 };
        f->coll_data.left_facing_wall.normal = (Vec3) { 1, 0, 0 };
        f->coll_data.right_facing_wall.normal = (Vec3) { -1, 0, 0 };
    }
    fighters[0].mv.ns.specialhi.thunderTimerLoop1 = 3;
    fighter_set("thunder_loop1", 3);
    fighters[0].mv.ns.specialhi.thunderTimerLoop2 = 2;
    fighter_set("thunder_loop2", 2);
    fighters[0].mv.ns.specialhi.gravityDelay = 1;
    fighter_set("thunder_gravity_delay", 1);
    fighters[0].mv.ns.specialhi.aerialVel = .6f;
    fighter_set("thunder_aerial_angle", bits(.6f));
    fighters[0].mv.ns.specialhi.unkVector1 = fighters[0].self_vel;
    fighter_set("thunder_previous_x", bits(2.4f));
    fighter_set("thunder_previous_y", bits(3.8f));
    fighter_set("thunder_previous_z", bits(.4f));
    fighter_set("thunder_launch_effect_frame", 0);
    fighter_set("thunder_fall_acceleration", 0);
    fighter_set("thunder_collision_mode", 0);
    fighter_set("flash_handle", UINT32_MAX);
    frames_remaining = true;
    ground_contact = true;
    floor_contact = false;
    cliff_contact = false;
    memset(last_flags, 0, sizeof(last_flags));
}
static float fighter_float(const char* n)
{
    uint32_t u = fighter_get(n);
    float v;
    memcpy(&v, &u, 4);
    return v;
}
static void compare_phase_fighter(void)
{
    Fighter* a = &fighters[0];
    Fighter* b = &fighters[1];
    check(a->motion_id == b->motion_id, "fighter motion");
    check(last_flags[0] == last_flags[1], "motion flags");
    eqf(a->cur_anim_frame, b->cur_anim_frame, "frame");
    eqf(a->facing_dir, b->facing_dir, "facing");
    if (compare_wall_slide_rounding) {
        // Editable pi/tau are binary32; the original compound arithmetic uses
        // double macros. Use a vector-relative bound only for this rotation,
        // so cancellation in one component does not turn it into a relative
        // scalar error. Other comparisons retain their original tighter bound.
        float scale = fmaxf(1, sqrtf(a->self_vel.x * a->self_vel.x +
                                     a->self_vel.y * a->self_vel.y +
                                     a->self_vel.z * a->self_vel.z));
        float delta[] = { fabsf(a->self_vel.x - b->self_vel.x),
                          fabsf(a->self_vel.y - b->self_vel.y),
                          fabsf(a->self_vel.z - b->self_vel.z) };
        for (unsigned i = 0; i < 3; ++i) {
            maximum_wall_slide_difference =
                fmaxf(maximum_wall_slide_difference, delta[i]);
            check(delta[i] <= 2e-6f * scale, "wall-slide velocity");
        }
        float original_bound[] = {
            2e-6f *
                fmaxf(1, fmaxf(fabsf(a->self_vel.x), fabsf(b->self_vel.x))),
            2e-6f *
                fmaxf(1, fmaxf(fabsf(a->self_vel.y), fabsf(b->self_vel.y))),
            2e-6f * fmaxf(1, fmaxf(fabsf(a->self_vel.z), fabsf(b->self_vel.z)))
        };
        if (delta[0] > original_bound[0] || delta[1] > original_bound[1] ||
            delta[2] > original_bound[2])
        {
            ++wall_slide_rounding_cases;
        }
    } else {
        eqf(a->self_vel.x, b->self_vel.x, "velocity x");
        eqf(a->self_vel.y, b->self_vel.y, "velocity y");
        eqf(a->self_vel.z, b->self_vel.z, "velocity z");
    }
    eqf(a->gr_vel, b->gr_vel, "ground velocity");
    eqf(a->cur_pos.x, b->cur_pos.x, "position x");
    eqf(a->cur_pos.y, b->cur_pos.y, "position y");
    eqf(a->cur_pos.z, b->cur_pos.z, "position z");
    check(a->x1968_jumpsUsed == b->x1968_jumpsUsed, "jumps used");
    check(a->ground_or_air == b->ground_or_air, "ground/air");
    check(a->mv.ns.specialhi.thunderTimerLoop1 ==
              (int) fighter_get("thunder_loop1"),
          "loop1");
    check(a->mv.ns.specialhi.thunderTimerLoop2 ==
              (int) fighter_get("thunder_loop2"),
          "loop2");
    check(a->mv.ns.specialhi.gravityDelay ==
              (int) fighter_get("thunder_gravity_delay"),
          "gravity delay");
    check(a->mv.ns.specialhi.thunderColl ==
              (int) fighter_get("thunder_collision_mode"),
          "contact mode");
    check(a->mv.ns.specialhi.jibakuGFX ==
              (int) fighter_get("thunder_launch_effect_frame"),
          "effect timer");
    eqf(a->mv.ns.specialhi.aerialVel, fighter_float("thunder_aerial_angle"),
        "launch angle");
    eqf(a->mv.ns.specialhi.fallAccel,
        fighter_float("thunder_fall_acceleration"), "fall acceleration");
    eqf(a->mv.ns.specialhi.unkVector1.x, fighter_float("thunder_previous_x"),
        "previous x");
    eqf(a->mv.ns.specialhi.unkVector1.y, fighter_float("thunder_previous_y"),
        "previous y");
    eqf(a->mv.ns.specialhi.unkVector1.z, fighter_float("thunder_previous_z"),
        "previous z");
    compare_fighter();
    trace();
}
static void fighter_cases(const NativeFighterParameters* params)
{
    HSD_GObjEvent original[9][4] = {
#define PHASES(n)                                                             \
    { ftNs_##n##_Anim, ftNs_##n##_IASA, ftNs_##n##_Phys, ftNs_##n##_Coll }
        PHASES(SpecialHiStart),     PHASES(SpecialHiHold),
        PHASES(SpecialHiEnd),       PHASES(SpecialHi),
        PHASES(SpecialAirHiStart),  PHASES(SpecialAirHiHold),
        PHASES(SpecialAirHiEnd),    PHASES(SpecialAirHi),
        PHASES(SpecialAirHiRebound)
#undef PHASES
    };
    for (unsigned face = 0; face < 2; face++) {
        for (unsigned m = 0; m < 9; m++) {
            for (unsigned p = 0; p < 4; p++) {
                for (unsigned branch = 0; branch < 5; branch++) {
                    phase = "fighter phases";
                    setup_fighter(358 + m, face ? -1 : 1, params);
                    frames_remaining = branch != 1;
                    ground_contact = branch != 2;
                    floor_contact = branch == 3;
                    cliff_contact = branch == 4;
                    if (branch == 2) {
                        fighters[0].u.ns.pkthunder_gobj = NULL;
                        fighter_set("thunder_ball_handle", UINT32_MAX);
                    }
                    if (branch == 4) {
                        fighters[0].coll_data.env_flags =
                            fighters[1].coll_data.env_flags =
                                Collide_LeftWallMask;
                    }
                    const MotionState* authored =
                        &definition
                             ->motion_states[358 + m -
                                             definition->motion_state_first];
                    HSD_GObjEvent callbacks[] = { authored->anim_cb,
                                                  authored->input_cb,
                                                  authored->phys_cb,
                                                  authored->coll_cb };
                    check(callbacks[p] != NULL, "phase exists");
                    lane = 0;
                    original[m][p](&owners[0]);
                    lane = 1;
                    callbacks[p](&owners[1]);
                    compare_phase_fighter();
                }
            }
        }
    }
    // Strict self-contact rectangle, platform bypass and slope/knockdown
    // split.
    for (unsigned face = 0; face < 2; face++) {
        for (unsigned air = 0; air < 2; air++) {
            for (unsigned platform = 0; platform < 2; platform++) {
                for (int x = -4; x <= 4; x += 4) {
                    for (int y = -4; y <= 4; y += 4) {
                        phase = "fighter contact launch";
                        setup_fighter(air ? 363 : 359, face ? -1 : 1, params);
                        for (unsigned l = 0; l < 2; l++) {
                            fighters[l].coll_data.floor.flags =
                                platform ? LINE_FLAG_PLATFORM : 0;
                        }
                        Vec3 point = { fighters[0].cur_pos.x - x,
                                       fighters[0].cur_pos.y +
                                           5 * fighters[0].x34_scale.y - y,
                                       0 };
                        items[0][0].xDD4_itemVar.pkthunder.positions[0] =
                            point;
                        set(0, "history_x", 0, bits(point.x));
                        set(0, "history_y", 0, bits(point.y));
                        set(0, "history_z", 0, bits(point.z));
                        lane = 0;
                        original[air ? 5 : 1][0](&owners[0]);
                        lane = 1;
                        definition
                            ->motion_states[(air ? 363 : 359) -
                                            definition->motion_state_first]
                            .anim_cb(&owners[1]);
                        compare_phase_fighter();
                    }
                }
            }
        }
    }
    // Collision priority, steep bounce and shallow wall slide (including
    // both).
    for (unsigned face = 0; face < 2; face++) {
        for (unsigned mask = 0; mask < 8; mask++) {
            for (unsigned normal = 0; normal < 16; normal++) {
                for (unsigned floor = 0; floor < 2; floor++) {
                    phase = "fighter launch geometry";
                    setup_fighter(365, face ? -1 : 1, params);
                    floor_contact = floor;
                    float angle = normal * 6.28318530717958647692f / 16;
                    Vec3 n = { cosf(angle), sinf(angle), 0 };
                    uint32_t flags = (mask & 1 ? Collide_CeilingMask : 0) |
                                     (mask & 2 ? Collide_LeftWallMask : 0) |
                                     (mask & 4 ? Collide_RightWallMask : 0);
                    for (unsigned l = 0; l < 2; l++) {
                        fighters[l].coll_data.env_flags = flags;
                        fighters[l].coll_data.floor.normal = n;
                        fighters[l].coll_data.ceiling.normal = n;
                        fighters[l].coll_data.left_facing_wall.normal = n;
                        fighters[l].coll_data.right_facing_wall.normal = n;
                    }
                    lane = 0;
                    original[7][3](&owners[0]);
                    lane = 1;
                    definition
                        ->motion_states[365 - definition->motion_state_first]
                        .coll_cb(&owners[1]);
                    compare_wall_slide_rounding = !floor && !(mask & 1) &&
                                                  (mask & 6) &&
                                                  fighters[0].motion_id == 365;
                    unsigned prior_rounding = wall_slide_rounding_cases;
                    compare_phase_fighter();
                    if (prior_rounding != wall_slide_rounding_cases) {
                        printf("Binary32 wall-slide case: facing %d, mask %u, "
                               "normal %u, floor %u\n",
                               face ? -1 : 1, mask, normal, floor);
                    }
                    compare_wall_slide_rounding = false;
                }
            }
        }
    }
    // Constructors: missing ball, previous/current origin, allocation failure.
    // Damage cleanup detaches a live or reflected chain before cancelling
    // fighter presentation. Boundary motions outside the move must leave GFX
    // ownership untouched but still clear callbacks/root rotation.
    for (int motion = 357; motion <= 367; ++motion) {
        for (unsigned present = 0; present < 2; ++present) {
            for (unsigned damage_owner = 0; damage_owner < 3; ++damage_owner) {
                for (unsigned reflected = 0; reflected < 2; ++reflected) {
                    phase = "fighter damage cleanup";
                    setup_fighter(motion, 1, params);
                    attach_trails();
                    for (unsigned l = 0; l < 2; ++l) {
                        if (damage_owner) {
                            items[l][0].owner =
                                damage_owner == 1 ? NULL : &foreign[l];
                        }
                    }
                    items[0][0].xDD4_itemVar.pkthunder.xF00 = reflected;
                    set(0, "reflected", 0, reflected);
                    if (!present) {
                        fighters[0].u.ns.pkthunder_gobj = NULL;
                        fighter_set("thunder_ball_handle", UINT32_MAX);
                    }
                    lane = 0;
                    ftNs_SpecialHi_TakeDamage(&owners[0]);
                    lane = 1;
                    script_fn_thunder_damage(&owners[1]);
                    compare_phase_fighter();
                    for (unsigned i = 0; i < 7; ++i) {
                        compare(i);
                    }
                }
            }
        }
    }
    for (unsigned face = 0; face < 2; face++) {
        for (unsigned air = 0; air < 2; air++) {
            for (unsigned success = 0; success < 2; success++) {
                phase = "fighter constructor";
                setup_fighter(air ? 362 : 358, face ? -1 : 1, params);
                frames_remaining = false;
                allocation = success;
                fighters[0].u.ns.pkthunder_gobj = NULL;
                fighter_set("thunder_ball_handle", UINT32_MAX);
                lane = 0;
                original[air ? 4 : 0][0](&owners[0]);
                lane = 1;
                definition
                    ->motion_states[(air ? 362 : 358) -
                                    definition->motion_state_first]
                    .anim_cb(&owners[1]);
                compare_phase_fighter();
                compare_spawns();
                if (success) {
                    compare(0);
                }
            }
        }
    }
    for (unsigned face = 0; face < 2; face++) {
        for (unsigned air = 0; air < 2; air++) {
            phase = "fighter entries";
            setup_fighter(air ? 362 : 358, face ? -1 : 1, params);
            lane = 0;
            if (air) {
                ftNs_SpecialAirHiStart_Enter(&owners[0]);
            } else {
                ftNs_SpecialHiStart_Enter(&owners[0]);
            }
            lane = 1;
            if (air) {
                definition->special_air_hi(&owners[1]);
            } else {
                definition->special_hi(&owners[1]);
            }
            compare_phase_fighter();
            check(fighters[0].mv.ns.specialhi.thunderTimerLoop1 ==
                      (int) fighter_get("thunder_loop1"),
                  "entry loop1");
            check(fighters[0].mv.ns.specialhi.thunderColl ==
                      (int) fighter_get("thunder_collision_mode"),
                  "entry contact mode");
        }
    }
    printf("Wall-slide binary32 maximum absolute difference %.9g; "
           "%u cases exceed original scalar bound\n",
           maximum_wall_slide_difference, wall_slide_rounding_cases);
}
