#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/ef/types.h>
#include <melee/ft/types.h>
#include <melee/ft/native_callback_lifetime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static HSD_GObj creator, object;
static Fighter fighter;
static EF_Effect effect;
static HSD_JObj origin, replacement;
static FighterBone parts[2];
static unsigned free_calls, spawn_calls;
static int spawn_case;
static const NativeFighterDefinition* active;
static void authored_update(EF_Effect* source) { (void)source; }
static const NativeFighterModelEffect policy[] = {
    { 1, 0x47C, authored_update }, { 2, 0x509, authored_update }
};
static const NativeFighterDefinition definition = {
    .joint_count = 2, .model_effect_count = 2, .model_effects = policy
};

HSD_ObjAllocData efAsync_AllocData;
EF_EffectUsage efLib_Usage;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
void* HSD_ObjAlloc(HSD_ObjAllocData* allocator) { (void)allocator; abort(); }
void efAsync_QueueProcessDeferred(HSD_GObj* source, EF_QueuedEffect* queued)
{ (void)source; (void)queued; abort(); }

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    return kind == Ft_Kind_CustomFirst ? active : NULL;
}

const EF_ModelRecipe* efSync_ModelRecipe(s32 recipe)
{
    static const EF_ModelRecipe joint = { EF_MODEL_JOINT, 1 };

    // This fixture covers binding/lifetime; real constructor catalog and
    // dispatcher provenance are checked separately against production source.
    return (recipe == 0x47C || recipe == 0x509) ? &joint : NULL;

}

void efLib_remove_user_data(void* data) { (void)data; }
void HSD_GObjFree(HSD_GObj* source)
{
    assert(source == &object);
    ++free_calls;
    // Deliberately deferred: membership alone must not imply a live callback.
}

EF_SpawnResult efSync_Spawn(s32 recipe, HSD_GObj* parent, ...)
{
    assert(recipe == 0x47C || recipe == 0x509);
    assert(parent == &creator);
    ++spawn_calls;
    EF_SpawnResult result = { .kind = EF_SPAWN_MODEL,
        .model_generation = effect.native_generation, .object.model = &effect,
        .member_count = 1, .members = { { &object, effect.native_generation } } };
    if (spawn_case == 1) ++fighter.x8_spawnNum;
    if (spawn_case == 2) ++effect.native_generation;
    if (spawn_case == 3) { result.kind = EF_SPAWN_GENERATOR; result.model_generation = 0; }
    if (spawn_case == 4) creator.hsd_obj = &replacement;
    if (spawn_case == 5) active = NULL;
    if (spawn_case == 6) heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = NULL;
    return result;
}

static void reset(void)
{
    memset(heads, 0, sizeof(heads));
    memset(&fighter, 0, sizeof(fighter));
    fighter.kind = Ft_Kind_CustomFirst;
    fighter.x8_spawnNum = 19;
    fighter.parts = parts;
    parts[0].joint = &origin;
    parts[1].joint = &replacement;
    creator = (HSD_GObj){ .user_data = &fighter, .hsd_obj = &origin };
    fighter.gobj = &creator;
    object = (HSD_GObj){ .classifier = HSD_GOBJ_CLASS_EFFECT,
        .user_data_kind = 8, .user_data = &effect,
        .user_data_remove_func = efLib_remove_user_data };
    effect = (EF_Effect){ .gobj = &object, .parent_gobj = &creator,
        .attach_jobj = &origin, .native_generation = 7,
        .native_attachment_part = -1 };
    heads[HSD_GOBJ_PLINK_FIGHTER] = &creator;
    heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = &object;
    HSD_GObjPLinkHead = heads;
    active = &definition;
    free_calls = spawn_calls = 0;
    spawn_case = 0;
}

int main(void)
{
    reset();
    uint32_t generation;
    assert(nativeModelEffect_Validate(&definition));
    assert(!nativeModelEffect_Validate(NULL));
    NativeFighterDefinition invalid = definition;
    invalid.model_effect_count = 33;
    assert(!nativeModelEffect_Validate(&invalid));
    invalid = definition; invalid.model_effects = NULL;
    assert(!nativeModelEffect_Validate(&invalid));
    NativeFighterModelEffect bad[2] = { policy[0], policy[1] };
    invalid = definition; invalid.model_effects = bad;
    bad[1].name_hash = 1;
    assert(!nativeModelEffect_Validate(&invalid));
    bad[1] = policy[1]; bad[0].gfx_id = 0x479;
    assert(!nativeModelEffect_Validate(&invalid));
    bad[0] = policy[0]; bad[1].update = NULL;
    assert(!nativeModelEffect_Validate(&invalid));

    assert(nativeCallbackSource_Snapshot(&object, NativeCallbackSource_ModelEffect, &generation));
    assert(generation == 7);
    assert(nativeCallbackSource_Alive(&object, generation, NativeCallbackSource_ModelEffect));
    assert(!nativeCallbackSource_Alive(&object, generation + 1, NativeCallbackSource_ModelEffect));
    assert(nativeModelEffect_Creator(&effect) == NULL);
    nativeModelEffect_Spawn(&creator, &policy[0], -1, NULL, false);
    assert(spawn_calls == 1 && free_calls == 0);
    assert(effect.update == authored_update && effect.native_author == active);
    assert(nativeModelEffect_Creator(&effect) == &creator);
    ++fighter.x8_spawnNum;
    assert(!nativeCallbackSource_Alive(&object, generation, NativeCallbackSource_ModelEffect));
    // An orphan cannot snapshot, but cleanup still resolves its own identity.
    assert(!nativeModelEffect_Snapshot(&object, &generation));
    nativeModelEffect_CancelGeneration(&object, 7);
    assert(effect.native_cancelled && effect.update == NULL && free_calls == 1);
    nativeModelEffect_Cancel(&effect);
    assert(free_calls == 1);

    for (unsigned link = 0; link < 2; ++link) {
        reset();
        heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = NULL;
        heads[HSD_GOBJ_PLINK_EFFECT_UNK0 + link] = &object;
        nativeModelEffect_Spawn(&creator, &policy[0], 0, NULL, false);
        assert(nativeModelEffect_Creator(&effect) == &creator);
        parts[0].joint = &replacement;
        assert(nativeModelEffect_Creator(&effect) == NULL);
        nativeModelEffect_Clear();
        assert(free_calls == 1);
    }

    reset();
    NativeFighterModelEffect foreign = policy[0];
    nativeModelEffect_Spawn(&creator, &foreign, -1, NULL, false);
    nativeModelEffect_Spawn(&creator, &policy[0], 2, NULL, false);
    nativeModelEffect_Spawn(&creator, &policy[0], -2, NULL, false);
    assert(spawn_calls == 0);
    object.classifier = 0;
    assert(nativeModelEffect_Resolve(&object, 7) == NULL);
    object.classifier = HSD_GOBJ_CLASS_EFFECT;
    object.user_data_remove_func = NULL;
    assert(nativeModelEffect_Resolve(&object, 7) == NULL);
    object.user_data_remove_func = efLib_remove_user_data;
    object.user_data_kind = 0;
    assert(nativeModelEffect_Resolve(&object, 7) == NULL);

    for (int scenario = 1; scenario <= 6; ++scenario) {
        reset(); spawn_case = scenario;
        nativeModelEffect_Spawn(&creator, &policy[0], -1, NULL, false);
        assert(spawn_calls == 1 && effect.native_policy == NULL);
        if (scenario == 1 || scenario == 4 || scenario == 5) assert(free_calls == 1);
        else assert(free_calls == 0); // Never cancel a generator or recycled result.
    }
    reset();
    nativeModelEffect_CancelGeneration(&object, 8);
    assert(free_calls == 0);
    effect.native_generation = 8;
    nativeModelEffect_CancelGeneration(&object, 7);
    assert(free_calls == 0);
    nativeModelEffect_Clear(); // Stock/unbound effect survives package cleanup.
    assert(free_calls == 0);
    HSD_GObjPLinkHead = NULL;
    nativeModelEffect_Cancel((EF_Effect*)(uintptr_t)1);
    assert(nativeModelEffect_Creator((EF_Effect*)(uintptr_t)1) == NULL);
    puts("Model effect registry, creator ownership, deferred cancellation and spawn lifetime passed.");
    return 0;
}
