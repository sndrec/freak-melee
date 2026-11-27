#include "native_fighter_runtime.h"

#ifdef MELEE_NATIVE

#include <stdlib.h>
#include <stdio.h>
#include "native_script_runtime.h"
#include <melee/gm/native_rollback_pages.h>
#include <melee/ef/native_script_effect_actor.h>
#include <melee/it/native_script_item_actor.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <string.h>

#include "ftcharacter_assets.h"
#include "ftcostume_assets.h"
#include "ftcpu_data.h"
#include "native_fighter_cpu.h"
#include "fighter.h"
#include "ftdata.h"
#include "ftdat.h"
#include "ftparts.h"
#include "ftanimation_bank.h"
#include "native_source_package.h"
#include "native_fighter_pose.h"
#include "native_costume_runtime.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include <melee/ef/native_model_effect.h>
#include "types.h"
#include <melee/ft/kinds/ftMario/ftmario_clone.h>
#include <melee/ft/kinds/ftDrMario/ftdrmario_clone.h>
#include <melee/ft/kinds/ftGanon/ftganon_clone.h>
#include <melee/ft/kinds/ftCaptain/ftcaptain_clone.h>
#include <melee/ft/kinds/ftLuigi/ftluigi_clone.h>
#include <melee/ft/kinds/ftPurin/ftpurin_clone.h>
#include <melee/ft/kinds/ftFox/ftfox_clone.h>
#include <melee/ft/kinds/ftFalco/ftfalco_clone.h>
#include <melee/ft/kinds/ftMars/ftmars_clone.h>
#include <melee/ft/kinds/ftEmblem/ftemblem_clone.h>
#include <melee/ft/kinds/ftDonkey/ftdonkey_clone.h>
#include <melee/ft/kinds/ftKoopa/ftkoopa_clone.h>
#include <melee/ft/kinds/ftPikachu/ftpikachu_clone.h>
#include <melee/ft/kinds/ftPichu/ftpichu_clone.h>
#include <melee/ft/kinds/ftSamus/ftsamus_clone.h>
#include <melee/ft/kinds/ftLink/ftlink_clone.h>
#include <melee/ft/kinds/ftCLink/ftclink_clone.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwo_clone.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi_clone.h>
#include <melee/ft/kinds/ftNess/ftness_clone.h>
#include <melee/ft/kinds/ftPeach/ftpeach_clone.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatch_clone.h>
#include <melee/ft/kinds/ftZelda/ftzelda_clone.h>
#include <melee/ft/kinds/ftSeak/ftsheik_clone.h>
#include <melee/ft/kinds/ftPopo/ftpopo_clone.h>
#include <melee/ft/kinds/ftNana/ftnana_clone.h>
#include <melee/ft/kinds/ftRook/ftrook.h>
#include <melee/pl/player.h>
#include <melee/it/native_holdable.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/dat.h>
#include <sysdolphin/baselib/axdriver.h>
#include <sysdolphin/baselib/synth.h>
#include <melee/lb/lbanim.h>

enum {
    // Four player seats plus the optional local practice opponent.
    NATIVE_FIGHTER_FAMILY_CAPACITY = 5,
    NATIVE_FIGHTER_ACTOR_CAPACITY =
        NATIVE_FIGHTER_FAMILY_CAPACITY * NATIVE_FIGHTER_FAMILY_MEMBERS
};

typedef struct NativeFighterFamily NativeFighterFamily;

typedef struct NativeFighterSlot {
    NativeFighterFamily* family;
    unsigned member_index;
    NativeSourcePackage* source;
    NativeFighterCpuProfile cpu;
    UnkCostumeStruct costume_entry;
    Fighter_CostumeStrings costume_names;
    CharacterKind character_kind;
    FighterKind fighter_kind;
    bool active;
} NativeFighterSlot;

struct NativeFighterFamily {
    NativeContentId id;
    uint8_t* package;
    size_t package_size;
    NativeFighterPackage metadata;
    NativeFighterPackageData data;
    NativeFighterSlot* members;
    CharacterKind character_kind;
    bool active;
};

static NativeFighterFamily family_banks[2][NATIVE_FIGHTER_FAMILY_CAPACITY];
static NativeFighterFamily* families = family_banks[0];
static NativeFighterFamily* staged_families = family_banks[1];
static NativeFighterSlot slot_banks[2][NATIVE_FIGHTER_ACTOR_CAPACITY];
static NativeFighterSlot* slots = slot_banks[0];
static NativeFighterSlot* staged_slots = slot_banks[1];
static unsigned staged_count;
static ftCo_AttackEntry empty_cpu_attack;

typedef struct NativeStockCloneBase {
    FighterKind kind;
    const NativeFighterDefinition* definition;
} NativeStockCloneBase;

static const NativeStockCloneBase stock_clone_bases[] = {
    { Ft_Kind_Mario, &ftMarioClone_Definition },
    { Ft_Kind_DrMario, &ftDrMarioClone_Definition },
    { Ft_Kind_Ganon, &ftGanonClone_Definition },
    { Ft_Kind_Captain, &ftCaptainClone_Definition },
    { Ft_Kind_Luigi, &ftLuigiClone_Definition },
    { Ft_Kind_Purin, &ftPurinClone_Definition },
    { Ft_Kind_Fox, &ftFoxClone_Definition },
    { Ft_Kind_Falco, &ftFalcoClone_Definition },
    { Ft_Kind_Mars, &ftMarsClone_Definition },
    { Ft_Kind_Emblem, &ftEmblemClone_Definition },
    { Ft_Kind_Donkey, &ftDonkeyClone_Definition },
    { Ft_Kind_Koopa, &ftKoopaClone_Definition },
    { Ft_Kind_Pikachu, &ftPikachuClone_Definition },
    { Ft_Kind_Pichu, &ftPichuClone_Definition },
    { Ft_Kind_Samus, &ftSamusClone_Definition },
    { Ft_Kind_Link, &ftLinkClone_Definition },
    { Ft_Kind_CLink, &ftCLinkClone_Definition },
    { Ft_Kind_Mewtwo, &ftMewtwoClone_Definition },
    { Ft_Kind_Yoshi, &ftYoshiClone_Definition },
    { Ft_Kind_Ness, &ftNessClone_Definition },
    { Ft_Kind_Peach, &ftPeachClone_Definition },
    { Ft_Kind_GameWatch, &ftGameWatchClone_Definition },
    { Ft_Kind_Zelda, &ftZeldaClone_Definition },
    { Ft_Kind_Seak, &ftSheikClone_Definition },
    { Ft_Kind_Popo, &ftPopoClone_Definition },
    { Ft_Kind_Nana, &ftNanaClone_Definition },
};

static bool runtime_quiescent(void)
{
    return nativeScriptRuntime_Idle() && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_804D7818 &&
        !HSD_GObj_DelayedProcInfo.flags;
}

FighterKind nativeFighterRuntime_AnimationFamily(Fighter* fighter)
{
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    return package ? (FighterKind)package->assets.facts.animation_tag : fighter->kind;
}

static bool release_slot(NativeFighterSlot* slot)
{
    if (slot->source && !nativeSourcePackage_Release(slot->source)) return false;
    free(slot->source);
    memset(slot,0,sizeof(*slot));
    return true;
}

static bool release_family(NativeFighterFamily* family)
{
    if (family->members)
        for(unsigned m=0;m<NATIVE_FIGHTER_FAMILY_MEMBERS;++m)
            if(!release_slot(&family->members[m]))return false;
    nativeContentStore_ReleaseFighterData(&family->data);
    free(family->package);
    memset(family,0,sizeof(*family));
    return true;
}

const NativeFighterDefinition* nativeFighterRuntime_FindDefinition(
    const char* code_key)
{
    if (code_key == NULL) return NULL;
    if (strcmp(code_key, ftRook_Definition.code_key) == 0)
        return &ftRook_Definition;
    for (unsigned i = 0;
         i < sizeof(stock_clone_bases) / sizeof(stock_clone_bases[0]); ++i)
        if (strcmp(code_key, stock_clone_bases[i].definition->code_key) == 0)
            return stock_clone_bases[i].definition;
    return NULL;
}

const NativeFighterDefinition* nativeFighterRuntime_FindStockDefinition(
    FighterKind source_kind)
{
    for (unsigned i = 0;
         i < sizeof(stock_clone_bases) / sizeof(stock_clone_bases[0]); ++i)
        if (stock_clone_bases[i].kind == source_kind)
            return stock_clone_bases[i].definition;
    return NULL;
}

const char* nativeFighterRuntime_Title(CharacterKind character_kind)
{
    for (unsigned i = 0; i < NATIVE_FIGHTER_ACTOR_CAPACITY; ++i) {
        const NativeFighterSlot* slot = &slots[i];
        if (slot->active && slot->character_kind == character_kind) {
            return slot->family->metadata.title;
        }
    }
    return NULL;
}

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    // Ordinary packages own source/data rather than a callback-bearing Definition.
    (void)kind;
    return NULL;
}

const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{
    for(unsigned i=0;i<NATIVE_FIGHTER_ACTOR_CAPACITY;++i)
        if(slots[i].active && slots[i].fighter_kind==kind)return slots[i].source;
    return NULL;
}
const NativeContentId* nativeFighterRuntime_PackageId(FighterKind kind)
{
    for (unsigned i=0;i<NATIVE_FIGHTER_ACTOR_CAPACITY;++i) {
        const NativeFighterSlot* slot=&slots[i];
        if (slot->active && slot->fighter_kind==kind && slot->family && slot->family->active)
            return &slot->family->id;
    }
    return NULL;
}

bool nativeFighterRuntime_AlternatePair(FighterKind first,FighterKind second)
{
    if(first==second)return false;
    for(unsigned i=0;i<NATIVE_FIGHTER_FAMILY_CAPACITY;++i) {
        const NativeFighterFamily* family=&families[i];
        if(!family->active || family->metadata.mode!=NativeFighterFamily_AlternateForm ||
           family->metadata.member_count!=2 || !family->members)continue;
        const NativeFighterSlot* a=&family->members[0];const NativeFighterSlot* b=&family->members[1];
        if(!a->active || !b->active || a->family!=family || b->family!=family || !a->source || !b->source ||
           a->source->target!=a->fighter_kind || b->source->target!=b->fighter_kind)continue;
        if((a->fighter_kind==first && b->fighter_kind==second) ||
           (a->fighter_kind==second && b->fighter_kind==first))return true;
    }
    return false;
}

uint8_t nativeFighterRuntime_SoundMuteChannels(FighterKind fighter_kind)
{
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter_kind);
    if(package)return package->assets.facts.sound_mute_channels;
    return fighter_kind == Ft_Kind_GameWatch || fighter_kind == Ft_Kind_Samus
               ? 0 : 0x44;
}

bool nativeFighterRuntime_PoseAssets(Fighter_GObj* object, NativeFighterPoseAssets* output)
{
    if(!output)return false;
    *output=(NativeFighterPoseAssets){0};
    uint32_t generation;
    if(!nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&generation))return false;
    Fighter* fighter=GET_FIGHTER(object);
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    if(!package || !package->assets.ready)return false;
    const NativeSourceImportedAssets* assets=&package->assets;
    if(fighter->ft_data!=&assets->character.data || gFtDataList[fighter->kind]!=&assets->character.data ||
       fighter->x24!=assets->character.data.xC || fighter->x58C!=assets->facts.actions ||
       !ftPartsTable || ftPartsTable[fighter->kind]!=&assets->parts)return false;
    uintptr_t arena=(uintptr_t)assets->character.import.arena,bank=(uintptr_t)fighter->x24;
    size_t used=assets->character.import.arena_used;
    if(!arena || used>assets->character.import.arena_size || bank<arena || bank-arena>used ||
       assets->facts.actions*sizeof(*fighter->x24)>used-(bank-arena))return false;
    const UnkCostumeStruct* costume=nativeCostumeRuntime_ForFighter(fighter);
    if(!costume) {
        for(unsigned i=0;i<NATIVE_FIGHTER_ACTOR_CAPACITY;++i)
            if(slots[i].active && slots[i].source==package){costume=&slots[i].costume_entry;break;}
    }
    if(!costume || !costume->native_assets || costume->joint!=fighter->x108_costume_joint ||
       costume->native_assets->joint!=fighter->x108_costume_joint)return false;
    *output=(NativeFighterPoseAssets){.character=&assets->character,.costume=costume->native_assets,
        .action_count=assets->facts.actions,.demo_count=assets->facts.demo_count,.pose_graph_count=assets->facts.pose_count,
        .animation_bytes=package->owned_input.assets.animation,.animation_size=package->owned_input.assets.animation_bytes,
        .animation_arena=assets->animation_arena,.animation_arena_size=FT_ANIMATION_IMPORT_ARENA_SIZE};
    return true;
}

FigaTree* nativeFighterRuntime_PoseClip(Fighter_GObj* object,int32_t index)
{
    NativeFighterPoseAssets assets;
    if(!nativeFighterRuntime_PoseAssets(object,&assets) || (uint32_t)index>=assets.action_count)return NULL;
    FigaTree* clip=assets.character->data.xC[index].x14;
    uintptr_t arena=(uintptr_t)assets.animation_arena,at=(uintptr_t)clip;
    if(!clip || !arena || at<arena || at-arena>assets.animation_arena_size ||
       sizeof(*clip)>assets.animation_arena_size-(at-arena))return NULL;
    return clip;
}

HSD_Joint* nativeFighterRuntime_PoseGraph(Fighter_GObj* object,int32_t index)
{
    NativeFighterPoseAssets assets;
    if(!nativeFighterRuntime_PoseAssets(object,&assets) || (uint32_t)index>=assets.pose_graph_count)return NULL;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(GET_FIGHTER(object)->kind);
    if(!package || &package->assets.character!=assets.character)return NULL;
    const NativeSourcePoseGraph* graph=&package->assets.facts.poses[index];
    if(graph->slot>=FT_CHARACTER_ITEM_CAPACITY || !(package->assets.facts.layout.joint_graph_mask&(1u<<graph->slot)))return NULL;
    return nativeFighterPose_SelectGraph(assets.character,graph->slot,graph->path);
}

bool nativeFighterRuntime_InstallOutlineVisibility(Fighter_GObj* object,
                                                  int32_t asset_slot)
{
    uint32_t generation;
    if((uint32_t)asset_slot>=FT_CHARACTER_ITEM_CAPACITY ||
       !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&generation))return false;
    Fighter* fighter=GET_FIGHTER(object);
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    if(!package || !package->assets.ready || fighter->ft_data!=&package->assets.character.data ||
       !(package->assets.facts.layout.visibility_mask&(1u<<asset_slot)) ||
       fighter->x5AC.model_num!=package->assets.character.model_data.x0.model_num ||
       !fighter->x5AC.model_num || fighter->x5AC.model_num>11)return false;
    FtPartsVisLookup* lookup=package->assets.character.visibility_assets[asset_slot];
    if (lookup == NULL) return false;
    for (unsigned group = 0; group < fighter->x5AC.model_num; ++group) {
        if (lookup[group].x0 < 0 || lookup[group].x0 > 256 ||
            (lookup[group].x0 != 0 && lookup[group].x4 == NULL)) return false;
        for (int variant = 0; variant < lookup[group].x0; ++variant) {
            const TempS* selection = &lookup[group].x4[variant];
            if (selection->x0 < 0 || selection->x0 > 256 ||
                (selection->x0 != 0 && selection->x4 == NULL)) return false;
            for (int draw = 0; draw < selection->x0; ++draw) {
                unsigned ordinal = selection->x4[draw];
                if (ordinal >= fighter->dobj_list.count ||
                    fighter->dobj_list.data == NULL ||
                    fighter->dobj_list.data[ordinal] == NULL) return false;
            }
        }
    }
    fighter->x5AC.xC[4] = lookup;
    return true;
}

typedef struct NativeFamilyMemberIdentity {
    NativeFighterLifetime actor;
    const NativeFighterSlot* slot;
    const NativeFighterFamily* family;
    const NativeSourcePackage* package;
    const NativeScriptFighterProgram* program;
    uint64_t program_identity;
    CharacterKind character;
    unsigned member_index,member_count;
    NativeFighterFamilyMode mode;
    int player;
} NativeFamilyMemberIdentity;

static const NativeFighterSlot* family_member_slot(FighterKind kind)
{
    for(unsigned i=0;i<NATIVE_FIGHTER_ACTOR_CAPACITY;++i) {
        const NativeFighterSlot* slot=&slots[i];
        const NativeFighterFamily* family=slot->family;
        const NativeSourcePackage* package=slot->source;
        if(!slot->active || slot->fighter_kind!=kind)continue;
        const NativeScriptFighterProgram* program=nativeScriptFighter_Program(kind);
        if(!family || !family->active || !family->members ||
           !family->metadata.member_count || family->metadata.member_count>NATIVE_FIGHTER_FAMILY_MEMBERS ||
           slot->member_index>=family->metadata.member_count ||
           &family->members[slot->member_index]!=slot ||
           slot->character_kind!=family->character_kind || !package || package->address!=package ||
           !package->ready || !package->assets.ready || package->target!=kind ||
           package->program.bank!=&package->compiler.bank || !package->compiler.bank.valid ||
           !program || program->bank!=&package->compiler.bank || program->identity!=package->program.identity ||
           gFtDataList[kind]!=&package->assets.character.data ||
           ftData_CharacterStateTables[kind]!=package->assets.motions)return NULL;
        return slot;
    }
    return NULL;
}

static bool family_member_current(const NativeFamilyMemberIdentity* saved)
{
    const NativeFighterSlot* slot=family_member_slot(saved->actor.kind);
    return nativeFighterLifetime_Current(&saved->actor) && slot==saved->slot &&
        slot && slot->source==saved->package && slot->family==saved->family &&
        slot->member_index==saved->member_index && slot->character_kind==saved->character &&
        saved->family->metadata.mode==saved->mode && saved->family->metadata.member_count==saved->member_count &&
        saved->actor.data->player_id==saved->player &&
        saved->actor.data->ft_data==&saved->package->assets.character.data &&
        Player_GetPlayerCharacter(saved->player)==saved->character &&
        nativeScriptFighter_Program(saved->actor.kind)==saved->program &&
        saved->program->identity==saved->program_identity &&
        (!saved->actor.source_owned || saved->actor.program==saved->program);
}

static bool family_member_capture(Fighter_GObj* object,bool require_owned,
                                  NativeFamilyMemberIdentity* saved)
{
    uint32_t generation;
    if(!nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&generation))return false;
    Fighter* fighter=GET_FIGHTER(object);
    if(fighter->player_id>=Gm_Player_NumMax)return false;
    const NativeFighterSlot* slot=family_member_slot(fighter->kind);
    if(!slot || !nativeFighterLifetime_Capture(fighter,&saved->actor) ||
       (require_owned && !saved->actor.source_owned))return false;
    saved->slot=slot;saved->package=slot->source;saved->player=fighter->player_id;
    saved->family=slot->family;saved->character=slot->character_kind;saved->member_index=slot->member_index;
    saved->member_count=slot->family->metadata.member_count;saved->mode=slot->family->metadata.mode;
    saved->program=nativeScriptFighter_Program(fighter->kind);saved->program_identity=saved->program->identity;
    return family_member_current(saved);
}

static bool family_member_published(const NativeFamilyMemberIdentity* saved)
{
    Fighter_GObj* first=Player_GetEntityAtIndex(saved->player,0);
    Fighter_GObj* second=Player_GetEntityAtIndex(saved->player,1);
    return (!first || first!=second) &&
        (first==saved->actor.object || second==saved->actor.object);
}

static uint32_t family_member_id(const NativeFamilyMemberIdentity* caller,unsigned member_index,
                                bool require_owned)
{
    const NativeFighterFamily* family=caller->family;
    if(member_index>=family->metadata.member_count)return 0;
    const NativeFighterSlot* requested=&family->members[member_index];
    NativeFamilyMemberIdentity member={0};
    for(unsigned role=0;role<NATIVE_FIGHTER_FAMILY_MEMBERS;++role) {
        Fighter_GObj* object=Player_GetEntityAtIndex(caller->player,role);
        if(!family_member_capture(object,require_owned,&member) || member.slot!=requested ||
           member.player!=caller->player)continue;
        // Roles can swap; the selected kind/package fixes the immutable ordinal.
        if(!family_member_current(caller) || !family_member_current(&member) ||
           !family_member_published(&member))return 0;
        return member.actor.generation;
    }
    return 0;
}

uint32_t nativeFighterRuntime_MemberId(Fighter_GObj* source,unsigned member_index)
{
    NativeFamilyMemberIdentity caller={0};
    if(!family_member_capture(source,true,&caller) || !family_member_published(&caller))return 0;
    uint32_t id=family_member_id(&caller,member_index,true);
    return family_member_current(&caller) && family_member_published(&caller)?id:0;
}

uint32_t nativeFighterRuntime_CpuHistorySource(Fighter_GObj* source)
{
    NativeFamilyMemberIdentity caller={0};
    // CPU preparation can inspect a constructing physical member before Player
    // publishes it. Retain actor storage when present, without requiring it.
    if(!family_member_capture(source,false,&caller) || caller.slot->member_index!=1 ||
       caller.slot->family->metadata.mode!=NativeFighterFamily_SimultaneousCompanion ||
       caller.slot->family->metadata.member_count!=2)return 0;
    uint32_t id=family_member_id(&caller,0,false);
    return family_member_current(&caller)?id:0;
}

uint64_t nativeFighterRuntime_CharacterSoundBanks(CharacterKind character_kind)
{
    // Both members must be audible before next-scene entities are created.
    // Prefer the prepared family to the outgoing scene's registration.
    const NativeFighterFamily* family = NULL;
    for (unsigned i = 0; i < staged_count; ++i)
        if (staged_families[i].character_kind == character_kind) {
            family = &staged_families[i];
            break;
        }
    if (family == NULL)
        for (unsigned i = 0; i < NATIVE_FIGHTER_FAMILY_CAPACITY; ++i)
            if (families[i].active && families[i].character_kind == character_kind) {
                family = &families[i];
                break;
            }
    uint64_t banks = 0;
    if (family != NULL)
        for (unsigned m = 0; m < family->metadata.member_count; ++m)
            banks |= family->members[m].source->assets.facts.sound_banks;
    return banks;
}

FighterKind nativeFighterRuntime_BehaviorKind(FighterKind kind)
{
    return kind;
}

bool nativeFighterRuntime_ResolveArticleForItem(const Fighter* owner,
                                                ItemKind item_kind,
                                                Article** article)
{
    if (article == NULL || owner == NULL ||
        owner->kind < Ft_Kind_CustomFirst) return false;
    *article = NULL;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(owner->kind);
    if(!package || owner->ft_data!=&package->assets.character.data)return false;
    for(unsigned i=0;i<package->assets.facts.article_count;++i) {
        if(package->assets.article_kinds[i]!=item_kind)continue;
        *article=package->assets.character.fighter_items[package->assets.facts.articles[i].slot];
        return true;
    }
    return false;
}

const NativeFighterArticleLogic* nativeFighterRuntime_ArticleLogic(const Fighter* owner,ItemKind kind)
{
    // Source item programs are selected by exact declaration owner, never C logic.
    (void)owner;(void)kind;
    return NULL;
}

Article* nativeFighterRuntime_ArticleAssets(FighterKind author,ItemKind kind)
{
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(author);
    if(!package)return NULL;
    Article* result=NULL;
    for(unsigned i=0;i<package->assets.facts.article_count;++i) {
        if(package->assets.article_kinds[i]!=kind)continue;
        Article* candidate=package->assets.character.fighter_items[package->assets.facts.articles[i].slot];
        if(result && result!=candidate)return NULL;
        result=candidate;
    }
    return result;
}

int nativeFighterRuntime_SoundClip(FighterKind kind, unsigned clip, unsigned channel)
{
    const NativeSourcePackage* package = nativeFighterRuntime_SourcePackage(kind);
    const HSD_SfxBank* bank = package ? package->sounds : NULL;
    if (!bank || clip >= bank->count || channel > 6) {
        return -1;
    }
    const HSD_SfxEntry* entry = &bank->entries[clip];
    if (!channel && (entry->voices[0].address.loopFlag ||
        (entry->voice_count == 2 && entry->voices[1].address.loopFlag)))
    {
        return -1;
    }
    return entry->id;
}

bool nativeFighterRuntime_CanClear(void)
{
    if(!runtime_quiescent() || nativeRollbackPages_IsInitialized())return false;
    for(unsigned i=0;i<NATIVE_FIGHTER_ACTOR_CAPACITY;++i) {
        const NativeFighterSlot* slot=&slots[i];
        if(!slot->active)continue;
        const NativeSourcePackage* source=slot->source;
        const NativeScriptFighterProgram* selected=nativeScriptFighter_Program(slot->fighter_kind);
        if(!source || source->address!=source || !source->ready || source->leases!=1 ||
           !nativeScriptFighter_CanPublish(slot->fighter_kind) || !selected || selected->bank!=&source->compiler.bank ||
           nativeScriptFighter_ReferencesProgram(&source->compiler.bank) || nativeScriptItem_ReferencesProgram(&source->compiler.bank) ||
           nativeScriptEffect_ReferencesProgram(&source->compiler.bank))return false;
        for(unsigned kind=0;kind<Ft_Kind_RuntimeCapacity;++kind) {
            const NativeScriptFighterProgram* other=nativeScriptFighter_Program((FighterKind)kind);
            if(other && other->bank==&source->compiler.bank && kind!=(unsigned)slot->fighter_kind)return false;
        }
    }
    return true;
}

bool nativeFighterRuntime_Clear(void)
{
    if(!nativeFighterRuntime_CanClear()) {
        fprintf(stderr,"Source retirement blocked: active callback, actor, lease or retained rollback history\n");
        return false;
    }
    static const NativeScriptFighterProgram empty_program;
    nativeArticleResource_Clear();
    nativeModelEffect_Clear();
    for (unsigned i = 0; i < NATIVE_FIGHTER_ACTOR_CAPACITY; ++i) {
        NativeFighterSlot* slot = &slots[i];
        if (slot->active) {
            FighterKind kind = slot->fighter_kind;
            HSD_SfxBank* sounds = slot->source->sounds;
            if (sounds) {
                AXDriverRetirePackageSounds(sounds->first_id, sounds->count);
                HSD_SynthRetirePackageSfxBank(sounds);
            }
            nativeScriptFighter_PublishProgram(kind,&empty_program);
            bool unpinned=nativeSourcePackage_Unpin(slot->source);
            HSD_ASSERTREPORT(0,unpinned,"Preflighted source lease retirement failed");
            gFtDataList[kind] = NULL;
            CostumeListsForeachCharacter[kind] = (struct UnkCostumeList){ 0 };
            ftData_803C2360[kind] = NULL;
            ftData_803C2468[kind] = NULL;
            ftData_Table_Unk0[kind] = (ftData_UnkCountStruct){ 0 };
            ftData_UnkIntPairs[kind] = (ftData_UnkCountStruct){ 0 };
            ftData_UnkBytePerCharacter[kind] = 0xFF;
            ftData_CharacterStateTables[kind] = NULL;
            ftData_AssignStockCallbacks(kind, Ft_Kind_None);
            if (ftPartsTable != NULL) {
                ftPartsTable[kind] = NULL;
            }
            if (Fighter_804D6540 != NULL) {
                Fighter_804D6540[kind] = NULL;
            }
            if (Fighter_804D64FC != NULL) {
                ftCo_AttackEntry** families[FT_CPU_ATTACK_FAMILIES] = {
                    Fighter_804D64FC->x4, Fighter_804D64FC->x8,
                    Fighter_804D64FC->xC, Fighter_804D64FC->x10,
                    Fighter_804D64FC->x14, Fighter_804D64FC->x18,
                    Fighter_804D64FC->x1C,
                };
                for (unsigned family = 0; family < FT_CPU_ATTACK_FAMILIES;
                     ++family)
                {
                    families[family][kind] = &empty_cpu_attack;
                }
                Fighter_804D64FC->x20[kind] = 0;
            }
        }
    }
    for (unsigned i = 0; i < NATIVE_FIGHTER_FAMILY_CAPACITY; ++i) {
        bool released=release_family(&families[i]);
        HSD_ASSERTREPORT(0,released,"Preflighted source package release failed");
    }
    Player_ClearNativeCharacters();
    return true;
}

static bool prepare_member(NativeFighterSlot* slot,NativeFighterFamily* family,
                           unsigned member_index,FighterKind kind)
{
    slot->family=family;slot->member_index=member_index;
    slot->character_kind=family->character_kind;slot->fighter_kind=kind;
    const NativeFighterMemberData* data=&family->data.members[member_index];
    slot->source=calloc(1,sizeof(*slot->source));
    if(!slot->source)return false;
    NativeSourcePackageInput input={.family=NativeSourcePackage_Single,
        .facts=data->asset_record,.facts_bytes=data->asset_record_size,
        .assets={data->fighter_dat,data->fighter_size,data->animation_dat,data->animation_size,data->costume_dat,data->costume_size},
        .source=(const char*)data->script_text,.source_bytes=data->script_size,
        .parameters=(const char*)data->parameter_text,.parameter_bytes=data->parameter_size,
        .resources=data->resources,.resource_bytes=data->resource_size};
    NativeScriptDiagnostic diagnostic={0};
    if(!nativeSourcePackage_Prepare(slot->source,&input,kind,NULL,&diagnostic) ||
       !nativeFighterCpu_Parse((const char*)data->cpu_text,data->cpu_size,&slot->cpu)) {
        fprintf(stderr,"Source package preparation failed: %s\n",diagnostic.message[0]?diagnostic.message:"invalid owned CPU profile");
        release_slot(slot);return false;
    }
    NativeSourceImportedAssets* assets=&slot->source->assets;
    slot->costume_entry.joint=assets->costume.joint;
    slot->costume_entry.x4=assets->costume.matanim_joint;
    slot->costume_entry.native_assets=&assets->costume;
    slot->costume_names=(Fighter_CostumeStrings){NULL,assets->facts.costume_joint,assets->facts.costume_matanim};
    return true;
}

static bool prepare_family(NativeFighterFamily* family,NativeFighterSlot* members,const NativeContentId* id,
                           CharacterKind character,FighterKind kind,const char* root,bool require_installed)
{
    if(!family || !members || !id || family->package || family->active || !runtime_quiescent() ||
       character<ChKind_CustomFirst || character>=ChKind_RuntimeCapacity || kind<Ft_Kind_CustomFirst ||
       kind>=Ft_Kind_RuntimeCapacity)return false;
    char default_root[512];
    if(!root){if(!nativeContentStore_DefaultRoot(default_root,sizeof(default_root)))return false;root=default_root;}
    if(require_installed && !nativeContentStore_IsInstalled(root,id))return false;
    family->members=members;family->id=*id;family->character_kind=character;
    if(!nativeContentStore_Read(root,id,&family->package,&family->package_size) ||
       !nativeContentStore_ParseFighter(family->package,family->package_size,&family->metadata,&family->data))goto failure;
    if((unsigned)kind+family->metadata.member_count>Ft_Kind_RuntimeCapacity)goto failure;
    for(unsigned m=0;m<family->metadata.member_count;++m) {
        if(!prepare_member(&members[m],family,m,(FighterKind)(kind+m)))goto failure;
        if(family->metadata.mode==NativeFighterFamily_AlternateForm &&
           members[m].source->program.hooks[NativeScriptFighter_FormActivated]==UINT32_MAX) {
            fprintf(stderr,"Source alternate member %s needs its own form_activated event\n",family->metadata.members[m].key);
            goto failure;
        }
    }
    return true;
failure:
    release_family(family);return false;
}

bool nativeFighterRuntime_ValidateCached(const NativeContentId* id,const char* root)
{
    NativeFighterFamily candidate={0};NativeFighterSlot members[NATIVE_FIGHTER_FAMILY_MEMBERS]={0};
    if (!id || !runtime_quiescent()) return false;
    bool valid=prepare_family(&candidate,members,id,ChKind_CustomFirst,Ft_Kind_CustomFirst,root,false);
    bool released=release_family(&candidate);
    return valid && released;
}

bool nativeFighterRuntime_CostumeInfoCached(
    const NativeContentId* id, const char* root, unsigned model_variant, NativeFighterCostumeFamilyInfo* info)
{
    if (id == NULL || info == NULL) return false;
    *info = (NativeFighterCostumeFamilyInfo){0};
    char default_root[512];
    if (root == NULL) {
        if (!nativeContentStore_DefaultRoot(default_root, sizeof(default_root))) return false;
        root = default_root;
    }
    const NativeFighterFamily* family = NULL;
    for (unsigned i = 0; i < staged_count; ++i)
        if (memcmp(&staged_families[i].id, id, sizeof(*id)) == 0)
            family = &staged_families[i];
    if (family == NULL)
        for (unsigned i = 0; i < NATIVE_FIGHTER_FAMILY_CAPACITY; ++i)
            if (families[i].active && memcmp(&families[i].id, id, sizeof(*id)) == 0)
                family = &families[i];
    if (family != NULL && !nativeContentStore_IsCached(root, id)) return false;
    NativeFighterFamily candidate = {0};
    NativeFighterSlot members[NATIVE_FIGHTER_FAMILY_MEMBERS] = {0};
    bool valid = family != NULL || prepare_family(&candidate, members, id,
        ChKind_CustomFirst, Ft_Kind_CustomFirst, root, false);
    if (family == NULL) family = &candidate;
    if (valid) {
        info->member_count = family->metadata.member_count;
        for (unsigned m = 0; valid && m < info->member_count; ++m) {
            NativeFighterCostumeInfo* member = &info->members[m];
            const NativeSourceAssetFacts* facts = &family->members[m].source->assets.facts;
            size_t joint_len = strnlen(facts->costume_joint, sizeof(member->joint_name));
            size_t matanim_len = strnlen(facts->costume_matanim, sizeof(member->matanim_name));
            valid = joint_len < sizeof(member->joint_name) && matanim_len < sizeof(member->matanim_name);
            if (valid) {
                memcpy(member->key, family->metadata.members[m].key, sizeof(member->key));
                memcpy(member->joint_name, facts->costume_joint, joint_len + 1);
                memcpy(member->matanim_name, facts->costume_matanim, matanim_len + 1);
                valid = ftCostumeBindings_FromCharacter(&family->members[m].source->assets.character,
                    facts->joints, facts->layout.visibility_mask, model_variant, &member->bindings);
            }
        }
    }
    release_family(&candidate);
    return valid;
}

bool nativeFighterRuntime_StageCached(const NativeContentId* ids,unsigned count,const char* root,bool require_installed)
{
    if(count>NATIVE_FIGHTER_FAMILY_CAPACITY || (count && !ids) || !runtime_quiescent())return false;
    for(unsigned i=0;i<count;++i)for(unsigned j=0;j<i;++j)if(!memcmp(&ids[i],&ids[j],sizeof(*ids)))return false;
    for(unsigned i=0;i<NATIVE_FIGHTER_ACTOR_CAPACITY;++i)
        if(staged_slots[i].source && !nativeSourcePackage_CanRelease(staged_slots[i].source))return false;
    // Candidate packages retain their stable heap addresses; only the plain
    // registration/CPU descriptors move after every selected package succeeds.
    typedef struct Candidates {
        NativeFighterFamily families[NATIVE_FIGHTER_FAMILY_CAPACITY];
        NativeFighterSlot slots[NATIVE_FIGHTER_ACTOR_CAPACITY];
    } Candidates;
    Candidates* candidates=calloc(1,sizeof(*candidates));
    if(!candidates)return false;
    bool valid=true;
    if(count && !ftPartsTable)Fighter_LoadCommonData();
    if(count && !ftPartsTable)valid=false;
    for(unsigned i=0;valid && i<count;++i)
        valid=prepare_family(&candidates->families[i],&candidates->slots[i*NATIVE_FIGHTER_FAMILY_MEMBERS],&ids[i],
            ChKind_CustomFirst+i,Ft_Kind_CustomFirst+i*NATIVE_FIGHTER_FAMILY_MEMBERS,root,require_installed);
    if(valid) {
        for(unsigned i=0;i<NATIVE_FIGHTER_FAMILY_CAPACITY;++i)release_family(&staged_families[i]);
        memcpy(staged_families,candidates->families,sizeof(candidates->families));
        memcpy(staged_slots,candidates->slots,sizeof(candidates->slots));
        for(unsigned i=0;i<count;++i) {
            staged_families[i].members=&staged_slots[i*NATIVE_FIGHTER_FAMILY_MEMBERS];
            for(unsigned m=0;m<staged_families[i].metadata.member_count;++m)
                staged_families[i].members[m].family=&staged_families[i];
        }
        staged_count=count;
    } else for(unsigned i=0;i<NATIVE_FIGHTER_FAMILY_CAPACITY;++i)release_family(&candidates->families[i]);
    free(candidates);return valid;
}

static void bind_member(NativeFighterSlot* slot)
{
    FighterKind kind=slot->fighter_kind;
    NativeSourceImportedAssets* assets=&slot->source->assets;
    const NativeSourceAssetFacts* facts=&assets->facts;
    ftData_AssignStockCallbacks(kind,Ft_Kind_None);
    gFtDataList[kind]=&assets->character.data;
    CostumeListsForeachCharacter[kind]=(struct UnkCostumeList){&slot->costume_entry,1};
    ftData_803C2360[kind]=&slot->costume_names;
    ftData_803C2468[kind]=&assets->demos;
    ftData_Table_Unk0[kind]=(ftData_UnkCountStruct){.data=assets->animation_arena,.count=facts->actions};
    ftData_UnkIntPairs[kind].count=facts->demo_count;
    ftData_UnkBytePerCharacter[kind]=facts->effect_bank;
    ftData_CharacterStateTables[kind]=assets->motions;
    ftPartsTable[kind]=&assets->parts;
    if(Fighter_804D6540)Fighter_804D6540[kind]=&assets->omissions;
    if(Fighter_804D64FC) {
        ftCo_AttackEntry** lists[FT_CPU_ATTACK_FAMILIES]={Fighter_804D64FC->x4,Fighter_804D64FC->x8,
            Fighter_804D64FC->xC,Fighter_804D64FC->x10,Fighter_804D64FC->x14,Fighter_804D64FC->x18,Fighter_804D64FC->x1C};
        for(unsigned family=0;family<FT_CPU_ATTACK_FAMILIES;++family)lists[family][kind]=slot->cpu.attacks[family];
        Fighter_804D64FC->x20[kind]=slot->cpu.distance_threshold;
    }
    if (slot->source->sounds) {
        HSD_SynthPublishPackageSfxBank(slot->source->sounds);
    }
    nativeScriptFighter_PublishProgram(kind,&slot->source->program);
    slot->active=true;
}

static void bind_family(NativeFighterFamily* family)
{
    for (unsigned m = 0; m < family->metadata.member_count; ++m)
        bind_member(&family->members[m]);
    Player_RegisterNativeCharacter(family->character_kind, family->members[0].fighter_kind,
        family->metadata.member_count == 2 ? family->members[1].fighter_kind : Ft_Kind_None,
        family->metadata.mode == NativeFighterFamily_AlternateForm);
    family->active = true;
}

static bool can_bind(const NativeFighterFamily* family)
{
    if(!family || !runtime_quiescent() || nativeRollbackPages_IsInitialized() || !ftPartsTable || !family->members ||
       !((family->metadata.mode==NativeFighterFamily_Single && family->metadata.member_count==1) ||
         ((family->metadata.mode==NativeFighterFamily_AlternateForm ||
           family->metadata.mode==NativeFighterFamily_SimultaneousCompanion) && family->metadata.member_count==2)))return false;
    for(unsigned m=0;m<family->metadata.member_count;++m) {
        const NativeFighterSlot* slot=&family->members[m];
        if(slot->family!=family || slot->member_index!=m || !slot->source || slot->active ||
           slot->source->target!=slot->fighter_kind || slot->source->leases!=0 ||
           !nativeSourcePackage_CanActivate(slot->source) || nativeScriptFighter_Program(slot->fighter_kind)!=NULL)return false;
    }
    return true;
}

bool nativeFighterRuntime_ActivateStaged(unsigned count)
{
    if(count!=staged_count || !runtime_quiescent() || nativeRollbackPages_IsInitialized())return false;
    for(unsigned i=0;i<NATIVE_FIGHTER_FAMILY_CAPACITY;++i)if(families[i].active)return false;
    if(!count)return true;
    for(unsigned i=0;i<count;++i)if(!can_bind(&staged_families[i]))return false;
    NativeSourcePackage* pinned[NATIVE_FIGHTER_ACTOR_CAPACITY];unsigned pinned_count=0;
    bool valid=true;
    for(unsigned i=0;valid && i<count;++i)
        for(unsigned m=0;m<staged_families[i].metadata.member_count;++m) {
            NativeSourcePackage* source=staged_families[i].members[m].source;
            if(!nativeSourcePackage_Pin(source)){valid=false;break;}
            pinned[pinned_count++]=source;
        }
    if(!valid) {
        while(pinned_count)nativeSourcePackage_Unpin(pinned[--pinned_count]);
        return false;
    }
    NativeFighterSlot* previous_slots=slots;NativeFighterFamily* previous_families=families;
    slots=staged_slots;families=staged_families;staged_slots=previous_slots;staged_families=previous_families;
    staged_count=0;
    for(unsigned i=0;i<count;++i)bind_family(&families[i]);
    return true;
}

bool nativeFighterRuntime_Load(const NativeContentId* id,CharacterKind character,FighterKind kind,
                               const char* root,bool require_installed,bool preflight)
{
    if(preflight)return nativeFighterRuntime_ValidateCached(id,root);
    if(!runtime_quiescent() || nativeRollbackPages_IsInitialized())return false;
    NativeFighterFamily* family=NULL;NativeFighterSlot* members=NULL;
    for(unsigned i=0;i<NATIVE_FIGHTER_FAMILY_CAPACITY;++i) {
        if(families[i].active && families[i].character_kind==character)return false;
        if(!families[i].active && !families[i].package && !family){family=&families[i];members=&slots[i*NATIVE_FIGHTER_FAMILY_MEMBERS];}
    }
    for(unsigned i=0;i<NATIVE_FIGHTER_ACTOR_CAPACITY;++i)if(slots[i].active && slots[i].fighter_kind==kind)return false;
    if(!ftPartsTable)Fighter_LoadCommonData();
    if(!family || !prepare_family(family,members,id,character,kind,root,require_installed))return false;
    if(!can_bind(family)){release_family(family);return false;}
    unsigned pinned=0;
    for(;pinned<family->metadata.member_count;++pinned)
        if(!nativeSourcePackage_Pin(members[pinned].source))break;
    if(pinned!=family->metadata.member_count) {
        while(pinned)nativeSourcePackage_Unpin(members[--pinned].source);
        release_family(family);return false;
    }
    bind_family(family);return true;
}

#endif
