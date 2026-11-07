#include "native_menu_reference.h"
#include "native_menu_paint.h"
#include <melee/lb/lbfile.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/memory.h>
#include <stdio.h>
#include <string.h>

enum { MAX_OBJECTS = 32, IMPORT_BYTES = 4 * 1024 * 1024 };
static struct {
    HSD_Dat dat;
    HSD_DatImport import;
    void* bytes;
    void* arena;
    HSD_GObj* objects[MAX_OBJECTS];
    HSD_CObj* camera;
    HSD_LObj* lights[8];
    HSD_JObj* models[16];
    HSD_ImageDesc* image;
    HSD_Fog* fog;
    unsigned live, object_count, light_count, model_count;
    bool active;
} reference;

static void forget(void* slot)
{
    // GObjFree removes user data before its attached object. Finish that
    // object's destruction while its imported descriptors are still alive.
    HSD_GObjObject_80390B0C(*(HSD_GObj**) slot);
    *(HSD_GObj**) slot = NULL;
    if (--reference.live) return;
    HSD_Free(reference.arena);
    HSD_Free(reference.bytes);
    memset(&reference, 0, sizeof(reference));
}

void nativeMenuReference_Unload(void)
{
    // The same callbacks handle ordinary scene teardown.
    unsigned count = reference.object_count;
    for (unsigned i = 0; i < count; ++i)
        if (reference.objects[i]) HSD_GObjFree(reference.objects[i]);
}

static HSD_GObj* own(unsigned kind, void* object)
{
    HSD_ASSERT(0, reference.object_count < MAX_OBJECTS);
    HSD_GObj** slot = &reference.objects[reference.object_count++];
    *slot = GObj_Create(0xFE10, 5, 0x80);
    ++reference.live;
    GObj_InitUserData(*slot, 0, forget, slot);
    if (object) HSD_GObjObject_80390A70(*slot, kind, object);
    return *slot;
}

static u32 pointer(u32 offset)
{
    u32 result = 0;
    HSD_ASSERT(0, HSD_DatReadU32(&reference.dat, offset, &result));
    return result;
}

static void* graph(u32 offset, HSD_DatImportType type)
{
    if (!offset) return NULL;
    void* result = HSD_DatImportGraph(&reference.import, offset, type);
    HSD_ASSERTREPORT(0, result, "Menu reference conversion: type=%u offset=%x status=%u error=%x\n",
                     type, offset, reference.import.status, reference.import.error_offset);
    return result;
}

static void* root(const char* name, HSD_DatImportType type)
{
    u32 offset;
    return HSD_DatFindRoot(&reference.dat, name, &offset) ? graph(offset, type) : NULL;
}

static void lights(u32 table)
{
    for (unsigned i = 0; table && i < 8; ++i) {
        u32 entry = pointer(table + i * 4);
        if (!entry) break;
        HSD_LObj* light = HSD_LObjLoadDesc(graph(pointer(entry), DAT_LIGHT));
        own(HSD_GObj_LightKind, light);
        reference.lights[reference.light_count++] = light;
    }
}

static void camera(HSD_CObjDesc* desc)
{
    HSD_ASSERT(0, desc);
    reference.camera = HSD_CObjLoadDesc(desc);
    own(HSD_GObj_CameraKind, reference.camera);
}

static void model(HSD_Joint* joint, HSD_AnimJoint* anim,
                  HSD_MatAnimJoint* mat, HSD_ShapeAnimJoint* shape)
{
    HSD_ASSERT(0, joint && reference.model_count < 16);
    HSD_JObj* object = HSD_JObjLoadJoint(joint);
    own(HSD_GObj_JObjKind, object);
    HSD_JObjAddAnimAll(object, anim, mat, shape);
    HSD_JObjReqAnimAll(object, 0);
    HSD_JObjAnimAll(object);
    reference.models[reference.model_count++] = object;
}

static void named_model(const char* prefix)
{
    char name[128];
    snprintf(name, sizeof(name), "%s_joint", prefix);
    HSD_Joint* joint = root(name, DAT_JOINT);
    snprintf(name, sizeof(name), "%s_animjoint", prefix);
    HSD_AnimJoint* anim = root(name, DAT_ANIMJOINT);
    snprintf(name, sizeof(name), "%s_matanim_joint", prefix);
    HSD_MatAnimJoint* mat = root(name, DAT_MATANIM_JOINT);
    snprintf(name, sizeof(name), "%s_shapeanim_joint", prefix);
    model(joint, anim, mat, root(name, DAT_SHAPE_JOINT));
}

static void named_camera_and_lights(void)
{
    for (u32 i = 0; i < reference.dat.root_count; ++i) {
        u32 offset;
        const char* name;
        HSD_DatSymbol(&reference.dat, false, i, &offset, &name);
        size_t length = strlen(name);
        if (!reference.camera && length >= 7 && !strcmp(name + length - 7, "_camera"))
            camera(graph(offset, DAT_CAMERA));
        if (length >= 13 && !strcmp(name + length - 13, "_scene_lights"))
            lights(offset);
    }
    HSD_ASSERT(0, reference.camera);
}

static void load(const char* filename)
{
    size_t size = lbFileGetSize(filename);
    reference.bytes = HSD_MemAlloc((size + 31) & ~(size_t) 31);
    reference.arena = HSD_MemAlloc(IMPORT_BYTES);
    own(0, NULL); // Archive owner, including sprite-only references.
    lbFile_8001668C(filename, reference.bytes, &size);
    HSD_ASSERT(0, HSD_DatOpen(&reference.dat, reference.bytes, size) == HSD_DAT_OK);
    HSD_ASSERT(0, HSD_DatImportInit(&reference.import, &reference.dat,
        reference.arena, IMPORT_BYTES, 8192) == HSD_DAT_OK);
}

void nativeMenuReference_Select(const char* preset)
{
    nativeMenuReference_Unload();
    if (!strcmp(preset, "off")) return;
    if (!strncmp(preset, "gallery-image-", 14)) {
        unsigned i = preset[14] - '0';
        if (i >= 3 || preset[15]) return;
        load("TyMnBg.dat");
        const char* names[] = { "ToyFigureBg3_sobjdesc", "ToyFigureBg5_sobjdesc", "ToyFigureBg6_sobjdesc" };
        u32 offset;
        HSD_ASSERT(0, HSD_DatFindRoot(&reference.dat, names[i], &offset));
        HSD_ASSERT(0, pointer(offset + 4) == 0); // These retail backgrounds use RGBA8.
        reference.image = graph(pointer(offset), DAT_IMAGE);
    } else if (!strncmp(preset, "tournament-", 11)) {
        bool backdrop = !strcmp(preset, "tournament-background");
        unsigned page = backdrop ? 0 : preset[11] - '1';
        if (page >= 2 || (!backdrop && preset[12])) return;
        load(page ? "GmTou2p.usd" : "GmTou1p.usd");
        u32 scene;
        HSD_ASSERT(0, HSD_DatFindRoot(&reference.dat, "ScGamTour_scene_data", &scene));
        camera(graph(pointer(pointer(scene + 4)), DAT_CAMERA));
        lights(pointer(scene + 8));
        if (pointer(scene + 12)) {
            reference.fog = HSD_FogLoadDesc(graph(pointer(pointer(scene + 12)), DAT_FOG));
            own(HSD_GObj_FogKind, reference.fog);
        }
        // Display the original authored objects together. The preview does not
        // execute the tournament logic that repeats and selects individual rows.
        u32 models = pointer(scene);
        for (unsigned i = 0; i < 16; ++i) {
            u32 entry = pointer(models + i * 4);
            if (!entry) break;
            // fn_801935B8 / fn_801910E0: model 5 is the continuously
            // animated background, separate from model 4's navigation panel.
            if (backdrop && i != 5) continue;
            model(graph(pointer(entry), DAT_JOINT),
                graph(pointer(entry + 4) ? pointer(pointer(entry + 4)) : 0, DAT_ANIMJOINT),
                graph(pointer(entry + 8) ? pointer(pointer(entry + 8)) : 0, DAT_MATANIM_JOINT),
                graph(pointer(entry + 12) ? pointer(pointer(entry + 12)) : 0, DAT_SHAPE_JOINT));
        }
    } else if (!strcmp(preset, "gallery") || !strcmp(preset, "collection") || !strcmp(preset, "lottery")) {
        bool gallery = !strcmp(preset, "gallery"), collection = !strcmp(preset, "collection");
        load(gallery ? "TyMnView.usd" : collection ? "TyMnDisp.usd" : "TyMnFigp.usd");
        named_camera_and_lights();
        named_model(gallery ? "ToyFigureBack_Top" : collection ? "ToyDspBg_Top" : "ToyFigurePonBg_Top");
    } else {
        fprintf(stderr, "[menu-reference] Unknown preset %s\n", preset);
        return;
    }
    reference.active = true;
    fprintf(stderr, "[hub-inspect] menu-reference %s models=%u lights=%u records=%lu\n",
        preset, reference.model_count, reference.light_count, reference.import.record_count);
}

bool nativeMenuReference_Draw(void)
{
    if (!reference.active) return false;
    HSD_CObj* previous = HSD_CObjGetCurrent();
    if (reference.camera) {
        if (!HSD_CObjSetCurrent(reference.camera)) return true;
        HSD_CObjEraseScreen(reference.camera, 1, 1, 1);
        HSD_LObjDeleteCurrentAll(NULL);
        for (unsigned i = 0; i < reference.light_count; ++i)
            HSD_LObjAddCurrent(reference.lights[i]);
        HSD_LObjSetupInit(reference.camera);
        HSD_FogSet(reference.fog);
        Mtx view;
        HSD_CObjGetViewingMtx(reference.camera, view);
        for (unsigned i = 0; i < reference.model_count; ++i) {
            HSD_JObjAnimAll(reference.models[i]);
            HSD_JObjDispAll(reference.models[i], view, HSD_TRSP_ALL, 0);
        }
        HSD_CObjEndCurrent();
        if (previous) HSD_CObjSetCurrent(previous);
    }
    if (reference.image) {
        HSD_ASSERT(0, reference.image->format == GX_TF_RGBA8);
        nativeMenuPaint_Begin(NULL);
        nativeMenuPaint_Image(reference.image->image_ptr, reference.image->width,
            reference.image->height, 0, 0, 640, 480);
        nativeMenuPaint_End();
    }
    return true;
}
