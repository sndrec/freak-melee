#include "native_menu_fighter.h"
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_action_editor.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/native_disc_archive.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MenuFighter {
    HSD_GObj* owner;
    HSD_JObj* root;
    FtCostumeAssets costume;
    struct Fighter_WaitAnimData idle;
    void *costume_bytes, *animation_bytes, *animation_arena;
    float low_y, height;
    FighterKind kind;
    HSD_DObj* draws[256];
    HSD_JObj* draw_owners[256];
    u32 render_modes[256];
    unsigned draw_count, visibility_frames;
    u8 visibility[1024][32], outlines[1024][32];
    u32 body_color, outline_color;
    float depth, facing, center_x, center_z;
    Vec3 minimum, maximum;
    bool flat_color;
    short initial_variants[11];
} MenuFighter;
static MenuFighter fighters[NATIVE_MENU_FIGHTERS];
static unsigned generations[NATIVE_MENU_FIGHTERS];
static HSD_GObj* light_owner;
static HSD_LObj* lighting;

static void forget(void* data)
{
    MenuFighter* fighter = data;
    ++generations[fighter - fighters];
    // Descriptors and compressed tracks must outlive their HSD objects.
    HSD_GObjObject_80390B0C(fighter->owner);
    free(fighter->costume.arena);
    free(fighter->animation_arena);
    _aligned_free(fighter->costume_bytes);
    _aligned_free(fighter->animation_bytes);
    memset(fighter, 0, sizeof(*fighter));
}

static void forget_lights(void* unused)
{
    lighting = NULL;
    light_owner = NULL;
}

static void load_lights(void)
{
    if (light_owner) return;
    HSD_LObj* ambient = HSD_LObjAlloc();
    HSD_LObj* key = HSD_LObjAlloc();
    HSD_LObjSetFlags(ambient, LOBJ_AMBIENT);
    HSD_LObjSetColor(ambient, (GXColor){ 150, 150, 160, 255 });
    HSD_LObjSetFlags(key, LOBJ_INFINITE | LOBJ_DIFFUSE);
    HSD_LObjSetColor(key, (GXColor){ 225, 220, 210, 255 });
    Vec3 position = { -200, 300, 500 }, interest = { 0, 0, 0 };
    HSD_LObjSetPosition(key, &position);
    HSD_LObjSetInterest(key, &interest);
    HSD_LObjSetNext(ambient, key);
    lighting = ambient;
    light_owner = GObj_Create(0xFE11, 5, 0x80);
    HSD_GObjObject_80390A70(light_owner, HSD_GObj_LightKind, lighting);
    GObj_InitUserData(light_owner, 0, forget_lights, NULL);
}

void nativeMenuFighter_Unload(unsigned slot)
{
    if (slot < NATIVE_MENU_FIGHTERS && fighters[slot].owner)
        HSD_GObjFree(fighters[slot].owner);
}

void nativeMenuFighter_UnloadAll(void)
{
    for (unsigned slot = 0; slot < NATIVE_MENU_FIGHTERS; ++slot)
        nativeMenuFighter_Unload(slot);
    if (light_owner) HSD_GObjFree(light_owner);
}

static bool pointer(const HSD_Dat* dat, u32 slot, u32* result)
{
    const char* external;
    return HSD_DatResolve(dat, slot, result, &external) == HSD_DAT_REF_LOCAL;
}

static bool collect(HSD_JObj* root, HSD_JObj** joints, unsigned* count,
                     HSD_DObj** draws, HSD_JObj** owners, unsigned* draw_count)
{
    for (HSD_JObj* joint = root; joint; joint = joint->next) {
        if (*count == MAX_FT_PARTS) return false;
        joints[(*count)++] = joint;
        for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw; draw = draw->next) {
            if (*draw_count == 256) return false;
            owners[*draw_count] = joint; draws[(*draw_count)++] = draw;
        }
        if (!(joint->flags & JOBJ_INSTANCE) && !collect(joint->child, joints, count, draws, owners, draw_count)) return false;
    }
    return true;
}

static bool visibility(const HSD_Dat* dat, u32 root, const NativeFighterDefinition* definition,
                       const int selected[11], unsigned draw_count, u8 shown[32], u8 outlined[32])
{
    u32 model, table, groups;
    if (!pointer(dat, root + 8, &model) || !HSD_DatReadU32(dat, model, &groups) ||
        groups > 11 || !pointer(dat, model + 4, &table)) return false;
    memset(shown, 255, 32); memset(outlined, 0, 32);
    u32 items = 0;
    if (definition->asset_layout.visibility_mask && !pointer(dat, root + 72, &items)) return false;
    for (unsigned pass = 0; pass < 2; ++pass) for (unsigned category = 0; category < 20; ++category) {
        if (category == 2 || (pass && category > 0 && category < 4)) continue;
        u32 lookups;
        if (category < 4) {
            if (!HSD_DatReadU32(dat, table + category * 4, &lookups)) return false;
        } else {
            unsigned slot = category - 4;
            if (!(definition->asset_layout.visibility_mask & (1u << slot))) continue;
            if (!HSD_DatReadU32(dat, items + slot * 4, &lookups)) return false;
        }
        if (!lookups) continue;
        for (unsigned group = 0; group < groups; ++group) {
            u32 count, variants;
            if (!HSD_DatReadU32(dat, lookups + group * 8, &count) || count > 256 ||
                !HSD_DatReadU32(dat, lookups + group * 8 + 4, &variants)) return false;
            for (unsigned v = 0; v < count; ++v) {
                if (pass && (int)v != selected[group]) continue;
                u32 size, indices;
                if (!HSD_DatReadU32(dat, variants + v * 8, &size) || size > 256 ||
                    !HSD_DatReadU32(dat, variants + v * 8 + 4, &indices)) return false;
                const u8* list = HSD_DatBytes(dat, indices, size);
                if (!list && size) return false;
                for (unsigned i = 0; i < size; ++i) {
                    if (list[i] >= draw_count) return false;
                    u8 bit = 1u << (list[i] % 8);
                    if (!pass) shown[list[i] / 8] &= ~bit;
                    else if (category == 0) shown[list[i] / 8] |= bit;
                    else outlined[list[i] / 8] |= bit;
                }
            }
        }
    }
    return true;
}

static bool visibility_timeline(MenuFighter* fighter, const NativeFighterDefinition* definition,
                                 const HSD_Dat* dat, u32 root, void* bytes, size_t size, unsigned action)
{
    NativeFighterProject project = {0};
    project.action_count = definition->action_count;
    project.fighter_dat = bytes; project.fighter_size = size;
    snprintf(project.fighter_root, sizeof(project.fighter_root), "%s", definition->fighter_root);
    NativeFighterActionOccurrence events[4096]; size_t count = 0; bool complete;
    if (!nativeFighterAction_Timeline(&project, action, events, 4096, &count, &complete)) return false;
    FigaTree* tree = fighter->idle.x14;
    if (tree->frames <= 0 || tree->frames > 1024) return false;
    fighter->visibility_frames = (unsigned)ceilf(tree->frames);
    int selected[11]; for (unsigned i = 0; i < 11; ++i) selected[i] = fighter->initial_variants[i];
    size_t next = 0;
    for (unsigned frame = 0; frame < fighter->visibility_frames; ++frame) {
        while (next < count && events[next].frame <= frame) {
            const NativeFighterActionOccurrence* event = &events[next++];
            if (event->opcode == 31) {
                u32 word; if (!HSD_DatReadU32(dat, event->source_offset, &word)) return false;
                int group = (int)((word >> 19) & 127), value = (int)(word & 0x7FFFF);
                if (group >= 64) group -= 128;
                if (value >= 0x40000) value -= 0x80000;
                if (group >= 0 && group < 11) selected[group] = value;
            } else if (event->opcode == 32 || event->opcode == 33) {
                for (unsigned i = 0; i < 11; ++i) selected[i] = event->opcode == 32 ? fighter->initial_variants[i] : -1;
            }
        }
        if (!visibility(dat, root, definition, selected, fighter->draw_count,
            fighter->visibility[frame], fighter->outlines[frame])) return false;
    }
    return true;
}

static bool bind_idle(MenuFighter* fighter, const NativeFighterDefinition* definition,
                       const HSD_Dat* dat, u32 root, size_t animation_size, void* bytes, size_t size)
{
    u32 table, name, word, flags;
    // Action descriptors are indexed independently of motion states. Select
    // the named Wait1 entry rather than assuming its index or archive offset.
    if (!pointer(dat, root + 12, &table)) return false;
    bool found = false; unsigned action = 0;
    for (unsigned i = 0; i < definition->action_count; ++i) {
        u32 entry = table + i * 24;
        if (!HSD_DatReadU32(dat, entry, &name)) return false;
        const char* label = name ? (const char*)HSD_DatBytes(dat, name, 1) : NULL;
        if (label && !memchr(label, 0, dat->data_size - name)) return false;
        if (!label || (!strstr(label, "_ACTION_Wait1_figatree") && !strstr(label, "_ACTION_Wait_figatree"))) continue;
        fighter->idle.x0 = (char*)label;
        if (!HSD_DatReadU32(dat, entry + 4, &word)) return false;
        fighter->idle.x4 = word;
        if (!HSD_DatReadU32(dat, entry + 8, &word)) return false;
        fighter->idle.x8 = word;
        if (!HSD_DatReadU32(dat, entry + 16, &flags)) return false;
        action = i; found = true; break;
    }
    if (!found) return false;
    fighter->animation_arena = malloc(FT_ANIMATION_IMPORT_ARENA_SIZE);
    FtAnimationBankTable entry = { &fighter->idle, 1, false };
    FtAnimationBankStats stats;
    if (!fighter->animation_arena || !ftAnimationBank_Import(fighter->animation_bytes,
        animation_size, &entry, 1, fighter->animation_arena, FT_ANIMATION_IMPORT_ARENA_SIZE, &stats)) return false;
    HSD_JObj* joints[MAX_FT_PARTS]; unsigned joint_count = 0;
    if (!collect(fighter->root, joints, &joint_count, fighter->draws, fighter->draw_owners, &fighter->draw_count) ||
        joint_count != definition->joint_count ||
        !visibility_timeline(fighter, definition, dat, root, bytes, size, action)) return false;
    for (unsigned d = 0; d < fighter->draw_count; ++d)
        if (fighter->draws[d]->mobj) fighter->render_modes[d] = fighter->draws[d]->mobj->rendermode;
    FigaTree* tree = fighter->idle.x14;
    FigaTrack* track = tree->tracks;
    unsigned animation_node = 0, costume_node = 0;
    u32 mask = (flags >> 9) & 0x1FFF;
    for (unsigned part = 0; part < joint_count + definition->omission_count; ++part) {
        int omission = -1;
        for (unsigned o = 0; o < definition->omission_count; ++o)
            if (definition->omissions[o].joint == part) omission = o;
        if (omission >= 0 && !(mask & (1u << omission))) continue;
        if (tree->nodes[animation_node] == -1) return false;
        unsigned channels = (u8)tree->nodes[animation_node++];
        if (omission < 0) {
            bool translation = part == 0;
            for (unsigned b = 0; b < definition->bone_count; ++b) {
                Fighter_Part role = definition->bones[b].part;
                if (definition->bones[b].joint == part && (role == FtPart_TransN || role == FtPart_XRotN ||
                    role == FtPart_YRotN || role == FtPart_HipN || role == FtPart_TransN2)) translation = true;
            }
            if (translation) lbAnim_8001E6D8(joints[costume_node], tree, track, channels);
            else lbAnim_8001E7E8(joints[costume_node], tree, track, channels);
            ++costume_node;
        }
        track += channels;
    }
    if (tree->nodes[animation_node] != -1 || costume_node != joint_count) return false;
    HSD_JObjReqAnimAll(fighter->root, 0);
    HSD_JObjAnimAll(fighter->root);
    Vec3 low = {INFINITY, INFINITY, INFINITY}, high = {-INFINITY, -INFINITY, -INFINITY};
    for (unsigned d = 0; d < fighter->draw_count; ++d) {
        if (!(fighter->visibility[0][d / 8] & (1u << (d % 8)))) continue;
        Vec3 a, b;
        if (!nativeModelView_MeshBounds(fighter->draw_owners[d], fighter->draws[d], &a, &b)) return false;
        low.x = fminf(low.x, a.x); high.x = fmaxf(high.x, b.x);
        low.y = fminf(low.y, a.y); high.y = fmaxf(high.y, b.y);
        low.z = fminf(low.z, a.z); high.z = fmaxf(high.z, b.z);
    }
    fighter->low_y = low.y; fighter->height = high.y - low.y;
    fighter->minimum = low; fighter->maximum = high;
    fighter->center_x = (low.x + high.x) * .5f; fighter->center_z = (low.z + high.z) * .5f;
    return isfinite(fighter->height) && fighter->height > .01f;
}

bool nativeMenuFighter_Load(unsigned slot, FighterKind kind, const short initial_variants[11])
{
    if (slot >= NATIVE_MENU_FIGHTERS || !initial_variants) return false;
    MenuFighter* fighter = &fighters[slot];
    if (fighter->root && fighter->kind == kind && !memcmp(fighter->initial_variants, initial_variants, sizeof(fighter->initial_variants))) return true;
    nativeMenuFighter_Unload(slot);
    // Nana's stock action bank is owned by Popo; her costume remains her own.
    FighterKind animation_kind = kind == Ft_Kind_Nana ? Ft_Kind_Popo : kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_FindStockDefinition(animation_kind);
    if (!definition || kind < Ft_Kind_Mario || kind >= Ft_Kind_StockCount) return false;
    const Fighter_CostumeStrings* names = ftData_803C2360[kind];
    void* bytes = NULL; size_t size = 0, costume_size = 0, animation_size = 0;
    fighter->kind = kind;
    memcpy(fighter->initial_variants, initial_variants, sizeof(fighter->initial_variants));
    fighter->owner = GObj_Create(0xFE11, 5, 0x80);
    GObj_InitUserData(fighter->owner, 0, forget, fighter);
    bool ok = names && nativeDiscArchive_Read(names[0].dat_filename, &fighter->costume_bytes, &costume_size) &&
        ftCostumeAssets_Load(fighter->costume_bytes, costume_size, names[0].joint_name,
            names[0].matanim_joint_name, &fighter->costume) &&
        nativeDiscArchive_Read(ftData_803C1F40[animation_kind].filename, &bytes, &size) &&
        nativeDiscArchive_Read(ftData_803C23E4[animation_kind], &fighter->animation_bytes, &animation_size);
    if (ok) {
        fighter->root = HSD_JObjLoadJoint(fighter->costume.joint);
        HSD_GObjObject_80390A70(fighter->owner, HSD_GObj_JObjKind, fighter->root);
        HSD_JObjAddAnimAll(fighter->root, NULL, fighter->costume.matanim_joint, NULL);
        HSD_Dat dat; u32 root;
        ok = HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
            HSD_DatFindRoot(&dat, definition->fighter_root, &root) &&
            bind_idle(fighter, definition, &dat, root, animation_size, bytes, size);
        // The imported FigaTree retains its animation archive, not this label.
        fighter->idle.x0 = NULL;
    }
    _aligned_free(bytes);
    if (!ok) {
        fprintf(stderr, "[menu-fighter] Could not prepare stock preview %u\n", kind);
        nativeMenuFighter_Unload(slot);
    } else { ++generations[slot]; fighter->depth = 1; load_lights(); }
    return ok;
}

unsigned nativeMenuFighter_Generation(unsigned slot)
{
    return slot < NATIVE_MENU_FIGHTERS && fighters[slot].root ? generations[slot] : 0;
}

void nativeMenuFighter_SetFlatStyle(unsigned slot, unsigned body, unsigned outline, float depth, float facing)
{
    if (slot >= NATIVE_MENU_FIGHTERS || !fighters[slot].root || !isfinite(depth) || depth <= 0) return;
    fighters[slot].flat_color = true;
    fighters[slot].body_color = body; fighters[slot].outline_color = outline;
    fighters[slot].depth = depth;
    fighters[slot].facing = facing;
}

static void render_fighter(MenuFighter* fighter, Mtx view, float frame)
{
    FigaTree* idle = fighter->idle.x14;
    HSD_JObjReqAnimAll(fighter->root, fmodf(fmaxf(0, frame), fmaxf(1, idle->frames)));
    HSD_JObjAnimAll(fighter->root);
    HSD_LObjDeleteCurrentAll(NULL);
    for (HSD_LObj* light = lighting; light; light = light->next) HSD_LObjAddCurrent(light);
    HSD_LObjSetupInit(HSD_CObjGetCurrent());
    unsigned visibility_frame = (unsigned)fmodf(fmaxf(0, frame), fighter->visibility_frames);
    for (unsigned pass = fighter->flat_color ? 0 : 1; pass < 2; ++pass) {
        const u8* visible = pass ? fighter->visibility[visibility_frame] : fighter->outlines[visibility_frame];
        u32 color = pass ? fighter->body_color : fighter->outline_color;
        for (unsigned d = 0; d < fighter->draw_count; ++d) {
            HSD_DObj* draw = fighter->draws[d];
            if (visible[d / 8] & (1u << (d % 8))) HSD_DObjClearFlags(draw, 1);
            else HSD_DObjSetFlags(draw, 1);
            if (fighter->flat_color && draw->mobj && draw->mobj->mat) {
                // The retail outline pass also leaves the depth buffer alone;
                // otherwise the shell hides the flattened body drawn after it.
                draw->mobj->rendermode = fighter->render_modes[d] | (pass ? 0 : RENDER_NO_ZUPDATE);
                draw->mobj->mat->diffuse = (GXColor){color >> 24, color >> 16, color >> 8, 255};
                draw->mobj->mat->alpha = (color & 255) / 255.f;
            }
        }
        HSD_StateInvalidate(-1); HSD_StateInitTev(); HSD_ClearVtxDesc();
        HSD_JObjDispAll(fighter->root, view, HSD_TRSP_ALL, 0);
    }
}

void nativeMenuFighter_Draw(unsigned slot, Mtx canvas, float x, float feet_y,
                            float height, float yaw, float frame)
{
    if (slot >= NATIVE_MENU_FIGHTERS || !fighters[slot].root) return;
    MenuFighter* fighter = &fighters[slot];
    float scale = height / fighter->height;
    float c = cosf(yaw) * scale, s = sinf(yaw) * scale;
    float a = cosf(fighter->facing), b = sinf(fighter->facing), depth = fighter->depth;
    float cx = a * fighter->center_x + b * fighter->center_z;
    float cz = (-b * fighter->center_x + a * fighter->center_z) * depth;
    Mtx model = { { c*a-s*depth*b, 0, c*b+s*depth*a, x-c*cx-s*cz },
                  { 0, scale, 0, -feet_y-fighter->low_y*scale },
                  { -s*a-c*depth*b, 0, -s*b+c*depth*a, -70+s*cx-c*cz } }, view;
    C_MTXConcat(canvas, model, view);
    render_fighter(fighter, view, frame);
}

void nativeMenuFighter_DrawCamera(unsigned slot, float yaw, float pitch, float zoom,
                                  float pan_x, float pan_y, float frame, float aspect)
{
    if (slot >= NATIVE_MENU_FIGHTERS || !fighters[slot].root) return;
    MenuFighter* fighter = &fighters[slot];
    Mtx facing, depth, orbit, tilt, a, b, view;
    C_MTXRotRad(facing, 'y', fighter->facing);
    C_MTXScale(depth, 1, 1, fighter->depth);
    C_MTXRotRad(orbit, 'y', yaw); C_MTXRotRad(tilt, 'x', pitch);
    C_MTXConcat(depth, facing, a); C_MTXConcat(orbit, a, b); C_MTXConcat(tilt, b, view);
    Vec3 low = {INFINITY, INFINITY, INFINITY}, high = {-INFINITY, -INFINITY, -INFINITY};
    for (unsigned corner = 0; corner < 8; ++corner) {
        Vec3 p = {corner & 1 ? fighter->maximum.x : fighter->minimum.x,
                  corner & 2 ? fighter->maximum.y : fighter->minimum.y,
                  corner & 4 ? fighter->maximum.z : fighter->minimum.z}, transformed;
        C_MTXMultVecSR(view, &p, &transformed);
        low.x = fminf(low.x, transformed.x); high.x = fmaxf(high.x, transformed.x);
        low.y = fminf(low.y, transformed.y); high.y = fmaxf(high.y, transformed.y);
    }
    float scale = 1.8f * zoom / fmaxf((high.x-low.x)/aspect, high.y-low.y);
    for (unsigned row = 0; row < 3; ++row) for (unsigned column = 0; column < 3; ++column)
        view[row][column] *= scale;
    Vec3 center = { fighter->center_x, fighter->low_y + fighter->height * .5f, fighter->center_z }, target;
    C_MTXMultVecSR(view, &center, &target);
    view[0][3] = pan_x - target.x; view[1][3] = pan_y - target.y; view[2][3] = -10 - target.z;
    Mtx44 projection; C_MTXOrtho(projection, 1, -1, -aspect, aspect, 1, 100);
    GXSetProjection(projection, GX_ORTHOGRAPHIC);
    render_fighter(fighter, view, frame);
}

void nativeMenuFighter_Inspect(void)
{
    for (unsigned i = 0; i < NATIVE_MENU_FIGHTERS; ++i) if (fighters[i].root)
        fprintf(stderr, "[hub-inspect] menu-fighter slot=%u kind=%u joints=%u frames=%.1f height=%.2f\n",
            i, fighters[i].kind, (unsigned)fighters[i].costume.joint_count, ((FigaTree*)fighters[i].idle.x14)->frames, fighters[i].height);
}
