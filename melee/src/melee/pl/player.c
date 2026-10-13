#include "player.h"

#include <melee/ft/forward.h>

#include "forward.h"
#include "plattack.h"
#include "plbonus.h"
#include "plcommon_data.h"
#include "plstale.h"
#include "types.h"
#include "player_internal.h"
#include <dolphin/mtx.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0D4D.h>
#include <melee/ft/ftdata.h>
#ifdef MELEE_NATIVE
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_home.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/it/native_article_resource_scene.h>
#include <melee/it/native_article_resource_model.h>
#include <melee/it/native_article_resource_animation.h>
#endif
#include <melee/ft/ftdemo.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>
#include <melee/ft/types.h>
#include <melee/gm/gm_unsplit.h>
#include <melee/if/ifstatus.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbfile.h>
#include <melee/lb/lbheap.h>
#include <sysdolphin/baselib/debug.h>
#ifdef MELEE_NATIVE
#include <sysdolphin/baselib/dat.h>
#endif
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/objalloc.h>

//// .data
char str_PdPmdat_start_of_data[] = "PdPm.dat";
char str_plLoadCommonData[] = "plLoadCommonData";

void Player_80031790(int slot)
{
    StaticPlayer* player;
    int i;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if ((player->slot_type == Gm_PKind_Human) ||
        (player->slot_type == Gm_PKind_Cpu))
    {
        for (i = 0; i < 2; i++) {
            /// transformed will either be [1,0] (normal) or [0,1]
            /// (transformed) checks to see if the player is in a transformed
            /// state, and calls the function only once depending on the state
            if ((player->player_entity[player->transformed[i]])) {
                ftLib_800867E8(player->player_entity[player->transformed[i]]);
            }
        }
    }
}

void Player_80031848(int slot)
{
    StaticPlayer* player;
    int i;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if ((player->slot_type == Gm_PKind_Human) ||
        (player->slot_type == Gm_PKind_Cpu))
    {
        for (i = 0; i < 2; i++) {
            /// transformed will either be [1,0] (normal) or [0,1]
            /// (transformed) checks to see if the player is in a transformed
            /// state, and calls the function only once depending on the state
            if ((player->player_entity[player->transformed[i]])) {
                ftLib_8008688C(player->player_entity[player->transformed[i]]);
            }
        }
    }
}

static void func_8008688C_wrapper(StaticPlayer* player)
{
    if ((player->slot_type == Gm_PKind_Human) ||
        (player->slot_type == Gm_PKind_Cpu))
    {
        s32 i;
        for (i = 0; i < 2; i++) {
            if ((player->player_entity[player->transformed[i]])) {
                ftLib_8008688C(player->player_entity[player->transformed[i]]);
            }
        }
    }
}

void Player_80031900(void)
{
    int slot;
    for (slot = 0; slot < 6; slot++) {
        StaticPlayer* player = &player_slots[slot];
        Player_CheckSlot(slot);
        func_8008688C_wrapper(player);
    }
}


#ifdef MELEE_NATIVE
typedef struct PlayerConstructorActor {
    HSD_GObj* object;
    Fighter* fighter;
    NativeMatchPoolReference object_reference, fighter_reference;
    NativeFighterLifetime lifetime;
    uint32_t generation;
    bool retained;
} PlayerConstructorActor;

static bool player_constructor_selection(const StaticPlayer* player, CharacterKind character,
                                          const ftMapping* mapping)
{
    const ftMapping* current=&ftMapping_list[character];
    return player->player_character==character && current->internal_id==mapping->internal_id &&
        current->extra_internal_id==mapping->extra_internal_id && current->has_transformation==mapping->has_transformation &&
        player->transformed[0]<2 && player->transformed[1]<2 && player->transformed[0]!=player->transformed[1];
}

static bool player_constructor_capture(HSD_GObj* object, int slot, FighterKind kind,
                                       PlayerConstructorActor* actor)
{
    actor->object=object;
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&actor->object_reference) ||
       !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&actor->generation))return false;
    actor->fighter=GET_FIGHTER(object);
    if(!nativeMatchPool_Reference(&nativeFighterPool,actor->fighter,&actor->fighter_reference) ||
       actor->fighter->gobj!=object)return false;
    actor->retained=nativeFighterLifetime_Capture(actor->fighter,&actor->lifetime);
    return actor->retained && actor->fighter->player_id==slot && actor->fighter->kind==kind;
}

static bool player_constructor_current(const PlayerConstructorActor* actor, int slot)
{
    return actor->retained && nativeMatchPool_Resolve(actor->fighter_reference)==actor->fighter &&
        nativeFighterLifetime_Current(&actor->lifetime) && actor->fighter->player_id==slot;
}

static void player_constructor_unwind(int slot, int member, const PlayerConstructorActor* actor)
{
    // A faulted actor still owns its original physical allocation. A retired or
    // reused allocation, including a changed userdata owner, belongs to nobody here.
    if(!actor->object || nativeMatchPool_Resolve(actor->object_reference)!=actor->object ||
       nativeMatchPool_Resolve(actor->fighter_reference)!=actor->fighter ||
       actor->object->user_data!=actor->fighter || actor->fighter->gobj!=actor->object)return;
    // Clear before destructors: their callbacks may publish a replacement.
    Player_80031FB0(slot,member,actor->object);
    HSD_GObjFree(actor->object);
}
#endif

void Player_80031AD0(int slot)
{
    struct plAllocInfo first_struct;
    s32 internal_id;
    s32 has_transformation;
    s8 byte_check;

    u8 unused0[4];

    StaticPlayer* player;
    ftMapping* mapping;

    struct plAllocInfo second_struct;

    u8 unused1[12];

    Player_CheckSlot(slot);
    player = &player_slots[slot];
    mapping = &ftMapping_list[player->player_character];
#ifdef MELEE_NATIVE
    CharacterKind character=player->player_character;
    ftMapping selected_mapping=*mapping;
    PlayerConstructorActor primary={0},secondary={0};
    if(player->player_entity[0] || player->player_entity[1])return;
#endif
#ifdef MELEE_NATIVE
    // Choose only this frozen preview's initial active form. The package's
    // immutable member mapping and every other player's creation stay intact.
    ftMapping initial_mapping;
    if (nativeHome_WorkshopInitialMember((unsigned) slot) == 1) {
        initial_mapping = *mapping;
        initial_mapping.internal_id = mapping->extra_internal_id;
        initial_mapping.extra_internal_id = mapping->internal_id;
        mapping = &initial_mapping;
    }
#endif

    if (mapping->extra_internal_id != -1) {
        player->flags.b2 = true;
    }

    internal_id = mapping->internal_id;
    Player_CheckSlot(slot);

    first_struct.internal_id = internal_id;
    first_struct.slot = slot;
    first_struct.b0 = false;
    first_struct.has_transformation = false;
    first_struct.x5 = -1;

#ifdef MELEE_NATIVE
    HSD_GObj* first=Fighter_Create(&first_struct);
    if(!player_constructor_capture(first,slot,first_struct.internal_id,&primary) ||
       !player_constructor_selection(player,character,&selected_mapping) ||
       (player->player_entity[0] && player->player_entity[0]!=first) ||
       player->player_entity[1])goto failed;
    player->player_entity[0]=first;
#else
    player->player_entity[0] = Fighter_Create(&first_struct);
#endif
    player->player_state = 2;

    internal_id = byte_check = mapping->extra_internal_id;

    if (byte_check != -1) {
        has_transformation = mapping->has_transformation;

        Player_CheckSlot(slot);

        second_struct.internal_id = internal_id;
        second_struct.slot = slot;
        second_struct.b0 = true;
        second_struct.has_transformation = has_transformation;
        second_struct.x5 = -1;

#ifdef MELEE_NATIVE
        HSD_GObj* second=Fighter_Create(&second_struct);
        if(!player_constructor_capture(second,slot,second_struct.internal_id,&secondary) ||
           !player_constructor_current(&primary,slot) || !player_constructor_selection(player,character,&selected_mapping) ||
           player->player_entity[0]!=primary.object ||
           (player->player_entity[1] && player->player_entity[1]!=second))goto failed;
        player->player_entity[1]=second;
#else
        player->player_entity[1] = Fighter_Create(&second_struct);
#endif

        if (player->player_state != 1) {
            player->player_state = 2;
        }
    }

    if (player->player_state == 2) {
        if (player->struct_func != NULL) {
            player->struct_func(slot);
        }
    }
#ifdef MELEE_NATIVE
    if(!player_constructor_current(&primary,slot) ||
       (byte_check!=-1 && !player_constructor_current(&secondary,slot)) ||
       !player_constructor_selection(player,character,&selected_mapping) ||
       !((player->player_entity[0]==primary.object || player->player_entity[1]==primary.object) &&
         (byte_check==-1 || player->player_entity[0]==secondary.object || player->player_entity[1]==secondary.object)) ||
       (player->player_entity[0] && player->player_entity[0]==player->player_entity[1]))goto failed;
    return;
failed:
    player_constructor_unwind(slot,1,&secondary);
    player_constructor_unwind(slot,0,&primary);
    if(!player->player_entity[0] && !player->player_entity[1])player->player_state=0;
#endif
}

void Player_80031CB0(CharacterKind kind, u8 color)
{
    if (ftMapping_list[kind].internal_id != -1) {
        ftData_800855C8(ftMapping_list[kind].internal_id, color);
    }

    if (hasExtraFighterId(&ftMapping_list[kind])) {
        ftData_800855C8(ftMapping_list[kind].extra_internal_id, color);
    }
}

void Player_80031D2C(CharacterKind kind, u8 color)
{
    if (ftMapping_list[kind].internal_id != -1) {
        ftData_8008578C(ftMapping_list[kind].internal_id, color);
    }

    if (hasExtraFighterId(&ftMapping_list[kind])) {
        ftData_8008578C(ftMapping_list[kind].extra_internal_id, color);
    }
}

void Player_80031DA8(s32 param_1, s32 param_2)
{
    ftKb_SpecialN_800EED50(param_1, param_2);
}

void Player_80031DC8(void func_arg(s32, s32))
{
    int slot;
    for (slot = 0; slot < 6; slot++) {
        Player_CheckSlot(slot);

        if (player_slots[slot].player_state) {
            func_arg(ftMapping_list[player_slots[slot].player_character]
                         .internal_id,
                     0);
            if (hasExtraFighterId(
                    &ftMapping_list[player_slots[slot].player_character]))
            {
                func_arg(ftMapping_list[player_slots[slot].player_character]
                             .extra_internal_id,
                         0);
            }
        }
    }
}

void Player_80031EBC(int slot)
{
    s32 i;
    Player_CheckSlot(slot);

    for (i = 0; i < 2; i++) {
        StaticPlayer* player = &player_slots[slot];
        if ((player->player_entity[player->transformed[i]])) {
            if (!ftLib_8008701C(player->player_entity[player->transformed[i]]))
            {
                ftCo_800D4F24(player->player_entity[player->transformed[i]],
                              1);
            }
            HSD_GObjFree(player->player_entity[player->transformed[i]]);
        }
    }
}


/// new match decomp.me/scratch/oHfiV  @todo
//// old match decomp.me/scratch/8otnq
void Player_80032070(int slot, bool bool_arg)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if (bool_arg == 0) {
        ftCo_800D4FF4(player->player_entity[player->transformed[0]]);

        if (player->flags.b2 &&
            !ftMapping_list[player->player_character].has_transformation &&
            ftLib_8008701C(player->player_entity[player->transformed[1]]))
        {
            ftCo_800D4FF4(player->player_entity[player->transformed[1]]);
        }

        ifStatus_802F6E1C(slot);
        return;
    }

    if (ftLib_800873CC(player->player_entity[player->transformed[0]])) {
        ftCo_800D4FF4(player->player_entity[player->transformed[1]]);
    }
}

bool Player_8003219C(int slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return ftLib_8008701C(player->player_entity[player->transformed[0]]);
}

bool Player_8003221C(int slot)
{
    StaticPlayer* player;

    if (Player_GetPlayerState(slot) == 2) {
        Player_CheckSlot(slot);
        player = &player_slots[slot];

        if (!ftLib_8008701C(player->player_entity[player->transformed[0]])) {
            return true;
        }
    }

    return false;
}


void Player_800328D4(int slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    int i;
    Vec3* player_vecs;
    Vec3* player_vecs2;

    Vec3* dst_vec;
    Vec3* dst_vec2;

    u8 _[4];

    Player_CheckSlot(slot);
    player = &player_slots[slot];
    Player_CheckSlot(slot);

    player_vecs = player->player_poses.byIndex;
    dst_vec = &player_vecs[player->transformed[0]];
    *dst_vec = *arg_vec;

    player_vecs2 = player->player_poses.byIndex;
    dst_vec2 = &player_vecs2[player->transformed[1]];
    *dst_vec2 = *arg_vec;

    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            ftLib_80087140(player->player_entity[player->transformed[i]]);
        }
    }
}

void Player_80032A04(int slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    int i;

    Player_CheckSlot(slot);

    for (i = 0; i < 2; i++) {
        player = &player_slots[slot];

        player->player_poses.byIndex[player->transformed[i]] = *arg_vec;

        if (player->player_entity[player->transformed[i]]) {
            ftLib_80086664(player->player_entity[player->transformed[i]],
                           arg_vec);
        }
    }
}

void Player_SetPlayerAndEntityFacingDirection(s32 slot, f32 facing_dir)
{
    StaticPlayer* player;
    int i;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    for (i = 0; i < 2; i++) {
        player->facing_direction = facing_dir;

        if (player->player_entity[player->transformed[i]]) {
            ftDemo_SetFacingDirection(
                player->player_entity[player->transformed[i]], facing_dir);
        }
    }
}

f32 Player_80032BB0(s32 slot)
{
    StaticPlayer* player;
    f32 temp;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    temp = ftLib_800869F8(player->player_entity[player->transformed[0]]);
    return temp / player->model_scale;
}

void Player_SetScale(s32 slot, f32 scale)
{
    StaticPlayer* player;
    s32 i;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->model_scale = scale;
    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            ftLib_SetScale(player->player_entity[player->transformed[i]],
                           scale);
        }
    }
}


void Player_SetPlayerAndEntityCpuLevel(int slot, int cpu_level)
{
    StaticPlayer* player;
    int i;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->cpu_level = cpu_level;

    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            ft_80087BEC(player->player_entity[player->transformed[i]],
                        cpu_level);
        }
    }
}


void Player_SetPlayerAndEntityCpuType(int slot, int cpu_type)
{
    StaticPlayer* player;
    int i;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->cpu_type = cpu_type;

    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            ft_80087BC0(player->player_entity[player->transformed[i]],
                        cpu_type);
        }
    }
}


s32 Player_80033BB8(int slot)
{
    return gm_8016C5C0(slot);
}


/// @remarks Mostly called by Ice Climbers code, must be because they have 2nd
///          entity


void Player_SetHUDDamage(s32 slot, s32 arg1)
{
    StaticPlayer* player;
    s32 i;
    Player_CheckSlot(slot);

    for (i = 0; i < 2; i++) {
        player = &player_slots[slot];
        player->staminas.byIndex[player->transformed[i]] = arg1;
        if (player->player_entity[player->transformed[i]]) {
            ftLib_800870F0(player->player_entity[player->transformed[i]],
                           arg1);
        }
    }
}


void Player_UpdateKOsBySlot(int slot, bool bool_arg, int other_slot)
{
    StaticPlayer* player;
    StaticPlayer* other_player;

    u8 _[16];

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if (!bool_arg) {
        unsigned kos = player->kos_by_player[other_slot];

        if (kos < U32_MAX) {
            player->kos_by_player[other_slot]++;
        }

        if (slot == other_slot) {
            ifStatus_802F6D10(other_slot);
        } else {
            if (gm_8016B168()) {
                Player_CheckSlot(slot);
                Player_CheckSlot(other_slot);
                other_player = &player_slots[other_slot];

                {
                    int player_team;
                    int other_player_team;
                    player_team = player->team;
                    other_player_team = other_player->team;
                    if (player_team == other_player_team) {
                        ifStatus_802F69C0(other_slot, slot);
                        return;
                    }
                }
            }

            ifStatus_802F6AF8(other_slot);
        }
    }
}


void Player_UpdateMatchFrameCount(int slot, bool condition)
{
    StaticPlayer* player;
    PAD_STACK(4);

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if ((condition == 0) && (player->match_frame_count + 0x10000) == 0xffff) {
        player->match_frame_count = gm_GetFrameCount();
    }
}


void Player_IncSuicideCount(s32 slot, s32 condition)
{
    StaticPlayer* player;
    u16 suicide_count;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if (condition == 0) {
        suicide_count = player->suicide_count;
        if (suicide_count < 0xffff) {
            player->suicide_count++;
            ifStatus_802F6C04(slot);
        }
    }
}


bool Player_80036058(s32 slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return ftLib_80086F4C(player->player_entity[player->transformed[0]]);
}

float Player_800360D8(s32 slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return ftLib_80086F80(player->player_entity[player->transformed[0]]);
}


FighterKind Player_80036394(s32 slot)
{
    StaticPlayer* player;
    HSD_GObj* entity;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    entity = player->player_entity[player->transformed[0]];

    if (entity) {
        return ftLib_GetKind(entity);
    }
    return -1;
}

s32 Player_80036428(s32 slot)
{
    StaticPlayer* player;
    HSD_GObj* entity;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    entity = player->player_entity[player->transformed[0]];

    if (entity) {
        return ftLib_80087300(entity);
    }
    return 6;
}


void Player_800366DC(s32 slot, s32 arg1)
{
    StaticPlayer* player;
    s32 i;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            ft_80087AC0(player->player_entity[player->transformed[i]], arg1);
        }
    }
}

void Player_80036790(s32 slot, f32 arg1)
{
    StaticPlayer* player;
    s32 i;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            ftLib_80086A4C(player->player_entity[player->transformed[i]],
                           arg1);
        }
    }
}

void Player_80036844(s32 slot, s32 arg1)
{
    StaticPlayer* player;
    s32 i;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            ft_80087BAC(player->player_entity[player->transformed[i]], arg1);
        }
    }
}

bool Player_800368F8(int slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return ftLib_80086BB4(player->player_entity[player->transformed[0]]);
}

void Player_80036978(s32 slot, Vec3* pos)
{
    StaticPlayer* player;

    u8 _[4];

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    ftLib_80086B90(player->player_entity[player->transformed[0]], pos);
}


void Player_80036CF0(s32 slot)
{
    Player_InitOrResetPlayer(slot);
    pl_8003891C(slot);
}

void Player_80036D24(s32 slot)
{
    Player_InitOrResetPlayer(slot);
    pl_80038F10(slot);
}

void Player_InitAllPlayers(void)
{
    int i;
    plStale_InitAttackInstance();
    plAttack_80037590();

    for (i = 0; i < Gm_Player_NumMax; i++) {
        Player_InitOrResetPlayer(i);
        pl_8003891C(i);
    }
}

void Player_80036DA4(void)
{
#ifdef MELEE_NATIVE
    if (!nativeFighterRuntime_Clear()) {
        HSD_ASSERTREPORT(0, false, "Source fighters still referenced at scene initialization");
        return;
    }
#endif
    HSD_ObjAllocInit(&Player_AllocData, 8, 4);
    Fighter_FirstInitialize_80067A84();
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0, nativeMatchLaunch_RegisterFighters(),
                     "Could not register selected native fighters");
    HSD_ASSERTREPORT(0, nativeArticleModelScene_Prepare(),
                     "Could not prepare selected secondary model scene storage");
    HSD_ASSERTREPORT(0, nativeArticleResourceScene_Prepare(),
                     "Could not prepare selected resource node storage");
    HSD_ASSERTREPORT(0, nativeArticleResourceModel_Prepare(),
                     "Could not prepare selected resource graph storage");
    HSD_ASSERTREPORT(0, nativeArticleResourceAnimation_Prepare(),
                     "Could not prepare selected resource animation ownership");
#endif
}

void Player_80036DD8(void)
{
#ifdef MELEE_NATIVE
    static pl_804D6470_t common_data;
    size_t length = lbFileGetSize(str_PdPmdat_start_of_data);
    void* bytes = lbHeap_80015BD0(0, (length + 31) & ~(size_t) 31);
    HSD_ASSERT(0, bytes != NULL);
    lbFile_8001668C(str_PdPmdat_start_of_data, bytes, &length);
    HSD_ASSERT(0, plCommonData_Import(bytes, length, &common_data));
    pl_804D6470 = &common_data;
#else
    void** sp8;

    lbArchive_LoadSymbols(str_PdPmdat_start_of_data, (void**) &sp8,
                          str_plLoadCommonData, 0);
    pl_804D6470 = *sp8;
#endif
}

void Player_80036E20(CharacterKind ckind, HSD_Archive* archive, s32 arg2)
{
    ftMapping* mapping = &ftMapping_list[ckind];
    ftDemo_SetArchiveData(mapping->internal_id, archive, arg2);
    if (mapping->extra_internal_id != -1 && !mapping->has_transformation)
    {
        ftDemo_SetArchiveData(mapping->extra_internal_id, archive, arg2);
    }
}

#ifdef MELEE_NATIVE
void Player_LoadResultMotion(CharacterKind ckind, const char* filename)
{
    ftMapping* mapping = &ftMapping_list[ckind];
    FighterKind kinds[2] = { mapping->internal_id,
                             mapping->extra_internal_id };
    size_t length = lbFileGetSize(filename);
    void* bytes = lbHeap_80015BD0(0, OSRoundUp32B(length));
    HSD_ASSERT(0, bytes != NULL);
    lbFile_8001668C(filename, bytes, &length);
    HSD_Dat dat;
    HSD_ASSERTREPORT(0, HSD_DatOpen(&dat, bytes, length) == HSD_DAT_OK,
                     "Invalid native result motion archive: %s", filename);
    for (u32 i = 0; i < 2; ++i) {
        FighterKind kind = kinds[i];
        if (kind < 0 || kind >= Ft_Kind_RuntimeCapacity ||
            (i != 0 && mapping->has_transformation))
        {
            continue;
        }
        ftData_UnkCountStruct* pair = &ftData_UnkIntPairs[kind];
        if (pair->data != NULL) {
            continue;
        }
        HSD_ASSERT(0, pair->count >= 10);
        Fighter_DemoStrings* names = ftData_803C2468[kind];
        u32 root;
        HSD_ASSERTREPORT(0,
            names != NULL && names->result_filename != NULL &&
                HSD_DatFindRoot(&dat, names->result_filename, &root),
            "Missing native result motion: %s kind=%d", filename, kind);
        ftData_8008572C(kind);
        void* arena = lbHeap_80015BD0(0, FT_ANIMATION_IMPORT_ARENA_SIZE);
        HSD_ASSERT(0, arena != NULL);
        FtAnimationBankStats stats;
        HSD_ASSERTREPORT(0,
            ftAnimationBank_Import(dat.data + root, dat.data_size - root,
                (const FtAnimationBankTable[]){{ gFtDataList[kind]->x14, 10, false }}, 1,
                arena, FT_ANIMATION_IMPORT_ARENA_SIZE, &stats),
            "Invalid native result motion: %s kind=%d clip=%lu status=%d",
            filename, kind, stats.failed_index, stats.status);
        pair->data = arena;
    }
}
#endif

HSD_JObj* Player_80036EA0(s32 slot)
{
    StaticPlayer* player;
    HSD_GObj* entity;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    entity = player->player_entity[player->transformed[0]];

    if (entity) {
        return ftLib_800865F0(entity);
    }

    return NULL;
}

void Player_CreateDemoFighters(s32 slot, s32 variant, FighterKind selected_kind)
{
    struct plAllocInfo2 some_struct;
    u8 _;
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];
    some_struct.internal_id =
        ftMapping_list[player->player_character].internal_id;
#ifdef MELEE_NATIVE
    // Results may finish in the alternate form. This is local creation order;
    // immutable package members and the global CSS mapping remain unchanged.
    const ftMapping* mapping = &ftMapping_list[player->player_character];
    if (player->player_character >= ChKind_CustomFirst &&
        mapping->has_transformation && selected_kind != Ft_Kind_None) {
        if ((selected_kind != mapping->internal_id &&
             selected_kind != mapping->extra_internal_id) ||
            nativeFighterRuntime_SourcePackage(selected_kind) == NULL) return;
        some_struct.internal_id = selected_kind;
    }
#else
    (void) selected_kind;
#endif
    some_struct.slot = slot;
    some_struct.has_transformation = 0;
    some_struct.unk8 = variant;
    some_struct.b0 = 0;

    player->slot_type = Gm_PKind_Demo;
    player->player_entity[0] = ftDemo_CreateFighter(&some_struct);
    if ((ftMapping_list[player->player_character].extra_internal_id != -1) &&
        (ftMapping_list[player->player_character].has_transformation == 0))
    {
        some_struct.internal_id =
            ftMapping_list[player->player_character].extra_internal_id;
        some_struct.has_transformation = 1;
        player->player_entity[1] = ftDemo_CreateFighter(&some_struct);
    }
}

void Player_80037054(s32 slot, s32 arg1)
{
    struct plAllocInfo2 some_struct;
    u8 _;
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];
    some_struct.internal_id =
        ftMapping_list[player->player_character].internal_id;
    some_struct.slot = slot;
    some_struct.has_transformation = 0;
    some_struct.unk8 = arg1;
    some_struct.b0 = 1;

    player->slot_type = Gm_PKind_Demo;
    player->player_entity[0] = ftDemo_CreateFighter(&some_struct);
    if ((ftMapping_list[player->player_character].extra_internal_id != -1) &&
        (ftMapping_list[player->player_character].has_transformation == 0))
    {
        some_struct.internal_id =
            ftMapping_list[player->player_character].extra_internal_id;
        some_struct.has_transformation = 1;
        player->player_entity[1] = ftDemo_CreateFighter(&some_struct);
    }
}
