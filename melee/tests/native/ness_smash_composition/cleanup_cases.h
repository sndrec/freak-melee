// Scoped production cleanup regression. The included block is extracted
// verbatim from Fighter_ChangeMotionState; this does not simulate its complete
// lifecycle.
#include "motion_cleanup.h"

static unsigned cleanup_cases;

static void enter_cleanup_motion(unsigned actor, FtMotionId motion,
                                 MotionFlags flags)
{
    fighters[actor].motion_id = motion;
    transition_hitbox_cleanup(&owners[actor], flags);
}

static void run_cleanup_regression(void)
{
    for (unsigned initial_latch = 0; initial_latch < 2; ++initial_latch) {
        for (unsigned control = 0; control < 3; ++control) {
            case_phase = 50;
            case_variant = control;
            seed(345, 16);
            for (unsigned actor = 0; actor < 2; ++actor) {
                fighters[actor].x2219_b3 = initial_latch;
                fighters[actor].x914[0].state = HitCapsule_Disabled;
                fighters[actor].cmd_vars[0] = control == 0;
            }
            int timer = control == 1 ? 0 : control == 2 ? 2 : 1;
            fighters[0].mv.ns.attackhi4.yoyoRehitTimer = timer;
            integer("yoyo_rehit_timer", timer);
            side = 0;
            ftNs_AttackHi4_YoyoCheckTimedRehit(&owners[0]);
            side = 1;
            script_fn_YoyoCheckTimedRehit(&owners[1]);
            check(fighters[0].x914[0].state == HitCapsule_Disabled &&
                      fighters[1].x914[0].state == HitCapsule_Disabled,
                  "inactive timer/command controls do not enable");
            check(fighters[0].x2219_b3 == initial_latch &&
                      fighters[1].x2219_b3 == initial_latch,
                  "inactive timer controls preserve existing latch");
            ++cleanup_cases;
        }
    }

    // Normal up/down completion and down-smash interruption. SkipHit must
    // retain active hitboxes/cleanup intent, while an ordinary exit clears the
    // new latch. Repeat with an already-latched stock hitbox to retain its
    // unchanged behavior.
    for (unsigned route = 0; route < 3; ++route) {
        for (unsigned endpoint = 0; endpoint < 2; ++endpoint) {
            for (unsigned skip = 0; skip < 2; ++skip) {
                for (unsigned initial_latch = 0; initial_latch < 2;
                     ++initial_latch)
                {
                    case_phase = 51 + route;
                    case_variant = endpoint * 4 + skip * 2 + initial_latch;
                    seed(route == 0 ? 344 : route == 1 ? 347 : 345, 16);
                    for (unsigned actor = 0; actor < 2; ++actor) {
                        fighters[actor].x2219_b3 = initial_latch;
                        fighters[actor].x914[0].state = HitCapsule_Disabled;
                        fighters[actor].cmd_vars[0] = 0;
                        fighters[actor].allow_interrupt = true;
                    }
                    fighters[0].mv.ns.attackhi4.yoyoRehitTimer = 1;
                    integer("yoyo_rehit_timer", 1);
                    side = 0;
                    ftNs_AttackHi4_YoyoCheckTimedRehit(&owners[0]);
                    side = 1;
                    script_fn_YoyoCheckTimedRehit(&owners[1]);
                    check(fighters[0].mv.ns.attackhi4.yoyoRehitTimer == 0 &&
                              get_int("yoyo_rehit_timer") == 0,
                          "delayed refresh reaches zero");
                    check(fighters[0].x914[0].state == HitCapsule_Enabled &&
                              fighters[1].x914[0].state == HitCapsule_Enabled,
                          "refresh still enables both hitboxes");
                    check(fighters[0].x2219_b3 == initial_latch &&
                              fighters[1].x2219_b3,
                          "only generic enable adds missing cleanup intent");

                    finished = true;
                    fighters[0].mv.ns.attackhi4.yoyoCurrentFrame = 61;
                    integer("yoyo_frame", 61);
                    apply_transition_cleanup = true;
                    cleanup_transition_flags = skip ? Ft_MF_SkipHit : 0;
                    side = 0;
                    if (route == 0) {
                        ftNs_AttackHi4Release_Anim(&owners[0]);
                    } else if (route == 1) {
                        ftNs_AttackLw4Release_Anim(&owners[0]);
                    } else {
                        ftNs_AttackLw4_IASA(&owners[0]);
                    }
                    side = 1;
                    if (route == 2) {
                        definition->motion_states[4].input_cb(&owners[1]);
                    } else {
                        definition->motion_states[route == 0 ? 3 : 6].anim_cb(
                            &owners[1]);
                    }
                    apply_transition_cleanup = false;
                    check(fighters[0].motion_id == (route == 2
                                                        ? ftCo_MS_KneeBend
                                                        : ftCo_MS_Wait) &&
                              fighters[1].motion_id == fighters[0].motion_id,
                          "completion/interruption selected transition");
                    bool original_active = skip || !initial_latch;
                    check((fighters[0].x914[0].state != HitCapsule_Disabled) ==
                              original_active,
                          "original missing-latch branch demonstrates "
                          "lingering hitbox");
                    check((fighters[1].x914[0].state != HitCapsule_Disabled) ==
                              (skip != 0),
                          "fixed exit clears hitbox unless SkipHit explicitly "
                          "preserves it");
                    check(fighters[1].x2219_b3 == (skip != 0),
                          "SkipHit retains intent; ordinary cleanup consumes "
                          "it");

                    // Preserve the existing hitboxes during the airborne
                    // transition to expose the old leak without creating a new
                    // aerial hitbox.
                    enter_cleanup_motion(0, ftCo_MS_AttackAirN, Ft_MF_SkipHit);
                    enter_cleanup_motion(1, ftCo_MS_AttackAirN, Ft_MF_SkipHit);
                    check((fighters[0].x914[0].state != HitCapsule_Disabled) ==
                                  original_active &&
                              (fighters[1].x914[0].state !=
                               HitCapsule_Disabled) == (skip != 0),
                          "airborne transition preserves explicit SkipHit "
                          "state");
                    enter_cleanup_motion(0, ftCo_MS_AttackAirN, 0);
                    enter_cleanup_motion(1, ftCo_MS_AttackAirN, 0);
                    check(fighters[0].x914[0].state ==
                                  (initial_latch ? HitCapsule_Disabled
                                                 : HitCapsule_Enabled) &&
                              fighters[1].x914[0].state == HitCapsule_Disabled,
                          "ordinary airborne entry consumes fixed preserved "
                          "cleanup intent");
                    FtMotionId next =
                        endpoint ? ftCo_MS_CliffWait : ftCo_MS_LandingAirN;
                    enter_cleanup_motion(0, next, 0);
                    enter_cleanup_motion(1, next, 0);
                    check(fighters[0].x914[0].state ==
                              (initial_latch ? HitCapsule_Disabled
                                             : HitCapsule_Enabled),
                          "original unlatched hitbox survives landing/ledge "
                          "cleanup");
                    check(fighters[1].x914[0].state == HitCapsule_Disabled &&
                              !fighters[1].x2219_b3,
                          "fixed landing/ledge cleanup disables hitbox");
                    // A subsequent real common hitbox activation remains
                    // normal: its existing engine latch is consumed at
                    // landing/ledge too.
                    for (unsigned actor = 0; actor < 2; ++actor) {
                        enter_cleanup_motion(actor, ftCo_MS_AttackAirN, 0);
                        ftColl_8007B064(&owners[actor], 1);
                        check(fighters[actor].x914[1].state ==
                                      HitCapsule_Enabled &&
                                  fighters[actor].x2219_b3,
                              "ordinary airborne attack activation retains "
                              "existing latch");
                        enter_cleanup_motion(actor, next, 0);
                        check(fighters[actor].x914[0].state ==
                                      HitCapsule_Disabled &&
                                  fighters[actor].x914[1].state ==
                                      HitCapsule_Disabled &&
                                  !fighters[actor].x2219_b3,
                              "subsequent registered aerial hitboxes clear at "
                              "landing/ledge");
                    }
                    for (unsigned actor = 0; actor < 2; ++actor) {
                        // Actual ftColl_8007AFF8 visits all four slots; no
                        // victim history, damage or per-victim bookkeeping is
                        // reset here.
                        for (unsigned hit = 1; hit < 4; ++hit) {
                            check(fighters[actor].x914[hit].state ==
                                      HitCapsule_Disabled,
                                  "cleanup visits other disabled slots "
                                  "without enabling them");
                        }
                        check(
                            fighters[actor].x914[0].damage == 10.75f &&
                                fighters[actor].x914[0].victims_1[0].x4 == 12,
                            "cleanup preserves damage and victim bookkeeping");
                    }
                    ++cleanup_cases;
                }
            }
        }
    }
    check(cleanup_cases == 30,
          "cleanup regression routes and preservation controls");
}
