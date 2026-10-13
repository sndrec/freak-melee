#include "initialize.h"

#include <stdarg.h>

#include "aobj.h"
#include "class.h"
#include "debug.h"
#include "displayfunc.h"
#include "fobj.h"
#include "id.h"
#include "list.h"
#include "lobj.h"
#include "mtx.h"
#include "memory.h"
#include "objalloc.h"
#include "object.h"
#include "random.h"
#include "robj.h"
#include "shadow.h"
#include "state.h"
#include "synth.h"
#include "tev.h"
#include "video.h"
#ifdef MELEE_NATIVE
#include "native_animation_pool.h"
#include "native_hsd_arena.h"
#endif
#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>

static void HSD_DVDInit(void);
static void HSD_GXInit(void);
static void HSD_OSInit(void);

static HSD_MemReport memReport;
static void* hsd_heap_next_arena_lo;
static void* hsd_heap_next_arena_hi;
static s32 init_done;
static s32 shown;

static GXRenderModeObj* rmode = &GXNtsc480IntDf;
static int iparam_heap_max_num = 4;
static u32 iparam_audio_heap_size = HSD_DEFAULT_AUDIO_SIZE;
static GXColor HSD_Init_804D5E1C = { 0 };

void HSD_InitComponent(void)
{
    HSD_OSInit();
    {
        HSD_VIStatus vi_status;
        GXColor black = { 0, 0, 0, 0 };

        vi_status.rmode = *rmode;
        vi_status.black = GX_TRUE;
        vi_status.vf = GX_TRUE;
        vi_status.gamma = GX_GM_1_0;
        vi_status.clear_clr = black;
        vi_status.clear_z = GX_MAX_Z24;
        vi_status.update_clr = GX_ENABLE;
        vi_status.update_alpha = GX_ENABLE;
        vi_status.update_z = GX_ENABLE;

        HSD_VIInit(&vi_status);
    }

    HSD_GXInit();
    HSD_DVDInit();
    HSD_IDSetup();
    HSD_ObjInit();
    HSD_LogInit();
    init_done = true;
}

void HSD_DVDInit(void) {}

void HSD_GXInit(void)
{
    GXLightObj lightobj;
    int i;

    GXInitLightPos(&lightobj, 1, 0, 0);
    GXInitLightDir(&lightobj, 1, 0, 0);
    GXInitLightAttn(&lightobj, 1, 0, 0, 1, 0, 0);
    GXInitLightColor(&lightobj, HSD_Init_804D5E1C);

    for (i = 0; i < 8; i++) {
        GXLoadLightObjImm(&lightobj, HSD_Index2LightID(i));
    }
    HSD_StateInvalidate(-1);
}

void HSD_OSInit(void)
{
    uintptr_t new_arena_lo;
    uintptr_t new_arena_hi;
    uintptr_t old_arena_lo = (uintptr_t) OSGetArenaLo();
    uintptr_t old_arena_hi = (uintptr_t) OSGetArenaHi();
    memReport.total = OSGetPhysicalMemSize();
    memReport.system = memReport.total - (uintptr_t) OSGetArenaHi() +
                       (uintptr_t) OSGetArenaLo() - memReport.xfb -
                       memReport.gxfifo;
    old_arena_lo = (uintptr_t) OSInitAlloc(
        (void*) old_arena_lo, (void*) old_arena_hi, iparam_heap_max_num);
    OSSetArenaLo((void*) old_arena_lo);

    new_arena_lo = OSRoundUp32B(old_arena_lo);
    new_arena_hi = OSRoundDown32B(old_arena_hi);
    HSD_Synth_804D6018 = OSCreateHeap(
        (void*) new_arena_lo, (void*) (new_arena_lo + iparam_audio_heap_size));
    new_arena_lo += iparam_audio_heap_size;
    hsd_heap_next_arena_lo = (void*) new_arena_lo;
    hsd_heap_next_arena_hi = (void*) new_arena_hi;
    HSD_SetHeap(OSCreateHeap((void*) new_arena_lo, (void*) new_arena_hi));
    OSSetCurrentHeap(HSD_GetHeap());
    memReport.heap = new_arena_hi - new_arena_lo;
    HSD_ObjSetHeap(new_arena_hi - new_arena_lo, NULL);
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0,nativeHsdArena_BeginHeap(),"Cannot reserve HSD object backing.\n");
#endif
    OSSetArenaLo((void*) new_arena_hi);
}

void HSD_GetNextArena(void** lo, void** hi)
{
    *lo = hsd_heap_next_arena_lo;
    *hi = hsd_heap_next_arena_hi;
}

OSHeapHandle HSD_CreateMainHeap(void* lo, void* hi)
{
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0, nativeAnimationPool_CanBeginHeap(),
        "Cannot recreate the HSD heap during an animation callback.\n");
#endif
    int i;
    void (*cb_table[])(void* lo, void* hi) = {
        _HSD_AObjForgetMemory,
        _HSD_DispForgetMemory,
        _HSD_IDForgetMemory,
        _HSD_ObjAllocForgetMemory,
        _HSD_RandForgetMemory,
        _HSD_RObjForgetMemory,
        NULL,
    };
    hsdForgetClassLibrary("sysdolphin_base_library");
    for (i = 0; cb_table[i] != NULL; i++) {
        cb_table[i](hsd_heap_next_arena_lo, hsd_heap_next_arena_hi);
    }
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0,nativeHsdArena_BeginHeap(),"Cannot reset HSD object backing.\n");
#endif
    // Forget the old registry before registering this heap's reset allocators.
    // Clearing it after ObjInit left live descriptors invisible to ownership.
    HSD_ObjInit();
    if (lo != NULL) {
        hsd_heap_next_arena_lo = lo;
    }
    if (hi != NULL) {
        hsd_heap_next_arena_hi = hi;
    }
    OSDestroyHeap(HSD_GetHeap());
    HSD_SetHeap(OSCreateHeap(hsd_heap_next_arena_lo, hsd_heap_next_arena_hi));
    OSSetCurrentHeap(HSD_GetHeap());
    HSD_ObjSetHeap((uintptr_t) hsd_heap_next_arena_hi -
                       (uintptr_t) hsd_heap_next_arena_lo,
                   NULL);
    return HSD_GetHeap();
}

#ifdef MUST_MATCH
static char str_pix_fmt_neq_gx_pf_rgb565_z16[] = "pix_fmt != GX_PF_RGB565_Z16";
#endif

void HSD_ObjDumpStat(void)
{
    struct {
        HSD_ObjAllocData* (*func)(void);
        char* label;
    } types[] = {
#ifndef MELEE_NATIVE
                  { HSD_AObjGetAllocData, "aobj" },
                  { HSD_FObjGetAllocData, "fobj" },
#endif
                  { HSD_IDGetAllocData, "id" },
                  { HSD_SListGetAllocData, "slist" },
                  { HSD_DListGetAllocData, "dlist" },
                  { HSD_VecGetAllocData, "vec" },
                  { HSD_MtxGetAllocData, "mtx" },
                  { HSD_RObjGetAllocData, "robj" },
                  { HSD_RvalueObjGetAllocData, "rval" },
                  { HSD_ShadowGetAllocData, "shadow" },
                  { HSD_RenderGetAllocData, "render" },
                  { HSD_ChanGetAllocData, "chan" },
                  { HSD_TevRegGetAllocData, "tevreg" },
                  { NULL, NULL } };
    int i;
#ifdef MELEE_NATIVE
    OSReport("animation pool: aobj\tusing %u\tfreed %u\tpeak %u\n",
        nativeAObjPool.used,nativeAObjPool.free_count,nativeAObjPool.peak);
    OSReport("animation pool: fobj\tusing %u\tfreed %u\tpeak %u\n",
        nativeFObjPool.used,nativeFObjPool.free_count,nativeFObjPool.peak);
#endif

    for (i = 0; types[i].label; i++) {
        OSReport("objalloc: %s\tusing %d\tfreed %d\tpeak %d\n", types[i].label,
                 HSD_ObjAllocGetUsing(types[i].func()),
                 HSD_ObjAllocGetFreed(types[i].func()),
                 HSD_ObjAllocGetPeak(types[i].func()));
    }
}

bool HSD_SetInitParameter(HSD_InitParam param, ...)
{
    va_list ap;
    bool ok = false;

    if (init_done) {
        if (!shown) {
            OSReport(
                "init parameter should be set before invoking HSD_Init().\n");
            shown = true;
        }
        return false;
    }

    va_start(ap, param);
    switch (param) {
    case HSD_INIT_HEAP_MAX_NUM: {
        u32 heap_size = va_arg(ap, u32);
        if (heap_size > 0) {
            iparam_heap_max_num = heap_size;
            ok = true;
        }
    } break;

    case HSD_INIT_AUDIO_HEAP_SIZE: {
        u32 heap_size = va_arg(ap, u32);
        if (heap_size > 0) {
            iparam_audio_heap_size = heap_size;
            ok = true;
        }
    } break;

    case HSD_INIT_RENDER_MODE_OBJ:
        rmode = va_arg(ap, GXRenderModeObj*);
        break;
    }
    va_end(ap);

    return ok;
}
