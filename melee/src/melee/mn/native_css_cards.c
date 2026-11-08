#include "native_css_cards.h"
#include "native_css_original.h"
#include "native_menu_font.h"
#include "native_menu_portrait.h"
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_stock_content.h>
#include <melee/pl/forward.h>
#include <melee/lb/lbspdisplay.h>
#include <melee/mn/types.h>
#include <melee/sc/types.h>
#include <melee/ui/native_hub.h>
#include <melee/ui/native_hub_startup_stamp.h>
#include <melee/ui/native_play_menu.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <dolphin/gx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    BACK,
    EMBLEM,
    DOOR,
    INDICATOR,
    PARTS
};
typedef struct CardPart {
    HSD_Joint descriptor;
    HSD_JObj* joint;
    Mtx parent;
} CardPart;
typedef struct CardArt {
    CardPart parts[PARTS];
    NativeMenuPortrait portrait;
    NativeLobbyPlayerSelection loaded_choice;
    char outfit[NATIVE_CONTENT_TITLE_BYTES];
    float original_left;
    unsigned appearance;
    bool attempted;
} CardArt;
typedef struct CarriedArt {
    NativeMenuPortrait portrait;
    NativeContentId id;
    float x, y;
} CarriedArt;
static struct {
    CSSData* css;
    HSD_GObj* owner;
    NativeCssParticipants people;
    CardArt art[4];
    CarriedArt carried[4];
    bool invalidate;
    uint32_t cache_revision;
} cards;

static bool nonzero(const NativeContentId* id)
{
    static const NativeContentId empty = { 0 };
    return id && memcmp(id, &empty, sizeof(empty));
}
static bool find_part(CardPart* part, unsigned wanted, unsigned* index,
                      HSD_Joint* joint, HSD_AnimJoint* animation,
                      HSD_MatAnimJoint* material, HSD_JObj* menu)
{
    if (!joint) {
        return false;
    }
    if ((*index)++ == wanted) {
        part->descriptor = *joint;
        part->descriptor.next = NULL;
        part->joint = HSD_JObjLoadJoint(&part->descriptor);
        if (!part->joint) {
            return false;
        }
        HSD_JObjAddAnimAll(part->joint, animation, material, NULL);
        HSD_JObj* original = NULL;
        lb_80011E24(menu, &original, wanted, -1);
        if (!original) {
            return false;
        }
        if (original->parent) {
            HSD_JObjSetupMatrix(original->parent);
            memcpy(part->parent, original->parent->mtx, sizeof(Mtx));
        } else {
            C_MTXIdentity(part->parent);
        }
        return true;
    }
    HSD_Joint* child = joint->child;
    HSD_AnimJoint* ca = animation ? animation->child : NULL;
    HSD_MatAnimJoint* cm = material ? material->child : NULL;
    while (child) {
        if (find_part(part, wanted, index, child, ca, cm, menu)) {
            return true;
        }
        child = child->next;
        ca = ca ? ca->next : NULL;
        cm = cm ? cm->next : NULL;
    }
    return false;
}
static void pose(CardPart* part, float frame)
{
    HSD_JObjReqAnimAll(part->joint, frame);
    HSD_JObjAnimAll(part->joint);
    HSD_ForeachAnim(part->joint, JOBJ_TYPE, ALL_TYPE_MASK, HSD_AObjStopAnim,
                    AOBJ_ARG_AOV, 0, 0);
}
static void destroy(void* unused)
{
    (void) unused;
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned p = 0; p < PARTS; ++p) {
            HSD_JObjRemoveAll(cards.art[i].parts[p].joint);
        }
        nativeMenuPortrait_Free(&cards.art[i].portrait);
        nativeMenuPortrait_Free(&cards.carried[i].portrait);
    }
    nativeMenuFont_Release();
    memset(&cards, 0, sizeof(cards));
}
static float card_left(unsigned index)
{
    return -(float) cards.people.count * 7.7f + index * 15.4f + .2f;
}
static void flat_state(Mtx view)
{
    GXLoadPosMtxImm(view, 0);
    GXSetCurrentMtx(0);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetNumChans(0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
                      GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetCullMode(GX_CULL_NONE);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetFog(GX_FOG_NONE, 0, 0, 0, 0, (GXColor){ 0 });
}
static void image(const NativeMenuPortrait* portrait, float x, float y,
                  float w, float h)
{
    if (!portrait->pixels) {
        return;
    }
    GXTexObj texture;
    GXInitTexObj(&texture, portrait->pixels, portrait->width, portrait->height,
                 GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&texture, GX_TEXMAP0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(x, y, .6f);
    GXTexCoord2f32(0, 0);
    GXPosition3f32(x + w, y, .6f);
    GXTexCoord2f32(1, 0);
    GXPosition3f32(x + w, y - h, .6f);
    GXTexCoord2f32(1, 1);
    GXPosition3f32(x, y - h, .6f);
    GXTexCoord2f32(0, 1);
    GXEnd();
}
static void draw(HSD_GObj* object, int pass)
{
    (void) object;
    if (pass < 0 || pass > 2) {
        return;
    }
    Mtx view;
    if (!nativeCssOriginal_ViewMatrix(view)) {
        return;
    }
    if (nativeHub_IsVisible()) {
        if (pass == 2) nativeHubStartupStamp_DrawCss(view);
        return;
    }
    HSD_StateInvalidate(-1);
    if (cards.people.online) {
        for (unsigned i = 0; i < cards.people.count; ++i) {
            CardArt* art = &cards.art[i];
            float x = card_left(i);
            Mtx shift, instance, parent;
            C_MTXTrans(shift, x - art->original_left, 0, 0);
            C_MTXConcat(view, shift, instance);
            for (unsigned p = 0; p < PARTS; ++p) {
                C_MTXConcat(instance, art->parts[p].parent, parent);
                HSD_JObjDispAll(art->parts[p].joint, parent,
                                HSD_GObj_80390EB8(pass), 0);
            }
        }
    }
    if (pass != 2) {
        return;
    }
    flat_state(view);
    {
        for (unsigned i = 0; i < cards.people.count; ++i) {
            CardArt* art = &cards.art[i];
            const NativeCssParticipant* person = &cards.people.cards[i];
            if (!cards.people.online &&
                (!person->choice.selected ||
                 !nonzero(&person->choice.character_package_id)))
            {
                continue;
            }
            float x = cards.people.online
                          ? card_left(i)
                          : cards.art[person->physical_port].original_left;
            if (art->portrait.pixels) {
                float w = 13,
                      h = w * art->portrait.height / art->portrait.width;
                if (h > 13) {
                    w *= 13 / h;
                    h = 13;
                }
                image(&art->portrait, x + (14 - w) * .5f, -7 - (13 - h) * .5f,
                      w, h);
            }
            nativeMenuFont_Draw(NativeMenuFont_Notice, person->name, x + .4f,
                                -4.8f, .8f, 13.2f, 1, 0xF5EFEAFF, true, true);
            const char* title = person->kind == Gm_PKind_NA ? "Waiting for player"
                                : person->choice.selected ? art->portrait.title
                                                        : "Choose a fighter";
            nativeMenuFont_Draw(NativeMenuFont_Tile, title, x + .4f, -20.5f,
                                .8f, 13.2f, 1.15f, 0xF5EFEAFF, true, true);
            if (art->outfit[0])
                nativeMenuFont_Draw(NativeMenuFont_Notice, art->outfit,
                    x + .4f, -23.2f, .8f, 13.2f, .65f, 0xF5EFEAFF, true, true);
            nativeMenuFont_Draw(NativeMenuFont_Notice,
                                person->kind == Gm_PKind_NA ? "Quick Play"
                                : person->ready             ? "Ready"
                                : person->choice.selected ? "Selected"
                                                          : "Choosing...",
                                x + .4f, -22, .8f, 13.2f, .75f,
                                person->ready ? 0x99DEB2FF : 0xE3CF96FF, true, true);
        }
    }
    flat_state(view);
    for (unsigned port = 0; port < 4; ++port) {
        CarriedArt* carried = &cards.carried[port];
        if (!nonzero(&carried->id)) {
            continue;
        }
        image(&carried->portrait, carried->x, carried->y, 6, 4.3f);
        nativeMenuFont_Draw(NativeMenuFont_Tile, carried->portrait.title,
                            carried->x, carried->y - 4.3f, .8f, 6, .9f,
                            0xFFF3DCFF, true, true);
        nativeMenuFont_Draw(NativeMenuFont_Notice,
                            "A Give   X Recipients   B Cancel", carried->x - 3,
                            carried->y - 5.5f, .8f, 13, .65f, 0xFFF3DCFF,
                            true, true);
    }
    nativePlayMenu_DrawCss(view);
    HSD_StateInvalidate(-1);
    HSD_StateInitTev();
    HSD_ClearVtxDesc();
}
void nativeCssCards_Begin(CSSData* css, CSSDoorsData* doors, HSD_JObj* menu,
                          const StaticModelDesc* source)
{
    nativeCssCards_End();
    cards.css = css;
    nativeMenuFont_Acquire();
    cards.owner = GObj_Create(0xFE02, 5, 0x80);
    if (!cards.owner) {
        destroy(NULL);
        return;
    }
    GObj_InitUserData(cards.owner, 0, destroy, NULL);
    GObj_SetupGXLink(cards.owner, draw, 1, 0xFE);
    for (unsigned i = 0; i < 4; ++i) {
        unsigned indices[PARTS] = { doors->doors[i].bg_joint,
                                    doors->doors[i].emblem_joint,
                                    doors->doors[i].door_joint,
                                    doors->doors[i].player_indicator_joint };
        for (unsigned p = 0; p < PARTS; ++p) {
            unsigned index = 0;
            if (!find_part(&cards.art[i].parts[p], indices[p], &index,
                           source->joint, source->animjoint,
                           source->matanim_joint, menu))
            {
                fprintf(stderr,
                        "Native CSS card component %u is unavailable\n",
                        indices[p]);
                nativeCssCards_End();
                return;
            }
        }
        HSD_JObj* background = NULL;
        lb_80011E24(menu, &background, indices[BACK], -1);
        HSD_JObjSetupMatrix(background);
        cards.art[i].original_left = background->mtx[0][3];
        cards.art[i].appearance = ~0U;
    }
}
void nativeCssCards_End(void)
{
    if (cards.owner) {
        HSD_GObjFree(cards.owner);
    }
}
void nativeCssCards_Update(void)
{
    if (!cards.css) {
        return;
    }
    uint32_t revision = nativeOnlineSession_CacheRevision();
    if (revision != cards.cache_revision) {
        cards.invalidate = true;
        cards.cache_revision = revision;
    }
    NativeCssLocalSlot local[4] = { 0 };
    for (unsigned port = 0; port < 4; ++port) {
        const PlayerInitData* player = &cards.css->vs.start.players[port];
        local[port].kind = player->slot_type;
        NativeLobbyPlayerSelection* choice = &local[port].choice;
        const NativeContentId* package = nativeHome_FighterPackage(port);
        if (package) {
            choice->character_package_id = *package;
            choice->selected = true;
        } else if (player->ckind < CKind_Playable_Count) {
            choice->character_id =
                nativeStockContent_CharacterId(player->ckind);
            choice->costume_id =
                nativeStockContent_CostumeId(player->ckind, player->color);
            choice->selected = true;
        }
        if (choice->selected) {
            choice->team = player->team;
            const NativeContentId* outfit = nativeHome_CostumePackage(port);
            if (outfit) choice->costume_package_id = *outfit;
        }
    }
    const NativeQuickAssignment* quick = nativeOnlineSession_Quick();
    if (!nativeCssParticipants_Build(
            nativeOnlineSession_Presence(), nativeOnlineSession_LocalPeerId(),
            nativeHub_LocalName(), local, quick ? quick->mode * 2 : 0,
            quick && quick->state == NativeQuickState_Searching, &cards.people))
    {
        memset(&cards.people, 0, sizeof(cards.people));
    }
    nativeCssOriginal_HidePlayerCards(cards.people.online || nativeHub_IsVisible());
    static const unsigned colors[4] = { 2, 0, 8, 1 };
    for (unsigned i = 0; i < cards.people.count; ++i) {
        CardArt* art = &cards.art[i];
        const NativeCssParticipant* person = &cards.people.cards[i];
        if (cards.invalidate || !art->attempted ||
            memcmp(&art->loaded_choice, &person->choice,
                   sizeof(person->choice)))
        {
            art->attempted = true;
            art->loaded_choice = person->choice;
            art->outfit[0] = 0;
            if (nonzero(&person->choice.costume_package_id)) {
                char default_root[512];
                const char* root = nativeMatchLaunch_ContentRoot();
                if (!root && nativeContentStore_DefaultRoot(default_root, sizeof(default_root))) root = default_root;
                uint8_t* bytes = NULL;
                size_t size;
                NativeCostumePackage metadata;
                NativeCostumePackageData data;
                if (root && nativeContentStore_Read(root, &person->choice.costume_package_id, &bytes, &size) &&
                    nativeContentStore_ParseCostume(bytes, size, &metadata, &data))
                    memcpy(art->outfit, metadata.title, sizeof(art->outfit));
                free(bytes);
            }
            nativeMenuPortrait_Load(&art->portrait,
                                    &person->choice.character_package_id,
                                    person->choice.character_id);
        }
        unsigned appearance = person->kind;
        if (appearance != art->appearance) {
            pose(&art->parts[BACK],
                 person->kind == Gm_PKind_Human ? colors[i] : 7);
            pose(&art->parts[EMBLEM],
                 person->kind == Gm_PKind_Human ? colors[i] : 7);
            pose(&art->parts[DOOR], 29);
            pose(&art->parts[INDICATOR],
                 person->kind == Gm_PKind_Human ? 1 : 0);
            art->appearance = appearance;
        }
    }
    cards.invalidate = false;
}
bool nativeCssCards_Active(void)
{
    return cards.css && cards.people.online;
}
unsigned nativeCssCards_DisplayPort(unsigned physical_port)
{
    for (unsigned i = 0; cards.people.online && i < cards.people.count; ++i) {
        if (cards.people.cards[i].local &&
            cards.people.cards[i].physical_port == physical_port)
        {
            return i;
        }
    }
    return physical_port;
}
const NativeCssParticipant* nativeCssCards_Hit(float x, float y)
{
    if (!cards.people.online || y > -2.5f || y < -26) {
        return NULL;
    }
    for (unsigned i = 0; i < cards.people.count; ++i) {
        if (x >= card_left(i) && x < card_left(i) + 14.8f) {
            return cards.people.cards[i].kind == Gm_PKind_NA ? NULL : &cards.people.cards[i];
        }
    }
    return NULL;
}
void nativeCssCards_Carry(unsigned port, const NativeContentId* package,
                          float x, float y)
{
    if (port >= 4) {
        return;
    }
    CarriedArt* art = &cards.carried[port];
    NativeContentId id = package ? *package : (NativeContentId){ 0 };
    if (memcmp(&art->id, &id, sizeof(id))) {
        art->id = id;
        nativeMenuPortrait_Free(&art->portrait);
        if (nonzero(&id)) {
            nativeMenuPortrait_Load(&art->portrait, &id, 0);
        }
    }
    art->x = x;
    art->y = y;
}
void nativeCssCards_InvalidateArt(void)
{
    cards.invalidate = true;
}
void nativeCssCards_Inspect(void)
{
    NativeNetplayContentProgress progress = {0};
    bool downloading = nativeOnlineSession_ContentProgress(&progress);
    fprintf(stderr, "[css-cache] revision=%u receiving=%u bytes=%u total=%u\n",
            nativeOnlineSession_CacheRevision(), downloading,
            progress.bytes_done, progress.bytes_total);
    fprintf(stderr, "[css-cards] online=%u count=%u\n", cards.people.online,
            cards.people.count);
    for (unsigned i = 0; i < cards.people.count; ++i) {
        const NativeCssParticipant* p = &cards.people.cards[i];
        fprintf(stderr,
                "[css-card] display=%u peer=%s seat=%u physical=%u "
                "canonical=%u local=%u ready=%u selected=%u center=%.3f,-14 portrait=%u title=%s outfit=%s\n",
                i, p->peer, p->seat, p->physical_port, p->canonical_port,
                p->local, p->ready, p->choice.selected, card_left(i) + 7.4f,
                cards.art[i].portrait.pixels != NULL, cards.art[i].portrait.title, cards.art[i].outfit);
    }
}
