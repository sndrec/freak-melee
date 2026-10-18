#include "native_menu_scene.h"
#include "mnassets.h"
#include "native_menu_paint.h"
#include <melee/lb/lbspdisplay.h>
#include <melee/sc/types.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <dolphin/gx.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <SDL3/SDL_timer.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static struct {
    HSD_GObj* owners[2];
    HSD_JObj* joints[2];
    MnMenuAssets assets;
    HSD_GObj* backdrop[2];
    HSD_GObj* ribbons[7];
    struct {
        HSD_VtxDescList attributes[4];
        Vec3 source[128], positions[128];
        unsigned count;
        HSD_JObj* ornament;
    } ribbon_geometry[7];
    HSD_ImageDesc* frame;
    HSD_JObj* caption;
    Mtx caption_pixels;
    u8 frame_border[48 * 32 / 2] ATTRIBUTE_ALIGN(32);
    HSD_GObj* camera;
    HSD_GObj* lights;
    Mtx idle_inverse;
    float fov, pixels_per_unit;
    float starts[2];
    Uint64 started;
    bool playing;
    Uint64 animated_tick;
} scene;

static HSD_JObj* runtime_joint(HSD_JObj* root, unsigned index);

static float be_float(const u8* p)
{
    u32 bits = (u32) p[0] << 24 | (u32) p[1] << 16 | (u32) p[2] << 8 | p[3];
    float value; memcpy(&value, &bits, sizeof(value));
    return value;
}

static void prepare_caption(void)
{
    scene.caption = runtime_joint(scene.backdrop[1]->hsd_obj, 86);
    HSD_DObj* object = scene.caption->u.dobj;
    HSD_PObj* polygon = object->pobj;
    HSD_VtxDescList* vertices = polygon->verts;
    HSD_ASSERT(0, vertices[0].attr == GX_VA_POS && vertices[0].comp_type == GX_F32 &&
        vertices[0].attr_type == GX_INDEX16 && vertices[0].comp_cnt == GX_POS_XYZ &&
        vertices[1].attr == GX_VA_TEX0 && vertices[1].attr_type == GX_INDEX16 &&
        vertices[2].attr == GX_VA_NULL && !vertices[0].vertex_little_endian);
    const u8* strip = polygon->display;
    HSD_ASSERT(0, strip[0] == GX_TRIANGLESTRIP && strip[1] == 0 && strip[2] == 4);
    Vec3 min = { INFINITY, INFINITY, INFINITY }, max = { -INFINITY, -INFINITY, -INFINITY };
    for (unsigned i = 0; i < 4; ++i) {
        unsigned index = (strip[3+i*4] << 8) | strip[4+i*4];
        const u8* source = (const u8*) vertices[0].vertex + index * vertices[0].stride;
        float* low = (float*) &min; float* high = (float*) &max;
        for (unsigned axis = 0; axis < 3; ++axis) {
            float value = be_float(source + axis * 4);
            low[axis] = fminf(low[axis], value); high[axis] = fmaxf(high[axis], value);
        }
    }
    const HSD_ImageDesc* image = object->mobj->tobj->imagedesc;
    C_MTXScale(scene.caption_pixels, (max.x-min.x)/image->width,
        (max.y-min.y)/image->height, 1);
    scene.caption_pixels[0][3] = min.x;
    scene.caption_pixels[1][3] = max.y;
    scene.caption_pixels[2][3] = min.z;
}

static void prepare_ribbon(unsigned slot)
{
    HSD_JObj* root = scene.ribbons[slot]->hsd_obj;
    HSD_PObj* polygon = runtime_joint(root, 2)->u.dobj->pobj;
    HSD_VtxDescList* source = polygon->verts;
    HSD_ASSERT(0, source[0].attr == GX_VA_POS && source[0].comp_type == GX_S16 &&
        source[0].comp_cnt == GX_POS_XYZ && source[0].stride == 6 &&
        !source[0].vertex_little_endian && source[3].attr == GX_VA_NULL);
    unsigned count = source[0].vertex_size / 6;
    HSD_ASSERT(0, count <= 128 && count * 6 == source[0].vertex_size);
    scene.ribbon_geometry[slot].count = count;
    memcpy(scene.ribbon_geometry[slot].attributes, source, sizeof(HSD_VtxDescList) * 4);
    HSD_VtxDescList* position = scene.ribbon_geometry[slot].attributes;
    position->comp_type = GX_F32;
    position->frac = 0;
    position->stride = sizeof(Vec3);
    position->vertex = scene.ribbon_geometry[slot].positions;
    position->vertex_size = count * sizeof(Vec3);
    position->vertex_little_endian = true;
    polygon->verts = position;
    const u8* bytes = source[0].vertex;
    for (unsigned i = 0; i < count; ++i) {
        float* xyz = (float*) &scene.ribbon_geometry[slot].source[i];
        for (unsigned axis = 0; axis < 3; ++axis) {
            const u8* p = bytes + i * 6 + axis * 2;
            xyz[axis] = ldexpf((s16) ((p[0] << 8) | p[1]), -(int) source[0].frac);
        }
    }
    scene.ribbon_geometry[slot].ornament = runtime_joint(root, 4);
}

static void forget_owner(void* owner)
{
    *(HSD_GObj**) owner = NULL;
    if (scene.camera || scene.lights) return;
    for (unsigned i = 0; i < 2; ++i)
        if (scene.owners[i] || scene.backdrop[i]) return;
    for (unsigned i = 0; i < 7; ++i) if (scene.ribbons[i]) return;
    // Scene teardown retires its heap after destroying these objects.
    memset(&scene.assets, 0, sizeof(scene.assets));
    scene.playing = false;
}

void nativeMenuScene_Unload(void)
{
    MnMenuAssets assets = scene.assets;
    for (unsigned i = 0; i < 2; ++i)
        if (scene.owners[i]) HSD_GObjFree(scene.owners[i]);
    for (unsigned i = 0; i < 2; ++i)
        if (scene.backdrop[i]) HSD_GObjFree(scene.backdrop[i]);
    for (unsigned i = 0; i < 7; ++i)
        if (scene.ribbons[i]) HSD_GObjFree(scene.ribbons[i]);
    if (scene.camera) HSD_GObjFree(scene.camera);
    if (scene.lights) HSD_GObjFree(scene.lights);
    mnAssets_Unload(&assets);
    memset(&scene, 0, sizeof(scene));
}

static HSD_Joint* joint_at(HSD_Joint* node, unsigned* index)
{
    for (; node; node = node->next) {
        if (!*index) return node;
        --*index;
        HSD_Joint* found = joint_at(node->child, index);
        if (found) return found;
    }
    return NULL;
}
static HSD_AnimJoint* animation_at(HSD_AnimJoint* node, unsigned* index)
{
    for (; node; node = node->next) {
        if (!*index) return node;
        --*index;
        HSD_AnimJoint* found = animation_at(node->child, index);
        if (found) return found;
    }
    return NULL;
}
static HSD_GObj* model(HSD_GObj** owner, const char* name)
{
    char symbol[96];
    snprintf(symbol, sizeof(symbol), "%s_joint", name);
    HSD_Joint* desc = mnAssets_Get(&scene.assets, symbol);
    *owner = GObj_Create(0xFE08, 5, 0x80);
    GObj_InitUserData(*owner, 0, forget_owner, owner);
    HSD_JObj* tree = HSD_JObjLoadJoint(desc);
    HSD_GObjObject_80390A70(*owner, HSD_GObj_JObjKind, tree);
    snprintf(symbol, sizeof(symbol), "%s_animjoint", name);
    HSD_AnimJoint* anim = mnAssets_Get(&scene.assets, symbol);
    snprintf(symbol, sizeof(symbol), "%s_matanim_joint", name);
    HSD_MatAnimJoint* mat = mnAssets_Get(&scene.assets, symbol);
    snprintf(symbol, sizeof(symbol), "%s_shapeanim_joint", name);
    HSD_ShapeAnimJoint* shape = mnAssets_Get(&scene.assets, symbol);
    HSD_JObjAddAnimAll(tree, anim, mat, shape);
    HSD_JObjReqAnimAll(tree, 0);
    HSD_JObjAnimAll(tree);
    return *owner;
}

void nativeMenuScene_Load(void)
{
    if (scene.owners[0] && scene.owners[1]) return;
    nativeMenuScene_Unload();
    mnAssets_Load(&scene.assets);
    HSD_Joint* root = mnAssets_Get(&scene.assets, "MenMainConTop_Top_joint");
    HSD_AnimJoint* anim = mnAssets_Get(&scene.assets, "MenMainConTop_Top_animjoint");
    HSD_CObjDesc* cam = mnAssets_Get(&scene.assets, "ScMenMain_cam_int1_camera");
    unsigned index = 2;
    HSD_Joint parent = *joint_at(root, &index);
    index = 3;
    HSD_Joint moving = *joint_at(root, &index);
    parent.next = NULL; parent.u.dobjdesc = NULL;
    moving.next = moving.child = NULL; moving.u.dobjdesc = NULL;
    index = 3;
    HSD_AnimJoint animation = { 0 };
    animation.aobjdesc = animation_at(anim, &index)->aobjdesc;
    scene.fov = cam->perspective.fov;
    float depth = cam->common.eyepos->pos.z - parent.position.z;
    scene.pixels_per_unit = 240 / (depth * tanf(scene.fov * .00872664626f));
    // Preserve the stock parent orientation, which turns this joint's local Z
    // rotation into the main menu's tilted, three-dimensional spin. Its world
    // placement is replaced by the hub content's pivot on the 640x480 canvas.
    parent.position = (Vec3){ 0 };
    parent.child = &moving;
    for (unsigned i = 0; i < 2; ++i) {
        scene.owners[i] = GObj_Create(0xFE02 + i, 5, 0x80);
        GObj_InitUserData(scene.owners[i], 0, forget_owner, &scene.owners[i]);
        HSD_JObj* tree = HSD_JObjLoadJoint(&parent);
        HSD_GObjObject_80390A70(scene.owners[i], HSD_GObj_JObjKind, tree);
        scene.joints[i] = tree->child;
        HSD_JObjAddAnim(scene.joints[i], &animation, NULL, NULL);
    }
    model(&scene.backdrop[0], "MenMainBack_Top");
    model(&scene.backdrop[1], "MenMainPanel_Top");
    prepare_caption();
    for (unsigned i = 0; i < 7; ++i) {
        model(&scene.ribbons[i], "MenMainCursor_Top");
        prepare_ribbon(i);
    }
    scene.frame = runtime_joint(scene.backdrop[1]->hsd_obj, 77)->u.dobj->mobj->tobj->imagedesc;
    HSD_ASSERT(0, scene.frame->width == 48 && scene.frame->height == 32 &&
        scene.frame->format == GX_TF_IA4);
    // Extract the border's coverage from the stock IA4 panel. Its translucent
    // black interior is supplied separately, so it cannot darken a button fill.
    const u8* pixels = scene.frame->image_ptr;
    for (unsigned y = 0; y < 32; ++y) for (unsigned x = 0; x < 48; x += 2) {
        unsigned src = ((y / 4) * 6 + x / 8) * 32 + (y % 4) * 8 + x % 8;
        unsigned dst = ((y / 8) * 6 + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2;
        unsigned a = pixels[src], b = pixels[src + 1];
        scene.frame_border[dst] = (((a & 15) * (a >> 4) + 7) / 15 << 4) |
            ((b & 15) * (b >> 4) + 7) / 15;
    }
    scene.camera = GObj_Create(0xFE09, 3, 0x80);
    GObj_InitUserData(scene.camera, 0, forget_owner, &scene.camera);
    HSD_GObjObject_80390A70(scene.camera, HSD_GObj_CameraKind, HSD_CObjLoadDesc(cam));
    scene.lights = GObj_Create(0xFE0A, 4, 0x80);
    GObj_InitUserData(scene.lights, 0, forget_owner, &scene.lights);
    HSD_GObjObject_80390A70(scene.lights, HSD_GObj_LightKind,
        lb_80011AC4(mnAssets_Get(&scene.assets, "ScMenMain_scene_lights")));

}

float nativeMenuScene_Frame(void)
{
    return scene.playing ? (float) (SDL_GetTicksNS() - scene.started) * .00000006f : 20;
}

bool nativeMenuScene_Active(void)
{
    return scene.owners[0] && scene.owners[1] && scene.playing && nativeMenuScene_Frame() < 20;
}

void nativeMenuScene_Start(bool backwards)
{
    if (!scene.owners[0] || !scene.owners[1]) return;
    // mnmain.c: ENTER_TO 320, ENTER_FROM 340, EXIT_TO 360, EXIT_FROM 380.
    scene.starts[0] = backwards ? 380 : 340;
    scene.starts[1] = backwards ? 360 : 320;
    HSD_JObjReqAnim(scene.joints[1], scene.starts[1] + 19);
    HSD_JObjAnim(scene.joints[1]);
    HSD_ASSERT(0, C_MTXInverse(HSD_JObjGetMtxPtr(scene.joints[1]), scene.idle_inverse));
    scene.started = SDL_GetTicksNS();
    scene.playing = true;
}

bool nativeMenuScene_Sample(bool outgoing, Mtx view, Mtx44 projection)
{
    if (!nativeMenuScene_Active()) return false;
    HSD_JObj* pose = scene.joints[outgoing ? 0 : 1];
    HSD_JObjReqAnim(pose, scene.starts[outgoing ? 0 : 1] + fminf(19, nativeMenuScene_Frame()));
    HSD_JObjAnim(pose);
    Mtx relative, a, b;
    C_MTXConcat(HSD_JObjGetMtxPtr(pose), scene.idle_inverse, relative);
    float scale = scene.pixels_per_unit;
    C_MTXScale(a, 1 / scale, 1 / scale, 1 / scale);
    C_MTXConcat(relative, a, b);
    C_MTXScale(a, scale, scale, scale);
    C_MTXConcat(a, b, relative);
    C_MTXTrans(a, -390, 230, 5);
    C_MTXConcat(relative, a, b);
    C_MTXTrans(a, 390, -230, -5);
    C_MTXConcat(a, b, relative);
    float distance = 240 / tanf(scene.fov * .00872664626f);
    C_MTXTrans(a, -320, 240, -distance + 5);
    C_MTXConcat(a, relative, view);
    C_MTXPerspective(projection, scene.fov, 4.f / 3, 1, 5000);
    return true;
}

static HSD_JObj* runtime_joint(HSD_JObj* root, unsigned index)
{
    HSD_JObj* result = NULL;
    lb_80011E24(root, &result, index, -1);
    HSD_ASSERT(0, result);
    return result;
}

void nativeMenuScene_DrawBackdrop(const char* caption)
{
    if (!scene.camera || !scene.backdrop[0] || !scene.backdrop[1]) return;
    HSD_CObj* previous = HSD_CObjGetCurrent();
    HSD_CObj* camera = scene.camera->hsd_obj;
    if (!HSD_CObjSetCurrent(camera)) return;
    HSD_LObjDeleteCurrentAll(NULL);
    for (HSD_LObj* light = scene.lights->hsd_obj; light; light = light->next)
        HSD_LObjAddCurrent(light);
    HSD_LObjSetupInit(camera);
    HSD_CObjEraseScreen(camera, 1, 1, 1);
    HSD_StateInvalidate(-1); HSD_StateInitTev(); HSD_ClearVtxDesc();
    const Uint64 tick = SDL_GetTicksNS() / 16666667;
    if (tick != scene.animated_tick) {
        HSD_JObjAnimAll(scene.backdrop[0]->hsd_obj);
        // Main-menu panel: the scrolling decoration and main-page idle track.
        HSD_JObj* panel = scene.backdrop[1]->hsd_obj;
        HSD_JObjReqAnimAll(runtime_joint(panel, 4), tick % 800);
        HSD_JObjReqAnimAll(runtime_joint(panel, 41), 100 + tick % 100);
        HSD_JObjAnimAll(panel);
        // Replace the baked page caption, keeping its panel and animation.
        for (unsigned i = 82; i <= 86; ++i)
            HSD_JObjSetFlags(runtime_joint(panel, i), JOBJ_HIDDEN);
        scene.animated_tick = tick;
    }
    Mtx view; HSD_CObjGetViewingMtx(camera, view);
    HSD_JObjDispAll(scene.backdrop[0]->hsd_obj, view, HSD_TRSP_ALL, 0);
    HSD_JObjDispAll(scene.backdrop[1]->hsd_obj, view, HSD_TRSP_ALL, 0);
    if (caption && *caption) {
        Mtx model, text_view;
        HSD_JObjSetupMatrix(scene.caption);
        C_MTXConcat(HSD_JObjGetMtxPtr(scene.caption), scene.caption_pixels, model);
        C_MTXConcat(view, model, text_view);
        nativeMenuPaint_Begin(text_view);
        const HSD_Material* material = scene.caption->u.dobj->mobj->mat;
        u32 rgba = (u32) material->diffuse.r << 24 | (u32) material->diffuse.g << 16 |
            (u32) material->diffuse.b << 8 | (u32) (material->alpha * 255);
        nativeMenuFont_Draw(NativeMenuFont_MenuCaption, caption, 0, 0, 0,
            168, 24, rgba, false, false);
        nativeMenuPaint_End();
    }
    HSD_CObjEndCurrent();
    if (previous) HSD_CObjSetCurrent(previous);
}

void nativeMenuScene_DrawRibbon(unsigned slot, float x, float y, float width,
                                float height, bool selected)
{
    if (slot >= 7 || !scene.ribbons[slot]) return;
    HSD_JObj* root = scene.ribbons[slot]->hsd_obj;
    // mnmain's idle and selected cursor tracks. The baked retail option name
    // is replaced by the hub's own glyph string, retaining the actual ribbon.
    HSD_JObjReqAnimAll(root, 0);
    HSD_JObjReqAnim(runtime_joint(root, 2), selected ? 70 : 20);
    HSD_JObjReqAnimAll(runtime_joint(root, 4), selected ? 80 : 49);
    HSD_JObjAnimAll(root);
    HSD_JObjSetFlags(runtime_joint(root, 3), JOBJ_HIDDEN);
    HSD_JObjSetFlagsAll(runtime_joint(root, 9), JOBJ_HIDDEN);
    HSD_JObjSetFlagsAll(runtime_joint(root, 11), JOBJ_HIDDEN);
    Mtx44 projection;
    C_MTXOrtho(projection, 0, -480, 0, 640, 1, 1000);
    GXSetProjection(projection, GX_ORTHOGRAPHIC);
    // The authored mesh already separates its left cap, middle and right cap.
    // Translate the cap vertices without scaling them; the triangles between
    // them supply the variable-width middle. Shared disc geometry stays intact.
    float scale = height / 2.72314f;
    float extension = fmaxf(3.6f, width / scale) - 15.146f;
    for (unsigned i = 0; i < scene.ribbon_geometry[slot].count; ++i) {
        Vec3 p = scene.ribbon_geometry[slot].source[i];
        p.x += p.x < -5.6f ? -extension * .5f : p.x > 5.6f ? extension * .5f : 0;
        scene.ribbon_geometry[slot].positions[i] = p;
    }
    HSD_JObjSetTranslateX(scene.ribbon_geometry[slot].ornament, 5.94998f + extension * .5f);
    Mtx view = { { scale, 0, 0, x + width * .5f - .03345f * scale },
                 { 0, scale, 0, -y - height * .48449f },
                 { 0, 0, 1, -100 } };
    HSD_StateInvalidate(-1); HSD_StateInitTev(); HSD_ClearVtxDesc();
    HSD_JObjDispAll(root, view, HSD_TRSP_ALL, 0);
}

void nativeMenuScene_DrawFrame(float x, float y, float width, float height, u32 rgba)
{
    if (!scene.frame) return;
    GXTexObj texture;
    GXInitTexObj(&texture, scene.frame_border, 48, 32, GX_TF_I4,
        GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&texture, GX_TEXMAP0);
    GXSetTevColor(GX_TEVREG0, (GXColor){ rgba >> 24, rgba >> 16, rgba >> 8, rgba });
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
    float edge = fminf(6, fminf(width, height) * .5f);
    const float px[] = { x, x + edge, x + width - edge, x + width };
    const float py[] = { y, y + edge, y + height - edge, y + height };
    const float u[] = { 0, 8.f / 48, 40.f / 48, 1 };
    const float v[] = { 0, 8.f / 32, 24.f / 32, 1 };
    GXBegin(GX_QUADS, GX_VTXFMT0, 36);
    for (unsigned row = 0; row < 3; ++row) for (unsigned col = 0; col < 3; ++col) {
        GXPosition3f32(px[col], -py[row], -5); GXTexCoord2f32(u[col], v[row]);
        GXPosition3f32(px[col+1], -py[row], -5); GXTexCoord2f32(u[col+1], v[row]);
        GXPosition3f32(px[col+1], -py[row+1], -5); GXTexCoord2f32(u[col+1], v[row+1]);
        GXPosition3f32(px[col], -py[row+1], -5); GXTexCoord2f32(u[col], v[row+1]);
    }
    GXEnd();
}

void nativeMenuScene_Inspect(void)
{
    fprintf(stderr, "[hub-inspect] menu-assets records=%u arena=%zu ribbons=%d camera=%d\n",
        (unsigned) scene.assets.import.record_count, scene.assets.import.arena_used,
        scene.ribbons[0] != NULL, scene.camera != NULL);
    if (!scene.ribbons[0]) return;
    HSD_JObj* root = scene.ribbons[0]->hsd_obj;
    for (unsigned i = 0; i < 4; ++i) {
        HSD_JObj* j = runtime_joint(root, i); HSD_JObjSetupMatrix(j);
        fprintf(stderr, "[hub-inspect] ribbon-joint=%u flags=%lx scale=%.3f,%.3f,%.3f pos=%.3f,%.3f,%.3f matrix=%.3f,%.3f,%.3f\n",
            i, (unsigned long) j->flags, j->scale.x, j->scale.y, j->scale.z,
            j->translate.x, j->translate.y, j->translate.z,
            j->mtx[0][0], j->mtx[1][1], j->mtx[2][2]);
        for (HSD_DObj* d = j->u.dobj; d; d = d->next)
            fprintf(stderr, "[hub-inspect] ribbon-dobj flags=%lx render=%lx alpha=%.3f\n",
                (unsigned long) d->flags, (unsigned long) d->mobj->rendermode, d->mobj->mat->alpha);
    }
}
