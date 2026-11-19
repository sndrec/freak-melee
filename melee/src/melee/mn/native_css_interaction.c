#include "native_css_interaction.h"
#include "native_css_original.h"
#include "native_css_cards.h"
#include "native_css_remote.h"
#include "mncharsel.h"
#include "inlines.h"
#include <melee/ft/native_costume_catalog.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/ui/native_hub.h>
#include <melee/ui/native_play_menu.h>
#include <melee/gm/native_stock_content.h>
#include <melee/pl/forward.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbspdisplay.h>
#include <melee/mn/types.h>
#include <melee/sc/types.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/controller.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static struct {
    CSSData* css;
    CSSDoorsData* doors;
    CSSCursorData** hands;
    CSSCharModel** cookies;
    CSSIcon* icons;
    NativeContentId carried[4];
    bool carrying[4];
    struct {
        struct {
            float u, v;
            NativeCssPointerEvent kind;
        } events[32];
        unsigned count, port;
        float press_x, press_y;
        bool enabled, active, down, moved, place_on_release;
    } pointer;
} selection;

void nativeCssInteraction_DrawLocalCursor(HSD_GObj* object, int pass)
{
    const CSSCursorData* hand = HSD_GObjGetUserData(object);
    const bool remote_lobby = nativeOnlineSession_Presence() && !nativeOnlineSession_Quick();
    const bool spectator = remote_lobby && nativeOnlineSession_LocalPlayers() == 0;
    if (!nativeHub_IsVisible() && selection.doors && hand->x4 < 4 &&
        (selection.doors->doors[hand->x4].p_kind == Gm_PKind_Human ||
         (spectator && hand->x4 == 0))) {
        HSD_JObj* root = object->hsd_obj;
        HSD_JObj* number = NULL;
        if (remote_lobby && !nativeCssCards_HasLocalCard(hand->x4))
            lb_80011E24(root, &number, 3, -1);
        const bool restore = number && !(number->flags & JOBJ_HIDDEN);
        if (restore) HSD_JObjSetFlags(number, JOBJ_HIDDEN);
        HSD_JObjDispAll(root, NULL, HSD_GObj_80390EB8(pass), 0);
        if (restore) HSD_JObjClearFlags(number, JOBJ_HIDDEN);
    }
}

void nativeCssInteraction_DrawLocalToken(HSD_GObj* object, int pass)
{
    const CSSCharModel* token = HSD_GObjGetUserData(object);
    if (!nativeHub_IsVisible() && selection.doors && token->x4 < 4 &&
        selection.doors->doors[token->x4].p_kind != Gm_PKind_NA) {
        HSD_JObj* root = object->hsd_obj;
        HSD_JObj* number = NULL;
        if (nativeCssCards_Active() && !nativeOnlineSession_Quick() &&
            !nativeCssCards_HasLocalCard(token->x4))
            lb_80011E24(root, &number, 4, -1);
        const bool restore = number && !(number->flags & JOBJ_HIDDEN);
        if (restore) HSD_JObjSetFlags(number, JOBJ_HIDDEN);
        HSD_JObjDispAll(root, NULL, HSD_GObj_80390EB8(pass), 0);
        if (restore) HSD_JObjClearFlags(number, JOBJ_HIDDEN);
    }
}

void nativeCssInteraction_EnablePointer(bool enabled)
{
    selection.pointer.enabled = enabled && selection.css;
    if (!selection.pointer.enabled) {
        selection.pointer.active = selection.pointer.down = false;
        selection.pointer.count = 0;
    }
}
void nativeCssInteraction_CancelPointer(void)
{
    selection.pointer.active = selection.pointer.down = false;
    selection.pointer.count = 0;
}
bool nativeCssInteraction_Pointer(float u, float v,
                                  NativeCssPointerEvent event)
{
    if (!selection.pointer.enabled || !isfinite(u) || !isfinite(v)) {
        return false;
    }
    bool inside = u >= 0 && u <= 1 && v >= 0 && v <= 1;
    bool captured = selection.pointer.down;
    for (unsigned i = 0; i < selection.pointer.count; ++i) {
        if (selection.pointer.events[i].kind == NativeCssPointer_Press) {
            captured = true;
        }
        if (selection.pointer.events[i].kind == NativeCssPointer_Release) {
            captured = false;
        }
    }
    if (!inside && !captured) {
        return false;
    }
    if (!selection.pointer.active) {
        selection.pointer.port = 0;
        for (unsigned port = 0; port < 4; ++port) {
            if (selection.css->vs.start.players[port].slot_type ==
                Gm_PKind_Human)
            {
                selection.pointer.port = port;
                break;
            }
        }
    }
    selection.pointer.active = true;
    unsigned count = selection.pointer.count;
    if (event == NativeCssPointer_Move && count &&
        selection.pointer.events[count - 1].kind == NativeCssPointer_Move)
    {
        --count;
    }
    if (count == 32) {
        // Preserve button boundaries. Discard motion first if the host
        // delivered more events than a menu tick can consume.
        unsigned drop = 32;
        for (unsigned i = 0; i < count; ++i) {
            if (selection.pointer.events[i].kind == NativeCssPointer_Move) {
                drop = i;
                break;
            }
        }
        if (drop == 32) {
            return false;
        }
        memmove(&selection.pointer.events[drop],
                &selection.pointer.events[drop + 1],
                (count - drop - 1) * sizeof(selection.pointer.events[0]));
        --count;
    }
    selection.pointer.events[count].u = fmaxf(0, fminf(1, u));
    selection.pointer.events[count].v = fmaxf(0, fminf(1, v));
    selection.pointer.events[count].kind = event;
    selection.pointer.count = count + 1;
    return true;
}
bool nativeCssInteraction_PointerPort(unsigned port)
{
    return selection.pointer.enabled && selection.pointer.active &&
           selection.pointer.port == port;
}
static u32 pointer_frame(CSSCursorData* hand)
{
    u32 trigger = 0;
    while (selection.pointer.count) {
        float u = selection.pointer.events[0].u,
              v = selection.pointer.events[0].v;
        NativeCssPointerEvent event = selection.pointer.events[0].kind;
        memmove(selection.pointer.events, selection.pointer.events + 1,
                (--selection.pointer.count) *
                    sizeof(selection.pointer.events[0]));
        nativeCssOriginal_PointFromCanvas(u, v, &hand->xC, &hand->x10);
        if (selection.pointer.down) {
            float dx = hand->xC - selection.pointer.press_x,
                  dy = hand->x10 - selection.pointer.press_y;
            selection.pointer.moved |= dx * dx + dy * dy > .25f;
        }
        if (event == NativeCssPointer_Press) {
            selection.pointer.down = true;
            selection.pointer.press_x = hand->xC;
            selection.pointer.press_y = hand->x10;
            selection.pointer.moved = selection.pointer.place_on_release =
                false;
            trigger = PAD_BUTTON_A;
            break;
        }
        if (event == NativeCssPointer_Release) {
            if (selection.pointer.down && selection.pointer.moved &&
                selection.pointer.place_on_release)
            {
                trigger = PAD_BUTTON_A;
            }
            selection.pointer.down = false;
            break;
        }
        if (event == NativeCssPointer_Back) {
            selection.pointer.down = false;
            selection.pointer.place_on_release = false;
            trigger = PAD_BUTTON_B;
            break;
        }
    }
    return trigger;
}

void nativeCssInteraction_Begin(CSSData* css, CSSDoorsData* doors,
                                CSSCursorData** hands, CSSCharModel** cookies,
                                CSSIcon* icons, HSD_JObj* menu,
                                const StaticModelDesc* source,
                                const StaticModelDesc* hand_art,
                                const StaticModelDesc* token_art)
{
    nativeCostumeCatalog_Refresh();
    selection.css = css;
    selection.doors = doors;
    selection.hands = hands;
    selection.cookies = cookies;
    selection.icons = icons;
    nativeCssCards_Begin(css, doors, menu, source);
    nativeCssRemote_Begin(hand_art, token_art);
    for (unsigned port = 0; port < 4; ++port) {
        CSSCursorData* hand = hands[port];
        CSSCharModel* cookie = cookies[port];
        if (css->vs.start.players[port].slot_type == Gm_PKind_Human &&
            css->vs.start.players[port].ckind == ChKind_None &&
            !nativeHome_FighterPackage(port))
        {
            hand->x5 = 1;
            hand->x6 = port;
            cookie->x5 = port + 1;
        } else if (css->vs.start.players[port].slot_type == Gm_PKind_Cpu &&
                   css->vs.start.players[port].ckind == ChKind_None &&
                   !nativeHome_FighterPackage(port))
        {
            nativeCssInteraction_Random(port);
        }
    }
    if (nativeOnlineSession_State() != NativeLobbySessionState_Disconnected &&
        nativeOnlineSession_State() != NativeLobbySessionState_Failed)
        nativeCssInteraction_SetLocalPlayers(nativeHome_QuickSinglesPort()>=0?nativeHub_LocalPlayers():nativeOnlineSession_LocalPlayers());
}
void nativeCssInteraction_End(void)
{
    nativeCssRemote_End();
    nativeCssCards_End();
    memset(&selection, 0, sizeof(selection));
}

void nativeCssInteraction_PublishPointers(void)
{
    if (!selection.css || !nativeOnlineSession_Presence() ||
        nativeOnlineSession_Quick()) return;
    NativeCssPointers frame = { 0 };
    if (!nativeHub_IsVisible()) {
        float left, right, top, bottom;
        nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
        const unsigned local_players = nativeOnlineSession_LocalPlayers();
        for (unsigned port = 0, seat = 0; port < 4; ++port) {
            if (local_players ? selection.doors->doors[port].p_kind != Gm_PKind_Human
                              : port != 0) continue;
            if (local_players && seat >= local_players) break;
            unsigned slot = seat++;
            if (HSD_PadCopyStatus[port].err && !nativeCssInteraction_PointerPort(port))
                continue;
            const CSSCursorData* hand = selection.hands[port];
            const long x = lroundf((hand->xC - left) / (right - left) *
                                   NATIVE_CSS_POINTER_UNITS);
            const long y = lroundf((top - hand->x10) / (top - bottom) *
                                   NATIVE_CSS_POINTER_UNITS);
            frame.point[slot][0] = x < -1024 ? -1024 : x > 17408 ? 17408 : x;
            frame.point[slot][1] = y < 0 ? 0 : y > 16384 ? 16384 : y;
            frame.visible |= 1u << slot;
            if (local_players && hand->x5 == 1 && hand->x6 == port)
                frame.held |= 1u << slot;
        }
    }
    nativeCssRemote_Publish(&frame);
}
static bool chosen(unsigned port)
{
    return nativeHome_FighterPackage(port) ||
           selection.css->vs.start.players[port].ckind < CKind_Playable_Count;
}

int nativeCssInteraction_WaitingSeats(void)
{
    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    if (!selection.css || !presence || nativeOnlineSession_Quick()) return -1;
    if (presence->rotation_policy == NativeRoomRotation_Manual) return 0;
    unsigned active = nativeOnlineSession_LocalActiveSeats(), seat = 0, waiting = 0;
    for (unsigned port = 0; port < 4; ++port) {
        if (selection.css->vs.start.players[port].slot_type != Gm_PKind_Human)
            continue;
        const unsigned bit = 1u << seat++;
        const CSSCharModel* token = selection.cookies[port];
        NativeCssTile tile;
        if (!(active & bit) && !token->x5 && chosen(port) &&
            nativeCssOriginal_HitTestTile(token->x8, token->xC, &tile))
            waiting |= bit;
    }
    return (int)waiting;
}
static void grab(CSSCursorData* hand, unsigned slot)
{
    hand->x5 = 1;
    hand->x6 = slot;
    selection.cookies[slot]->x5 = hand->x4 + 1;
    if (nativeCssInteraction_PointerPort(hand->x4) && selection.pointer.down) {
        selection.pointer.place_on_release = true;
    }
    lbAudioAx_800237A8(0xB7, 0x7F, 0x40);
}
static void choose_tile(unsigned slot, const NativeCssTile* tile)
{
    if (nativeHome_QuickSinglesPort()>=0) {
        nativeHome_ClaimQuickSinglesPort(slot);
        float left,right,top,bottom; nativeCssOriginal_Viewport(&left,&right,&top,&bottom);
        for (unsigned other=0;other<4;++other) {
            if (other==slot || !chosen(other)) continue;
            nativeHome_SelectFighterPackage(other,NULL);
            selection.css->vs.start.players[other].ckind=ChKind_None;
            CSSDoor* door=&selection.doors->doors[other];
            door->sel_icon=25; door->selected_since_load=0;
            CSSCharModel* cookie=selection.cookies[other];
            cookie->x5=0; cookie->x8=cookie->x10=left+7+other*6; cookie->xC=cookie->x14=3;
            for (unsigned port=0;port<4;++port)
                if (selection.hands[port]->x5==1 && selection.hands[port]->x6==other) selection.hands[port]->x5=2;
            mnCharSel_8025DB34(other);
        }
    }

    CSSDoor* door = &selection.doors->doors[slot];
    if (!tile->stock_character_id) {
        const NativeContentId* previous = nativeHome_FighterPackage(slot);
        const bool changed = previous &&
            memcmp(previous, &tile->fighter_package_id, sizeof(*previous)) != 0;
        nativeHome_SelectFighterPackage(slot, &tile->fighter_package_id);
        selection.css->vs.start.players[slot].ckind = ChKind_None;
        selection.css->vs.start.players[slot].color = 0;
        door->sel_icon = 25;
        door->costume = 0;
        if (changed) sfxMove();
    } else {
        uint8_t character;
        if (nativeStockContent_CharacterFromId(tile->stock_character_id,
                                               &character))
        {
            nativeHome_SelectFighterPackage(slot, NULL);
            selection.css->vs.start.players[slot].ckind = character;
            for (unsigned i = 0; i < 25; ++i) {
                if (selection.icons[i].char_kind == character) {
                    door->sel_icon = i;
                    break;
                }
            }
        }
    }
    door->selected_since_load = 1;
    mnCharSel_8025DB34(slot);
}
static void place(CSSCursorData* hand)
{
    const unsigned slot = hand->x6;
    CSSCharModel* cookie = selection.cookies[slot];
    NativeCssTile tile;
    if (nativeCssOriginal_HitTestTile(cookie->x8, cookie->xC, &tile)) {
        choose_tile(slot, &tile);
    }
    // Position and fighter choice are independent. Empty space retains the
    // previous choice; an unchosen cookie can also be placed and picked up.
    cookie->x5 = 0;
    cookie->x10 = cookie->x8;
    cookie->x14 = cookie->xC;
    hand->x5 = 2;
    lbAudioAx_800237A8(0xB8, 0x7F, 0x40);
}
bool nativeCssInteraction_Random(unsigned slot)
{
    if (!selection.css || slot >= 4) {
        return false;
    }
    NativeCssTile tile;
    if (!nativeCssOriginal_RandomTile(&tile)) {
        return false;
    }
    choose_tile(slot, &tile);
    float left, right, top, bottom;
    nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
    CSSCharModel* cookie = selection.cookies[slot];
    cookie->x8 = cookie->x10 =
        left + (right - left) * (tile.x + tile.width * .5f) / 1000;
    cookie->xC = cookie->x14 =
        top - (top - bottom) * (tile.y + tile.height * .5f) / 1000;
    cookie->x5 = 0;
    for (unsigned port = 0; port < 4; ++port) {
        if (selection.hands[port]->x5 == 1 &&
            selection.hands[port]->x6 == slot)
        {
            selection.hands[port]->x5 = 2;
        }
    }
    return true;
}
static bool cycle_outfit(unsigned port, bool reverse)
{
    const NativeContentId* fighter = nativeHome_FighterPackage(port);
    if (!fighter) return false;
    const NativeContentId* current = nativeHome_CostumePackage(port);
    size_t count;
    const NativeCostumeEntry* entries = nativeCostumeCatalog_Entries(&count);
    // Choice zero is the fighter's bundled outfit; installed outfits follow.
    unsigned choices[NATIVE_COSTUME_CATALOG_CAPACITY], total = 0, selected = 0;
    for (size_t i = 0; i < count; ++i) {
        if (memcmp(&entries[i].metadata.fighter_id, fighter, sizeof(*fighter))) continue;
        choices[total++] = i;
        if (current && !memcmp(&entries[i].id, current, sizeof(*current))) selected = total;
    }
    if (!total) return true;
    unsigned next = reverse ? (selected ? selected - 1 : total) : (selected + 1) % (total + 1);
    if (nativeHome_SelectCostumePackage(port, next ? &entries[choices[next - 1]].id : NULL))
        lbAudioAx_80024030(2);
    return true;
}

bool nativeCssInteraction_Hand(CSSCursorData* hand, u32* menu_trigger)
{
    if (!selection.css) {
        return false;
    }
    unsigned port = hand->x4;
    unsigned kind = selection.doors->doors[port].p_kind;
    bool joining_local = kind == Gm_PKind_NA &&
                         nativeOnlineSession_State() == NativeLobbySessionState_Disconnected;
    const bool watching = port == 0 && nativeOnlineSession_Presence() &&
        !nativeOnlineSession_Quick() && !nativeOnlineSession_LocalPlayers();
    if (kind != Gm_PKind_Human && !joining_local && !watching) {
        *menu_trigger = 0;
        return true;
    }
    const HSD_PadStatus* pad = &HSD_PadCopyStatus[port];
    bool pointer = nativeCssInteraction_PointerPort(port);
    if (pad->err && !pointer) {
        return false;
    }
    u32 trigger = pad->err ? 0 : pad->trigger;
    if (pointer) {
        trigger |= pointer_frame(hand);
    }
    *menu_trigger = trigger;
    if (!selection.carrying[port] &&
        nativePlayMenu_Hand(hand->xC, hand->x10, trigger)) {
        return true;
    }
    if (watching) return true;
    if (selection.carrying[port]) {
        nativeCssCards_Carry(port, &selection.carried[port], hand->xC,
                             hand->x10);
        bool finished = false;
        if (trigger & PAD_BUTTON_B) {
            selection.pointer.place_on_release = false;
            finished = true;
        } else if (trigger & PAD_BUTTON_X) {
            finished =
                nativeHub_OpenFighterDelivery(&selection.carried[port], NULL);
        } else if (trigger & PAD_BUTTON_A) {
            const NativeCssParticipant* target =
                nativeCssCards_Hit(hand->xC, hand->x10);
            if (target && !target->local) {
                finished = nativeHub_OpenFighterDelivery(
                    &selection.carried[port], target->peer);
            } else {
                finished = true;
            }
        }
        if (finished) {
            selection.carrying[port] = false;
            hand->x5 = 2;
            nativeCssCards_Carry(port, NULL, 0, 0);
        }
        return true;
    }
    if ((trigger & PAD_BUTTON_A) &&
        nativeCssCards_ChangeSeat(hand->xC, hand->x10, port)) return true;
    if ((trigger & (PAD_BUTTON_X | PAD_BUTTON_Y)) &&
        cycle_outfit(port, (trigger & PAD_BUTTON_X) == 0)) return true;
    if (hand->x5 == 1 && hand->x6 >= 4) {
        return false;
    }
    if (hand->x5 == 1 && hand->x6 < 4) {
        CSSCharModel* cookie = selection.cookies[hand->x6];
        cookie->x8 = hand->xC + 2.7f;
        cookie->xC = hand->x10 - 2;
        if (trigger & PAD_BUTTON_A) {
            place(hand);
        }
        return true;
    }
    if (trigger & PAD_BUTTON_B) {
        if (selection.doors->doors[port].p_kind == Gm_PKind_NA) {
            return false;
        }
        if (selection.cookies[port]->x5 == 0) {
            grab(hand, port);
        }
        return true;
    }
    if (!(trigger & PAD_BUTTON_A)) {
        if (selection.doors->doors[port].sel_icon < 25 &&
            (trigger & (PAD_BUTTON_X | PAD_BUTTON_Y)))
        {
            mnCharSel_CostumeChange(port, trigger);
            return true;
        }
        // Online cards own token placement, including an ejected unchosen
        // token. Retail would automatically reattach that token to its hand.
        return nativeCssCards_Active();
    }
    // Pick up the closest permitted cookie. Human cookies stay with their
    // player; a local player can also position a configured CPU's cookie.
    unsigned closest = 4;
    float distance = 25;
    for (unsigned slot = 0; slot < 4; ++slot) {
        CSSDoor* door = &selection.doors->doors[slot];
        CSSCharModel* cookie = selection.cookies[slot];
        if (cookie->x5 || door->p_kind == Gm_PKind_NA ||
            (slot != port && door->p_kind != Gm_PKind_Cpu))
        {
            continue;
        }
        float dx = hand->xC + 2.7f - cookie->x10;
        float dy = hand->x10 - 2 - cookie->x14;
        float d = dx * dx + dy * dy;
        if (d < distance) {
            distance = d;
            closest = slot;
        }
    }
    if (closest < 4) {
        grab(hand, closest);
        return true;
    }
    NativeCssTile tile;
    if (!nativeCssOriginal_HitTestTile(hand->xC, hand->x10, &tile)) {
        // Online player panels represent room members. The stock CPU controls
        // underneath them must not change a local slot through a remote card.
        return nativeCssCards_Active();
    }
    if (nativeCssCards_Active() && !tile.stock_character_id) {
        selection.carried[port] = tile.fighter_package_id;
        selection.carrying[port] = true;
        hand->x5 = 1;
        hand->x6 = 255;
        if (pointer && selection.pointer.down) {
            selection.pointer.place_on_release = true;
        }
        nativeCssCards_Carry(port, &tile.fighter_package_id, hand->xC,
                             hand->x10);
        lbAudioAx_800237A8(0xB7, 0x7F, 0x40);
        return true;
    }
    CSSDoor* own = &selection.doors->doors[port];
    if (own->p_kind == Gm_PKind_NA) {
        own->p_kind = Gm_PKind_Human;
        selection.css->vs.start.players[port].slot_type = Gm_PKind_Human;
        mnCharSel_8025DB34(port);
    }
    grab(hand, port);
    return true;
}
void nativeCssInteraction_Cookie(CSSCharModel* cookie, HSD_JObj* joint)
{
    if (!selection.css) {
        return;
    }
    unsigned slot = cookie->x4;
    if (selection.doors->doors[slot].p_kind == Gm_PKind_NA) {
        HSD_JObjSetFlagsAll(joint, JOBJ_HIDDEN);
        return;
    }
    HSD_JObjClearFlagsAll(joint, JOBJ_HIDDEN);
    unsigned display = nativeCssCards_DisplayPort(slot);
    unsigned color =
        selection.doors->doors[slot].p_kind == Gm_PKind_Cpu ? 8 : display;
    if (selection.css->vs.start.rules.is_teams) {
        static const unsigned team_colors[3] = { 0, 1, 3 };
        color = team_colors[selection.doors->doors[slot].team % 3] +
                (selection.doors->doors[slot].p_kind == Gm_PKind_Cpu ? 4 : 0);
    }
    if (nativeCssCards_Active() && !nativeOnlineSession_Quick() &&
        !nativeCssCards_HasLocalCard(slot)) color = 8;
    ++cookie->x7;
    if (cookie->x6 != color || cookie->x7 >= 40) {
        HSD_JObj* child;
        lb_80011E24(joint, &child, 4, -1);
        HSD_ForeachAnim(child, JOBJ_TYPE, TOBJ_MASK, HSD_AObjReqAnim,
                        AOBJ_ARG_AF,
                        color < 4 ? (float) (display * 4) : 16.0f);
        HSD_JObjAnimAll(child);
        HSD_ForeachAnim(child, JOBJ_TYPE, TOBJ_MASK, HSD_AObjStopAnim,
                        AOBJ_ARG_AOV, 0, 0);
        lb_80011E24(joint, &child, 3, -1);
        HSD_ForeachAnim(child, JOBJ_TYPE, MOBJ_MASK, HSD_AObjReqAnim,
                        AOBJ_ARG_AF, (float) (color * 40));
        cookie->x6 = color;
        cookie->x7 = 0;
    }
    if (cookie->x5) {
        const CSSCursorData* hand = selection.hands[cookie->x5 - 1];
        cookie->x8 = hand->xC + 2.7f;
        cookie->xC = hand->x10 - 2;
    }
    cookie->x10 = cookie->x8;
    cookie->x14 = cookie->xC;
    Vec3 position = { cookie->x10, cookie->x14, 1 };
    nativeCssOriginal_TransformPoint(&position);
    HSD_JObjSetTranslate(joint, &position);
    HSD_JObjAnimAll(joint);
}
bool nativeCssInteraction_SetLocalPlayers(unsigned count)
{
    if (!selection.css || count < 1 || count > 4) return false;
    unsigned keep = 0;
    for (unsigned port = 0; port < 4 && count; ++port) {
        if (selection.css->vs.start.players[port].slot_type == Gm_PKind_Human) {
            keep |= 1U << port;
            --count;
        }
    }
    for (unsigned port = 0; port < 4 && count; ++port) {
        if (!(keep & (1U << port))) { keep |= 1U << port; --count; }
    }
    for (unsigned port = 0; port < 4; ++port) {
        uint8_t kind = keep & (1U << port) ? Gm_PKind_Human : Gm_PKind_NA;
        if (selection.doors->doors[port].p_kind == kind) continue;
        selection.css->vs.start.players[port].slot_type = kind;
        selection.doors->doors[port].p_kind = kind;
        if (kind == Gm_PKind_Human && !chosen(port)) {
            selection.hands[port]->x5 = 1;
            selection.hands[port]->x6 = port;
            selection.cookies[port]->x5 = port + 1;
        }
        if (kind == Gm_PKind_NA) {
            selection.carrying[port] = false;
            nativeCssCards_Carry(port, NULL, 0, 0);
        }
        mnCharSel_8025DB34(port);
    }
    return true;
}

unsigned nativeCssInteraction_ReadyPorts(void)
{
    if (!selection.css) return 0;
    int single=nativeHome_QuickSinglesPort();
    if (single>=0) return 1U<<single;
    bool online=nativeOnlineSession_Presence()!=NULL;
    unsigned seats=nativeOnlineSession_LocalActiveSeats(),mask=0,seat=0;
    for (unsigned port=0;port<4;++port) {
        unsigned kind=selection.doors->doors[port].p_kind;
        if (kind==Gm_PKind_NA) continue;
        if (!online || (kind==Gm_PKind_Human && (seats ? seats&(1U<<seat) : chosen(port)))) mask|=1U<<port;
        if (kind==Gm_PKind_Human) ++seat;
    }
    return mask;
}
bool nativeCssInteraction_CanReady(void)
{
    if (nativeHub_IsVisible() || !selection.css) {
        return false;
    }
    const NativeQuickAssignment* quick=nativeOnlineSession_Quick();
    if (quick && quick->state!=NativeQuickState_Searching && quick->state!=NativeQuickState_Assigned) return false;
    unsigned ports=nativeCssInteraction_ReadyPorts();
    for (unsigned port = 0; port < 4; ++port) {
        if (!(ports&(1U<<port))) continue;
        if (selection.carrying[port]) {
            return false;
        }
    }
    for (unsigned slot = 0; slot < 4; ++slot) {
        if (!(ports&(1U<<slot))) continue;
        if (!chosen(slot) || selection.cookies[slot]->x5) {
            return false;
        }
    }
    return ports != 0;
}
void nativeCssInteraction_Inspect(void)
{
    if (!selection.css) {
        NativeHomePracticeWatch watch;
        fprintf(stderr, "[css-selection] %s\n",
                nativeHome_IsPractice() ? "practice" : "inactive");
        return;
    }
    float left, right, top, bottom;
    nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
    fprintf(stderr, "[css-selection] ready=%u viewport=%.3f,%.3f,%.3f,%.3f\n",
            nativeCssInteraction_CanReady(), left, right, top, bottom);
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    for (unsigned i = 0; layout && i < layout->count; ++i) {
        const NativeCssTile* tile = &layout->tiles[i];
        fprintf(stderr, "[css-tile] index=%u center=%.3f,%.3f custom=%u\n", i,
                left + (right - left) * (tile->x + tile->width * .5f) / 1000,
                top - (top - bottom) * (tile->y + tile->height * .5f) / 1000,
                tile->stock_character_id == 0);
    }
    for (unsigned slot = 0; slot < 4; ++slot) {
        const CSSCursorData* hand = selection.hands[slot];
        const CSSCharModel* cookie = selection.cookies[slot];
        fprintf(
            stderr,
            "[css-slot] slot=%u kind=%u chosen=%u hand=%u holding=%u "
            "hand_pos=%.3f,%.3f cookie_pos=%.3f,%.3f character=%u custom=%u\n",
            slot, selection.doors->doors[slot].p_kind, chosen(slot), hand->x5,
            cookie->x5, hand->xC, hand->x10, cookie->x10, cookie->x14,
            selection.css->vs.start.players[slot].ckind,
            nativeHome_FighterPackage(slot) != NULL);
    }
}
