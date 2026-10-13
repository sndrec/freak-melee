#include <melee/ef/native_model_effect.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efdata.h>
#include <sysdolphin/baselib/generator.h>
#include <melee/ef/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/inlines.h>
#include <melee/cm/camera.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/debug.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>

HSD_GObj** HSD_GObjPLinkHead;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj;
struct _unk_gobj_struct HSD_GObj_DelayedProcInfo;
u8 HSD_GObj_JObjKind = 3;
u32 efLib_EffectCount;
EF_ParamEntry efLib_ParamTable[8];
HSD_ObjAllocData efAsync_AllocData;
static HSD_GObj* heads[32];
static HSD_GObj owner, objects[2];
static EF_Effect effects[2];
static HSD_JObj origin, roots[2], other_origin;
static Fighter fighter;
static FighterBone parts[1];
static EF_QueuedEffect queue[4];
static bool used[4];
static unsigned frees, samples, spawns, animations[2], updates;
static bool partial, fail_alloc;
static Vec3 received;
static const NativeFighterDefinition* active;
static void update(EF_Effect* effect)
{ assert(effect == &effects[0]); ++updates; }
static const NativeFighterModelEffect policies[] = {
    {1, 0x4DA, update}, {2, 0x4CC, update}
};
static const NativeFighterDefinition definition = {
    .joint_count = 1, .model_effect_count = 2, .model_effects = policies
};
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == Ft_Kind_CustomFirst ? active : NULL; }
void __assert(char* file, u32 line, char* message)
{ fprintf(stderr, "%s:%lu %s\n", file, line, message); abort(); }
void OSReport(char* format, ...) { (void)format; abort(); }
void Camera_RequestQuake(CmQuakeKind kind, Vec* position)
{ (void)kind; (void)position; abort(); }
void HSD_JObjSetupMatrixSub(HSD_JObj* joint) { (void)joint; abort(); }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void HSD_MtxGetScale(Mtx matrix, Vec3* scale)
{ (void)matrix; (void)scale; abort(); }
void HSD_JObjAnimAll(HSD_JObj* joint)
{ assert(joint == &roots[0] || joint == &roots[1]); ++animations[joint == &roots[1]]; }
void lb_8000B1CC(HSD_JObj* joint, Vec3* offset, Vec3* position)
{ assert(joint == &origin && offset == NULL); ++samples; *position = joint->translate; }
void* HSD_ObjAlloc(HSD_ObjAllocData* allocator)
{
    assert(allocator == &efAsync_AllocData);
    if (fail_alloc) return NULL;
    for (unsigned i = 0; i < 4; ++i) if (!used[i]) { used[i] = true; return &queue[i]; }
    abort();
}
void HSD_ObjFree(HSD_ObjAllocData* allocator, void* memory)
{
    assert(allocator == &efAsync_AllocData);
    for (unsigned i = 0; i < 4; ++i) if (memory == &queue[i]) {
        assert(used[i]); used[i] = false; return;
    }
    abort();
}
void HSD_JObjWalkTree(HSD_JObj* joint, HSD_JObjWalkTreeCallback callback, f32** args)
{ (void)joint; (void)callback; (void)args; abort(); }
void hsd_8039D688(HSD_JObj* joint, f32** args, s32 mode)
{ (void)joint; (void)args; (void)mode; abort(); }
void efLib_remove_user_data(void* data) { nativeModelEffect_Remove(data); }
void HSD_GObjFree(HSD_GObj* object)
{
    assert(object == &objects[0] || object == &objects[1]);
    assert(object->user_data != NULL);
    // Exact HSD_GObjFree deferred-current boundary: no callback, unlink or count.
    if (!HSD_GObj_DelayedProcInfo.in_delayed_proc &&
        object == HSD_GObj_CurrentInvokedProcGObj) {
        HSD_GObj_DelayedProcInfo.delay_remove_gobj = 1;
        return;
    }
    ++frees;
    if (!((EF_Effect*)object->user_data)->is_async) --efLib_EffectCount;
    object->user_data_remove_func(object->user_data);
    HSD_GObj** cursor = &heads[HSD_GOBJ_PLINK_EFFECT_UNK0];
    while (*cursor && *cursor != object) cursor = &(*cursor)->next;
    assert(*cursor == object); *cursor = object->next;
    object->user_data = NULL;
}
EF_SpawnResult efSync_Spawn(s32 recipe, HSD_GObj* parent, ...)
{
    assert(parent == &owner); ++spawns;
    const EF_ModelRecipe* layout = efSync_ModelRecipe(recipe);
    assert(layout != NULL);
    va_list args; va_start(args, parent);
    if (layout->family == EF_MODEL_POSITION) received = *va_arg(args, Vec3*);
    else assert(va_arg(args, HSD_JObj*) == &origin);
    va_end(args);
    unsigned count = partial ? 1 : layout->model_count;
    for (unsigned i = 0; i < count; ++i) {
        effects[i].attach_jobj = layout->family == EF_MODEL_JOINT ? &origin : NULL;
        objects[i].next = heads[HSD_GOBJ_PLINK_EFFECT_UNK0];
        heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = &objects[i];
    }
    EF_SpawnResult result = { .kind = EF_SPAWN_MODEL, .object.model = &effects[0],
        .model_generation = effects[0].native_generation, .member_count = count };
    for (unsigned i = 0; i < count; ++i)
        result.members[i] = (EF_ModelMember){ &objects[i], effects[i].native_generation };
    return result;
}
#include "model_effect_group_fixture.h"

static void reset(void)
{
    memset(heads, 0, sizeof(heads)); memset(used, 0, sizeof(used));
    fighter = (Fighter){ .kind = Ft_Kind_CustomFirst, .x8_spawnNum = 0xFEDCBA98,
        .parts = parts, .gobj = &owner };
    owner = (HSD_GObj){ .user_data = &fighter, .hsd_obj = &origin };
    parts[0].joint = &origin;
    for (unsigned i = 0; i < 2; ++i) {
        roots[i] = (HSD_JObj){0};
        objects[i] = (HSD_GObj){ .classifier = HSD_GOBJ_CLASS_EFFECT,
            .user_data_kind = 8, .user_data_remove_func = efLib_remove_user_data,
            .user_data = &effects[i], .hsd_obj = &roots[i] };
        effects[i] = (EF_Effect){ .gobj = &objects[i], .parent_gobj = &owner,
            .native_generation = 0xCDEF0123 + i };
    }
    heads[HSD_GOBJ_PLINK_FIGHTER] = &owner;
    HSD_GObjPLinkHead = heads; HSD_GObj_CurrentInvokedProc = NULL;
    active = &definition;
    HSD_GObj_CurrentInvokedProcGObj = NULL;
    HSD_GObj_DelayedProcInfo.flags = 0; efLib_EffectCount = 2;
    frees = samples = spawns = animations[0] = animations[1] = updates = 0;
    partial = fail_alloc = false;
}
int main(void)
{
    reset();
    assert(efSync_ModelRecipe(0x4DA)->model_count == 2);
    assert(efSync_ModelRecipe(0x4CC)->family == EF_MODEL_POSITION);
    assert(efSync_ModelRecipe(0x4DB) == NULL);
    nativeModelEffect_Spawn(&owner, &policies[0], -1, NULL, false);
    assert(nativeModelEffect_Joint(&effects[0], 0) == &roots[0]);
    assert(nativeModelEffect_Joint(&effects[0], 1) == &roots[1]);
    assert(nativeModelEffect_Joint(&effects[0], 2) == NULL);
    efLib_Update(&objects[1]); efLib_Update(&objects[0]);
    assert(animations[0] == 1 && animations[1] == 1 && updates == 1);
    nativeModelEffect_Cancel(&effects[1]); assert(frees == 2);

    reset(); partial = true;
    nativeModelEffect_Spawn(&owner, &policies[0], -1, NULL, false);
    assert(frees == 1 && effects[0].native_policy == NULL);

    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        reset(); nativeModelEffect_Spawn(&owner, &policies[0], -1, NULL, false);
        if (scenario == 0) ++effects[1].native_generation;
        if (scenario == 1) heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = &objects[0];
        if (scenario == 2) effects[1].native_author = NULL;
        if (scenario == 3) effects[1].native_member_index = 0;
        if (scenario == 4) active = NULL;
        if (scenario == 5) effects[1].parent_gobj = &objects[0];
        if (scenario == 6) ++effects[1].native_creator_generation;
        if (scenario == 7) effects[1].native_attachment_part = 0;
        if (scenario == 8) effects[1].native_origin_joint = &other_origin;
        if (scenario == 9) effects[1].attach_jobj = &other_origin;
        assert(nativeModelEffect_Joint(&effects[0], 1) == NULL);
        assert(nativeModelEffect_Resolve(&objects[0], effects[0].native_generation) == NULL);
        nativeModelEffect_Cancel(&effects[0]);
        assert(frees == (scenario < 2 ? 1U : 2U));
    }
    reset(); nativeModelEffect_Spawn(&owner, &policies[0], -1, NULL, false);
    effects[1].lifetime = 1; efLib_Update(&objects[1]);
    assert(frees == 2 && animations[0] == 0 && animations[1] == 0);

    // Actual RemoveLast must bypass the unauthored current object and remove
    // the next one; a broken repeated-head loop hangs on this exact boundary.
    reset(); heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = &objects[0];
    objects[0].next = &objects[1]; efLib_EffectCount = 64;
    HSD_GObj_CurrentInvokedProcGObj = &objects[0];
    efLib_RemoveLast();
    assert(frees == 1 && objects[0].user_data == &effects[0]);
    assert(objects[1].user_data == NULL && efLib_EffectCount == 63);
    assert(!HSD_GObj_DelayedProcInfo.delay_remove_gobj);
    // In the delayed-proc phase, the same object can be freed synchronously.
    HSD_GObj_DelayedProcInfo.in_delayed_proc = 1; efLib_EffectCount = 64;
    efLib_RemoveLast(); assert(frees == 2 && efLib_EffectCount == 63);
    reset(); nativeModelEffect_Spawn(&owner, &policies[0], -1, NULL, false);
    efLib_EffectCount = 64; efLib_RemoveLast();
    assert(frees == 2 && efLib_EffectCount == 62 && heads[HSD_GOBJ_PLINK_EFFECT_UNK0] == NULL);

    static HSD_GObjProc proc = { .s_link = 8 };
    for (unsigned scenario = 0; scenario < 6; ++scenario) {
        reset(); HSD_GObj_CurrentInvokedProc = &proc;
        nativeModelEffect_Spawn(&owner, &policies[1], 0, NULL, true);
        assert(spawns == 0 && samples == 0 && fighter.x60C != NULL);
        origin.translate = (Vec3){ 7, 9, -3 };
        if (scenario == 1) ++fighter.x8_spawnNum;
        if (scenario == 2) active = NULL;
        if (scenario == 3) parts[0].joint = &other_origin;
        if (scenario == 4) heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
        if (scenario == 5) queue[0].native_policy = (void*)(uintptr_t)1;
        efAsync_QueueFlush(&owner, &fighter.x60C);
        assert(!used[0] && fighter.x60C == NULL);
        assert(spawns == (scenario == 0) && samples == (scenario == 0));
        if (scenario == 0) {
            assert(!memcmp(&received, &origin.translate, sizeof(Vec3)));
            assert(nativeModelEffect_Joint(&effects[0], 0) == &roots[0]);
            owner.hsd_obj = &other_origin;
            assert(nativeModelEffect_Joint(&effects[0], 0) == NULL);
            nativeModelEffect_Clear(); assert(frees == 1);
        }
    }
    reset(); fail_alloc = true;
    nativeModelEffect_Spawn(&owner, &policies[1], 0, NULL, true);
    assert(spawns == 0 && fighter.x60C == NULL);
    reset(); Vec3 position = { 2, 4, 6 };
    nativeModelEffect_Spawn(&owner, &policies[1], -1, &position, false);
    assert(spawns == 1 && !memcmp(&received, &position, sizeof(position)));
    nativeModelEffect_Clear(); assert(frees == 1);
    puts("Model groups: actual catalog, both animation phases, atomic failure, generation/provenance, deferred sampling and cleanup passed.");
    return 0;
}
