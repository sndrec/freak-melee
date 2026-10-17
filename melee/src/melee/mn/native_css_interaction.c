#include "native_css_interaction.h"
#include "native_css_original.h"
#include "mncharsel.h"
#include <melee/gm/native_home.h>
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
} selection;

void nativeCssInteraction_Begin(CSSData* css, CSSDoorsData* doors,
                                CSSCursorData** hands, CSSCharModel** cookies,
                                CSSIcon* icons)
{
    selection.css = css;
    selection.doors = doors;
    selection.hands = hands;
    selection.cookies = cookies;
    selection.icons = icons;
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
}
void nativeCssInteraction_End(void)
{
    memset(&selection, 0, sizeof(selection));
}
static bool chosen(unsigned port)
{
    return nativeHome_FighterPackage(port) ||
           selection.css->vs.start.players[port].ckind < CKind_Playable_Count;
}
static void grab(CSSCursorData* hand, unsigned slot)
{
    hand->x5 = 1;
    hand->x6 = slot;
    selection.cookies[slot]->x5 = hand->x4 + 1;
    lbAudioAx_800237A8(0xB7, 0x7F, 0x40);
}
static void choose_tile(unsigned slot, const NativeCssTile* tile)
{
    CSSDoor* door = &selection.doors->doors[slot];
    if (!tile->stock_character_id) {
        nativeHome_SelectFighterPackage(slot, &tile->fighter_package_id);
        selection.css->vs.start.players[slot].ckind = ChKind_None;
        selection.css->vs.start.players[slot].color = 0;
        door->sel_icon = 25;
        door->costume = 0;
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
bool nativeCssInteraction_Hand(CSSCursorData* hand)
{
    if (!selection.css) {
        return false;
    }
    unsigned port = hand->x4;
    const HSD_PadStatus* pad = &HSD_PadCopyStatus[port];
    if (pad->err) {
        return false;
    }
    if (hand->x5 == 1 && hand->x6 >= 4) {
        return false;
    }
    if (hand->x5 == 1 && hand->x6 < 4) {
        CSSCharModel* cookie = selection.cookies[hand->x6];
        cookie->x8 = hand->xC + 2.7f;
        cookie->xC = hand->x10 - 2;
        if (pad->trigger & PAD_BUTTON_A) {
            place(hand);
        }
        return true;
    }
    if (pad->trigger & PAD_BUTTON_B) {
        if (selection.doors->doors[port].p_kind == Gm_PKind_NA) {
            return false;
        }
        if (selection.cookies[port]->x5 == 0) {
            grab(hand, port);
        }
        return true;
    }
    if (!(pad->trigger & PAD_BUTTON_A)) {
        if (selection.doors->doors[port].sel_icon < 25 &&
            (pad->trigger & (PAD_BUTTON_X | PAD_BUTTON_Y)))
        {
            mnCharSel_CostumeChange(port, pad->trigger);
            return true;
        }
        return false;
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
        return false;
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
    unsigned color =
        selection.doors->doors[slot].p_kind == Gm_PKind_Cpu ? 8 : slot;
    if (selection.css->vs.start.rules.is_teams) {
        static const unsigned team_colors[3] = { 0, 1, 3 };
        color = team_colors[selection.doors->doors[slot].team % 3] +
                (selection.doors->doors[slot].p_kind == Gm_PKind_Cpu ? 4 : 0);
    }
    ++cookie->x7;
    if (cookie->x6 != color || cookie->x7 >= 40) {
        HSD_JObj* child;
        lb_80011E24(joint, &child, 4, -1);
        HSD_ForeachAnim(child, JOBJ_TYPE, TOBJ_MASK, HSD_AObjReqAnim,
                        AOBJ_ARG_AF, color < 4 ? (float) (slot * 4) : 16.0f);
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
    HSD_JObjSetTranslateX(joint, cookie->x10);
    HSD_JObjSetTranslateY(joint, cookie->x14);
    HSD_JObjSetTranslateZ(joint, 1);
    HSD_JObjAnimAll(joint);
}
bool nativeCssInteraction_CanReady(void)
{
    if (!selection.css) {
        return false;
    }
    unsigned active = 0;
    for (unsigned slot = 0; slot < 4; ++slot) {
        if (selection.doors->doors[slot].p_kind == Gm_PKind_NA) {
            continue;
        }
        ++active;
        if (!chosen(slot) || selection.cookies[slot]->x5) {
            return false;
        }
    }
    return active != 0;
}
void nativeCssInteraction_Inspect(void)
{
    if (!selection.css) {
        NativeHomePracticeWatch watch;
        fprintf(stderr, "[css-selection] %s\n",
                nativeHome_PracticeWatch(0, &watch) ? "practice" : "inactive");
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
