#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/gobj.h>

static MotionState states[1] = { { .anim_id = 0 } };
static ItemStateTable article_states[1] = { { .anim_id = -1 } };
static ItemLogicTable article_logic = { .states = article_states };
static const NativeFighterArticleLogic article_logics[1] = {
    { It_Kind_Mario_Fire, &article_logic, NULL, 1 },
};

static void fixture_special_n(Fighter_GObj* gobj)
{
    Fighter* fighter = gobj->user_data;
    ++fighter->native_module_state[0];
}

static void fixture_death(Fighter_GObj* gobj)
{
    Fighter* fighter = gobj->user_data;
    fighter->native_module_state[1] = 0;
}

static const NativeFighterDefinition definition = {
    .code_key = "module_fixture",
    .source_kind = Ft_Kind_Mario,
    .animation_source_kind = Ft_Kind_Mario,
    .fighter_root = "ftDataMario",
    .costume_joint = "PlyMario5K_Share_joint",
    .costume_matanim = "PlyMario5K_Share_matanim_joint",
    .action_count = 303,
    .demo_count = 16,
    .motion_state_count = 1,
    .motion_states = states,
    .joint_count = 61,
    .state_bytes = 2,
    .special_n = fixture_special_n,
    .on_death = fixture_death,
    .article_logics = article_logics,
    .article_logic_count = 1,
};
static NativeFighterDefinition inherited;

__declspec(dllexport) uint64_t __cdecl
FreakMelee_GetFighterModuleLayout(void)
{
    return nativeFighterModule_LayoutFingerprint();
}

__declspec(dllexport) const NativeFighterDefinition* __cdecl
FreakMelee_GetFighterModule(const NativeFighterDefinition* base,
                           const NativeFighterHost* host)
{
    if (host == NULL || host->change_motion_state == NULL ||
        host->start_animation == NULL) return NULL;
    if (base != NULL) {
        inherited = *base;
        inherited.code_key = "module_fixture";
        inherited.state_bytes = 2;
        inherited.special_n = fixture_special_n;
        inherited.on_death = fixture_death;
        inherited.article_logics = article_logics;
        inherited.article_logic_count = 1;
        return &inherited;
    }
    return &definition;
}
