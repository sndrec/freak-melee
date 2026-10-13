#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <melee/ft/native_fighter_sound_policy.h>
#include COMMON_INIT_MODULE

static unsigned checks;
static bool alive(HSD_GObj* gobj, uint32_t generation, uint32_t source_class)
{
    return gobj && source_class == 0 && ((Fighter*)gobj->user_data)->x8_spawnNum == generation;
}
int main(void)
{
    MotionState motion = {0};
    NativeFighterHost host = { .callback_source_alive = alive };
    NativeFighterDefinition base = { .code_key = "common_init_policy", .fighter_root = "ftDataTest", .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host); assert(definition);
    assert(definition->ignore_surface_friction); assert(!base.ignore_surface_friction);
    assert(nativeFighterSoundPolicy_Validate(definition->sound_policy));
    Fighter fighter = { .x8_spawnNum = 12 }; HSD_GObj gobj = { .user_data = &fighter };
    fighter.x2222_b5 = true;
#if COMMON_INIT_STATE
    definition->motion_states[0].phys_cb(&gobj);
#else
    definition->on_tick(&gobj);
#endif
    assert(fighter.x2222_b4 && !fighter.x2222_b5);
    for (unsigned costume = 0; costume < 32; ++costume)
        for (int sound = 0x1FBF0; sound <= 0x1FCD0; ++sound) {
            int expected = sound;
            if (costume >= 2 && expected >= 0x1FBFD && expected <= 0x1FC62) expected += 0x66;
            if (costume < 2 && expected >= 0x1FC63 && expected <= 0x1FCC8) expected -= 0x66;
            assert(nativeFighterSoundPolicy_MapId(definition->sound_policy, costume, 13, sound) == expected); ++checks;
            assert(nativeFighterSoundPolicy_MapId(definition->sound_policy, costume, 12, sound) == sound); ++checks;
        }
    for (unsigned costume = 0; costume < 33; ++costume) {
        assert(nativeFighterSoundPolicy_KoCue(definition->sound_policy, costume, 100) == (costume >= 2 && costume < 32 ? 0x1FBD1 : 100)); ++checks;
    }
    puts("PASS generated common-init policy and unchanged copied base");
    printf("PASS %u generated sound routing checks\n", checks);
    return 0;
}
