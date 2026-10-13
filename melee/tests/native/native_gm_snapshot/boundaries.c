#include <melee/gm/gm_1A45.h>
#include <melee/gm/native_match_classes.h>
// This fixture has no renderer class instances to initialize.
void nativeMatchClasses_Initialize(void) {}
#include <melee/gm/gmmain.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gmvs.h>
#include <melee/gm/gmscdata.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_match_object_census.h>
#include <melee/gm/native_netplay_match.h>
#include <melee/gm/native_rollback_benchmark.h>
#include <melee/gm/native_rollback_probe.h>
#include <melee/gm/native_rollback_census.h>
#include <melee/gm/native_replay.h>
#include <melee/gm/native_profile_input.h>
#include <melee/db/db.h>
#include <melee/db/dbnativecontrol.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/lb_0192.h>
#include <melee/lb/lbheap.h>
#include <melee/ui/native_shell.h>
#include <melee/ui/native_shell_profiles.h>
#include <melee/ui/native_shell_rules.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/perf.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/sobjlib.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <melee/cm/camera.h>
#include <sysdolphin/baselib/random.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftlib.h>
#include <aurora/aurora.h>
#include <aurora/event.h>
#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void fixture_process(void);
extern void fixture_activity(void);
unsigned fixture_reads, fixture_motors, fixture_audio, fixture_constructor, fixture_frames;
static unsigned replay_samples;
NativeMatchPool nativeGObjPool, nativeGObjProcPool;
bool nativeScriptRuntime_Idle(void) { return true; }
void fn_8016A488(s32 slot) { (void)slot; abort(); }
bool ftLib_80086960(Fighter_GObj* object) { (void)object; abort(); }
bool gmMain_ExitRequested;
struct gmMainLib_8046B0F0_t gmMainLib_8046B0F0;
DbLKind DbLevel = DbLKind_DebugRom;
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
HSD_GObj* HSD_GObj_804D7818;
u32 HSD_GObj_RenderCallbackDepth;
struct _unk_gobj_struct HSD_GObj_DelayedProcInfo;
u8 HSD_SObjLib_804D7960;
GObjFuncs HSD_SObjLib_8040C3A4;
HSD_VIInfo HSD_VIData;

// Hardware/host/scheduler boundaries are controlled. gm_1A45.c's actual loop,
// pause decisions, counters and masks and all LB/controller/RNG code run.
BOOL PADInit(void) { return true; }
u32 PADRead(PADStatus* status)
{
    ++fixture_reads;
    memset(status, 0, 4*sizeof(*status));
    for (unsigned i=0;i<4;++i) {
        status[i].button = gm_801A4BA8()%3 ? PAD_BUTTON_A : PAD_BUTTON_B;
        status[i].stickX = (s8)(gm_801A4BA8()+i);
    }
    return 0;
}
int PADReset(unsigned long mask) { (void)mask; return true; }
void PADControlMotor(s32 port,u32 command) { (void)port; (void)command; ++fixture_motors; }
bool nativeReplay_IsPlayback(void) { return true; }
void nativeReplay_RecordPads(const PADStatus* status) { (void)status; ++replay_samples; }
u64 nativeReplay_SampleCount(void) { return replay_samples; }
void nativeProfileInput_Apply(PADStatus status[4]) { (void)status; abort(); }
void HSD_DevComPoll(void) {}
void HSD_AudioOutputPoll(void) { ++fixture_audio; }
void lb_800192A8(Event callback) { (void)callback; }
void __assert(char* file,u32 line,char* expression)
{ fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,expression);abort(); }
void OSReport(char* format, ...) { (void)format; }

bool nativeMatchLaunch_Fast(void) { return true; }
bool nativeMatchLaunch_SnapshotCensus(void) { return true; }
bool nativeMatchLaunch_ObjectCensus(void) { return false; }
bool nativeMatchLaunch_RollbackCensus(void) { return false; }
bool nativeMatchLaunch_RollbackProbe(void) { return false; }
bool nativeMatchLaunch_RollbackBenchmark(void) { return false; }
bool nativeMatchLaunch_RollbackCorrectionProbe(void) { return false; }
unsigned nativeMatchLaunch_RollbackCorrectionFrame(void) { return 0; }
const NativeNetplayLaunchConfig* nativeMatchLaunch_Netplay(void) { return NULL; }
bool nativeRollbackCensus_Init(void) { abort(); }
void nativeRollbackCensus_BeginSimulation(void) { abort(); }
void nativeRollbackCensus_EndSimulation(void) { abort(); }
void nativeRollbackCensus_BeginRender(void) { abort(); }
void nativeRollbackCensus_EndRender(void) { abort(); }
void nativeRollbackCensus_Shutdown(void) { abort(); }
bool nativeRollbackProbe_Init(bool correction,uint32_t frame) { (void)correction;(void)frame;abort(); }
bool nativeRollbackProbe_BeginFrame(u64 frame) { (void)frame;abort(); }
bool nativeRollbackProbe_IsResimulating(void) { abort(); }
void nativeRollbackProbe_Abort(const char* reason) { (void)reason;abort(); }
NativeRollbackProbeStep nativeRollbackProbe_EndFrame(u64 frame,u64* next) { (void)frame;(void)next;abort(); }
void nativeRollbackProbe_Shutdown(void) { abort(); }
bool nativeRollbackBenchmark_Init(void) { abort(); }
bool nativeRollbackBenchmark_BeginFrame(u64 frame) { (void)frame;abort(); }
void nativeRollbackBenchmark_BeginInput(void) { abort(); }
void nativeRollbackBenchmark_InputPoll(bool waiting) { (void)waiting;abort(); }
void nativeRollbackBenchmark_EndInput(void) { abort(); }
void nativeRollbackBenchmark_BeginSimulation(void) { abort(); }
void nativeRollbackBenchmark_EndSimulation(void) { abort(); }
void nativeRollbackBenchmark_BeginRender(void) { abort(); }
void nativeRollbackBenchmark_EndRender(void) { abort(); }
bool nativeRollbackBenchmark_IsResimulating(void) { abort(); }
void nativeRollbackBenchmark_Abort(const char* reason) { (void)reason;abort(); }
bool nativeRollbackBenchmark_EndFrame(u64 frame,u64* next) { (void)frame;(void)next;abort(); }
void nativeRollbackBenchmark_Shutdown(void) { abort(); }
bool nativeNetplayMatch_Init(void) { abort(); }
bool nativeNetplayMatch_BeginFrame(u64* frame) { (void)frame;abort(); }
bool nativeNetplayMatch_IsResimulating(void) { abort(); }
void nativeNetplayMatch_Abort(const char* reason) { (void)reason;abort(); }
NativeConfirmedCheckpointStep nativeNetplayMatch_EndFrame(u64 frame,u64* next,bool ending) { (void)frame;(void)next;(void)ending;abort(); }
void nativeMatchObjectCensus_Sample(u64 frame,u64 sample,bool hidden) { (void)frame;(void)sample;(void)hidden;abort(); }
void gm_Scene_Vs_Abort(MatchOutcome outcome) { (void)outcome;abort(); }

void dbNativeControl_Poll(void) {}
void dbNativeControl_OnInputSample(void) {}
void db_RequestScreenshot(void) {}
void db_TakeScreenshotIfPending(void) {}
void db_CheckScreenshot(void) {}
void nativeHome_Tick(void) {}
void nativeHome_ApplyResultsInput(void) {}
bool nativeHome_IsSelection(void) { return false; }
bool nativeShellProfiles_IsOpen(void) { return false; }
bool nativeShellRules_IsOpen(void) { return false; }
bool nativeShell_IsEditingText(void) { return false; }
void nativeShell_Update(void) {}
const AuroraEvent* aurora_update(void) { return NULL; }
bool aurora_begin_frame(void) { return true; }
bool aurora_begin_frame_without_present(void) { return true; }
bool aurora_begin_frame_without_output(void) { abort(); }
void aurora_end_frame_without_output(void) { abort(); }
void aurora_end_frame(void) { ++fixture_frames; }
void aurora_end_frame_without_present(void) { abort(); }
Uint64 SDL_GetTicksNS(void) { abort(); }
void SDL_Delay(Uint32 milliseconds) { (void)milliseconds; }
void HSD_PerfSetStartTime(void) {}
void HSD_PerfSetCPUTime(void) {}
void HSD_PerfSetDrawTime(void) {}
void HSD_PerfSetTotalTime(void) {}
void HSD_PerfInitStat(void) {}
void lbAudioAx_80027DF8(void) { ++fixture_audio; }
void lbAudioAx_8002835C(void) { ++fixture_constructor; }
void HSD_GObj_803912E0(HSD_GObjLibInitDataType* config)
{ memset(config,0,sizeof(*config));config->p_link_max=3;config->gx_link_max=3; }
u8 HSD_GObj_803912A8(HSD_GObjLibInitDataType* config,GObjFuncs* funcs)
{ (void)config;(void)funcs;return 1; }
void HSD_GObj_80391304(HSD_GObjLibInitDataType* config) { (void)config;++fixture_constructor; }
void HSD_SObjLib_803A44A4(void) {}
void hsd_80392474(void) {}
void un_802FF78C(void) {}
void lb_80014534(void) {}
HSD_GObj* GObj_Create(u16 classifier,u8 link,u8 priority)
{ (void)link;(void)priority;NativeMatchPoolReference ref;HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&ref);if(!object)abort();object->classifier=classifier;return object; }
HSD_GObjProc* HSD_GObj_SetupProc(HSD_GObj* object,HSD_GObjEvent callback,u8 link)
{ (void)object;(void)callback;(void)link;return NULL; }
void HSD_GObj_80390CFC(void) { fixture_process(); }
void HSD_StartRender(HSD_RenderPass pass) { (void)pass;fixture_activity(); }
void HSD_GObj_80390FC0(void) { (void)HSD_Rand(); }
void HSD_Init_803755A8(void) {}
void HSD_ClearVtxDesc(void) { abort(); }
void GXInvalidateVtxCache(void) {}
void GXInvalidateTexAll(void) {}
void GXDrawDone(void) {}
void GXSetCopyFilter(u8 aa,const u8 pattern[12][2],u8 vf,const u8 filter[7]) { (void)aa;(void)pattern;(void)vf;(void)filter; }
void GXSetDispCopyGamma(GXGamma gamma) { (void)gamma; }
void GXSetCopyClear(GXColor color,u32 depth) { (void)color;(void)depth; }
void HSD_StateSetColorUpdate(int enable) { (void)enable; }
void HSD_StateSetAlphaUpdate(int enable) { (void)enable; }
void HSD_StateSetZMode(int enable,int compare,int update) { (void)enable;(void)compare;(void)update; }
void lbHeap_80015DF8(void) { abort(); }
void hsdDumpClassStat(HSD_ClassInfo* info,bool recursive,s32 level) { (void)info;(void)recursive;(void)level;abort(); }
void HSD_ObjDumpStat(void) { abort(); }
void db_PrintEntityCounts(void) { abort(); }
void db_PrintThreadInfo(void) { abort(); }
HSD_GObj* gm_80165388(u16 a,u8 b,u8 c,s32 d) { (void)a;(void)b;(void)c;(void)d;abort(); }
void gm_801653C8(HSD_GObj* object) { (void)object;abort(); }
GameScene* gm_GetAllGameScenes(void) { abort(); }
GameMode* gm_GetAllGameModes(void) { abort(); }

// No main-camera graph is constructed in this GM fixture. BeginScene must
// refuse its null root before asking the class owner; the connected camera
// fixture runs actual class references and companion restoration separately.
NativeClassReference nativeClass_Reference(const void* p,size_t n)
{ (void)p;(void)n;abort(); }
void* nativeClass_Resolve(NativeClassReference ref) { (void)ref;abort(); }
bool nativeClass_TypeKnown(const HSD_ClassInfo* info,size_t bytes)
{ (void)info;(void)bytes;abort(); }
// This GM fixture only exercises effect-control scene lifetime hooks. Actual
// ObjAlloc exclusion and effect-control Save/Load run in the owner fixture.
bool nativeObjAlloc_BufferIndependent(const void* data,size_t bytes)
{ (void)data;(void)bytes;abort(); }
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
NativeObjAllocReference nativeObjAlloc_Reference(HSD_ObjAllocData* allocator,const void* data,size_t bytes)
{ (void)allocator;(void)data;(void)bytes;abort(); }
void* nativeObjAlloc_Resolve(NativeObjAllocReference reference)
{ (void)reference;abort(); }
HSD_ObjAllocData* HSD_SListGetAllocData(void) {abort();}
// No item actor is constructed by this control/player fixture.
#include <melee/it/item.h>
void Item_OnUserDataRemove(void* actor) { (void)actor;abort(); }
u8 HSD_GObj_CameraKind=4,HSD_GObj_JObjKind=5;
s32 Camera_SetBounds(Vec4* x) { (void)x;abort(); }
s32 fn_8002F908(HSD_RectF32* x) { (void)x;abort(); }
s32 fn_8002FBA0(HSD_RectF32* x) { (void)x;abort(); }
bool fn_8015B4EC(Vec3* x) { (void)x;abort(); }
bool ftMh_MS_343_80151428(Vec3* x) { (void)x;abort(); }

// No stage graph/startup runs in this fixture. The actual GM exit still
// retires the stage lease; class/ground restoration is exercised separately.
#include <melee/gr/stage.h>
StKind Stage_80225194(void) { abort(); }
s8 HSD_GObj_FogKind=6;

// No class candidate graph is constructed by this GM-only fixture. Connected
// owner and live cohorts run the real methods; unexpected use here terminates.
void* nativeClass_ResolveSnapshot(NativeClassSnapshotView view,NativeClassReference reference)
{(void)view;(void)reference;abort();}
bool nativeClass_SnapshotObjectType(NativeClassSnapshotView view,NativeClassReference reference,const HSD_ClassInfo* type)
{(void)view;(void)reference;(void)type;abort();}

// This fixture observes the GM loop without the live renderer/census harness.
void nativeSnapshotCensus_AdmitArenas(uint32_t scene) { (void)scene; }
