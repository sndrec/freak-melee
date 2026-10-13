#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/it/types.h>

// This native-only predicate fixture has no interpreted actor. The constructor's
// code-ID clear must still name the exact existing ShieldHit slot.
bool nativeScriptFighter_BindCallback(Fighter_GObj* object,uint32_t callback,uint32_t helper)
{
    (void)object;
    assert(callback==NativeScriptFighterCallback_ShieldHit && helper==UINT32_MAX);
    return false;
}
#include "script_guard_front_filter_generated.inc"
#include "script_guard_front_filter_original.inc"

static HSD_GObj object;
static void no_reaction(HSD_GObj* gobj) { (void) gobj; }

int main(void)
{
    Fighter* fighter = calloc(1, sizeof(*fighter));
    Fighter* attacker = calloc(1, sizeof(*attacker));
    Item* projectile = calloc(1, sizeof(*projectile));
    assert(fighter && attacker && projectile);
    object.user_data = fighter; fighter->x8_spawnNum = UINT32_C(0xFEDCBA98);
    MotionState state = {0};
    NativeFighterDefinition base = { .code_key = "guard_front_filter",
        .fighter_root = "ftDataTest", .motion_state_first = 341,
        .motion_state_count = 1, .motion_states = &state };
    NativeFighterHost host = {0}; // Pure field operations require no Host API.
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && definition->on_tick);
    unsigned bool_offset = UINT32_MAX;
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name, "front_seen"))
            bool_offset = definition->state_fields[i].offset;
    assert(bool_offset != UINT32_MAX);
    unsigned pairs = 0;
    const float facings[] = {-1, 1, 0};
    const float positions[] = {-2, 0, 2};
    for (unsigned active = 0; active < 2; ++active)
        for (unsigned reactive = 0; reactive < 2; ++reactive)
            for (unsigned auxiliary = 0; auxiliary < 2; ++auxiliary) {
                fighter->x221B_b0 = active; fighter->x221B_b1 = reactive;
                fighter->x221B_b2 = auxiliary; fighter->x221B_b3 = false;
                fighter->x221B_b4 = auxiliary;
                definition->on_tick(&object);
                assert(fighter->x221B_b0 && !fighter->x221B_b1 && fighter->x221B_b3);
                assert(fighter->x221B_b2 == auxiliary && fighter->x221B_b4 == auxiliary);
                assert(fighter->native_module_state[bool_offset]);
                for (unsigned facing = 0; facing < 3; ++facing)
                    for (unsigned defender = 0; defender < 3; ++defender)
                        for (unsigned incoming = 0; incoming < 3; ++incoming) {
                            fighter->facing_dir = facings[facing];
                            fighter->cur_pos.x = positions[defender];
                            attacker->cur_pos.x = projectile->pos.x = positions[incoming];
                            bool expected = facings[facing] == -1 ?
                                positions[incoming] <= positions[defender] :
                                positions[incoming] >= positions[defender];
                            assert(actual_fighter_front_filter(fighter, attacker) == expected);
                            assert(actual_item_front_filter(fighter, projectile) == expected);
                            ++pairs;
                        }
                fighter->x221B_b0 = active; fighter->x221B_b1 = reactive;
                assert(fighter->take_dmg_cb);
                fighter->take_dmg_cb(&object);
                assert(!fighter->x221B_b3 && fighter->x221B_b0 == active && fighter->x221B_b1 == reactive);
                assert(fighter->x221B_b2 == auxiliary && fighter->x221B_b4 == auxiliary);
                attacker->cur_pos.x = projectile->pos.x = -100;
                assert(actual_fighter_front_filter(fighter, attacker));
                assert(actual_item_front_filter(fighter, projectile));
            }
    FighterBone bone = {0}; HSD_JObj joint = {0}; bone.joint = &joint; fighter->parts = &bone;
    fighter->x221B_b0 = false; fighter->x221B_b1 = fighter->x221B_b2 = true;
    fighter->x221B_b3 = fighter->x221B_b4 = true;
    ShieldDesc descriptor = { .bone = 0, .radius = 3, .pos = {1, 2, 4} };
    ftColl_8007B1B8(&object, &descriptor, no_reaction);
    assert(fighter->x221B_b0 && !fighter->x221B_b1 && !fighter->x221B_b2 &&
        !fighter->x221B_b3 && !fighter->x221B_b4);
    assert(fighter->shield_hit_cb == no_reaction && fighter->shield_hit.bone == &joint &&
        fighter->shield_hit.size == 3 && !memcmp(&fighter->shield_hit.offset, &descriptor.pos, sizeof(Vec3)));
    free(projectile); free(attacker); free(fighter);
    printf("%u generated front-filter/fighter-item predicate pairs plus actual guard reset PASS; collision traversal excluded.\n", pairs);
    return 0;
}
