#include "native_replay.h"
#include "native_semantic_state.h"
#include "native_replay_item_fields.h"
#include "native_match_launch.h"
#include "native_replay_resources.h"

#ifdef MELEE_NATIVE

#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/forward.h>
#include <melee/gm/gmvs.h>
#include <melee/gm/types.h>
#include <melee/gr/forward.h>
#include <melee/gr/types.h>
#include <melee/it/forward.h>
#include <melee/it/types.h>
#include <melee/lb/lb_00F9.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/random.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(float) == 4, "Replay expects 32-bit floats");

// Normalize the stock trail's private extra model and authored model slots
// into the same ordered graph view for stock/script parity traces.
HSD_JObj* nativeReplay_AdditionalItemModel(const Item* item, unsigned slot)
{
    if (!item || slot >= 3) return NULL;
    // Source items use native_models even when their physical kind is a stock
    // trail. Their union contains attachment identity, not foxillusion.xDDC.
    if (slot == 0 && item->native_script_owner == 0 &&
        (item->kind == It_Kind_Fox_Illusion ||
         item->kind == It_Kind_Falco_Phantasm))
        return item->xDD4_itemVar.foxillusion.xDDC;
    if (item->native_script_owner == 0 && slot < 2 &&
        (item->kind == It_Kind_Link_Boomerang ||
         item->kind == It_Kind_CLink_Boomerang))
        return item->xDD4_itemVar.linkboomerang.xF90[slot];
    if (item->native_script_owner == 0 && slot < 2 &&
        (item->kind == It_Kind_Link_Arrow ||
         item->kind == It_Kind_CLink_Arrow))
        return item->xDD4_itemVar.linkarrow.xB4[slot];
    return item->native_models[slot];
}

static bool nonfinite_reported;

enum {
    STATE_TRACE_FRAMES = 64,
    STATE_TRACE_SEGMENTS = 192,
    STATE_TRACE_ITEM_COMMANDS = 16,
};

typedef enum StateTraceKind {
    StateTrace_Controller,
    StateTrace_Camera,
    StateTrace_Player,
    StateTrace_Objects,
    StateTrace_GroundKind,
    StateTrace_GroundCore,
    StateTrace_GroundPose,
    StateTrace_Ground,
    StateTrace_ItemKind,
    StateTrace_ItemIdentity,
    StateTrace_ItemMotion,
    StateTrace_ItemAnimation,
    StateTrace_ItemHitboxState,
    StateTrace_ItemHitbox,
    StateTrace_ItemHurtboxCount,
    StateTrace_ItemHurtboxState,
    StateTrace_ItemHurtboxShape,
    StateTrace_ItemHurtboxSkip,
    StateTrace_ItemHurtboxPosition,
    StateTrace_ItemHurtbox,
    StateTrace_ItemHurtboxes,
    StateTrace_ItemHitboxes,
    StateTrace_ItemScalar,
    StateTrace_ItemDamage,
    StateTrace_ItemCommands,
    StateTrace_ItemCore,
    StateTrace_ItemRelations,
    StateTrace_ItemPose,
    StateTrace_Items,
    StateTrace_Fighter,
    StateTrace_Final,
} StateTraceKind;

typedef struct StateTraceSegment {
    uint64_t hash;
    uint8_t kind;
    uint8_t index;
} StateTraceSegment;

typedef struct StateTraceFrame {
    uint64_t sample;
    uint8_t count;
    uint8_t item_command_count;
    bool valid;
    StateTraceSegment segments[STATE_TRACE_SEGMENTS];
    struct {
        uint32_t kind;
        uint32_t words[8];
    } item_commands[STATE_TRACE_ITEM_COMMANDS];
} StateTraceFrame;

static StateTraceFrame state_traces[STATE_TRACE_FRAMES];
static bool trace_enabled;
static bool action_trace_enabled;
static bool fighter_trace_enabled;
bool nativeReplay_StateObserversEnabled(void)
{
    return trace_enabled || action_trace_enabled || fighter_trace_enabled;
}
static HSD_GObj* action_trace_entities[GM_MAX_PLAYERS * 2];
static int action_trace_motions[GM_MAX_PLAYERS * 2];
static float action_trace_damage[GM_MAX_PLAYERS * 2];

_Static_assert(GM_MAX_PLAYERS*2==NativeReplay_TraceActors,"Trace actor count changed");
void nativeReplay_CaptureActionTrace(NativeReplayActionTraceCheckpoint* checkpoint)
{
    memcpy(checkpoint->entities,action_trace_entities,sizeof(action_trace_entities));
    memcpy(checkpoint->motions,action_trace_motions,sizeof(action_trace_motions));
    memcpy(checkpoint->damage,action_trace_damage,sizeof(action_trace_damage));
}
void nativeReplay_RestoreActionTrace(const NativeReplayActionTraceCheckpoint* checkpoint)
{
    memcpy(action_trace_entities,checkpoint->entities,sizeof(action_trace_entities));
    memcpy(action_trace_motions,checkpoint->motions,sizeof(action_trace_motions));
    memcpy(action_trace_damage,checkpoint->damage,sizeof(action_trace_damage));
}

void nativeReplay_LogItemEvent(int kind, const char* event)
{
    if (fighter_trace_enabled) {
        fprintf(stderr, "[item-event] sample %llu kind=%d event=%s\n",
                (unsigned long long) nativeReplay_SampleCount(), kind,
                event);
    }
}

void nativeReplay_SetStateTraceEnabled(bool enabled)
{
    trace_enabled = enabled;
}

void nativeReplay_LogBodyContact(const Fighter* fighter)
{
    if (!fighter_trace_enabled || !fighter->x221C_b5 ||
        fighter->unk_gobj == NULL || fighter->unk_gobj->user_data == NULL) return;
    const Fighter* other = fighter->unk_gobj->user_data;
    fprintf(stderr,
            "[body-contact] sample %llu P%u motion=%d other=P%u other_motion=%d\n",
            (unsigned long long) nativeReplay_SampleCount(),
            fighter->player_id + 1, fighter->motion_id,
            other->player_id + 1, other->motion_id);
}

void nativeReplay_SetActionTraceEnabled(bool enabled)
{
    action_trace_enabled = enabled;
    memset(action_trace_entities, 0, sizeof(action_trace_entities));
    memset(action_trace_motions, 0, sizeof(action_trace_motions));
    memset(action_trace_damage, 0, sizeof(action_trace_damage));
}

void nativeReplay_SetFighterTraceEnabled(bool enabled)
{
    fighter_trace_enabled = enabled;
}

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

// Normalize only the storage location for stock-versus-script comparisons.
// Hash the real accessory joints/materials, not an inferred presence flag.
static uint64_t fighter_attachment_digest(const Fighter* fighter)
{
    uint64_t hash = nativeSemanticState_Begin();
    const HSD_JObj* stock = fighter->kind == Ft_Kind_Purin
        ? fighter->u.pr.x223C : NULL;
    if (stock != NULL) {
        hash = nativeSemanticState_AddJointTree(hash, stock);
        hash = nativeSemanticState_AddJointVisuals(hash, stock);
    }
    for (unsigned slot = 0; slot < NATIVE_FIGHTER_ATTACHMENTS; ++slot) {
        const HSD_JObj* joint = fighter->native_attachments[slot].joint;
        if (joint == NULL) continue;
        hash = nativeSemanticState_AddJointTree(hash, joint);
        hash = nativeSemanticState_AddJointVisuals(hash, joint);
    }
    return hash;
}

static void trace_fighter_actions(uint64_t sample_index)
{
    if (!action_trace_enabled && !fighter_trace_enabled) {
        return;
    }
    if (sample_index == nativeMatchLaunch_TraceGeometryFrame()) {
        nativeReplay_LogGeometry(sample_index);
    }
    if (fighter_trace_enabled) {
        fprintf(stderr, "[rng] sample %llu seed=%08x\n",
                (unsigned long long) sample_index, (unsigned) *seed_ptr);
    }
    for (unsigned actor = 0; actor < GM_MAX_PLAYERS * 2; ++actor) {
        unsigned slot = actor / 2;
        unsigned role = actor % 2;
        HSD_GObj* entity = Player_GetEntityAtIndex(slot, role);
        Fighter* fighter = entity != NULL ? GET_FIGHTER(entity) : NULL;
        if (fighter == NULL) {
            action_trace_entities[actor] = NULL;
            continue;
        }
        if (fighter_trace_enabled) {
            uint64_t visibility = nativeSemanticState_AddU32(
                nativeSemanticState_Begin(), fighter->x5AC.model_num);
            visibility = nativeSemanticState_AddU32(visibility, fighter->x221D_b2);
            for (unsigned i = 0; i < sizeof(fighter->x5F4_arr) / sizeof(fighter->x5F4_arr[0]); ++i) {
                visibility = nativeSemanticState_AddU32(visibility, (uint8_t) fighter->x5F4_arr[i].prev);
                visibility = nativeSemanticState_AddU32(visibility, (uint8_t) fighter->x5F4_arr[i].idx);
            }
            fprintf(stderr,
                    "[%s] sample %llu P%u motion=%d frame=%08x "
                    "ga=%d cmd=%lu,%lu,%lu stick=%08x,%08x "
                    "buttons=%08x,%08x angle=%08x "
                    "facing=%08x vel=%08x,%08x,%08x gr=%08x "
                    "hitlag=%08x speed=%08x scale=%08x "
                    "pos=%08x,%08x,%08x offset=%08x,%08x "
                    "pose=%016llx hitbox=%016llx hurtbox=%016llx attachment=%016llx "
                    "wind=%d visibility=%016llx\n",
                    role ? "companion" : "fighter",
                    (unsigned long long) sample_index, slot + 1,
                    fighter->motion_id, float_bits(fighter->cur_anim_frame),
                    fighter->ground_or_air, fighter->cmd_vars[0],
                    fighter->cmd_vars[1], fighter->cmd_vars[2],
                    float_bits(fighter->input.lstick[0].x),
                    float_bits(fighter->input.lstick[0].y),
                    (unsigned) fighter->input.pressed_buttons,
                    (unsigned) fighter->input.held_buttons[0],
                    float_bits(fighter->lstick_angle),
                    float_bits(fighter->facing_dir),
                    float_bits(fighter->self_vel.x),
                    float_bits(fighter->self_vel.y),
                    float_bits(fighter->self_vel.z),
                    float_bits(fighter->gr_vel),
                    float_bits(fighter->dmg.x195c_hitlag_frames),
                    float_bits(fighter->frame_speed_mul),
                    float_bits(fighter->x34_scale.y),
                    float_bits(fighter->cur_pos.x),
                    float_bits(fighter->cur_pos.y),
                    float_bits(fighter->cur_pos.z),
                    float_bits(fighter->x6A4_transNOffset.y),
                    float_bits(fighter->x6A4_transNOffset.z),
                    (unsigned long long) nativeSemanticState_AddJointTree(
                        nativeSemanticState_Begin(), entity->hsd_obj),
                    (unsigned long long) nativeSemanticState_FighterHitboxDigest(
                        fighter),
                    (unsigned long long) nativeSemanticState_FighterHurtboxDigest(
                        fighter),
                    (unsigned long long) fighter_attachment_digest(fighter),
                    lb_80011ABC(), (unsigned long long) visibility);
        }
        if (!action_trace_enabled) continue;
        float damage = fighter->dmg.x1830_percent;
        if (action_trace_entities[actor] != entity) {
            action_trace_damage[actor] = damage;
        } else if (action_trace_damage[actor] != damage) {
            fprintf(stderr, "[%s] sample %llu P%u %.2f -> %.2f\n",
                    role ? "companion-damage" : "damage",
                    (unsigned long long) sample_index, slot + 1,
                    action_trace_damage[actor], damage);
            action_trace_damage[actor] = damage;
        }
        if (action_trace_entities[actor] != entity ||
            action_trace_motions[actor] != fighter->motion_id) {
            fprintf(stderr,
                    "[%s] sample %llu P%u kind=%d behavior=%d "
                    "motion=%d anim=%d frame=%.2f\n",
                    role ? "companion-action" : "action",
                    (unsigned long long) sample_index, slot + 1,
                    fighter->kind, fighter->behavior_kind,
                    fighter->motion_id, fighter->anim_id,
                    fighter->cur_anim_frame);
            action_trace_entities[actor] = entity;
            action_trace_motions[actor] = fighter->motion_id;
        }
    }
    if (fighter_trace_enabled) {
        NativeSemanticObjectMap objects;
        nativeSemanticState_BuildObjectMap(
            &objects, HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER],
            HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM],
            HSD_GObjPLinkHead[HSD_GOBJ_PLINK_GROUND]);
        unsigned index = 0;
        for (HSD_GObj* entity = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
             entity != NULL; entity = entity->next) {
            Item* item = entity->user_data;
            if (item == NULL) continue;
            int owner_port = 0;
            for (int slot = 0; slot < GM_MAX_PLAYERS; ++slot) {
                if (item->owner != NULL &&
                    item->owner == Player_GetEntity(slot)) {
                    owner_port = slot + 1;
                    break;
                }
            }
            NativeSemanticItemTrace item_fields;
            uint64_t item_digest = nativeSemanticState_AddItem(
                nativeSemanticState_Begin(), item, &item_fields);
            uint64_t relation_digest = nativeSemanticState_AddItemRelations(
                nativeSemanticState_Begin(), item, &objects);
            uint64_t pose_digest = entity->obj_kind == HSD_GObj_JObjKind
                ? nativeSemanticState_AddJointTree(nativeSemanticState_Begin(),
                                                   entity->hsd_obj)
                : 0;
            uint64_t visual_digest = entity->obj_kind == HSD_GObj_JObjKind
                ? nativeSemanticState_AddJointVisuals(nativeSemanticState_Begin(),
                                                      entity->hsd_obj)
                : 0;
            for (unsigned slot = 0; slot < 3; ++slot) {
                HSD_JObj* model = nativeReplay_AdditionalItemModel(item, slot);
                if (model == NULL) continue;
                pose_digest = nativeSemanticState_AddJointTree(pose_digest, model);
                visual_digest = nativeSemanticState_AddJointVisuals(visual_digest, model);
            }
            fprintf(stderr,
                    "[item] sample %llu I%u kind=%d state=%d "
                    "core=%016llx relations=%016llx pose=%016llx "
                    "visual=%016llx owner=P%d pos=(%.2f,%.2f) "
                    "vel=(%.2f,%.2f)\n",
                    (unsigned long long) sample_index, index++, item->kind,
                    item->msid, (unsigned long long) item_digest,
                    (unsigned long long) relation_digest,
                    (unsigned long long) pose_digest,
                    (unsigned long long) visual_digest, owner_port, item->pos.x,
                    item->pos.y, item->x40_vel.x, item->x40_vel.y);
            fprintf(stderr,
                    "[item-fields] sample %llu I%u identity=%016llx "
                    "motion=%016llx animation=%016llx hitboxes=%016llx "
                    "hurtboxes=%016llx damage=%016llx commands=%016llx\n",
                    (unsigned long long) sample_index, index - 1,
                    (unsigned long long) item_fields.identity,
                    (unsigned long long) item_fields.motion,
                    (unsigned long long) item_fields.animation,
                    (unsigned long long) item_fields.hitboxes,
                    (unsigned long long) item_fields.hurtboxes,
                    (unsigned long long) item_fields.damage,
                    (unsigned long long) item_fields.commands);
            uint32_t command_flags;
            memcpy(&command_flags, &item->xDC8_word, sizeof(command_flags));
            fprintf(stderr,
                    "[item-commands] sample %llu I%u %08x %08x %08x %08x "
                    "%08x %08x %08x %08x\n",
                    (unsigned long long) sample_index, index - 1,
                    item->xDAC_itcmd_var0, item->xDB0_itcmd_var1,
                    item->xDB4_itcmd_var2, item->xDB8_itcmd_var3,
                    item->xDBC_itcmd_var4_word, item->xDC0, item->xDC4,
                    command_flags);
            nativeReplayItemFields_Trace(sample_index,index-1,item,&objects,&item_fields);
            nativeReplayResources_Trace(sample_index, index - 1, entity);
        }
    }
}

static void trace_segment(StateTraceFrame* frame, StateTraceKind kind,
                          uint8_t index, uint64_t hash)
{
    if (frame != NULL && frame->count < STATE_TRACE_SEGMENTS) {
        frame->segments[frame->count++] = (StateTraceSegment){
            .hash = hash,
            .kind = kind,
            .index = index,
        };
    }
}

void nativeReplay_LogStateDigestSegments(uint64_t sample_index)
{
    static const char* labels[] = {
        "controller", "camera", "player", "objects",
        "ground-kind", "ground-core", "ground-pose",
        "ground",
        "item-kind", "item-identity", "item-motion", "item-animation",
        "item-hitbox-state", "item-hitbox", "item-hurtbox-count",
        "item-hurtbox-state", "item-hurtbox-shape",
        "item-hurtbox-skip", "item-hurtbox-position", "item-hurtbox",
        "item-hurtboxes",
        "item-hitboxes", "item-scalar", "item-damage", "item-commands", "item-core",
        "item-relations", "item-pose",
        "items", "fighter", "final",
    };
    const StateTraceFrame* frame =
        &state_traces[sample_index % STATE_TRACE_FRAMES];
    if (!frame->valid || frame->sample != sample_index) {
        return;
    }
    for (uint32_t i = 0; i < frame->count; ++i) {
        const StateTraceSegment* segment = &frame->segments[i];
        fprintf(stderr, "[state] frame %llu %s %u %016llx\n",
                (unsigned long long) sample_index, labels[segment->kind],
                segment->index, (unsigned long long) segment->hash);
    }
    for (uint32_t i = 0; i < frame->item_command_count; ++i) {
        const uint32_t* words = frame->item_commands[i].words;
        fprintf(stderr,
                "[state] frame %llu item-command-words %u kind=%u "
                "%08x %08x %08x %08x %08x %08x %08x %08x\n",
                (unsigned long long) sample_index, i,
                frame->item_commands[i].kind, words[0], words[1], words[2],
                words[3], words[4], words[5], words[6], words[7]);
    }
}

extern GameSceneInfo* gm_804D6720;

uint64_t nativeReplay_StateDigest(uint64_t sample_index)
{
    uint64_t hash = nativeSemanticState_Begin();
    // Fighter GObjs and their bones are owned by the VS scene. Its transition
    // can free the fighter parts pool while stale GObj links still exist;
    // the diagnostic must never inspect those links outside live gameplay.
    if (gm_804D6720 == NULL || gm_804D6720->scene_kind != GS_VS ||
        HSD_GObjPLinkHead == NULL) {
        if (action_trace_enabled) {
            memset(action_trace_entities, 0, sizeof(action_trace_entities));
        }
        return hash;
    }
    trace_fighter_actions(sample_index);
    StateTraceFrame* trace = NULL;
    if (trace_enabled) {
        trace = &state_traces[sample_index % STATE_TRACE_FRAMES];
        trace->sample = sample_index;
        trace->count = 0;
        trace->item_command_count = 0;
        trace->valid = false;
    }
    hash = nativeSemanticState_AddController(hash, gmVs_GetController_0());
    trace_segment(trace, StateTrace_Controller, 0, hash);
    hash = nativeSemanticState_AddCamera(hash, Camera_GetState(),
                                         cm_804D6468);
    trace_segment(trace, StateTrace_Camera, 0, hash);
    for (uint32_t slot = 0; slot < GM_MAX_PLAYERS; ++slot) {
        hash = nativeSemanticState_AddPlayer(hash, Player_GetPtrForSlot(slot));
        trace_segment(trace, StateTrace_Player, slot, hash);
    }

    NativeSemanticObjectMap objects;
    nativeSemanticState_BuildObjectMap(
        &objects, HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER],
        HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM],
        HSD_GObjPLinkHead[HSD_GOBJ_PLINK_GROUND]);
    hash = nativeSemanticState_AddObjectMap(hash, &objects);
    trace_segment(trace, StateTrace_Objects, 0, hash);

    uint32_t ground_count = 0;
    for (HSD_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_GROUND];
         gobj != NULL; gobj = gobj->next) {
        Ground* ground = gobj->user_data;
        if (ground == NULL) {
            continue;
        }
        ++ground_count;
        trace_segment(trace, StateTrace_GroundKind, ground_count - 1,
                      gobj->classifier);
        hash = nativeSemanticState_AddU32(hash, gobj->classifier);
        hash = nativeSemanticState_AddGround(hash, ground);
        trace_segment(trace, StateTrace_GroundCore, ground_count - 1, hash);
        if (gobj->obj_kind == HSD_GObj_JObjKind) {
            hash = nativeSemanticState_AddJointTree(hash, gobj->hsd_obj);
        }
        trace_segment(trace, StateTrace_GroundPose, ground_count - 1, hash);
    }
    hash = nativeSemanticState_AddU32(hash, ground_count);
    trace_segment(trace, StateTrace_Ground, 0, hash);

    uint32_t item_count = 0;
    for (HSD_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         gobj != NULL; gobj = gobj->next) {
        Item* item = gobj->user_data;
        if (item == NULL) {
            continue;
        }
        ++item_count;
        trace_segment(trace, StateTrace_ItemKind, item_count - 1,
                      (uint32_t) item->kind);
        hash = nativeSemanticState_AddU32(hash, gobj->classifier);
        NativeSemanticItemTrace item_trace;
        hash = nativeSemanticState_AddItem(hash, item,
                                           trace != NULL ? &item_trace : NULL);
        if (trace != NULL) {
            if (trace->item_command_count < STATE_TRACE_ITEM_COMMANDS) {
                uint32_t* words =
                    trace->item_commands[trace->item_command_count].words;
                trace->item_commands[trace->item_command_count].kind =
                    item->kind;
                words[0] = item->xDAC_itcmd_var0;
                words[1] = item->xDB0_itcmd_var1;
                words[2] = item->xDB4_itcmd_var2;
                words[3] = item->xDB8_itcmd_var3;
                words[4] = item->xDBC_itcmd_var4_word;
                words[5] = item->xDC0;
                words[6] = item->xDC4;
                memcpy(&words[7], &item->xDC8_word, sizeof(words[7]));
                ++trace->item_command_count;
            }
            trace_segment(trace, StateTrace_ItemIdentity, item_count - 1,
                          item_trace.identity);
            trace_segment(trace, StateTrace_ItemMotion, item_count - 1,
                          item_trace.motion);
            trace_segment(trace, StateTrace_ItemAnimation, item_count - 1,
                          item_trace.animation);
            for (uint8_t slot = 0; slot < 4; ++slot) {
                const uint8_t index = (item_count - 1) * 4 + slot;
                trace_segment(trace, StateTrace_ItemHitboxState, index,
                              item_trace.hitbox_state[slot]);
                trace_segment(trace, StateTrace_ItemHitbox, index,
                              item_trace.hitbox[slot]);
            }
            trace_segment(trace, StateTrace_ItemHurtboxCount,
                          item_count - 1, item_trace.hurtbox_count);
            uint8_t hurt_count = item_trace.hurtbox_count > 2
                                     ? 2 : item_trace.hurtbox_count;
            for (uint8_t slot = 0; slot < hurt_count; ++slot) {
                const uint8_t index = (item_count - 1) * 2 + slot;
                trace_segment(trace, StateTrace_ItemHurtboxState, index,
                              item_trace.hurtbox_state[slot]);
                trace_segment(trace, StateTrace_ItemHurtboxShape, index,
                              item_trace.hurtbox_shape[slot]);
                trace_segment(trace, StateTrace_ItemHurtboxSkip, index,
                              item_trace.hurtbox_skip[slot]);
                trace_segment(trace, StateTrace_ItemHurtboxPosition, index,
                              item_trace.hurtbox_position[slot]);
                trace_segment(trace, StateTrace_ItemHurtbox, index,
                              item_trace.hurtbox[slot]);
            }
            trace_segment(trace, StateTrace_ItemHurtboxes, item_count - 1,
                          item_trace.hurtboxes);
            trace_segment(trace, StateTrace_ItemHitboxes, item_count - 1,
                          item_trace.hitboxes);
            for (uint8_t field = 0;
                 field < NATIVE_SEMANTIC_ITEM_SCALAR_COUNT; ++field) {
                trace_segment(trace, StateTrace_ItemScalar,
                              (item_count - 1) * NATIVE_SEMANTIC_ITEM_SCALAR_COUNT +
                                  field,
                              item_trace.scalar[field]);
            }
            trace_segment(trace, StateTrace_ItemDamage, item_count - 1,
                          item_trace.damage);
            trace_segment(trace, StateTrace_ItemCommands, item_count - 1,
                          item_trace.commands);
        }
        trace_segment(trace, StateTrace_ItemCore, item_count - 1, hash);
        hash = nativeSemanticState_AddItemRelations(hash, item, &objects);
        trace_segment(trace, StateTrace_ItemRelations, item_count - 1, hash);
        if (gobj->obj_kind == HSD_GObj_JObjKind) {
            hash = nativeSemanticState_AddJointTree(hash, gobj->hsd_obj);
        }
        for (unsigned slot = 0; slot < 3; ++slot) {
            HSD_JObj* model = nativeReplay_AdditionalItemModel(item, slot);
            if (model != NULL) hash = nativeSemanticState_AddJointTree(hash, model);
        }
        trace_segment(trace, StateTrace_ItemPose, item_count - 1, hash);
    }
    hash = nativeSemanticState_AddU32(hash, item_count);
    trace_segment(trace, StateTrace_Items, 0, hash);

    uint32_t fighter_count = 0;
    for (HSD_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         gobj != NULL; gobj = gobj->next) {
        Fighter* fp = gobj->user_data;
        if (fp == NULL) {
            continue;
        }
        ++fighter_count;
        if (!nonfinite_reported &&
            (!isfinite(fp->cur_pos.x) || !isfinite(fp->cur_pos.y) ||
             !isfinite(fp->cur_pos.z) || !isfinite(fp->cur_anim_frame))) {
            fprintf(stderr,
                    "[replay] Nonfinite fighter position or animation at sample %llu, player %u, action %d.\n",
                    (unsigned long long) sample_index, fp->player_id,
                    fp->motion_id);
            nonfinite_reported = true;
        }

        if (fp->parts == NULL || ftPartsTable == NULL ||
            fp->kind < 0 || fp->kind >= Ft_Kind_StockCount ||
            ftPartsTable[fp->kind] == NULL) {
            hash = nativeSemanticState_AddFighter(hash, fp, 0);
            hash = nativeSemanticState_AddFighterRelations(hash, fp, &objects);
            trace_segment(trace, StateTrace_Fighter, fp->player_id, hash);
            continue;
        }
        uint32_t parts_num = ftPartsTable[fp->kind]->parts_num;
        hash = nativeSemanticState_AddFighter(hash, fp, parts_num);
        hash = nativeSemanticState_AddFighterRelations(hash, fp, &objects);
        trace_segment(trace, StateTrace_Fighter, fp->player_id, hash);
        if (parts_num > 256) {
            fprintf(stderr, "[replay] Invalid part count %u for fighter %u.\n",
                    parts_num, fp->player_id);
            continue;
        }
        for (uint32_t bone = 0; bone < parts_num; ++bone) {
            HSD_JObj* joint = fp->parts[bone].joint;
            if (joint == NULL) {
                continue;
            }
            bool quaternion = (joint->flags & JOBJ_USE_QUATERNION) != 0;
            if (!nonfinite_reported &&
                (!isfinite(joint->rotate.x) || !isfinite(joint->rotate.y) ||
                 !isfinite(joint->rotate.z) ||
                 (quaternion && !isfinite(joint->rotate.w)) ||
                 !isfinite(joint->translate.x) ||
                 !isfinite(joint->translate.y) ||
                 !isfinite(joint->translate.z) ||
                 !isfinite(joint->scale.x) || !isfinite(joint->scale.y) ||
                 !isfinite(joint->scale.z))) {
                fprintf(stderr,
                        "[replay] Nonfinite bone pose at sample %llu, player %u, action %d, bone %u.\n",
                        (unsigned long long) sample_index, fp->player_id,
                        fp->motion_id, bone);
                nonfinite_reported = true;
            }
        }
    }
    hash = nativeSemanticState_AddU32(hash, fighter_count);
    trace_segment(trace, StateTrace_Final, 0, hash);
    if (trace != NULL) {
        trace->valid = true;
    }
    return hash;
}

#endif
