#include "native_workshop_practice.h"
#include "gm_1601.h"
#include "native_confirmed_checkpoint.h"
#include "native_match_state_admission.h"
#include "native_practice.h"
#include <melee/ft/fighter.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/it/inlines.h>
#include <melee/it/item.h>
#include <melee/lb/lb_0195.h>
#include <melee/mn/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/audio_rollback.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <sysdolphin/baselib/random.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ArenaCount = 2, SnapshotCount = 2, ScratchBytes = 64 * 1024 * 1024 };
typedef struct Checkpoint {
    NativeMatchArenaFrame spans[ArenaCount];
} Checkpoint;

static struct {
    NativeWorkshopPracticeConfig config;
    NativeWorkshopPracticeState state;
    NativeWorkshopPracticeTrace trace[NativeWorkshopPractice_TraceFrames];
    NativeWorkshopPracticeHit pending_hits[NativeWorkshopPractice_TraceHits];
    NativeWorkshopPracticeFault pending_faults[NativeWorkshopPractice_TraceFaults];
    unsigned hit_count, fault_count;
    PADStatus pads[NativeWorkshopPractice_MaxRun][4];
    Checkpoint checkpoint[SnapshotCount];
    uint8_t* backing;
    uint32_t begin_frame;
    NativeWorkshopPracticeRequest request;
    bool armed, entering, active;
} practice;

static void fail(const char* error)
{
    practice.state.status = NativeWorkshopPractice_Failed;
    ++practice.state.revision;
    snprintf(practice.state.error, sizeof(practice.state.error), "%s", error);
}

bool nativeWorkshopPractice_Arm(const NativeWorkshopPracticeConfig* config)
{
    if (!config || practice.armed || practice.entering || practice.active) return false;
    const uint32_t id = practice.state.id + 1;
    practice.config = *config;
    practice.state = (NativeWorkshopPracticeState){
        .status = NativeWorkshopPractice_Opening, .id = id, .revision = 1, .seed = config->seed,
    };
    memcpy(practice.state.fighter_revision, config->fighter_revision, sizeof(practice.state.fighter_revision));
    practice.armed = true;
    practice.hit_count = practice.fault_count = 0;
    return true;
}

int nativeWorkshopPractice_StockCharacter(unsigned fighter_id)
{
    for (int character = 0; character < CKind_Playable_Count; ++character)
        if (Player_800325C8(character, false) == (int) fighter_id) return character;
    return -1;
}

void nativeWorkshopPractice_CancelStart(const char* error)
{
    if (!practice.armed && !practice.entering) return;
    practice.armed = practice.entering = false;
    fail(error);
}

bool nativeWorkshopPractice_Packages(NativeContentId packages[4])
{
    if (!practice.armed) return false;
    practice.state.package = packages[0];
    for (unsigned port = 1; port < NativeWorkshopPractice_Ports; ++port)
        packages[port] = practice.config.seats[port].package;
    return true;
}

void nativeWorkshopPractice_Configure(StartMeleeData* start, const uint8_t kinds[4])
{
    for (unsigned port = 0; port < NativeWorkshopPractice_Ports; ++port) {
        const NativeWorkshopPracticeSeat* seat = &practice.config.seats[port];
        PlayerInitData* player = &start->players[port];
        gm_SetupPlayerDefaults(player);
        if (!seat->present) continue;
        player->slot_type = seat->cpu_level ? Gm_PKind_Cpu : Gm_PKind_Human;
        player->cpu_level = seat->cpu_level;
        player->ckind = kinds[port] >= ChKind_CustomFirst ? kinds[port] : seat->character;
        player->slot = port + 1;
        player->spawn_pos = seat->spawn;
        player->spawn_dir = seat->facing;
        player->damage = seat->damage;
        player->xC_b1 = false;
        player->rumble_enabled = false;
    }
}

void nativeWorkshopPractice_EnterScene(void)
{
    if (!practice.armed) return;
    practice.armed = false;
    practice.entering = true;
    *seed_ptr = practice.config.seed;
    HSD_PadFlushQueue(HSD_PAD_FLUSH_QUEUE_THROWAWAY);
    memset(HSD_PadGameStatus, 0, sizeof(HSD_PadGameStatus));
    memset(HSD_PadCopyStatus, 0, sizeof(HSD_PadCopyStatus));
    memset(HSD_PadMasterStatus, 0, sizeof(HSD_PadMasterStatus));
}

void nativeWorkshopPractice_Dummy(StartMeleeData* start)
{
    if (!practice.armed) return;
    PlayerInitData* sandbag = &start->players[NATIVE_PRACTICE_SANDBAG_SLOT];
    if (practice.config.sandbag) sandbag->damage = practice.config.sandbag_damage;
    else gm_SetupPlayerDefaults(sandbag);
}

static int fighter_index(const Fighter* fighter)
{
    return fighter ? fighter->player_id * 2 + fighter->x221F_b4 : -1;
}

static int item_owner(const Item* item)
{
    return item->owner && item->owner->p_link == HSD_GOBJ_PLINK_FIGHTER ?
        fighter_index(GET_FIGHTER(item->owner)) : -1;
}

void nativeWorkshopPractice_ItemHit(const Item* item, const Fighter* victim, float damage)
{
    if (!nativeWorkshopPractice_Stepping()) return;
    const unsigned index = practice.hit_count++;
    if (index >= NativeWorkshopPractice_TraceHits) return;
    practice.pending_hits[index] = (NativeWorkshopPracticeHit){
        .item_kind = item->kind, .attacker = item_owner(item),
        .victim = fighter_index(victim), .damage = damage,
    };
}

void nativeWorkshopPractice_ScriptFault(const char* source_kind, unsigned generation,
    unsigned line, unsigned column, const char* message)
{
    if (!practice.entering && !nativeWorkshopPractice_Stepping()) return;
    const unsigned index = practice.fault_count++;
    if (index >= NativeWorkshopPractice_TraceFaults) return;
    NativeWorkshopPracticeFault* fault = &practice.pending_faults[index];
    *fault = (NativeWorkshopPracticeFault){
        .source_kind = source_kind, .generation = generation, .line = line, .column = column,
    };
    snprintf(fault->message, sizeof(fault->message), "%s", message);
}

static void sample(void)
{
    const uint32_t frame = practice.state.frame;
    NativeWorkshopPracticeTrace* row = &practice.trace[frame % NativeWorkshopPractice_TraceFrames];
    *row = (NativeWorkshopPracticeTrace){ .frame = frame, .rng = *seed_ptr };
    row->hit_count = practice.hit_count;
    row->fault_count = practice.fault_count;
    memcpy(row->hits, practice.pending_hits, sizeof(row->hits));
    memcpy(row->faults, practice.pending_faults, sizeof(row->faults));
    practice.hit_count = practice.fault_count = 0;
    for (unsigned i = 0; i < NativeWorkshopPractice_Fighters; ++i) {
        Fighter_GObj* entity = Player_GetEntityAtIndex(i / 2, i % 2);
        if (!entity) continue;
        const Fighter* fighter = GET_FIGHTER(entity);
        NativeWorkshopPracticeFighter* actor = &row->fighters[i];
        *actor = (NativeWorkshopPracticeFighter){
            .present = true, .kind = fighter->kind, .action = fighter->motion_id,
            .animation = fighter->anim_id,
            .animation_frame = fighter->cur_anim_frame,
            .damage = fighter->dmg.x1830_percent, .x = fighter->cur_pos.x, .y = fighter->cur_pos.y,
            .vx = fighter->self_vel.x, .vy = fighter->self_vel.y,
            .facing = fighter->facing_dir, .airborne = fighter->ground_or_air != GA_Ground,
            .hitlag = fighter->dmg.x195c_hitlag_frames,
            .script_state = nativeScriptFighter_StateFingerprint(entity),
        };
        for (unsigned slot = 0; slot < 4; ++slot)
            if (fighter->x914[slot].state != HitCapsule_Disabled) actor->hitboxes |= 1 << slot;
    }
    for (Item_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]; gobj; gobj = gobj->next) {
        const unsigned index = row->items++;
        const Item* item = GET_ITEM(gobj);
        if (!item) continue;
        if (item->owner) ++row->projectiles;
        if (index < NativeWorkshopPractice_TraceItems)
            row->item_rows[index] = (NativeWorkshopPracticeItem){
                .kind = item->kind, .action = item->msid, .owner = item_owner(item),
                .x = item->pos.x, .y = item->pos.y, .vx = item->x40_vel.x, .vy = item->x40_vel.y,
            };
    }
    if (frame - practice.state.oldest_frame >= NativeWorkshopPractice_TraceFrames)
        practice.state.oldest_frame = frame - NativeWorkshopPractice_TraceFrames + 1;
}

static void filter_inputs(PADStatus pads[4])
{
    memcpy(pads, practice.pads[practice.state.frame - practice.begin_frame], sizeof(practice.pads[0]));
}

static bool transfer(Checkpoint* slot, bool save)
{
    for (unsigned i = 0; i < ArenaCount; ++i) {
        if (save ? !nativeMatchArena_Save(&slot->spans[i]) : !nativeMatchArena_Load(&slot->spans[i]))
            return false;
    }
    return true;
}

static bool bind_history(void)
{
    NativeMatchArena* owners[] = { &nativeHsdArena, &nativeMatchSceneArena };
    if ((nativeHsdArena.sealed && !nativeHsdArena_ResetHistory()) ||
        (nativeMatchSceneArena.sealed && !nativeMatchScene_ResetHistory())) return false;
    void* scratch = malloc(ScratchBytes);
    bool admitted = scratch && nativeMatchState_Admit(scratch, ScratchBytes);
    free(scratch);
    size_t bytes = admitted ? nativeConfirmedCheckpoint_Bytes(owners, ArenaCount) : 0;
    practice.backing = bytes && bytes <= SIZE_MAX / SnapshotCount ? malloc(bytes * SnapshotCount) : NULL;
    if (!practice.backing) return false;
    for (unsigned slot = 0; slot < SnapshotCount; ++slot) {
        uint8_t* cursor = practice.backing + slot * bytes;
        for (unsigned i = 0; i < ArenaCount; ++i) {
            const size_t capacity = owners[i]->capacity + owners[i]->root_bytes;
            if (!nativeMatchArena_BindFrame(owners[i], &practice.checkpoint[slot].spans[i], cursor, capacity))
                return false;
            cursor += capacity;
        }
    }
    return transfer(&practice.checkpoint[0], true);
}

void nativeWorkshopPractice_BeginLoop(void)
{
    if (!practice.entering) return;
    practice.entering = false;
    practice.active = true;
    HSD_PadSetRawFilter(filter_inputs);
    lbInput_SetGameSpeed(1.f);
    lbInput_RebaseClock(0);
    if (!bind_history()) {
        fail("The practice scene could not admit its gameplay snapshot owners.");
        return;
    }
    sample();
    practice.state.status = NativeWorkshopPractice_Ready;
    ++practice.state.revision;
}

void nativeWorkshopPractice_Boundary(void)
{
    if (practice.state.status != NativeWorkshopPractice_CheckpointPending) return;
    const bool save = practice.request == NativeWorkshopPractice_Save;
    const bool reset = practice.request == NativeWorkshopPractice_Reset;
    if (!transfer(&practice.checkpoint[reset ? 0 : 1], save)) {
        fail("Could not transfer the practice checkpoint.");
        return;
    }
    if (save) {
        practice.state.checkpoint_frame = practice.state.frame;
        practice.state.has_checkpoint = true;
    } else {
        practice.state.frame = reset ? 0 : practice.state.checkpoint_frame;
        practice.state.target_frame = practice.state.frame;
        practice.state.oldest_frame = practice.state.frame;
        HSD_AudioRollbackReset();
        sample();
    }
    practice.state.status = NativeWorkshopPractice_Ready;
    ++practice.state.revision;
}

void nativeWorkshopPractice_EndFrame(void)
{
    if (!nativeWorkshopPractice_Stepping()) return;
    ++practice.state.frame;
    sample();
    if (practice.state.frame == practice.state.target_frame) {
        practice.state.status = NativeWorkshopPractice_Ready;
        ++practice.state.revision;
    }
}

void nativeWorkshopPractice_EndLoop(void)
{
    if (!practice.active) return;
    practice.active = false;
    HSD_PadSetRawFilter(NULL);
    free(practice.backing);
    practice.backing = NULL;
    memset(practice.checkpoint, 0, sizeof(practice.checkpoint));
    practice.state.has_checkpoint = false;
    practice.state.status = NativeWorkshopPractice_Closed;
    ++practice.state.revision;
}

bool nativeWorkshopPractice_Controlled(void) { return practice.active; }
bool nativeWorkshopPractice_Stepping(void)
{
    return practice.active && practice.state.status == NativeWorkshopPractice_Running;
}
const NativeWorkshopPracticeState* nativeWorkshopPractice_State(void) { return &practice.state; }

const NativeWorkshopPracticeTrace* nativeWorkshopPractice_Trace(uint32_t frame)
{
    if (!practice.active || practice.state.status == NativeWorkshopPractice_Failed ||
        frame < practice.state.oldest_frame || frame > practice.state.frame) return NULL;
    return &practice.trace[frame % NativeWorkshopPractice_TraceFrames];
}

bool nativeWorkshopPractice_Run(const PADStatus (*pads)[4], unsigned frames)
{
    if (!practice.active || practice.state.status != NativeWorkshopPractice_Ready ||
        !frames || frames > NativeWorkshopPractice_MaxRun || frames > UINT32_MAX - practice.state.frame)
        return false;
    memcpy(practice.pads, pads, frames * sizeof(practice.pads[0]));
    practice.begin_frame = practice.state.frame;
    practice.state.target_frame = practice.state.frame + frames;
    practice.state.status = NativeWorkshopPractice_Running;
    ++practice.state.revision;
    return true;
}

bool nativeWorkshopPractice_Checkpoint(NativeWorkshopPracticeRequest request)
{
    if (!practice.active || practice.state.status != NativeWorkshopPractice_Ready ||
        (request == NativeWorkshopPractice_Restore && !practice.state.has_checkpoint)) return false;
    practice.request = request;
    practice.state.status = NativeWorkshopPractice_CheckpointPending;
    ++practice.state.revision;
    return true;
}
