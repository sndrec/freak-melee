#include "native_menu_motion.h"
#include <melee/lb/lbfile.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/wobj.h>
#include <SDL3/SDL_timer.h>
#include <math.h>
#include <string.h>

static struct {
    HSD_GObj* owners[2];
    HSD_JObj* joints[2];
    void* bytes;
    void* arena;
    Mtx idle_inverse;
    float fov, pixels_per_unit;
    float starts[2];
    Uint64 started;
    bool playing;
} motion;

static void forget_owner(void* owner)
{
    *(HSD_GObj**) owner = NULL;
    if (!motion.owners[0] && !motion.owners[1]) {
        // The scene heap owns these buffers when objects are torn down by the
        // scene manager. Explicit unload keeps its own copies until both joints
        // have released their animation streams.
        motion.bytes = motion.arena = NULL;
        motion.playing = false;
    }
}

void nativeMenuMotion_Unload(void)
{
    void* bytes = motion.bytes;
    void* arena = motion.arena;
    for (unsigned i = 0; i < 2; ++i)
        if (motion.owners[i]) HSD_GObjFree(motion.owners[i]);
    if (arena) HSD_Free(arena);
    if (bytes) HSD_Free(bytes);
    memset(&motion, 0, sizeof(motion));
}

static u32 reference(const HSD_Dat* dat, u32 slot)
{
    u32 target;
    const char* external;
    return HSD_DatResolve(dat, slot, &target, &external) == HSD_DAT_REF_LOCAL ? target : 0;
}

static u32 nth(const HSD_Dat* dat, u32 node, unsigned child, unsigned next, unsigned* index)
{
    while (node) {
        if (*index == 0) return node;
        --*index;
        u32 found = nth(dat, reference(dat, node + child), child, next, index);
        if (found) return found;
        node = reference(dat, node + next);
    }
    return 0;
}

static HSD_Joint joint(const HSD_Dat* dat, u32 offset)
{
    HSD_Joint desc = { 0 };
    HSD_ASSERT(0, HSD_DatReadU32(dat, offset + 4, &desc.flags));
    f32 values[9];
    for (unsigned i = 0; i < 9; ++i)
        HSD_ASSERT(0, HSD_DatReadF32(dat, offset + 20 + i * 4, &values[i]));
    desc.rotation = (Vec3){ values[0], values[1], values[2] };
    desc.scale = (Vec3){ values[3], values[4], values[5] };
    desc.position = (Vec3){ values[6], values[7], values[8] };
    return desc;
}

void nativeMenuMotion_Load(void)
{
    if (motion.owners[0] && motion.owners[1]) return;
    nativeMenuMotion_Unload();
    size_t size = lbFileGetSize("MnMaAll");
    motion.bytes = HSD_MemAlloc((size + 31) & ~(size_t) 31);
    lbFile_8001668C("MnMaAll", motion.bytes, &size);
    HSD_Dat dat;
    HSD_ASSERT(0, HSD_DatOpen(&dat, motion.bytes, size) == HSD_DAT_OK);
    u32 root, anim, camera;
    HSD_ASSERT(0, HSD_DatFindRoot(&dat, "MenMainConTop_Top_joint", &root));
    HSD_ASSERT(0, HSD_DatFindRoot(&dat, "MenMainConTop_Top_animjoint", &anim));
    HSD_ASSERT(0, HSD_DatFindRoot(&dat, "ScMenMain_cam_int1_camera", &camera));
    unsigned index = 2;
    HSD_Joint parent = joint(&dat, nth(&dat, root, 8, 12, &index));
    index = 3;
    HSD_Joint moving = joint(&dat, nth(&dat, root, 8, 12, &index));
    index = 3;
    u32 animated = nth(&dat, anim, 0, 4, &index);
    motion.arena = HSD_MemAlloc(64 * 1024);
    HSD_DatImport imported;
    HSD_ASSERT(0, HSD_DatImportInit(&imported, &dat, motion.arena, 64 * 1024, 128) == HSD_DAT_OK);
    HSD_AnimJoint animation = { 0 };
    animation.aobjdesc = HSD_DatImportGraph(&imported, reference(&dat, animated + 8), DAT_AOBJ);
    HSD_CObjDesc* cam = HSD_DatImportGraph(&imported, camera, DAT_CAMERA);
    HSD_ASSERT(0, animation.aobjdesc && cam && imported.status == HSD_DAT_OK);
    motion.fov = cam->perspective.fov;
    float depth = cam->common.eyepos->pos.z - parent.position.z;
    motion.pixels_per_unit = 240 / (depth * tanf(motion.fov * .00872664626f));
    // Preserve the stock parent orientation, which turns this joint's local Z
    // rotation into the main menu's tilted, three-dimensional spin. Its world
    // placement is replaced by the hub content's pivot on the 640x480 canvas.
    parent.position = (Vec3){ 0 };
    parent.child = &moving;
    for (unsigned i = 0; i < 2; ++i) {
        motion.owners[i] = GObj_Create(0xFE02 + i, 5, 0x80);
        GObj_InitUserData(motion.owners[i], 0, forget_owner, &motion.owners[i]);
        HSD_JObj* tree = HSD_JObjLoadJoint(&parent);
        HSD_GObjObject_80390A70(motion.owners[i], HSD_GObj_JObjKind, tree);
        motion.joints[i] = tree->child;
        HSD_JObjAddAnim(motion.joints[i], &animation, NULL, NULL);
    }
}

float nativeMenuMotion_Frame(void)
{
    return motion.playing ? (float) (SDL_GetTicksNS() - motion.started) * .00000006f : 20;
}

bool nativeMenuMotion_Active(void)
{
    return motion.owners[0] && motion.owners[1] && motion.playing && nativeMenuMotion_Frame() < 20;
}

void nativeMenuMotion_Start(bool backwards)
{
    if (!motion.owners[0] || !motion.owners[1]) return;
    // mnmain.c: ENTER_TO 320, ENTER_FROM 340, EXIT_TO 360, EXIT_FROM 380.
    motion.starts[0] = backwards ? 380 : 340;
    motion.starts[1] = backwards ? 360 : 320;
    HSD_JObjReqAnim(motion.joints[1], motion.starts[1] + 19);
    HSD_JObjAnim(motion.joints[1]);
    HSD_ASSERT(0, C_MTXInverse(HSD_JObjGetMtxPtr(motion.joints[1]), motion.idle_inverse));
    motion.started = SDL_GetTicksNS();
    motion.playing = true;
}

bool nativeMenuMotion_Sample(bool outgoing, Mtx view, Mtx44 projection)
{
    if (!nativeMenuMotion_Active()) return false;
    HSD_JObj* pose = motion.joints[outgoing ? 0 : 1];
    HSD_JObjReqAnim(pose, motion.starts[outgoing ? 0 : 1] + fminf(19, nativeMenuMotion_Frame()));
    HSD_JObjAnim(pose);
    Mtx relative, a, b;
    C_MTXConcat(HSD_JObjGetMtxPtr(pose), motion.idle_inverse, relative);
    float scale = motion.pixels_per_unit;
    C_MTXScale(a, 1 / scale, 1 / scale, 1 / scale);
    C_MTXConcat(relative, a, b);
    C_MTXScale(a, scale, scale, scale);
    C_MTXConcat(a, b, relative);
    C_MTXTrans(a, -390, 230, 5);
    C_MTXConcat(relative, a, b);
    C_MTXTrans(a, 390, -230, -5);
    C_MTXConcat(a, b, relative);
    float distance = 240 / tanf(motion.fov * .00872664626f);
    C_MTXTrans(a, -320, 240, -distance + 5);
    C_MTXConcat(a, relative, view);
    C_MTXPerspective(projection, motion.fov, 4.f / 3, 1, 5000);
    return true;
}
