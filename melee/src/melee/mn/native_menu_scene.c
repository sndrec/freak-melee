#include "native_menu_scene.h"
#include "mnassets.h"
#include "mncamera.h"
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
    HSD_JObj* caption;
    Mtx caption_pixels;
    HSD_GObj* camera;
    MnCamera camera_data;
    HSD_CObj* previous_camera;
    HSD_JObj* canvas_anchor;
    Mtx canvas_pixels;
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

static void retire_import(void)
{
    if (scene.camera || scene.lights) return;
    for (unsigned i = 0; i < 2; ++i)
        if (scene.owners[i] || scene.backdrop[i]) return;
    for (unsigned i = 0; i < 7; ++i) if (scene.ribbons[i]) return;
    // Scene teardown retires its heap after destroying these objects.
    memset(&scene.assets, 0, sizeof(scene.assets));
    scene.playing = false;
}
static void forget_owner(void* owner)
{
    *(HSD_GObj**) owner = NULL;
    retire_import();
}
static void forget_camera(void* data)
{
    scene.camera = NULL;
    scene.previous_camera = NULL;
    memset(&scene.canvas_pixels, 0, sizeof(scene.canvas_pixels));
    retire_import();
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
    scene.camera = GObj_Create(0xFE09, 3, 0x80);
    scene.camera_data.rest = cam;
    GObj_InitUserData(scene.camera, 0, forget_camera, &scene.camera_data);
    HSD_GObjObject_80390A70(scene.camera, HSD_GObj_CameraKind, HSD_CObjLoadDesc(cam));
    HSD_CObj* camera = scene.camera->hsd_obj;
    Vec3 eye; HSD_CObjGetEyePosition(camera, &eye);
    // The visible main-panel frame is joint 2: its authored depth is distinct
    // from the panel root/background. Keep the new controls on that surface.
    scene.canvas_anchor = runtime_joint(scene.backdrop[1]->hsd_obj, 2);
    HSD_JObjSetupMatrix(scene.canvas_anchor);
    MtxPtr anchor = HSD_JObjGetMtxPtr(scene.canvas_anchor);
    float half_height = (eye.z - anchor[2][3]) * tanf(scene.fov * .00872664626f);
    float units = half_height / 240;
    C_MTXScale(scene.canvas_pixels, units, units, units);
    scene.canvas_pixels[0][3] = eye.x - anchor[0][3] - half_height * HSD_CObjGetAspect(camera);
    scene.canvas_pixels[1][3] = eye.y - anchor[1][3] + half_height;
    scene.canvas_pixels[2][3] = units * 5;
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

void nativeMenuScene_Tick(void)
{
    if (scene.camera) mnCamera_Tick(scene.camera);
}

bool nativeMenuScene_View(bool transition, bool outgoing, Mtx view, Mtx44 projection)
{
    if (!scene.camera || !scene.backdrop[1]) return false;
    HSD_CObj* camera = scene.camera->hsd_obj;
    HSD_JObj* anchor = scene.canvas_anchor;
    HSD_JObjSetupMatrix(anchor);
    Mtx canvas, camera_view;
    C_MTXConcat(HSD_JObjGetMtxPtr(anchor), scene.canvas_pixels, canvas);
    HSD_CObjGetViewingMtx(camera, camera_view);
    C_MTXConcat(camera_view, canvas, view);
    makeProjectionMtx(camera, projection);
    if (!transition || !nativeMenuScene_Active()) return true;
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
    C_MTXCopy(view, a);
    C_MTXConcat(a, relative, view);
    return true;
}

bool nativeMenuScene_Project(float x, float y, float* u, float* v)
{
    Mtx view; Mtx44 projection;
    if (!nativeMenuScene_View(false, false, view, projection)) return false;
    Vec3 point = { x, -y, -5 };
    C_MTXMultVec(view, &point, &point);
    if (point.z >= -.00001f) return false;
    HSD_CObj* camera = scene.camera->hsd_obj;
    float half = tanf(HSD_CObjGetFov(camera) * .00872664626f);
    *u = .5f + point.x / (-2 * point.z * half * HSD_CObjGetAspect(camera));
    *v = .5f - point.y / (-2 * point.z * half);
    return true;
}

bool nativeMenuScene_Unproject(float u, float v, float* x, float* y)
{
    Mtx view, inverse; Mtx44 projection;
    if (u < 0 || u > 1 || v < 0 || v > 1 ||
        !nativeMenuScene_View(false, false, view, projection) || !C_MTXInverse(view, inverse)) return false;
    HSD_CObj* camera = scene.camera->hsd_obj;
    float half = tanf(HSD_CObjGetFov(camera) * .00872664626f);
    Vec3 origin = { 0 }, direction = { (2*u-1)*half*HSD_CObjGetAspect(camera), (1-2*v)*half, -1 };
    C_MTXMultVec(inverse, &origin, &origin);
    C_MTXMultVecSR(inverse, &direction, &direction);
    if (fabsf(direction.z) < .00001f) return false;
    float distance = (-5-origin.z)/direction.z;
    if (distance < 0) return false;
    *x = origin.x + distance*direction.x;
    *y = -(origin.y + distance*direction.y);
    return true;
}

static HSD_JObj* runtime_joint(HSD_JObj* root, unsigned index)
{
    HSD_JObj* result = NULL;
    lb_80011E24(root, &result, index, -1);
    HSD_ASSERT(0, result);
    return result;
}

bool nativeMenuScene_Begin(const char* caption)
{
    if (!scene.camera || !scene.backdrop[0] || !scene.backdrop[1]) return false;
    scene.previous_camera = HSD_CObjGetCurrent();
    HSD_CObj* camera = scene.camera->hsd_obj;
    if (!HSD_CObjSetCurrent(camera)) return false;
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
    return true;
}

void nativeMenuScene_End(void)
{
    HSD_CObjEndCurrent();
    if (scene.previous_camera) HSD_CObjSetCurrent(scene.previous_camera);
    scene.previous_camera = NULL;
}

void nativeMenuScene_DrawRibbon(unsigned slot, const Mtx canvas, float x, float y, float width,
                                float height, bool selected, const char* label)
{
    if (slot >= 7 || !scene.ribbons[slot]) return;
    HSD_JObj* root = scene.ribbons[slot]->hsd_obj;
    // mnmain's idle and selected cursor tracks. The baked retail option name
    // is replaced by the hub's own glyph string, retaining the actual ribbon.
    HSD_JObjReqAnimAll(root, 0);
    HSD_JObjReqAnim(runtime_joint(root, 2), selected ? 70 : 20);
    // mnmain selects the lettering material with frames 0 (gold) and 1
    // (black), independently of the ribbon's longer animation tracks.
    HSD_JObjReqAnimAll(runtime_joint(root, 3), selected ? 1 : 0);
    HSD_JObjReqAnimAll(runtime_joint(root, 4), selected ? 80 : 49);
    HSD_JObjAnimAll(root);
    HSD_JObjSetFlags(runtime_joint(root, 3), JOBJ_HIDDEN);
    HSD_JObjSetFlagsAll(runtime_joint(root, 9), JOBJ_HIDDEN);
    HSD_JObjSetFlagsAll(runtime_joint(root, 11), JOBJ_HIDDEN);
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
    Mtx model = { { scale, 0, 0, x + width * .5f - .03345f * scale },
                 { 0, scale, 0, -y - height * .48449f },
                 { 0, 0, 1, -5 } };
    Mtx view; C_MTXConcat(canvas, model, view);
    HSD_StateInvalidate(-1); HSD_StateInitTev(); HSD_ClearVtxDesc();
    HSD_JObjDispAll(root, view, HSD_TRSP_ALL, 0);
    const HSD_MObj* lettering = runtime_joint(root, 3)->u.dobj->mobj;
    // The retail label's ink is a texture TEV constant, not its white
    // diffuse material. Its two-frame track supplies the actual menu colours.
    GXColor ink = lettering->tobj->tev->konst;
    u32 rgba = (u32) ink.r << 24 | (u32) ink.g << 16 |
        (u32) ink.b << 8 | (u32) (lettering->mat->alpha * 255);
    nativeMenuPaint_Begin(canvas);
    nativeMenuPaint_Text(label, x + width * (13.f / 127), y + height * (9.f / 31),
        width * (99.f / 127), height * (14.f / 31), rgba,
        NativeMenuFont_MenuOption, true, !selected);
    nativeMenuPaint_End();
}

void nativeMenuScene_Inspect(void)
{
    fprintf(stderr, "[hub-inspect] menu-assets records=%u arena=%zu ribbons=%d camera=%d\n",
        (unsigned) scene.assets.import.record_count, scene.assets.import.arena_used,
        scene.ribbons[0] != NULL, scene.camera != NULL);
    if (!scene.ribbons[0]) return;
    Vec3 eye; HSD_CObjGetEyePosition(scene.camera->hsd_obj, &eye);
    fprintf(stderr, "[hub-inspect] camera-eye=%.4f,%.4f,%.4f\n", eye.x, eye.y, eye.z);
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
