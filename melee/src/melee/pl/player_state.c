#include "player_internal.h"
#include <sysdolphin/baselib/objalloc.h>

// Actual player records and family mapping; no alternate representation.
ftMapping ftMapping_list[ChKind_RuntimeCapacity] = {
    //////ftMapping_list
    /* CKind_Captain   */ { Ft_Kind_Captain, 0xFF },
    /* CKind_Donkey    */ { Ft_Kind_Donkey, 0xFF },
    /* CKind_Fox       */ { Ft_Kind_Fox, 0xFF },
    /* CKind_GameWatch */ { Ft_Kind_GameWatch, 0xFF },
    /* CKind_Kirby     */ { Ft_Kind_Kirby, 0xFF },
    /* CKind_Koopa     */ { Ft_Kind_Koopa, 0xFF },
    /* CKind_Link      */ { Ft_Kind_Link, 0xFF },
    /* CKind_Luigi     */ { Ft_Kind_Luigi, 0xFF },
    /* CKind_Mario     */ { Ft_Kind_Mario, 0xFF },
    /* CKind_Mars      */ { Ft_Kind_Mars, 0xFF },
    /* CKind_Mewtwo    */ { Ft_Kind_Mewtwo, 0xFF },
    /* CKind_Ness      */ { Ft_Kind_Ness, 0xFF },
    /* CKind_Peach     */ { Ft_Kind_Peach, 0xFF },
    /* CKind_Pikachu   */ { Ft_Kind_Pikachu, 0xFF },
    /* CKind_PopoNana  */ { Ft_Kind_Popo, Ft_Kind_Nana },
    /* CKind_Purin     */ { Ft_Kind_Purin, 0xFF },
    /* CKind_Samus     */ { Ft_Kind_Samus, 0xFF },
    /* CKind_Yoshi     */ { Ft_Kind_Yoshi, 0xFF },
    /* CKind_Zelda     */ { Ft_Kind_Zelda, Ft_Kind_Seak, true },
    /* CKind_Seak      */ { Ft_Kind_Seak, Ft_Kind_Zelda, true },
    /* CKind_Falco     */ { Ft_Kind_Falco, 0xFF },
    /* CKind_CLink     */ { Ft_Kind_CLink, 0xFF },
    /* CKind_DrMario   */ { Ft_Kind_DrMario, 0xFF },
    /* CKind_Emblem    */ { Ft_Kind_Emblem, 0xFF },
    /* CKind_Pichu     */ { Ft_Kind_Pichu, 0xFF },
    /* CKind_Ganon     */ { Ft_Kind_Ganon, 0xFF },
    /* CKind_MasterH   */ { Ft_Kind_MasterH, 0xFF },
    /* CKind_Boy       */ { Ft_Kind_Boy, 0xFF },
    /* CKind_Girl      */ { Ft_Kind_Girl, 0xFF },
    /* CKind_GKoops    */ { Ft_Kind_GKoops, 0xFF },
    /* CKind_CrezyH    */ { Ft_Kind_CrezyH, 0xFF },
    /* ChKind_Sandbag  */ { Ft_Kind_Sandbag, 0xFF },
    /* ChKind_Popo     */ { Ft_Kind_Popo, 0xFF }
};

StaticPlayer player_slots[Gm_Player_NumMax];
HSD_ObjAllocData Player_AllocData;
pl_804D6470_t* pl_804D6470;

void Player_RegisterNativeCharacter(CharacterKind character, FighterKind fighter,
                                    FighterKind partner, bool alternate)
{
    if (character >= ChKind_CustomFirst &&
        character < ChKind_RuntimeCapacity &&
        fighter >= Ft_Kind_CustomFirst && fighter < Ft_Kind_RuntimeCapacity &&
        (partner == Ft_Kind_None ? !alternate :
         partner >= Ft_Kind_CustomFirst && partner < Ft_Kind_RuntimeCapacity && partner != fighter)) {
        ftMapping_list[character] = (ftMapping){ (s8) fighter,
            partner == Ft_Kind_None ? -1 : (s8) partner, alternate };
    }
}

void Player_ClearNativeCharacters(void)
{
    for (int character = ChKind_CustomFirst;
         character < ChKind_RuntimeCapacity; ++character) {
        ftMapping_list[character] = (ftMapping){ -1, -1, false };
    }
}

StaticPlayer* Player_GetPtrForSlot(int slot)
{
    Player_CheckSlot(slot);
    return &player_slots[slot];
}

bool Player_HasAlternateMember(s32 slot)
{
    if ((unsigned)slot >= Gm_Player_NumMax) return false;
    CharacterKind character = player_slots[slot].player_character;
    if ((unsigned)character >= ChKind_RuntimeCapacity) return false;
    const ftMapping* mapping = &ftMapping_list[character];
    return mapping->has_transformation && mapping->internal_id >= 0 &&
           mapping->extra_internal_id >= 0 &&
           mapping->internal_id != mapping->extra_internal_id;
}

bool Player_800319C4(int slot, bool arg1)
{
    u8 _[4];

    int i;
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = (&player_slots[slot]);

    if (arg1) {
        for (i = 0; i < 2; i++) {
            if (!player->player_entity[player->transformed[i]]) {
                return false;
            }
        }

        return true;
    }

    for (i = 0; i < 2; i++) {
        if (player->player_entity[player->transformed[i]]) {
            return false;
        }
    }

    return true;
}

void Player_80031FB0(int slot, s32 entity_index, HSD_GObj* expected_object)
{
    u8 _[4];

    StaticPlayer* player;
    Player_CheckSlot(slot);

    player = &player_slots[slot];
    if(!expected_object || entity_index<0 || entity_index>1)return;
    bool cleared=false;
    for(unsigned i=0;i<2;++i) {
        unsigned physical=player->transformed[(entity_index+i)&1];
        if(physical<2 && player->player_entity[physical]==expected_object) {
            player->player_entity[physical]=NULL;
            cleared=true;
        }
    }
    // A replacement in either physical role keeps the slot active, including
    // single-member slots. Role permutation does not change that ownership.
    if (cleared && !player->player_entity[0] && !player->player_entity[1]) {
        player->player_state = 0;
    }
}

s32 Player_GetPlayerState(s32 slot)
{
    s32 state;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    state = player->player_state;
    return state;
}

CharacterKind Player_GetPlayerCharacter(int slot)
{
    u8 _[4];

    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return player->player_character;
}

void Player_SetPlayerCharacter(s32 slot, CharacterKind value)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->player_character = value;
}

Gm_PKind Player_GetPlayerSlotType(s32 slot)
{
    Gm_PKind slot_type;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    slot_type = player->slot_type;
    return slot_type;
}

Gm_PKind Player_8003248C(s32 slot, bool arg1)
{
    Gm_PKind slot_type;
    StaticPlayer* player;

    Player_CheckSlot(slot);

    player = &player_slots[slot];

    if (arg1 == 1) {
        if (!ftMapping_list[player->player_character].has_transformation) {
            if (player->slot_type == Gm_PKind_Human ||
                player->slot_type == Gm_PKind_Cpu)
            {
                return 1;
            }
        }
    }
    slot_type = player->slot_type;

    return slot_type;
}

void Player_SetSlottype(s32 slot, Gm_PKind value)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->slot_type = value;
}

s8 Player_800325C8(CharacterKind kind, bool b)
{
    if (!b) {
        return ftMapping_list[kind].internal_id;
    } else if (b == 1) {
        return ftMapping_list[kind].extra_internal_id;
    }

    return -1;
}

s8 Player_80032610(s32 slot, bool arg1)
{ //// decomp.me/scratch/pHTx2
    StaticPlayer* player;
    s32 error_value = -1;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if (arg1 == 0) {
        return ftMapping_list[player->player_character].internal_id;
    }
    if (arg1 == 1) {
        return ftMapping_list[player->player_character].extra_internal_id;
    }

    return error_value;
}

void Player_LoadPlayerCoords(s32 slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    Vec3* player_vecs;

    Player_CheckSlot(slot);

    player = &player_slots[slot];
    player_vecs = player->player_poses.byIndex;
    *arg_vec = player_vecs[player->transformed[0]];
}

void Player_80032768(s32 slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    Vec3* player_vecs;

    Vec3* dst_vec;
    Vec3* dst_vec2;

    Player_CheckSlot(slot);

    player = &player_slots[slot];

    player_vecs = player->player_poses.byIndex;
    dst_vec = &player_vecs[player->transformed[0]];
    *dst_vec = *arg_vec;

    player_vecs = player->player_poses.byIndex;
    dst_vec2 = &player_vecs[player->transformed[1]];
    *dst_vec2 = *arg_vec;
}

void Player_80032828(s32 slot, s32 index, Vec3* arg_vec)
{
    StaticPlayer* player;
    Vec3* player_vecs;

    Player_CheckSlot(slot);

    player = &player_slots[slot];

    player_vecs = player->player_poses.byIndex;
    player_vecs[player->transformed[index]] = *arg_vec;
}

void Player_GetSpawnPlatformPos(s32 slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    *arg_vec = player->player_poses.byVecName.spawn_platform_final_pos;
}

void Player_SetSpawnPlatformPos(s32 slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->player_poses.byVecName.spawn_platform_final_pos = *arg_vec;
}

void Player_GetSomePos(s32 slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    *arg_vec = player->player_poses.byVecName.some_other_player_pos;
}

void Player_SetSomePos(s32 slot, Vec3* arg_vec)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->player_poses.byVecName.some_other_player_pos = *arg_vec;
}

s32 Player_80032F30(s32 slot)
{
    StaticPlayer* player;
    s32 value;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    value = player->unk4E;
    return value;
}

void Player_80032FA4(s32 slot, s32 arg)
{
    StaticPlayer* player;

    u8 _[4];

    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->unk4E = arg;
}

f32 Player_GetFacingDirection(s32 slot)
{
    u8 _[4];
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return player->facing_direction;
}

void Player_SetFacingDirection(s32 slot, f32 direction)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->facing_direction = direction;
}

void Player_SetFacingDirectionConditional(s32 slot, bool b, f32 direction)
{
    u8 _[4];

    if (!b) {
        StaticPlayer* player;
        Player_CheckSlot(slot);
        player = &player_slots[slot];
        player->facing_direction = direction;
    }
}

u32 Player_GetCostumeId(int slot)
{
    u8 costume_id;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    costume_id = player->costume_id;
    return costume_id;
}

void Player_SetCostumeId(int slot, int costume_id)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->costume_id = costume_id;
}

u8 Player_GetControllerIndex(int slot)
{
    s8 controller_index;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    controller_index = player->controller_index;
    return controller_index;
}

void Player_SetControllerIndex(int slot, s8 controller_index)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->controller_index = controller_index;
}

int Player_GetTeam(int slot)
{
    int team;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    team = player->team;
    return team;
}

void Player_SetTeam(int slot, s8 team)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->team = team;
}

int Player_GetPlayerId(int slot)
{
    u8 player_id;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player_id = player->player_id;
    return player_id;
}

void Player_SetPlayerId(int slot, int player_id)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->player_id = player_id;
}

int Player_GetCpuLevel(int slot)
{
    int cpu_level;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    cpu_level = player->cpu_level;
    return cpu_level;
}

int Player_GetCpuType(int slot)
{
    int cpu_type;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    cpu_type = player->cpu_type;
    return cpu_type;
}

int Player_GetHandicap(int slot)
{
    u8 handicap;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    handicap = player->handicap;
    return handicap;
}

void Player_SetHandicap(int slot, s8 handicap)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->handicap = handicap;
}

f32 Player_GetUnk50(int slot)
{
    f32 unk50;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    unk50 = player->unk50;
    return unk50;
}

f32 Player_GetAttackRatio(int slot)
{
    f32 attack_ratio;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    attack_ratio = player->attack_ratio;
    return attack_ratio;
}

void Player_SetAttackRatio(int slot, f32 attack_ratio)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->attack_ratio = attack_ratio;
}

f32 Player_GetDefenseRatio(int slot)
{
    f32 defense_ratio;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    defense_ratio = player->defense_ratio;
    return defense_ratio;
}

void Player_SetDefenseRatio(int slot, f32 defense_ratio)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->defense_ratio = defense_ratio;
}

f32 Player_GetModelScale(int slot)
{
    f32 model_scale;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    model_scale = player->model_scale;
    return model_scale;
}

void Player_SetModelScale(int slot, f32 model_scale)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->model_scale = model_scale;
}

s32 Player_GetStocks(int slot)
{
    s8 stocks;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    stocks = player->stocks;
    return stocks;
}

int Player_GetP1Stock(void)
{
    return player_slots[0].stocks;
}

void Player_SetStocks(int slot, int stocks)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->stocks = stocks;
}

void Player_LoseStock(int slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if (player->stocks > 0) {
        player->stocks = player->stocks - 1;
    }
}

int Player_GetCoins(int slot)
{
    int current_coins;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    current_coins = player->current_coins;
    return current_coins;
}

void Player_SetCoins(int slot, int current_coins)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->current_coins = current_coins;
}

int Player_GetTotalCoins(int slot)
{
    int total_coins;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    total_coins = player->total_coins;
    return total_coins;
}

void Player_SetTotalCoins(int slot, int coins)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->total_coins = coins;
}

s32 Player_GetUnk98(s32 slot)
{
    s32 unk98;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    unk98 = player->unk98;
    return unk98;
}

void Player_SetUnk98(s32 slot, s32 unk98)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->unk98 = unk98;
}

s32 Player_GetUnk9C(s32 slot)
{
    s32 unk9C;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    unk9C = player->unk9C;
    return unk9C;
}

void Player_SetUnk9C(s32 slot, s32 unk9C)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->unk9C = unk9C;
}

HSD_GObj* Player_GetEntity(s32 slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return player->player_entity[player->transformed[0]];
}

HSD_GObj* Player_GetEntityAtIndex(int slot, int index)
{
    StaticPlayer* player;
    int entity_index;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    entity_index = player->transformed[index];
    return player->player_entity[entity_index];
}

void Player_SwapTransformedStates(s32 slot, s32 arg1, s32 arg2)
{
    u8 temp_r5;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    temp_r5 = player->transformed[arg1];
    player->transformed[arg1] = player->transformed[arg2];
    player->transformed[arg2] = temp_r5;
}

s32 Player_GetDamage(s32 slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return player->staminas.byIndex[player->transformed[0]];
}

void Player_SetHPByIndex(s32 slot, s32 arg1, s32 arg2)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);

    player = &player_slots[slot];
    player->staminas.byIndex[player->transformed[arg1]] = arg2;
}

int Player_GetOtherStamina(s32 slot)
{
    s16 stamina;
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    stamina = player->staminas.byName.stamina;
    return stamina;
}

void Player_SetOtherStamina(s32 slot, s32 stamina)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->staminas.byName.stamina = stamina;
}

int Player_GetRemainingHP(s32 slot)
{
    StaticPlayer* player;
    s32 result;
    u32 result2;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    result = player->staminas.byName.stamina -
             player->staminas.byIndex[player->transformed[0]];
    result2 = result;
    if (result < 0) {
        result2 = 0;
    }
    return result2;
}

bool Player_GetMoreFlagsBit2(s32 slot)
{
    StaticPlayer* player;
    u8 bit2;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    bit2 = player->more_flags.b2;
    return bit2;
}

void Player_SetMoreFlagsBit2(s32 slot, u8 bit2)
{
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    player->more_flags.b2 = bit2;
}

bool Player_GetMoreFlagsBit3(s32 slot)
{
    StaticPlayer* player;
    s32 bit3;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    bit3 = player->more_flags.b3;
    return bit3;
}

void Player_SetMoreFlagsBit3(s32 slot, u8 bit3)
{
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    player->more_flags.b3 = bit3;
}

void Player_SetMoreFlagsBit4(s32 slot, u8 bit4)
{
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    player->more_flags.b4 = bit4;
}

bool Player_GetMoreFlagsBit4(s32 slot)
{
    StaticPlayer* player;
    u8 bit4;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    bit4 = player->more_flags.b4;
    return bit4;
}

bool Player_GetMoreFlagsBit5(s32 slot)
{
    StaticPlayer* player;
    u8 bit5;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    bit5 = player->more_flags.b5;
    return bit5;
}

void Player_SetMoreFlagsBit5(s32 slot, u8 bit5)
{
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    player->more_flags.b5 = bit5;
}

bool Player_GetMoreFlagsBit6(s32 slot)
{
    StaticPlayer* player;
    u8 bit6;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    bit6 = player->more_flags.b6;
    return bit6;
}

void Player_SetMoreFlagsBit6(s32 slot, u8 bit6)
{
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    player->more_flags.b6 = bit6;
}

u8 Player_GetFlagsAEBit0(s32 slot)
{
    StaticPlayer* player;
    u8 bit0;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    bit0 = player->flagsAE.b0;
    return bit0;
}

void Player_SetFlagsAEBit0(s32 slot, u8 bit0)
{
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    player->flagsAE.b0 = bit0;
}

s32 Player_GetRemainingHPByIndex(s32 slot, s32 index)
{
    StaticPlayer* player;
    u32 stamina;
    s32 result;
    s32 result2;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    stamina = player->staminas.byName.stamina;
    result = stamina - player->staminas.byIndex[player->transformed[index]];
    result2 = result;
    if (result < 0) {
        result2 = 0;
    }
    return result2;
}

s32 Player_GetFalls(s32 slot)
{ /// decomp.me/scratch/8ijor
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if (ftMapping_list[player->player_character].extra_internal_id != -1 &&
        ftMapping_list[player->player_character].has_transformation)
    {
        return player->falls[player->transformed[0]] +
               player->falls[player->transformed[1]];
    }

    return player->falls[player->transformed[0]];
}

s32 Player_GetFallsByIndex(s32 slot, s32 arg1)
{
    StaticPlayer* player;
    s32 falls;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    falls = player->falls[player->transformed[arg1]];
    return falls;
}

void Player_SetFalls(int slot, s32 falls)
{
    u8 _[4];

    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->falls[player->transformed[0]] = falls;
}

void Player_SetFallsByIndex(int slot, enum_t index, s32 falls)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->falls[player->transformed[index]] = falls;
}

s32 Player_GetKOsByPlayerIndex(int slot, int idx)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    return player->kos_by_player[idx];
}

u32 Player_GetMatchFrameCount(int slot)
{
    StaticPlayer* player;
    u32 count;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    count = player->match_frame_count;
    return count;
}

u32 Player_GetSuicideCount(int slot)
{
    StaticPlayer* player;
    u32 count;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    count = player->suicide_count;
    return count;
}

void Player_SetSuicideCount(s32 slot, u32 suicide_count)
{
    StaticPlayer* player;

    if (suicide_count > 0xffff || 0 > (s32) suicide_count) {
        return;
    }

    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->suicide_count = suicide_count;
}

bool Player_800353BC(s32 slot)
{
    StaticPlayer* player;

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    if (player->player_state == 2 && (player->flags.b0)) {
        return 1;
    }

    return 0;
}

bool Player_8003544C(s32 slot, bool condition)
{
    u8 _[4];

    if (!condition) {
        StaticPlayer* player;

        Player_CheckSlot(slot);
        player = &player_slots[slot];

        if (player->player_state == 2 && player->flags.b0) {
            return true;
        }
    }

    return false;
}

void Player_SetFlagsBit0(int slot, bool bit0)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flags.b0 = bit0;
}

u8 Player_GetNametagSlotID(int slot)
{
    StaticPlayer* player;
    int nametag_slot_id;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    nametag_slot_id = player->nametag_slot_id;
    return nametag_slot_id;
}

void Player_SetNametagSlotID(int slot, int nametag_slot_id)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->nametag_slot_id = nametag_slot_id;
}

s32 Player_GetFlagsBit1(s32 slot)
{
    StaticPlayer* player;
    u8 bit1;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit1 = player->flags.b1;
    return bit1;
}

void Player_SetFlagsBit1(s32 slot)
{
    StaticPlayer* player;
    s8 one = 1;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flags.b1 = one;
}

void Player_UnsetFlagsBit1(int slot)
{
    u8 _[1];

    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flags.b1 = false;
}

s32 Player_GetFlagsBit3(s32 slot)
{
    StaticPlayer* player;
    u8 bit3;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit3 = player->flags.b3;
    return bit3;
}

void Player_SetFlagsBit3(s32 slot, u8 bit3)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flags.b3 = bit3;
}

int Player_GetFlagsBit4(int slot)
{
    StaticPlayer* player;
    u8 bit4;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit4 = player->flags.b4;
    return bit4;
}

u8 Player_GetFlagsBit5(s32 slot)
{
    StaticPlayer* player;
    u8 is_metal;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    is_metal = player->flags.is_metal;
    return is_metal;
}

void Player_SetFlagsBit5(s32 slot, u8 is_metal)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flags.is_metal = is_metal;
}

u8 Player_GetFlagsBit6(s32 slot)
{
    StaticPlayer* player;
    u8 bit6;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit6 = player->flags.b6;
    return bit6;
}

void Player_SetFlagsBit6(s32 slot, u8 bit6)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flags.b6 = bit6;
}

u8 Player_GetFlagsBit7(s32 slot)
{
    StaticPlayer* player;
    u8 bit7;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit7 = player->flags.b7;
    return bit7;
}

void Player_SetFlagsBit7(s32 slot, u8 bit7)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flags.b7 = bit7;
}

bool Player_GetMoreFlagsBit0(s32 slot)
{
    StaticPlayer* player;
    u8 bit0;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit0 = player->more_flags.b0;
    return bit0;
}

bool Player_GetMoreFlagsBit1(s32 slot)
{
    StaticPlayer* player;
    u8 bit1;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit1 = player->more_flags.b1;
    return bit1;
}

void Player_SetMoreFlagsBit1(s32 slot, u8 bit1)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->more_flags.b1 = bit1;
}

s32 Player_GetUnk4D(s32 slot)
{
    StaticPlayer* player;
    u8 unk4D;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    unk4D = player->unk4D;
    return unk4D;
}

void Player_SetUnk4D(s32 slot, s8 unk4D)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->unk4D = unk4D;
}

u8 Player_GetFlagsAEBit1(s32 slot)
{
    StaticPlayer* player;
    u8 bit1;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    bit1 = player->flagsAE.b1;
    return bit1;
}

u8 Player_SetFlagsAEBit1(int slot, u8 bit1)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->flagsAE.b1 = bit1;
}

int Player_GetUnk4C(s32 slot)
{
    StaticPlayer* player;
    u8 unk4C;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    unk4C = player->unk4C;
    return unk4C;
}

void Player_SetUnk4C(s32 slot, u8 unk4C)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->unk4C = unk4C;
}

void Player_SetStructFunc(s32 slot, void (*arg_func)(s32))
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->struct_func = arg_func;
}

plActionStats* Player_GetActionStats(int slot)
{
    StaticPlayer* player;
    plActionStats* attack_count;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    attack_count = &player->stale_moves.total_attack_count_struct;
    return attack_count;
}

StaleMoveTable* Player_GetStaleMoveTableIndexPtr(s32 slot)
{
    StaticPlayer* player;
    StaleMoveTable* stale_move_table;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    stale_move_table = &player->stale_moves;
    return stale_move_table;
}

struct pl_x5EC_t* Player_GetUnk6A8Ptr(int slot)
{
    StaticPlayer* player;
    struct pl_x5EC_t* unk6A8;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    unk6A8 = &player->stale_moves.x5EC;
    return unk6A8;
}

pl_StaleMoveTableExt_t* Player_GetStaleMoveTableIndexPtr2(s32 slot)
{
    StaticPlayer* player;
    pl_StaleMoveTableExt_t* stale_move_table;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    stale_move_table = (pl_StaleMoveTableExt_t*) (&player->stale_moves);
    return stale_move_table;
}

void Player_SetUnk45(s32 slot, int unk45)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    player->unk45 = unk45;
}

u32 Player_GetUnk45(s32 slot)
{
    StaticPlayer* player;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    if (player->slot_type == Gm_PKind_Human) {
        return player->unk45;
    }

    return 4;
}

void Player_UpdateJoystickCountByIndex(s32 slot, s32 index)
{
    StaticPlayer* player;
    s32 transformed;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    transformed = player->transformed[index];
    player->joystick_direction_input_count[transformed]++;
}

s32 Player_GetJoystickCountByIndex(s32 slot, s32 index)
{
    StaticPlayer* player;
    s32 transformed;
    Player_CheckSlot(slot);
    player = &player_slots[slot];
    transformed = player->transformed[index];
    return player->joystick_direction_input_count[transformed];
}

void Player_InitOrResetPlayer(s32 slot)
{
    StaticPlayer* player;
    u8* transformed0;
    u8* transformed1;
    f32 zerofloat;
    f32 onefloat;

    u8 _[56];

    Player_CheckSlot(slot);
    player = &player_slots[slot];

    player->player_state = 0;
    player->player_character = CKind_Mario;
    transformed0 = &player->transformed[0];
    transformed1 = &player->transformed[1];

    zerofloat = 0.0f;
    player->player_poses.byIndex[*transformed0].z = zerofloat;
    player->player_poses.byIndex[*transformed0].y = zerofloat;
    player->player_poses.byIndex[*transformed0].x = zerofloat;

    player->player_poses.byIndex[*transformed1].z = zerofloat;
    player->player_poses.byIndex[*transformed1].y = zerofloat;
    player->player_poses.byIndex[*transformed1].x = zerofloat;

    player->player_poses.byIndex[2].z = zerofloat;
    player->player_poses.byIndex[2].y = zerofloat;
    player->player_poses.byIndex[2].x = zerofloat;
    player->player_poses.byIndex[3].z = zerofloat;
    player->player_poses.byIndex[3].y = zerofloat;
    player->player_poses.byIndex[3].x = zerofloat;

    player->costume_id = 0;
    player->slot_type = Gm_PKind_NA;
    player->transformed[0] = 0;
    player->transformed[1] = 1;

    player->unk45 = 0;
    player->controller_index = 0;
    player->team = 0;
    player->player_id = 0;
    player->cpu_level = 0;

    player->cpu_type = 4;

    player->handicap = 0;
    player->unk4C = 0;

    player->unk4D = 4;
    player->unk4E = -1;

    onefloat = 1.0;
    player->unk50 = onefloat;
    player->attack_ratio = onefloat;
    player->defense_ratio = onefloat;
    player->model_scale = onefloat;

    player->staminas.byIndex[player->transformed[0]] = 0;
    player->staminas.byIndex[player->transformed[1]] = 0;
    player->staminas.byIndex[2] = 0;

    player->falls[player->transformed[0]] = 0;
    player->falls[player->transformed[1]] = 0;

    player->kos_by_player[0] = 0;
    player->kos_by_player[1] = 0;
    player->kos_by_player[2] = 0;
    player->kos_by_player[3] = 0;
    player->kos_by_player[4] = 0;
    player->kos_by_player[5] = 0;

    player->match_frame_count = -1;
    player->suicide_count = 0;
    player->stocks = 0;

    player->current_coins = 0;
    player->total_coins = 0;
    player->unk98 = 0;
    player->unk9C = 0;

    player->joystick_direction_input_count[*transformed0] = 0;
    player->joystick_direction_input_count[*transformed1] = 0;

    player->flags.b0 = 0;
    player->nametag_slot_id = 0;
    player->flags.b1 = 0;
    player->flags.b2 = 0;
    player->flags.b3 = 0;
    player->flags.b4 = 0;
    player->flags.is_metal = false;
    player->flags.b6 = 0;
    player->flags.b7 = 0;

    player->more_flags.b0 = 0;
    player->more_flags.b1 = 0;
    player->more_flags.b2 = 0;
    player->more_flags.b3 = 0;
    player->more_flags.b4 = 0;
    player->more_flags.b5 = 0;
    player->more_flags.b6 = 0;

    player->flagsAE.b0 = 0;
    player->flagsAE.b1 = 0;

    player->player_entity[*transformed0] = 0;
    player->player_entity[*transformed1] = 0;

    player->struct_func = 0;
}
