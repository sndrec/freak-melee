#include "ftcommon.h"
#include "fighter.h"
#include "ftdata.h"
#include "ft_081B.h"
#include "ft_0877.h"
#include "ft_0C88.h"
#include "ftcamera.h"
#include "ftchangeparam.h"
#include "ftcolanim.h"
#include "ftcoll.h"
#include "ftlib.h"
#include "ftmetal.h"
#include "native_callback_lifetime.h"
#include "native_fighter_runtime.h"
#include "native_source_package.h"
#include "native_script_fighter.h"
#include "kinds/ftCommon/ftpickupitem.h"
#include "types.h"
#include <string.h>
#include <melee/gm/gm_unsplit.h>
#include <melee/it/types.h>
#include <melee/it/it_26B1.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/kinds/it_2E5A.h>
#include <melee/it/kinds/itrabbitc.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/gobj.h>

typedef struct PartnerTransferIdentity {
    Fighter_GObj* object;
    Fighter* fighter;
    uint32_t generation;
    FighterKind kind;
    ftData* data;
    const NativeSourcePackage* package;
    const NativeScriptFighterProgram* program;
    const NativeScriptCompileBank* bank;
    const NativeScriptStorageLayout* layout;
    const NativeScriptStorageParameters* parameters;
    NativeScriptReceiverView view;
    uint64_t program_identity,layout_identity,parameter_identity;
    bool scripted;
} PartnerTransferIdentity;

typedef struct PartnerTransferItem {
    Item_GObj* object;
    Item* item;
    uint32_t generation;
    ItemKind kind;
    ItemLogicTable* logic;
    const NativeScriptItemProgram* program;
    const NativeScriptCompileBank* bank;
    const NativeScriptStorageLayout* layout;
    const NativeScriptStorageParameters* parameters;
    NativeScriptReceiverView view;
    uint64_t program_identity,layout_identity,parameter_identity;
    bool scripted;
} PartnerTransferItem;

static bool partner_transfer_source_capture(PartnerTransferIdentity* identity)
{
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(identity->kind);
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program(identity->kind);
    NativeScriptReceiverView view;
    if(!package || identity->kind<Ft_Kind_CustomFirst || identity->kind>=Ft_Kind_RuntimeCapacity ||
       identity->fighter->behavior_kind!=identity->kind || package->address!=package || !package->ready || !package->assets.ready ||
       package->target!=identity->kind || identity->data!=&package->assets.character.data ||
       gFtDataList[identity->kind]!=identity->data ||
       ftData_CharacterStateTables[identity->kind]!=package->assets.motions ||
       !program || program->bank!=&package->compiler.bank || program->identity!=package->program.identity ||
       nativeScriptFighter_Status(identity->object)!=NativeScriptVM_Ok ||
       !nativeScriptFighter_View(identity->object,identity->generation,&view) ||
       view.program!=&program->bank->declarations || !view.storage || !view.storage->live ||
       view.storage->owner!=0 || view.storage->generation!=identity->generation ||
       view.storage->identity!=identity->generation || view.storage->layout!=&program->bank->storage_layout ||
       view.storage->parameters!=&program->bank->storage_parameters)return false;
    identity->package=package;identity->program=program;identity->view=view;
    identity->bank=program->bank;identity->program_identity=program->identity;
    identity->layout=view.storage->layout;identity->parameters=view.storage->parameters;
    identity->layout_identity=program->bank->storage_layout.identity;
    identity->parameter_identity=program->bank->storage_parameters.identity;
    return true;
}

static bool partner_transfer_item_snapshot(PartnerTransferItem* identity,
                                          Item_GObj* object)
{
    identity->object = object;
    if (object == NULL) return true;
    if (!nativeCallbackSource_Snapshot(object, NativeCallbackSource_Item,
                                       &identity->generation)) return false;
    Item* item = object->user_data;
    if (item->native_removal_in_progress) return false;
    identity->item = item;
    identity->kind = item->kind;
    identity->logic = item->xB8_itemLogicTable;
    identity->scripted=nativeScriptItem_IsAttached(object) || item->native_script_owner!=0;
    if(identity->scripted) {
        NativeScriptReceiverView view;
        identity->program=nativeScriptItem_Spec(object);
        if(!identity->program || nativeScriptItem_Status(object)!=NativeScriptVM_Ok ||
           !nativeScriptItem_View(object,identity->generation,&view) || !view.storage ||
           !view.storage->live || !view.storage->layout || !view.storage->parameters ||
           view.program!=&identity->program->bank->declarations ||
           view.storage->layout!=&identity->program->bank->storage_layout ||
           view.storage->parameters!=&identity->program->bank->storage_parameters)return false;
        identity->view=view;identity->layout_identity=view.storage->layout->identity;
        identity->bank=identity->program->bank;identity->program_identity=identity->program->identity;
        identity->layout=view.storage->layout;identity->parameters=view.storage->parameters;
        identity->parameter_identity=view.storage->parameters->identity;
    }
    return true;
}

static bool partner_transfer_item_alive(const PartnerTransferItem* identity,
                                       Item_GObj* source_binding)
{
    if (source_binding != identity->object) return false;
    if (identity->object == NULL) return true;
    if (!nativeCallbackSource_Alive(identity->object, identity->generation,
                                    NativeCallbackSource_Item)) return false;
    Item* item = identity->object->user_data;
    if(item!=identity->item || item->kind!=identity->kind || item->xB8_itemLogicTable!=identity->logic ||
       item->native_removal_in_progress)return false;
    if(!identity->scripted)return !nativeScriptItem_IsAttached(identity->object) && !item->native_script_owner;
    NativeScriptReceiverView view;
    return nativeScriptItem_Status(identity->object)==NativeScriptVM_Ok &&
        nativeScriptItem_Spec(identity->object)==identity->program &&
        identity->program->bank==identity->bank && identity->program->identity==identity->program_identity &&
        nativeScriptItem_View(identity->object,identity->generation,&view) &&
        view.program==identity->view.program && view.storage==identity->view.storage && view.storage &&
        view.storage->live && view.storage->generation==identity->generation && view.storage->identity==identity->generation &&
        view.storage->owner==identity->program->owner && view.storage->layout==identity->layout && view.storage->parameters==identity->parameters &&
        view.storage->layout->identity==identity->layout_identity &&
        view.storage->parameters->identity==identity->parameter_identity;
}

// Transient synchronous reentrancy protection, never persisted gameplay state.
static bool partner_transfer_active[GM_MAX_PLAYERS];

static bool partner_transfer_identity_alive(const PartnerTransferIdentity* identity)
{
    if (!nativeCallbackSource_Alive(identity->object, identity->generation,
                                    NativeCallbackSource_Fighter)) return false;
    Fighter* fighter = identity->object->user_data;
    if(fighter!=identity->fighter || fighter->kind!=identity->kind || fighter->ft_data!=identity->data)return false;
    if(!identity->scripted)
        return !nativeScriptFighter_IsAttached(identity->object) && !nativeFighterRuntime_SourcePackage(identity->kind);
    NativeScriptReceiverView view;
    return nativeFighterRuntime_SourcePackage(identity->kind)==identity->package &&
        nativeScriptFighter_Program(identity->kind)==identity->program &&
        identity->program->bank==identity->bank && identity->bank==&identity->package->compiler.bank &&
        identity->program->identity==identity->program_identity && identity->program_identity==identity->package->program.identity &&
        gFtDataList[identity->kind]==identity->data &&
        ftData_CharacterStateTables[identity->kind]==identity->package->assets.motions &&
        nativeScriptFighter_Status(identity->object)==NativeScriptVM_Ok &&
        nativeScriptFighter_View(identity->object,identity->generation,&view) &&
        view.program==identity->view.program && view.storage==identity->view.storage && view.storage &&
        view.storage->live && view.storage->generation==identity->generation &&
        view.storage->identity==identity->generation && view.storage->owner==0 &&
        view.storage->layout==identity->layout && view.storage->parameters==identity->parameters &&
        view.storage->layout->identity==identity->layout_identity &&
        view.storage->parameters->identity==identity->parameter_identity;
}

bool ftCommon_ActivatePartner(Fighter_GObj* gobj, HSD_GObjEvent stock_finish)
{
    PartnerTransferIdentity source={0}, destination={0};
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter,
                                       &source.generation)) return false;
    Fighter* src = gobj->user_data;
    const int player = src->player_id;
    if (player < 0 || player >= GM_MAX_PLAYERS || partner_transfer_active[player] ||
        !Player_HasAlternateMember(player) ||
        Player_GetEntityAtIndex(player, 0) != gobj) return false;
    Fighter_GObj* dst_gobj = Player_GetEntityAtIndex(player, 1);
    if (dst_gobj == gobj ||
        !nativeCallbackSource_Snapshot(dst_gobj, NativeCallbackSource_Fighter,
                                       &destination.generation)) return false;
    Fighter* dst = dst_gobj->user_data;
    if (dst->player_id != player || src->x221F_b4 == dst->x221F_b4) return false;
    source.object = gobj; source.fighter = src; source.kind = src->kind;
    source.data = src->ft_data;
    destination.object = dst_gobj; destination.fighter = dst; destination.kind = dst->kind;
    destination.data = dst->ft_data;
    source.scripted=nativeScriptFighter_IsAttached(gobj) || nativeFighterRuntime_SourcePackage(source.kind)!=NULL;
    destination.scripted=nativeScriptFighter_IsAttached(dst_gobj) || nativeFighterRuntime_SourcePackage(destination.kind)!=NULL;
    bool scripted=source.scripted || destination.scripted;
    if(scripted) {
        if(!source.scripted || !destination.scripted || stock_finish ||
           !nativeFighterRuntime_AlternatePair(source.kind,destination.kind) ||
           !partner_transfer_source_capture(&source) || !partner_transfer_source_capture(&destination) ||
           !nativeScriptFighter_CanActivateForm(dst_gobj) ||
           !nativeScriptFighter_HasHook(dst_gobj,NativeScriptFighter_FormActivated))return false;
    } else if(!stock_finish)return false;
    PartnerTransferItem worn[2]={{0}}, held={0};
    if (src->ft_data == NULL || dst->ft_data == NULL ||
        dst->ft_data->x8 == NULL ||
        !partner_transfer_item_snapshot(&worn[0], src->x197C) ||
        !partner_transfer_item_snapshot(&worn[1], src->x1980) ||
        !partner_transfer_item_snapshot(&held, src->item_gobj) ||
        !nativeCallbackDispatch_Enter()) return false;
    partner_transfer_active[player] = true;
    bool completed = false;
    s32 tmp_bit;
#define TRANSFER_ALIVE() do { \
    if (!partner_transfer_identity_alive(&source) || \
        !partner_transfer_identity_alive(&destination) || \
        !partner_transfer_item_alive(&worn[0], src->x197C) || \
        !partner_transfer_item_alive(&worn[1], src->x1980) || \
        !partner_transfer_item_alive(&held, src->item_gobj)) goto transfer_done; \
} while (0)

    Player_SwapTransformedStates(src->player_id, src->x221F_b4, dst->x221F_b4);
    tmp_bit = src->x221F_b4;
    src->x221F_b4 = dst->x221F_b4;
    dst->x221F_b4 = tmp_bit;
    Fighter_UnkInitReset_80067C98(dst);
    // This actual common reset intentionally starts a new destination generation.
    // It does not invoke a user callback or clear the per-form script state.
    uint32_t reset_generation;
    if (!nativeCallbackSource_Snapshot(dst_gobj, NativeCallbackSource_Fighter,
                                       &reset_generation) ||
        dst_gobj->user_data != destination.fighter || dst->kind != destination.kind ||
        dst->ft_data != destination.data)
        goto transfer_done;
    destination.generation = reset_generation;
    // NewLife updates this same destination view in place. Preserve its bank,
    // state bytes and schema; only its native/storage generation may advance.
    TRANSFER_ALIVE();
    ftCommon_8007E2FC(dst_gobj);
    dst->cur_pos = src->cur_pos;
    dst->prev_pos = src->prev_pos;
    dst->pos_delta = src->pos_delta;
    dst->facing_dir = src->facing_dir;
    dst->dmg.x1830_percent = src->dmg.x1830_percent;
    Player_SetHPByIndex(dst->player_id, dst->x221F_b4, dst->dmg.x1830_percent);
    TRANSFER_ALIVE();
    dst->dmg.x18F0 = src->dmg.x18F0;
    dst->self_vel = src->self_vel;
    dst->ground_or_air = src->ground_or_air;
    dst->gr_vel = src->gr_vel;
    dst->xF0_ground_kb_vel = src->xF0_ground_kb_vel;
    dst->xF4_ground_attacker_shield_kb_vel =
        src->xF4_ground_attacker_shield_kb_vel;
    dst->xF8_playerNudgeVel.x = src->xF8_playerNudgeVel.x;
    dst->xF8_playerNudgeVel.y = src->xF8_playerNudgeVel.y;
    dst->input = src->input;
    dst->action_buffer = src->action_buffer;
    dst->action_buffer.checking = false;
    memcpy(dst->ucf_stick, src->ucf_stick, sizeof(dst->ucf_stick));
    dst->ucf_shield_drop_frames = src->ucf_shield_drop_frames;
    dst->x670_timer_lstick_tilt_x = src->x670_timer_lstick_tilt_x;
    dst->x671_timer_lstick_tilt_y = src->x671_timer_lstick_tilt_y;
    dst->trigger_analog_timer = src->trigger_analog_timer;
    dst->x673 = src->x673;
    dst->x674 = src->x674;
    dst->x675 = src->x675;
    dst->x679_x = src->x679_x;
    dst->x67A_y = src->x67A_y;
    dst->x67B = src->x67B;
    dst->x685 = src->x685;
    dst->x686 = src->x686;
    dst->x687 = src->x687;
    dst->x688 = src->x688;
    dst->x689 = src->x689;
    dst->x68A = src->x68A;
    dst->x68B = src->x68B;
    dst->x67C = src->x67C;
    dst->x67D = src->x67D;
    dst->x67E = src->x67E;
    dst->x681 = src->x681;
    dst->x682 = src->x682;
    dst->x680 = src->x680;
    dst->x683 = src->x683;
    dst->x684 = src->x684;
    dst->dmg.x18c4_source_ply = src->dmg.x18c4_source_ply;
    dst->dmg.x18C8 = src->dmg.x18C8;
    dst->x221F_b5 = src->x221F_b5;
    dst->dmg.x18CC = src->dmg.x18CC;
    dst->dmg.x18D0 = src->dmg.x18D0;
    dst->dmg.x18d4 = src->dmg.x18d4;
    dst->dmg.x18ec_instancehitby = src->dmg.x18ec_instancehitby;
    dst->x21FC_flag.u8 = src->x21FC_flag.u8;
    dst->x2221_b7 = src->x2221_b7;
    dst->x2221_b6 = src->x2221_b6;
    dst->x2221_b4 = src->x2221_b4;
    dst->x2221_b5 = src->x2221_b5;
    dst->x2104 = src->x2104;
    dst->dmg.x1910 = src->dmg.x1910;
    dst->x1968_jumpsUsed = src->x1968_jumpsUsed;
    dst->x1969_walljumpUsed = src->x1969_walljumpUsed;
    dst->shield_health = src->shield_health;
    dst->bury_timer_2 = src->bury_timer_2;
    dst->x2330 = src->x2330;
    dst->x2338 = src->x2338;
    dst->x2224_b2 = src->x2224_b2;
    if (src->x221D_b6) {
        ftColl_ApplyStarInvincibility(dst, src->x2004);
        TRANSFER_ALIVE();
        ftCo_800C0358(src, dst, 0x6B);
        TRANSFER_ALIVE();
        ft_800880D8(src);
        TRANSFER_ALIVE();
    } else {
        ftCo_800C0200(dst, 9);
        TRANSFER_ALIVE();
    }
    if ((src->x198C = 2) && (src->x1990 != 0)) {
        ftColl_8007B760(dst_gobj, src->x1990);
        TRANSFER_ALIVE();
    }
    dst->x2220_b5 = src->x2220_b5;
    dst->x2220_b6 = src->x2220_b6;
    dst->x2008 = src->x2008;
    ftLib_SetScale(dst_gobj, src->x34_scale.y);
    TRANSFER_ALIVE();
    if (src->is_metal) {
        ftCo_800C8348(dst_gobj, src->metal_timer, src->metal_health);
        TRANSFER_ALIVE();
        ftCo_800C8540(gobj);
        TRANSFER_ALIVE();
    }
    if (src->x2226_b4) {
        if (!src->x2226_b7) {
            ftCo_800C88D4(dst_gobj, src->x2030, 0);
            TRANSFER_ALIVE();
        }
        ftCo_800C8A64(gobj);
        TRANSFER_ALIVE();
    }
    dst->x221E_b4 = src->x221E_b4;
    if (src->x197C != NULL) {
        it_8026B9A8(src->x197C, dst_gobj, dst->ft_data->x8->x12);
        TRANSFER_ALIVE();
        it_802950D4(src->x197C, false);
        TRANSFER_ALIVE();
        ftCommon_8007F948(dst_gobj, src->x197C, src->x2014);
        TRANSFER_ALIVE();
        if (dst->x197C != worn[0].object) goto transfer_done;
        ftCommon_8007FA00(gobj);
        TRANSFER_ALIVE();
    } else {
        dst->x197C = NULL;
    }
    if (src->x1980 != NULL) {
        it_8026B9A8(src->x1980, dst_gobj, dst->ft_data->x8->x12);
        TRANSFER_ALIVE();
        ftCommon_8007FE84(dst_gobj, src->x1980, src->x2018, src->x2024);
        TRANSFER_ALIVE();
        if (dst->x1980 != worn[1].object) goto transfer_done;
        ftCommon_8007FDA0(gobj);
        TRANSFER_ALIVE();
    } else {
        dst->x1980 = NULL;
    }
    ftCo_800D105C(dst_gobj);
    TRANSFER_ALIVE();
#ifdef MELEE_NATIVE
    if(nativeScriptFighter_IsAttached(dst_gobj)&&nativeScriptFighter_Status(dst_gobj)!=NativeScriptVM_Ok)goto transfer_done;
#endif
    ftCamera_80076064(dst);
    TRANSFER_ALIVE();
    ft_800849EC(src, dst);
    TRANSFER_ALIVE();
    ft_80081C88(dst_gobj, src->x34_scale.y);
    TRANSFER_ALIVE();
    if (src->item_gobj != NULL) {
        dst->item_gobj = src->item_gobj;
        dst->x221E_b3 = src->x221E_b3;
        ftpickupitem_80094818(dst_gobj, 1);
        TRANSFER_ALIVE();
        if (dst->item_gobj != held.object) goto transfer_done;
        it_8026B9A8(src->item_gobj, dst_gobj, dst->ft_data->x8->x10);
        TRANSFER_ALIVE();
    }
    un_80322314();
    TRANSFER_ALIVE();
    ftCo_800BFD04(gobj);
    TRANSFER_ALIVE();
    if(scripted) {
        if(nativeScriptFighter_Run(dst_gobj,NativeScriptFighter_FormActivated,NULL,0, NULL,NULL)!=NativeScriptVM_Ok)
            goto transfer_done;
    } else stock_finish(dst_gobj);
    TRANSFER_ALIVE();
    completed = true;
transfer_done:
    partner_transfer_active[player] = false;
    nativeCallbackDispatch_Leave();
#undef TRANSFER_ALIVE
    return completed;
}
