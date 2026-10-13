#include <aurora/aurora.h>
#include <aurora/event.h>
#include <aurora/main.h>
#include <SDL3/SDL_timer.h>
#include <melee/lb/lbanim_dat.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/types.h>
#include <math.h>

void native_fighter_preview_bind(HSD_GObj*, const char*, FigaTree*, HSD_Joint*, float, const char*);
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/state.h>
#include <dolphin/vi.h>
#include <stdio.h>
#include <stdlib.h>

static void require(bool ok, const char* message)
{
    if (!ok) { fprintf(stderr, "%s\n", message); exit(1); }
}

static void log_message(AuroraLogLevel level, const char* module, const char* text, unsigned len)
{
    fprintf(stderr, "[%s] %.*s\n", module, (int) len, text);
    if (level == LOG_FATAL) abort();
}

static void update_model(HSD_GObj* object)
{
    if (object->user_data) {
        Fighter* fighter = object->user_data;
        float before = fighter->x8A8_anim_frame;
        ftAnim_8006E9B4(object);
        if (before < fighter->x8A4_animBlendFrames &&
            fighter->x8A8_anim_frame == fighter->x8A4_animBlendFrames) {
            printf("Completed original fighter blend at %.1f frames; animation frame %.1f\n",
                   fighter->x8A8_anim_frame, fighter->cur_anim_frame);
        }
    }
    else HSD_JObjAnimAll(GET_JOBJ(object));
}

static void bind_clip(HSD_JObj* joint, FigaTree* clip, u32 node_count,
                      u32* node, FigaTrack** tracks)
{
    for (; joint; joint = joint->next) {
        require(*node < node_count && !(joint->flags & JOBJ_INSTANCE),
                "Clip does not describe this complete model skeleton");
        s8 count = clip->nodes[(*node)++];
        lbAnim_8001E6D8(joint, clip, *tracks, count);
        if (joint->aobj) HSD_AObjSetFlags(joint->aobj, AOBJ_LOOP);
        *tracks += count;
        bind_clip(joint->child, clip, node_count, node, tracks);
    }
}

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    setvbuf(stdout, NULL, _IONBF, 0);
    require(argc >= 3, "Usage: native_model_view <DAT> <joint-offset> [frame-count] [clip-DAT clip-root] [Mario-PlCo-DAT blend-frames Mario-data-DAT [screenshot-BMP [--test-blanking]]]");
    require(argc < 7 || argc >= 9, "Fighter preview requires PlCo.dat, blend duration and PlMr.dat");
    float blend_frames = argc >= 8 ? strtof(argv[7], NULL) : 0;
    require(isfinite(blend_frames) && blend_frames >= 0, "Invalid blend duration");
    unsigned frames = argc > 3 ? strtoul(argv[3], NULL, 0) : 300;
    FILE* file = fopen(argv[1], "rb");
    require(file != NULL, "Cannot open model DAT");
    require(fseek(file, 0, SEEK_END) == 0, "Cannot seek model");
    long size = ftell(file);
    require(size > 0 && fseek(file, 0, SEEK_SET) == 0, "Cannot size model");
    void* bytes = malloc(size);
    require(bytes && fread(bytes, 1, size, file) == size, "Cannot read model");
    fclose(file);
    void* descriptors = malloc(4 * 1024 * 1024);
    HSD_Dat dat;
    HSD_DatImport import;
    require(descriptors && HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
        HSD_DatImportInit(&import, &dat, descriptors, 4 * 1024 * 1024, 16384) == HSD_DAT_OK,
        "Cannot prepare model import");
    HSD_Joint* root = HSD_DatImportGraph(&import, strtoul(argv[2], NULL, 0), DAT_JOINT);
    require(root != NULL, "Cannot import model geometry");

    AuroraConfig config = { .appName = "Freak Melee - HSD model viewer",
        .windowWidth = 960, .windowHeight = 720, .vsync = true,
        .logCallback = log_message, .logLevel = LOG_INFO };
    aurora_initialize(1, argv, &config);
    GXInit(NULL, 0);
    u8* heap = malloc(32 * 1024 * 1024);
    require(heap != NULL, "Cannot reserve HSD heap");
    void* start = OSInitAlloc(heap, heap + 32 * 1024 * 1024, 1);
    OSHeapHandle handle = OSCreateHeap(start, heap + 32 * 1024 * 1024);
    require(handle >= 0, "Cannot create HSD heap");
    HSD_SetHeap(handle);
    OSSetCurrentHeap(handle);
    HSD_ObjSetHeap(32 * 1024 * 1024, NULL);
    HSD_ObjInit();
    HSD_IDSetup();
    HSD_GObjLibInitDataType object_config;
    HSD_GObj_803912E0(&object_config);
    HSD_GObj_80391304(&object_config);
    if (argc >= 7) ftPartsPObjSetDefaultClass();
    HSD_JObj* model = HSD_JObjLoadJoint(root);
    if (argc >= 7) ftPartsPObjClearDefaultClass();
    require(model != NULL, "Cannot load HSD model");
    void* clip_bytes = NULL;
    FigaTrack* clip_tracks = NULL;
    FigaTree clip;
    if (argc >= 6) {
        FILE* clip_file = fopen(argv[4], "rb");
        require(clip_file && fseek(clip_file, 0, SEEK_END) == 0, "Cannot open clip DAT");
        long clip_size = ftell(clip_file);
        require(clip_size > 0 && fseek(clip_file, 0, SEEK_SET) == 0, "Cannot size clip DAT");
        clip_bytes = malloc(clip_size);
        clip_tracks = malloc(4096 * sizeof(*clip_tracks));
        require(clip_bytes && clip_tracks && fread(clip_bytes, 1, clip_size, clip_file) == clip_size,
                "Cannot read clip DAT");
        fclose(clip_file);
        HSD_Dat clip_dat;
        u32 nodes;
        require(HSD_DatOpen(&clip_dat, clip_bytes, clip_size) == HSD_DAT_OK &&
            lbAnim_DecodeFigaTree(&clip_dat, strtoul(argv[5], NULL, 0), &clip,
                                  clip_tracks, 4096, &nodes) == HSD_DAT_OK,
            "Cannot decode animation clip");
        u32 node = 0;
        FigaTrack* track = clip_tracks;
        if (argc < 7) {
            bind_clip(model, &clip, nodes, &node, &track);
            require(node == nodes, "Clip contains more nodes than the model skeleton");
        } else {
            for (node = 0; node < nodes; ++node) track += clip.nodes[node];
        }
        HSD_JObjReqAnimAll(model, 0);
        printf("Bound %lu animation nodes, %llu tracks, %.1f frames\n", nodes,
               (unsigned long long) (track - clip_tracks), clip.frames);
    }
    HSD_CObj* camera = HSD_CObjAlloc();
    Vec3 eye = { 0, 10, 45 }, interest = { 0, 10, 0 };
    HSD_CObjSetEyePosition(camera, &eye);
    HSD_CObjSetInterest(camera, &interest);
    HSD_CObjSetPerspective(camera, 40, 4.0f / 3.0f);
    HSD_CObjSetNear(camera, 0.1f);
    HSD_CObjSetFar(camera, 1000);
    HSD_CObjSetViewportfx4(camera, 0, 640, 0, 480);
    HSD_CObjSetScissorx4(camera, 0, 640, 0, 480);
    *HSD_VIGetRenderMode() = GXNtsc480IntDf;
    HSD_VIGetRenderMode()->viTVmode = VI_TVMODE_NTSC_PROG;
    VIConfigure(HSD_VIGetRenderMode());
    HSD_LObj* light = HSD_LObjAlloc();
    HSD_LObjSetFlags(light, LOBJ_INFINITE | LOBJ_DIFFUSE | LOBJ_SPECULAR);
    Vec3 light_position = { 20, 30, 40 };
    HSD_LObjSetPosition(light, &light_position);
    HSD_LObjSetInterest(light, &interest);
    HSD_LObjSetColor(light, (GXColor) { 255, 255, 255, 255 });
    HSD_LObjAddCurrent(light);
    HSD_LObj* ambient = HSD_LObjAlloc();
    HSD_LObjSetColor(ambient, (GXColor) { 255, 255, 255, 255 });
    HSD_LObjAddCurrent(ambient);

    HSD_GObj* model_object = GObj_Create(0, 0, 0);
    HSD_GObj* camera_object = GObj_Create(1, 0, 0);
    require(model_object && camera_object, "Cannot create scene game objects");
    HSD_GObjObject_80390A70(model_object, HSD_GObj_JObjKind, model);
    HSD_GObjObject_80390A70(camera_object, HSD_GObj_CameraKind, camera);
    if (argc >= 7) native_fighter_preview_bind(model_object, argv[6], &clip, root, blend_frames, argv[8]);
    HSD_GObj_SetupProc(model_object, update_model, 0);
    GObj_SetupGXLink(model_object, HSD_GObj_JObjCallback, 0, 0);
    camera_object->gxlink_prios = 1;

    bool exiting = false;
    unsigned drawn = 0;
    uint64_t clock_start = SDL_GetTicksNS();
    while (!exiting && drawn < frames) {
        const AuroraEvent* event = aurora_update();
        for (; event && event->type != AURORA_NONE; ++event)
            if (event->type == AURORA_EXIT) exiting = true;
        if (exiting) break;
        uint64_t deadline = clock_start + (uint64_t) drawn * 1000000000 / 60;
        uint64_t now = SDL_GetTicksNS();
        if (now < deadline) SDL_DelayNS(deadline - now);
        if (!aurora_begin_frame()) continue;
        HSD_GObj_80390CFC();
        GXSetCopyClear((GXColor) { 32, 40, 56, 255 }, GX_MAX_Z24);
        HSD_StateInvalidate(HSD_STATE_ALL);
        HSD_StartRender(HSD_RP_SCREEN);
        require(HSD_CObjSetCurrent(camera), "Cannot activate model camera");
        HSD_LObjSetupInit(camera);
        HSD_GObj_80390ED0(camera_object, 7);
        HSD_CObjEndCurrent();
        if (argc >= 10 && drawn == 10) {
            require(!aurora_request_screenshot(NULL), "Null screenshot path accepted");
            require(!aurora_request_screenshot(""), "Empty screenshot path accepted");
            require(!aurora_request_screenshot("screenshot.bmp"), "BMP screenshot path accepted");
            require(aurora_request_screenshot(argv[9]), "Screenshot request rejected");
            require(!aurora_request_screenshot(argv[9]), "Overlapping screenshot request accepted");
        }
        if (argc >= 11 && strcmp(argv[10], "--test-blanking") == 0) {
            if (drawn == 60) HSD_VISetBlack(true);
            if (drawn == 180) HSD_VISetBlack(false);
            if (drawn == 30 || drawn == 120 || drawn == 240)
                printf("BLANKING_CHECK %u\n", drawn);
        }
        aurora_end_frame();
        ++drawn;
    }
    GXDrawDone();
    if (model_object->user_data) {
        Fighter* fighter = model_object->user_data;
        if (fighter->x8AC_animSkeleton) HSD_JObjUnref(fighter->x8AC_animSkeleton);
    }
    HSD_GObjFree(model_object);
    HSD_LObjDeleteCurrentAll(NULL);
    HSD_LObjUnrefThis(light);
    HSD_LObjUnrefThis(ambient);
    HSD_GObjFree(camera_object);
    GXDrawDone();
    aurora_shutdown();
    require(OSCheckHeap(handle) >= 0, "Rendering damaged the HSD heap");
    hsdForgetClassLibrary(NULL);
    OSDestroyHeap(handle);
    free(heap); free(descriptors); free(bytes); free(clip_tracks); free(clip_bytes);
    printf("Presented %u model frames\n", drawn);
    return 0;
}
