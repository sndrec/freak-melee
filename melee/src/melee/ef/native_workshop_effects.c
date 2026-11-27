#include "native_workshop_effects.h"

#include "efasync.h"
#include "eflib.h"
#include "native_model_effect.h"
#include "types.h"

#include <melee/ft/fighter.h>
#include <melee/ft/ftaction.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_09F7.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/gm/native_home.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/dat.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_particle_limits.h>
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/random.h>

#include <math.h>
#include <string.h>

typedef struct PreviewSpawn {
    unsigned frame;
    union CmdUnion words[5];
} PreviewSpawn;

bool nativeWorkshopEffects_Advancing;
static struct {
    Fighter_GObj* actor;
    uint32_t generation;
    u32 seed;
    const NativeFighterProject* draft;
    NativeFighterAnimationClip clip;
    PreviewSpawn events[4096];
    size_t count, next;
    int frame;
    unsigned original_joint_cursor;
} preview;

bool nativeWorkshopEffects_Holds(const Fighter_GObj* actor)
{
    return actor != NULL && actor == preview.actor;
}

void nativeWorkshopEffects_Clear(void)
{
    if (preview.actor == NULL) return;
    // Cancellation can create retirement effects. Keep those in the preview
    // links too, so changing action or closing the editor removes them.
    u32* previous_seed = seed_ptr;
    seed_ptr = &preview.seed;
    nativeWorkshopEffects_Advancing = true;
    if (nativeCallbackSource_Alive(preview.actor, preview.generation,
                                   NativeCallbackSource_Fighter)) {
        Fighter* fighter = GET_FIGHTER(preview.actor);
        fighter->x2220_b0 = preview.original_joint_cursor;
        efAsync_QueueClear(&fighter->x60C);
        efLib_DestroyAll(preview.actor);
    }
    psClearLinks(NativeWorkshopEffect_Links);
    for (HSD_Generator* gen = hsd_804D78FC; gen != NULL;) {
        HSD_Generator* next = gen->next;
        if (NativeWorkshopEffect_Links & (1u << gen->linkNo))
            hsd_8039D4DC(gen);
        gen = next;
    }
    nativeWorkshopEffects_Advancing = false;
    seed_ptr = previous_seed;
    preview.actor = NULL;
}

static bool start(Fighter_GObj* actor, const NativeFighterProject* draft,
                  NativeFighterAnimationClip clip,
                  const NativeFighterActionOccurrence* events, size_t count)
{
    HSD_Dat dat;
    uint32_t generation;
    if (count > ARRAY_SIZE(preview.events) ||
        !nativeCallbackSource_Snapshot(actor, NativeCallbackSource_Fighter, &generation) ||
        HSD_DatOpen(&dat, draft->fighter_dat, draft->fighter_size) != HSD_DAT_OK)
        return false;
    preview.count = preview.next = 0;
    for (size_t i = 0; i < count; ++i) {
        if (events[i].opcode != 10) continue;
        PreviewSpawn* spawn = &preview.events[preview.count];
        for (unsigned n = 0; n < ARRAY_SIZE(spawn->words); ++n) {
            u32 raw;
            if (!HSD_DatReadU32(&dat, events[i].source_offset + n * 4, &raw) ||
                (n == 0 && raw >> 26 != 10)) return false;
            const u32 word = ftActionData_NativeWord(10, n, raw);
            memcpy(&spawn->words[n], &word, sizeof(word));
        }
        spawn->frame = events[i].frame;
        ++preview.count;
    }
    preview.actor = actor;
    preview.generation = generation;
    preview.draft = draft;
    preview.clip = clip;
    preview.seed = 0x4d656c65;
    preview.original_joint_cursor = GET_FIGHTER(actor)->x2220_b0;
    preview.frame = -1;
    return true;
}

static bool bone_present(Fighter* fighter, const union CmdUnion* command)
{
    unsigned bone = command->spawn_gfx_0.useUnkBone ?
        fighter->ft_data->x8->x12 : command->spawn_gfx_0.boneId;
    if (bone == FT_EFFECT_ROTATING_JOINT) {
        if (fighter->ft_data->x54 == NULL || fighter->x2220_b0 >= 5) return false;
        bone = ((const int*) fighter->ft_data->x54)[fighter->x2220_b0];
    } else if (bone == FT_EFFECT_MODEL_JOINT) {
        bone = fighter->ft_data->x8->x10;
    } else if (command->spawn_gfx_0.useCommonBoneIDs) {
        if (bone >= FT_COMMON_PART_COUNT) return false;
        bone = ftParts_GetBoneIndex(fighter, bone);
    }
    return bone < ftPartsTable[fighter->kind]->parts_num &&
           fighter->parts[bone].joint != NULL;
}

static void step(Fighter_GObj* actor, unsigned frame)
{
    const unsigned links[] = { HSD_GOBJ_PLINK_EFFECT_UNK0, HSD_GOBJ_PLINK_EFFECT_UNK1 };
    // Record generations before callbacks that may retire effects. Newly
    // created models receive their first normal tick on the next frame.
    struct { HSD_GObj* object; u32 generation; } models[64];
    unsigned count = 0;
    for (unsigned n = 0; n < ARRAY_SIZE(links); ++n) {
        for (HSD_GObj* gobj = HSD_GObjPLinkHead[links[n]]; gobj; gobj = gobj->next) {
            const EF_Effect* effect = gobj->user_data;
            if (effect && effect->parent_gobj == actor && count < ARRAY_SIZE(models)) {
                models[count].object = gobj;
                models[count++].generation = effect->native_generation;
            }
        }
    }
    while (preview.next < preview.count && preview.events[preview.next].frame == frame) {
        PreviewSpawn* spawn = &preview.events[preview.next++];
        if (bone_present(GET_FIGHTER(actor), &spawn->words[0])) {
            CommandInfo command = { .u = spawn->words };
            ftAction_80071028(actor, &command);
        }
    }
    efAsync_QueueFlush(actor, &GET_FIGHTER(actor)->x60C);
    for (unsigned i = 0; i < count; ++i) {
        if (nativeModelEffect_Resolve(models[i].object, models[i].generation) != NULL)
            efLib_Update(models[i].object);
    }
    hsd_8039CEAC(~(u32)NativeWorkshopEffect_Mask);
    hsd_8039EE24(~(u32)NativeWorkshopEffect_Mask);
}

bool nativeWorkshopEffects_Seek(Fighter_GObj* actor,
    const NativeFighterProject* draft, NativeFighterAnimationClip clip,
    float pose, unsigned frame, const NativeFighterActionOccurrence* events,
    size_t count)
{
    if (actor == NULL || draft == NULL || clip.bank != NativeFighterAnimation_Action)
        return false;
    if (preview.actor != actor || preview.draft != draft ||
        preview.clip.index != clip.index ||
        (int)frame < preview.frame ||
        !nativeCallbackSource_Alive(actor, preview.generation, NativeCallbackSource_Fighter)) {
        nativeWorkshopEffects_Clear();
        if (!start(actor, draft, clip, events, count)) return false;
    }
    if (preview.count == 0 || (preview.next == 0 && frame < preview.events[0].frame)) {
        preview.frame = (int)frame;
        return true;
    }
    if (preview.frame == (int)frame) return true;
    if (preview.next == 0) preview.frame = (int)preview.events[0].frame - 1;
    u32* previous_seed = seed_ptr;
    const HSD_ParticleWork previous_work = particle_work_this_tick;
    seed_ptr = &preview.seed;
    nativeWorkshopEffects_Advancing = true;
    bool posed = true;
    while (preview.frame < (int)frame) {
        ++preview.frame;
        posed = nativeHome_WorkshopPose(clip, fminf(pose, preview.frame), draft);
        if (!posed) break;
        particle_work_this_tick = (HSD_ParticleWork){ 0 };
        step(actor, preview.frame);
    }
    const bool finished = posed && preview.actor == actor && preview.frame == (int)frame;
    nativeWorkshopEffects_Advancing = false;
    seed_ptr = previous_seed;
    particle_work_this_tick = previous_work;
    if (!finished) {
        nativeWorkshopEffects_Clear();
        return false;
    }
    return nativeHome_WorkshopPose(clip, pose, draft);
}
