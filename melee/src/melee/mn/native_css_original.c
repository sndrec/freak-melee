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
#include <sysdolphin/baselib/random.h>

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
enum { PORTRAIT_JOINTS = 512 };
static struct {
    HSD_JObj* joint;
    u32 hidden;
    unsigned icon;
    bool masked;
} portrait_joints[PORTRAIT_JOINTS];
static unsigned portrait_joint_count;
static bool visibility_masked;
static bool hide_player_cards;
static HSD_JObj* scene_root;
static HSD_CObj* scene_camera;
static Mtx initial_inverse;

static void capture_portrait_joints(HSD_JObj* joint, unsigned icon)
{
    if (!joint) return;
    if (portrait_joint_count == PORTRAIT_JOINTS) {
        fprintf(stderr, "CSS portrait hierarchy exceeds visibility capacity\n");
        abort();
    }
    portrait_joints[portrait_joint_count].joint = joint;
    portrait_joints[portrait_joint_count].icon = icon;
    portrait_joints[portrait_joint_count].masked = false;
    portrait_joints[portrait_joint_count++].hidden = joint->flags & JOBJ_HIDDEN;
    for (HSD_JObj* child = joint->child; child; child = child->next) {
        capture_portrait_joints(child, icon);
    }
}
void nativeCssOriginal_BeginAnimation(void)
{
    if (!visibility_masked) return;
    for (unsigned i = 0; i < portrait_joint_count; ++i) {
        if (!portrait_joints[i].masked) continue;
        if (portrait_joints[i].hidden) {
            HSD_JObjSetFlags(portrait_joints[i].joint, JOBJ_HIDDEN);
        } else {
            HSD_JObjClearFlags(portrait_joints[i].joint, JOBJ_HIDDEN);
        }
        portrait_joints[i].masked = false;
    }
    visibility_masked = false;
}
void nativeCssOriginal_HidePlayerCards(bool hidden)
{
    hide_player_cards = hidden;
}

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
    for (unsigned member = 0; valid && member < fighter.member_count; ++member) {
        valid = data.members[member].script_size != 0 &&
            nativeSourceAssets_Read(data.members[member].asset_record,
                                   data.members[member].asset_record_size, &facts, NULL);
    }
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
    nativeCssOriginal_BeginAnimation();
    for (unsigned i=0;i<layout.count;++i) {
        const NativeCssTile* t=&layout.tiles[i];
        nativeMenuVisuals_Rect(menu_visuals,i,world_x(t->x),world_y(t->y),
            (world_right-world_left)*t->width/1000.0f,
            (world_top-world_bottom)*t->height/1000.0f);
    }
    for (unsigned icon = 0; icon < STOCK_ICONS; ++icon) {
        const NativeCssTile* tile = tile_for(icon);
        if (tile == NULL) {
            scene_icons[icon].state = 0;
            continue;
        }
        scene_icons[icon].state = 2;
        scene_icons[icon].bound_l = world_x(tile->x);
        scene_icons[icon].bound_r = world_x(tile->x + tile->width);
        scene_icons[icon].bound_u = world_y(tile->y);
        scene_icons[icon].bound_d = world_y(tile->y + tile->height);
        set_portrait_transform(icon, tile);
    }
    nativeCssOriginal_Tick();
}

void nativeCssOriginal_Enter(CSSIcon* icons, HSD_JObj* menu,
                             HSD_CObj* camera)
{
    scene_icons = icons;
    scene_root = menu;
    scene_camera = camera;
    HSD_JObjSetupMatrix(menu);
    C_MTXInverse(menu->mtx, initial_inverse);
    portrait_joint_count = 0;
    visibility_masked = false;
    hide_player_cards = false;
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
        capture_portrait_joints(joint, i);
        root_translate[i] = joint->translate;
        root_scale[i] = joint->scale;
    }
    // Direct children 40 onward are the four player panels, portraits,
    // names, CPU controls and doors in MnSelectChrDataTable's menu model.
    static const unsigned player_groups[] = {
        40, 45, 50, 55, 60, 86, 111, 132, 140, 148, 156, 164
    };
    for (unsigned i = 0; i < sizeof(player_groups)/sizeof(*player_groups); ++i) {
        HSD_JObj* group = NULL;
        lb_80011E24(menu, &group, player_groups[i], -1);
        capture_portrait_joints(group, STOCK_ICONS);
    }
    nativeMenuVisuals_Text(menu_visuals, menu, world_x(300), world_y(95), 0,
                           (world_right-world_left)*0.50f,
                           (world_top-world_bottom)*0.033f, NativeMenuFont_Heading, "CHOOSE A FIGHTER");
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
    // Restore our previous mask before reapplying it. Scene animations may
    // independently hide parts of an unlocked portrait; preserve those bits.
    nativeCssOriginal_BeginAnimation();
    bool visible[STOCK_ICONS + 1];
    visible[STOCK_ICONS] = !hide_player_cards;
    for (unsigned icon = 0; icon < STOCK_ICONS; ++icon) {
        const NativeCssTile* tile = tile_for(icon);
        visible[icon] = tile != NULL;
        if (tile != NULL) {
            set_portrait_transform(icon, tile);
        }
    }
    for (unsigned i = 0; i < portrait_joint_count; ++i) {
        HSD_JObj* joint = portrait_joints[i].joint;
        portrait_joints[i].hidden = joint->flags & JOBJ_HIDDEN;
        portrait_joints[i].masked = !visible[portrait_joints[i].icon];
        if (portrait_joints[i].masked) HSD_JObjSetFlags(joint, JOBJ_HIDDEN);
    }
    visibility_masked = true;
}

void nativeCssOriginal_Exit(void)
{
    nativeMenuVisuals_End(&menu_visuals);
    scene_icons = NULL;
    scene_root = NULL;
    scene_camera = NULL;
    portrait_joint_count = 0;
    visibility_masked = false;
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
    if (id == NULL || (scene_icons && !editable)) {
        return false;
    }
    if (!installed_fighter(id)) {
        return false;
    }
    NativeCssLayout candidate = { 0 };
    char disk_path[MAX_PATH];
    if (scene_icons) {
        candidate = layout;
    } else {
        bool missing = false;
        if (!nativeCssLayout_DefaultPath(disk_path, sizeof(disk_path)) ||
            (!nativeCssLayout_LoadFile(&candidate, disk_path, &missing) &&
             !missing))
        {
            return false;
        }
    }
    for (unsigned i = 0; i < candidate.count; ++i) {
        if (!memcmp(&candidate.tiles[i].fighter_package_id, id, sizeof(*id))) {
            return true;
        }
    }
    NativeCssTile tile = { 0 };
    tile.fighter_package_id = *id;
    tile.width = 92;
    tile.height = 120;
    for (unsigned row = 0; row < 3; ++row) {
        tile.y = 205 + row * 145;
        for (unsigned column = 0; column < 9; ++column) {
            tile.x = 45 + column * 101;
            bool occupied = false;
            for (unsigned i = 0; i < candidate.count; ++i) {
                const NativeCssTile* other = &candidate.tiles[i];
                occupied |= tile.x < other->x + other->width &&
                            tile.x + tile.width > other->x &&
                            tile.y < other->y + other->height &&
                            tile.y + tile.height > other->y;
            }
            if (!occupied) {
                return nativeCssLayout_Add(&candidate, &tile) &&
                       (scene_icons
                            ? save_layout(&candidate)
                            : nativeCssLayout_SaveFile(&candidate, disk_path));
            }
        }
    }
    return false;
}

bool nativeCssOriginal_HitTestTile(float x, float y, NativeCssTile* result)
{
    if (!scene_icons || !result) {
        return false;
    }
    for (unsigned i = layout.count; i > 0; --i) {
        unsigned index = i - 1;
        const NativeCssTile* tile = &layout.tiles[index];
        if (!tile->stock_character_id && !fighter_available[index]) {
            continue;
        }
        if (x >= world_x(tile->x) && x < world_x(tile->x + tile->width) &&
            y <= world_y(tile->y) && y > world_y(tile->y + tile->height))
        {
            *result = *tile;
            return true;
        }
    }
    return false;
}
bool nativeCssOriginal_RandomTile(NativeCssTile* result)
{
    if (!scene_icons || !result) return false;
    unsigned count = 0;
    for (unsigned i = 0; i < layout.count; ++i) {
        const NativeCssTile* tile = &layout.tiles[i];
        if (tile->stock_character_id || fighter_available[i]) {
            if (HSD_Randi(++count) == 0) *result = *tile;
        }
    }
    return count != 0;
}
void nativeCssOriginal_Viewport(float* left, float* right, float* top, float* bottom)
{
    if (left) *left = world_left;
    if (right) *right = world_right;
    if (top) *top = world_top;
    if (bottom) *bottom = world_bottom;
}
static void menu_matrix(Mtx result)
{
    HSD_JObjSetupMatrix(scene_root);
    C_MTXConcat(scene_root->mtx, initial_inverse, result);
}
bool nativeCssOriginal_ViewMatrix(Mtx result)
{
    if (!scene_root || !scene_camera) return false;
    Mtx model, view;
    menu_matrix(model);
    HSD_CObjGetViewingMtx(scene_camera, view);
    C_MTXConcat(view, model, result);
    return true;
}
void nativeCssOriginal_TransformPoint(Vec3* point)
{
    if (!scene_root || !point) return;
    Mtx model;
    menu_matrix(model);
    C_MTXMultVec(model, point, point);
}
bool nativeCssOriginal_PointFromCanvas(float u, float v, float* x, float* y)
{
    if (!scene_root || !scene_camera || !x || !y || u < 0 || u > 1 || v < 0 || v > 1)
        return false;
    Mtx model, inverse, camera_to_menu;
    menu_matrix(model);
    if (!C_MTXInverse(model, inverse)) return false;
    C_MTXConcat(inverse, HSD_CObjGetInvViewingMtxPtr(scene_camera), camera_to_menu);
    float half = tanf(HSD_CObjGetFov(scene_camera) * .00872664626f);
    Vec3 origin = {0}, direction = {(2*u-1)*half*HSD_CObjGetAspect(scene_camera),
                                  (1-2*v)*half, -1};
    C_MTXMultVec(camera_to_menu, &origin, &origin);
    C_MTXMultVecSR(camera_to_menu, &direction, &direction);
    if (fabsf(direction.z) < .00001f) return false;
    float distance = -origin.z / direction.z;
    if (distance < 0) return false;
    *x = origin.x + direction.x*distance;
    *y = origin.y + direction.y*distance;
    return true;
}

bool nativeCssOriginal_PointToCanvas(float x, float y, float* u, float* v)
{
    Mtx view;
    if (!u || !v || !nativeCssOriginal_ViewMatrix(view)) return false;
    Vec3 point = { x, y, 0 };
    C_MTXMultVec(view, &point, &point);
    if (point.z >= -.00001f) return false;
    float half = tanf(HSD_CObjGetFov(scene_camera) * .00872664626f);
    *u = .5f + point.x / (-2 * point.z * half * HSD_CObjGetAspect(scene_camera));
    *v = .5f - point.y / (-2 * point.z * half);
    return true;
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
