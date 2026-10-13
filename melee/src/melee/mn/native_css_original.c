#include "native_css_original.h"
#include "native_menu_visuals.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far

#include <melee/gm/native_stock_content.h>
#include <melee/gm/native_match_launch.h>
#include <melee/ft/native_source_asset_facts.h>
#include <melee/lb/lbspdisplay.h>
#include <melee/mn/types.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/jobj.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { STOCK_ICONS = 25 };

static NativeMenuVisuals* menu_visuals;
static CSSIcon* scene_icons;
static HSD_JObj* icon_roots[STOCK_ICONS];
static Vec3 root_translate[STOCK_ICONS];
static Vec3 root_scale[STOCK_ICONS];
static float stock_bounds[STOCK_ICONS][4];
static bool bounds_captured;
static bool editable;
static char layout_path[MAX_PATH];
static NativeCssLayout layout;
static NativeCssLayout saved_layout;
static NativeCssLayout defaults;
static float world_left, world_right, world_top, world_bottom;
static bool fighter_available[NATIVE_CSS_LAYOUT_MAX_TILES];

static bool installed_fighter(const NativeContentId* id)
{
    char default_root[MAX_PATH];
    const char* root = nativeMatchLaunch_ContentRoot();
    if (root == NULL) {
        if (!nativeContentStore_DefaultRoot(default_root,
                                            sizeof(default_root))) {
            return false;
        }
        root = default_root;
    }
    uint8_t* package = NULL;
    size_t package_size = 0;
    NativeFighterPackage fighter;
    NativeFighterPackageData data;
    bool valid = nativeContentStore_IsInstalled(root, id) &&
        nativeContentStore_Read(root, id, &package, &package_size) &&
        nativeContentStore_ParseFighter(package, package_size, &fighter,
                                        &data);
    NativeSourceAssetFacts facts;
    valid = valid && fighter.mode == NativeFighterFamily_Single &&
        fighter.member_count == 1 && data.members[0].script_size != 0 &&
        nativeSourceAssets_Read(data.members[0].asset_record,
                               data.members[0].asset_record_size, &facts, NULL);
    free(package);
    return valid;
}

static void refresh_fighters(void)
{
    memset(fighter_available, 0, sizeof(fighter_available));
    for (unsigned i = 0; i < layout.count; ++i) {
        if (layout.tiles[i].stock_character_id == 0) {
            fighter_available[i] =
                installed_fighter(&layout.tiles[i].fighter_package_id);
        }
    }
}

static uint16_t normalized(float value)
{
    long rounded = lroundf(value);
    return (uint16_t) (rounded < 0 ? 0 : rounded > 1000 ? 1000 : rounded);
}

static float world_x(uint16_t value)
{
    return world_left + (world_right - world_left) * value / 1000.0f;
}

static float world_y(uint16_t value)
{
    return world_top - (world_top - world_bottom) * value / 1000.0f;
}

void nativeCssOriginal_RestoreBounds(CSSIcon* icons)
{
    if (!bounds_captured) {
        for (unsigned i = 0; i < STOCK_ICONS; ++i) {
            stock_bounds[i][0] = icons[i].bound_l;
            stock_bounds[i][1] = icons[i].bound_r;
            stock_bounds[i][2] = icons[i].bound_u;
            stock_bounds[i][3] = icons[i].bound_d;
        }
        bounds_captured = true;
    } else {
        for (unsigned i = 0; i < STOCK_ICONS; ++i) {
            icons[i].bound_l = stock_bounds[i][0];
            icons[i].bound_r = stock_bounds[i][1];
            icons[i].bound_u = stock_bounds[i][2];
            icons[i].bound_d = stock_bounds[i][3];
        }
    }
}

static void make_defaults(void)
{
    memset(&defaults, 0, sizeof(defaults));
    defaults.count = STOCK_ICONS;
    const float width = world_right - world_left;
    const float height = world_top - world_bottom;
    for (unsigned i = 0; i < STOCK_ICONS; ++i) {
        NativeCssTile* tile = &defaults.tiles[i];
        const CSSIcon* icon = &scene_icons[i];
        tile->stock_character_id = nativeStockContent_CharacterId(icon->char_kind);
        tile->x = normalized((icon->bound_l - world_left) * 1000.0f / width);
        tile->y = normalized((world_top - icon->bound_u) * 1000.0f / height);
        tile->width = normalized((icon->bound_r - icon->bound_l) * 1000.0f / width);
        tile->height = normalized((icon->bound_u - icon->bound_d) * 1000.0f / height);
    }
}

static const NativeCssTile* tile_for(unsigned icon)
{
    const uint64_t id = defaults.tiles[icon].stock_character_id;
    for (unsigned tile = 0; tile < layout.count; ++tile) {
        if (layout.tiles[tile].stock_character_id == id) {
            return &layout.tiles[tile];
        }
    }
    return NULL;
}

static void set_portrait_transform(unsigned icon, const NativeCssTile* tile)
{
    HSD_JObj* root = icon_roots[icon];
    const NativeCssTile* original = &defaults.tiles[icon];
    const float scale_x = (float) tile->width / original->width;
    const float scale_y = (float) tile->height / original->height;
    const float initial_left = world_x(original->x);
    const float initial_top = world_y(original->y);
    const float left = world_x(tile->x);
    const float top = world_y(tile->y);
    HSD_JObjSetScaleX(root, root_scale[icon].x * scale_x);
    HSD_JObjSetScaleY(root, root_scale[icon].y * scale_y);
    HSD_JObjSetTranslateX(root,
        root_translate[icon].x + left - initial_left -
        (scale_x - 1.0f) * (initial_left - root_translate[icon].x));
    HSD_JObjSetTranslateY(root,
        root_translate[icon].y + top - initial_top -
        (scale_y - 1.0f) * (initial_top - root_translate[icon].y));
}

static void apply_layout(void)
{
    if (scene_icons == NULL) {
        return;
    }
    for (unsigned i=0;i<layout.count;++i) {
        const NativeCssTile* t=&layout.tiles[i];
        nativeMenuVisuals_Rect(menu_visuals,i,world_x(t->x),world_y(t->y),
            (world_right-world_left)*t->width/1000.0f,
            (world_top-world_bottom)*t->height/1000.0f);
    }
    for (unsigned icon = 0; icon < STOCK_ICONS; ++icon) {
        const NativeCssTile* tile = tile_for(icon);
        HSD_JObj* root = icon_roots[icon];
        if (tile == NULL) {
            scene_icons[icon].state = 0;
            HSD_JObjSetFlags(root, JOBJ_HIDDEN);
            continue;
        }
        scene_icons[icon].state = 2;
        HSD_JObjClearFlags(root, JOBJ_HIDDEN);
        scene_icons[icon].bound_l = world_x(tile->x);
        scene_icons[icon].bound_r = world_x(tile->x + tile->width);
        scene_icons[icon].bound_u = world_y(tile->y);
        scene_icons[icon].bound_d = world_y(tile->y + tile->height);
        set_portrait_transform(icon, tile);
    }
}

void nativeCssOriginal_Enter(CSSIcon* icons, HSD_JObj* menu,
                             HSD_CObj* camera)
{
    scene_icons = icons;
    nativeMenuVisuals_Begin(&menu_visuals, menu, 1);
    Vec3 eye;
    HSD_CObjGetEyePosition(camera, &eye);
    const float half_height = (eye.z * tanf(HSD_CObjGetFov(camera) *
                                            0.00872664626f));
    const float half_width = half_height * HSD_CObjGetAspect(camera);
    world_left = eye.x - half_width;
    world_right = eye.x + half_width;
    world_top = eye.y + half_height;
    world_bottom = eye.y - half_height;
    for (unsigned i = 0; i < STOCK_ICONS; ++i) {
        HSD_JObj* joint;
        lb_80011E24(menu, &joint, icons[i].joint_id_vs, -1);
        switch (i) {
        case 0: case 8: case 9: case 17: case 18: case 24:
            joint = HSD_JObjGetParent(joint);
            break;
        }
        icon_roots[i] = joint;
        root_translate[i] = joint->translate;
        root_scale[i] = joint->scale;
    }
    nativeMenuVisuals_Text(menu_visuals, menu, world_x(300), world_y(95), 0,
                           (world_right-world_left)*0.50f,
                           (world_top-world_bottom)*0.048f, NativeMenuFont_Heading, "CHOOSE A FIGHTER");
    make_defaults();
    bool missing = false;
    editable = nativeCssLayout_DefaultPath(layout_path, sizeof(layout_path));
    if (!editable || !nativeCssLayout_LoadFile(&layout, layout_path, &missing)) {
        if (!missing) {
            fprintf(stderr, "Original CSS layout unavailable or damaged; "
                            "editing disabled for this run.\n");
            editable = false;
        }
        layout = defaults;
    }
    saved_layout = layout;
    apply_layout();
    refresh_fighters();
}

void nativeCssOriginal_Tick(void)
{
    if (scene_icons == NULL) {
        return;
    }
    for (unsigned icon = 0; icon < STOCK_ICONS; ++icon) {
        const NativeCssTile* tile = tile_for(icon);
        if (tile != NULL) {
            set_portrait_transform(icon, tile);
        }
    }
}

void nativeCssOriginal_Exit(void)
{
    nativeMenuVisuals_End(&menu_visuals);
    scene_icons = NULL;
    memset(icon_roots, 0, sizeof(icon_roots));
}

void nativeCssOriginal_ClearPortraits(void)
{
    nativeMenuVisuals_ClearSprites(menu_visuals);
}

void nativeCssOriginal_SetPortrait(unsigned index, const u8* png, unsigned bytes, const char* title)
{
    if (index >= layout.count || !scene_icons) return;
    if (nativeMenuVisuals_Sprite(menu_visuals, index, png, bytes, title, NativeMenuFont_Tile)) {
        const NativeCssTile* tile = &layout.tiles[index];
        nativeMenuVisuals_Rect(menu_visuals, index, world_x(tile->x), world_y(tile->y),
            (world_right-world_left)*tile->width/1000.0f,
            (world_top-world_bottom)*tile->height/1000.0f);
    }
}

const NativeCssLayout* nativeCssOriginal_Layout(void)
{
    return scene_icons != NULL ? &layout : NULL;
}

void nativeCssOriginal_ReloadInstalledFighters(void)
{
    if (scene_icons == NULL || !editable) return;
    NativeCssLayout disk;
    bool missing = false;
    if (!nativeCssLayout_LoadFile(&disk, layout_path, &missing)) return;
    layout = disk;
    saved_layout = disk;
    apply_layout();
    refresh_fighters();
}

bool nativeCssOriginal_CanEdit(void)
{
    return scene_icons != NULL && editable;
}

static bool save_layout(const NativeCssLayout* candidate)
{
    if (!nativeCssOriginal_CanEdit() ||
        !nativeCssLayout_SaveFile(candidate, layout_path)) {
        return false;
    }
    layout = *candidate;
    saved_layout = *candidate;
    apply_layout();
    refresh_fighters();
    return true;
}

bool nativeCssOriginal_UpdateTile(unsigned index, const NativeCssTile* tile)
{
    if (!nativeCssOriginal_CanEdit()) {
        return false;
    }
    NativeCssLayout candidate = layout;
    return nativeCssLayout_Update(&candidate, index, tile) &&
           save_layout(&candidate);
}

bool nativeCssOriginal_PreviewTile(unsigned index, const NativeCssTile* tile)
{
    if (!nativeCssOriginal_CanEdit()) {
        return false;
    }
    NativeCssLayout candidate = saved_layout;
    if (!nativeCssLayout_Update(&candidate, index, tile)) {
        return false;
    }
    layout = candidate;
    apply_layout();
    return true;
}

void nativeCssOriginal_CancelPreview(void)
{
    if (scene_icons != NULL) {
        layout = saved_layout;
        apply_layout();
    }
}

bool nativeCssOriginal_RemoveTile(unsigned index)
{
    if (!nativeCssOriginal_CanEdit()) {
        return false;
    }
    NativeCssLayout candidate = layout;
    return nativeCssLayout_Remove(&candidate, index) &&
           save_layout(&candidate);
}

bool nativeCssOriginal_AddStock(unsigned stock_index)
{
    if (!nativeCssOriginal_CanEdit() || stock_index >= STOCK_ICONS) {
        return false;
    }
    NativeCssLayout candidate = layout;
    return nativeCssLayout_Add(&candidate, &defaults.tiles[stock_index]) &&
           save_layout(&candidate);
}

bool nativeCssOriginal_AddInstalled(const NativeContentId* id)
{
    if (!nativeCssOriginal_CanEdit() || id == NULL) {
        return false;
    }
    if (!installed_fighter(id)) {
        return false;
    }
    NativeCssLayout candidate = layout;
    NativeCssTile tile = { 0 };
    tile.fighter_package_id = *id;
    tile.x = 45;
    tile.y = 405;
    tile.width = 92;
    tile.height = 120;
    return nativeCssLayout_Add(&candidate, &tile) && save_layout(&candidate);
}

bool nativeCssOriginal_HitTestFighter(float x, float y,
                                      NativeContentId* id, float* left,
                                      float* top)
{
    if (scene_icons == NULL || id == NULL) {
        return false;
    }
    for (unsigned i = layout.count; i > 0; --i) {
        const unsigned index = i - 1;
        const NativeCssTile* tile = &layout.tiles[index];
        if (fighter_available[index] &&
            x >= world_x(tile->x) && x < world_x(tile->x + tile->width) &&
            y <= world_y(tile->y) &&
            y > world_y(tile->y + tile->height)) {
            *id = tile->fighter_package_id;
            if (left != NULL) {
                *left = world_x(tile->x);
            }
            if (top != NULL) {
                *top = world_y(tile->y);
            }
            return true;
        }
    }
    return false;
}

bool nativeCssOriginal_FighterBounds(const NativeContentId* id,
                                      float* left, float* right,
                                      float* top, float* bottom)
{
    if (scene_icons == NULL || id == NULL) {
        return false;
    }
    for (unsigned i = 0; i < layout.count; ++i) {
        const NativeCssTile* tile = &layout.tiles[i];
        if (fighter_available[i] &&
            memcmp(&tile->fighter_package_id, id, sizeof(*id)) == 0) {
            if (left != NULL) *left = world_x(tile->x);
            if (right != NULL) *right = world_x(tile->x + tile->width);
            if (top != NULL) *top = world_y(tile->y);
            if (bottom != NULL) *bottom = world_y(tile->y + tile->height);
            return true;
        }
    }
    return false;
}

bool nativeCssOriginal_Reset(void)
{
    return save_layout(&defaults);
}

#endif
