#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#ifndef SOUND_MODULE
#define SOUND_MODULE "decl_action.inc"
#endif
#include SOUND_MODULE
int main(void)
{
    MotionState state={0};NativeFighterHost host={0};
    NativeFighterDefinition base={.code_key="sound_policy",.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&state};
    for(unsigned mask=0;mask<128;++mask){
        base.sound_mute_channels=(uint8_t)mask;
        const NativeFighterDefinition* definition=FreakMelee_GetFighterModule(&base,&host);assert(definition);
        assert(definition->sound_mute_channels==(SOUND_INHERIT?mask:127));assert(base.sound_mute_channels==mask);
        assert(definition->sound_banks==base.sound_banks);
    }
    puts("PASS generated module sound policy127/inherited128values");return 0;
}
