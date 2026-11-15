#include "native_menu_prize.h"
#include <melee/lb/lbfile.h>
#include <melee/lb/lbspdisplay.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <string.h>

enum { IMPORT_BYTES = 256 * 1024, MAX_OBJECTS = 12 };
static struct {
    HSD_Dat dat;
    HSD_DatImport import;
    void* bytes;
    void* arena;
    HSD_GObj* objects[MAX_OBJECTS];
    HSD_CObj* camera;
    HSD_JObj* model;
    HSD_LObj* lights[8];
    unsigned count, live, light_count;
} prize;

static void forget(void* slot)
{
    // Destroy each attached object before freeing its imported descriptors.
    HSD_GObjObject_80390B0C(*(HSD_GObj**)slot);
    *(HSD_GObj**)slot = NULL;
    if (--prize.live) return;
    HSD_Free(prize.arena); HSD_Free(prize.bytes);
    memset(&prize, 0, sizeof(prize));
}
static void own(unsigned kind, void* object)
{
    HSD_ASSERT(0, prize.count < MAX_OBJECTS);
    HSD_GObj** slot = &prize.objects[prize.count++];
    *slot = GObj_Create(0xFE11, 5, 0x80); ++prize.live;
    GObj_InitUserData(*slot, 0, forget, slot);
    if (object) HSD_GObjObject_80390A70(*slot, kind, object);
}
static u32 pointer(u32 offset)
{
    u32 result; HSD_ASSERT(0, HSD_DatReadU32(&prize.dat, offset, &result));
    return result;
}
static void* graph(u32 offset, HSD_DatImportType type)
{
    if (!offset) return NULL;
    void* result = HSD_DatImportGraph(&prize.import, offset, type);
    HSD_ASSERTREPORT(0, result, "Prize conversion: type=%u offset=%x status=%u error=%x\n",
        type, offset, prize.import.status, prize.import.error_offset);
    return result;
}
void nativeMenuPrize_Unload(void)
{
    unsigned count = prize.count;
    for (unsigned i = 0; i < count; ++i)
        if (prize.objects[i]) HSD_GObjFree(prize.objects[i]);
}
void nativeMenuPrize_Load(void)
{
    if (prize.model) return;
    size_t size = lbFileGetSize("IfPrize.usd");
    prize.bytes = HSD_MemAlloc((size + 31) & ~(size_t)31);
    prize.arena = HSD_MemAlloc(IMPORT_BYTES);
    own(0, NULL);
    lbFile_8001668C("IfPrize.usd", prize.bytes, &size);
    HSD_ASSERT(0, HSD_DatOpen(&prize.dat, prize.bytes, size) == HSD_DAT_OK);
    HSD_ASSERT(0, HSD_DatImportInit(&prize.import, &prize.dat, prize.arena, IMPORT_BYTES, 1024) == HSD_DAT_OK);
    u32 scene; HSD_ASSERT(0, HSD_DatFindRoot(&prize.dat, "ScInfPrize_scene_data", &scene));
    prize.camera = HSD_CObjLoadDesc(graph(pointer(pointer(scene + 4)), DAT_CAMERA));
    own(HSD_GObj_CameraKind, prize.camera);
    u32 lights = pointer(scene + 8);
    for (unsigned i = 0; lights && i < 8; ++i) {
        u32 entry = pointer(lights + i * 4); if (!entry) break;
        HSD_LObj* light = HSD_LObjLoadDesc(graph(pointer(entry), DAT_LIGHT));
        own(HSD_GObj_LightKind, light); prize.lights[prize.light_count++] = light;
    }
    u32 model = pointer(pointer(scene));
    prize.model = HSD_JObjLoadJoint(graph(pointer(model), DAT_JOINT));
    own(HSD_GObj_JObjKind, prize.model);
    HSD_JObjAddAnimAll(prize.model,
        graph(pointer(model + 4) ? pointer(pointer(model + 4)) : 0, DAT_ANIMJOINT),
        graph(pointer(model + 8) ? pointer(pointer(model + 8)) : 0, DAT_MATANIM_JOINT),
        graph(pointer(model + 12) ? pointer(pointer(model + 12)) : 0, DAT_SHAPE_JOINT));
}
void nativeMenuPrize_Draw(float frame, unsigned category)
{
    if (!prize.model) return;
    HSD_CObj* previous = HSD_CObjGetCurrent();
    if (!HSD_CObjSetCurrent(prize.camera)) return;
    HSD_LObjDeleteCurrentAll(NULL);
    for (unsigned i = 0; i < prize.light_count; ++i) HSD_LObjAddCurrent(prize.lights[i]);
    HSD_LObjSetupInit(prize.camera);
    // ifprize.c holds the entrance at frame 10; category zero is the stock
    // milestone notice. The material track selects the original heading art.
    HSD_JObjReqAnimAll(prize.model, frame);
    HSD_JObj* heading = NULL;
    lb_80011E24(prize.model, &heading, 2, -1);
    HSD_DObjReqAnimAll(heading->u.dobj, category);
    HSD_DObjReqAnimAll(heading->child->u.dobj, category);
    HSD_JObjAnimAll(prize.model);
    Mtx view; HSD_CObjGetViewingMtx(prize.camera, view);
    HSD_StateInvalidate(-1); HSD_StateInitTev(); HSD_ClearVtxDesc();
    HSD_JObjDispAll(prize.model, view, HSD_TRSP_ALL, 0);
    HSD_CObjEndCurrent();
    if (previous) HSD_CObjSetCurrent(previous);
}
