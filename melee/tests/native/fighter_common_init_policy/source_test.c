#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/mpcoll.h>

static NativeFighterDefinition owned;
static int bank, map_calls, surface_calls;
static float surface;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    return kind == Ft_Kind_CustomFirst ? &owned : NULL;
}
int lbAudioAx_800233EC(int sound) { ++map_calls; return sound; }
int lbAudioAx_80023130(int sound) { (void)sound; return bank; }
int lbAudioAx_800230C8(int id, int* offset, int* unused) { (void)id; *offset = 0; *unused = 0; return 1; }
int lbAudioAx_80023220(int id) { (void)id; return 0; }
s32 ft_80087C70(Fighter* fp, s32 sound) { (void)fp; return sound + 10; }
float mpColl_8004CA6C(CollData* data) { (void)data; ++surface_calls; return surface; }
#include "original_common_init.inc"
#include "candidate_common_init.inc"

int main(void)
{
    unsigned pairs = 0, checks = 0;
    NativeFighterSoundIdRule rules[] = {
        { UINT32_C(0xFFFFFFFC), 13, 0x1FBFD, 0x1FC62, 0x66 },
        { UINT32_C(3), 13, 0x1FC63, 0x1FCC8, -0x66 },
    };
    NativeFighterKoCue cue = { UINT32_C(0xFFFFFFFC), 0x1FBD1 };
    NativeFighterSoundPolicy policy = { rules, &cue, 2, 1 };
    owned.sound_policy = &policy; owned.ignore_surface_friction = true;
    assert(nativeFighterSoundPolicy_Validate(&policy));
    ftData data = {0}; FtSFX sfx = {0};
    data.x4C_sfx = &sfx; sfx.x34 = 1234;
    for (unsigned kind = Ft_Kind_Popo; kind <= Ft_Kind_Nana; ++kind)
        for (unsigned costume = 0; costume < 8; ++costume) {
            Fighter old = { .kind = kind, .x619_costume_id = costume, .ft_data = &data };
            Fighter current = old; current.kind = Ft_Kind_CustomFirst;
            HSD_GObj old_gobj = {.user_data = &old}, current_gobj = {.user_data = &current};
            assert(original_ftLib_8008746C(&old_gobj) == ftLib_8008746C(&current_gobj)); ++checks; ++pairs;
            for (bank = 0; bank < 16; ++bank)
                for (int sound = 0x1FBF0; sound <= 0x1FCD0; ++sound) {
                    int before = map_calls;
                    assert(original_ft_80087D0C(&old, sound) == ft_80087D0C(&current, sound));
                    assert(map_calls == before + 2); checks += 2; ++pairs;
                }
            for (unsigned i = 0; i < 4; ++i) {
                surface = (float[]){0.2f, 0.7f, 1.0f, 1.4f}[i];
                surface_calls = 0;
                assert(original_ft_GetGroundFrictionMultiplier(&old) == ft_GetGroundFrictionMultiplier(&current));
                assert(surface_calls == 0); checks += 2; ++pairs;
            }
        }
    owned.ignore_surface_friction = false;
    Fighter regular = { .kind = Ft_Kind_Mario }, custom = { .kind = Ft_Kind_CustomFirst };
    for (unsigned i = 0; i < 4; ++i) {
        surface = (float[]){0.2f, 0.7f, 1.0f, 1.4f}[i]; surface_calls = 0;
        assert(original_ft_GetGroundFrictionMultiplier(&regular) == ft_GetGroundFrictionMultiplier(&custom));
        assert(surface_calls == 2); checks += 2; ++pairs;
    }
    assert(nativeFighterSoundPolicy_MapId(&policy, 32, 13, 0x1FBFD) == 0x1FBFD);
    assert(nativeFighterSoundPolicy_MapId(NULL, 0, 13, 10) == 10);
    assert(nativeFighterSoundPolicy_KoCue(NULL, 0, 10) == 10);
    rules[0].offset = INT32_MAX; assert(!nativeFighterSoundPolicy_Validate(&policy)); rules[0].offset = 0x66;
    rules[0].bank = 64; assert(!nativeFighterSoundPolicy_Validate(&policy)); rules[0].bank = 13;
    policy.rule_count = 17; assert(!nativeFighterSoundPolicy_Validate(&policy)); policy.rule_count = 2;
    cue.costume_mask = 0; assert(!nativeFighterSoundPolicy_Validate(&policy)); cue.costume_mask = UINT32_C(0xFFFFFFFC);
    NativeFighterKoCue conflict[] = {cue, cue}; policy.ko_cues = conflict; policy.ko_cue_count = 2;
    assert(!nativeFighterSoundPolicy_Validate(&policy));
    printf("PASS %u actual common sound/KO/friction source pairs, %u checks; routing validation boundaries\n", pairs, checks);
    return 0;
}
