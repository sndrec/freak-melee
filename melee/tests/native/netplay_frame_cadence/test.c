#include "match_state.inc"
#include <stdarg.h>
#include <stdio.h>
static unsigned checks,received,sent,reconciled,rumble_ends;
static uint64_t clock_ms;
static bool receive_failed,send_failed,queued;
static uint64_t queue_first,queue_end;
static const char* failure;
static NativeMatchArena arena;
static _Alignas(64) uint8_t live[2048];
static _Alignas(8) uint8_t saved[4096];
static uint32_t* world;
static NativeNetplayExchange exchange;
NativeMatchPool nativeGObjPool;
bool nativeNetplayTimingEnabled;
static u32 seed;u32* seed_ptr=&seed;
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"cadence line %u: %s (%s)\n",__LINE__,#c,failure?failure:"");exit(1);}} while(0)
static uint64_t fake_clock(void){return clock_ms;}
static void fake_sleep(DWORD ms){clock_ms+=ms;}
#define GetTickCount64 fake_clock
#define Sleep fake_sleep
static void fail(const char* reason){failure=reason;netplay.failed=netplay.checkpoint.failed=true;}
static bool receive_packets(void)
{
    ++received;if(receive_failed)return false;
    if(queued){for(uint64_t f=queue_first;f<queue_end;++f){PADStatus pad={.button=PAD_BUTTON_A,.err=PAD_ERR_NONE};
        CHECK(nativeRollbackInput_SubmitRemote(&netplay.inputs,f,1,&pad));
        CHECK(nativeRollbackInput_SubmitRemote(&netplay.inputs,f,3,&pad));}queued=false;}
    return true;
}
static bool send_packets(void){++sent;return !send_failed;}
static NativeConfirmedCheckpointStep reconcile_inputs(uint64_t* frame)
{
    ++reconciled;
    NativeConfirmedCheckpointStep step=nativeConfirmedCheckpoint_Reconcile(&netplay.checkpoint,&netplay.inputs,nativeGObjPool.lifetime,frame);
    if(step==NativeConfirmedCheckpoint_Failed)fail("controlled reconciliation failed");
    return step;
}
static uint64_t semantic_state_digest(uint64_t frame){return *world+frame;}
uint64_t nativeReplay_SampleCount(void){return netplay.current_frame;}
void nativeRandom_TraceFrame(uint64_t frame,bool enabled,bool replaying){(void)frame;(void)enabled;(void)replaying;}
void nativeRandom_TraceBytes(const char* name,const void* data,size_t bytes){(void)name;(void)data;(void)bytes;abort();}
void nativeNetplayTiming_Begin(NativeNetplayTimingPhase phase){(void)phase;abort();}
void nativeNetplayTiming_End(NativeNetplayTimingPhase phase){(void)phase;abort();}
void nativeNetplayTiming_Checkpoints(uint64_t a,uint64_t b,size_t bytes){(void)a;(void)b;(void)bytes;abort();}
void HSD_PadRumbleEndResimulation(u8 mask){CHECK(mask==5);++rumble_ends;}
bool AXDriverKeyOff(int voice){(void)voice;abort();}
VsSceneController* gmVs_GetController_0(void){abort();}
bool nativeMatchSnapshot_Capture(NativeMatchSnapshotPart parts[NativeMatchSnapshot_Count],void* backing,size_t capacity,size_t* written,NativeMatchSnapshotMeasurement* profile)
{(void)parts;(void)backing;(void)capacity;(void)written;(void)profile;abort();}
void OSReport(char* format,...){(void)format;abort();}
#include "match_boundaries.inc"

static void initialize(void)
{
    memset(&netplay,0,sizeof(netplay));memset(&exchange,0,sizeof(exchange));
    received=sent=reconciled=rumble_ends=0;clock_ms=0;
    queued=receive_failed=send_failed=false;failure=NULL;
    nativeGObjPool.lifetime=29;netplay.exchange=&exchange;netplay.active=true;
    CHECK(nativeMatchArena_Begin(&arena,live,sizeof(live),11));
    world=nativeMatchArena_Allocate(&arena,sizeof(*world),8);CHECK(world);*world=0;
    NativeMatchArena* owners[]={&arena};
    CHECK(nativeConfirmedCheckpoint_Bind(&netplay.checkpoint,owners,1,saved,sizeof(saved),29));
    CHECK(nativeRollbackInput_Init(&netplay.inputs,5));HSD_AudioRollbackReset();
}
static NativeConfirmedCheckpointStep finish(uint64_t* frame,bool ending)
{
    PADStatus pads[4]={0},resolved[4];
    if(netplay.checkpoint.replaying)CHECK(nativeRollbackInput_GetFrame(&netplay.inputs,*frame,resolved));
    else CHECK(nativeRollbackInput_PrepareFrame(&netplay.inputs,*frame,pads,resolved));
    for(unsigned p=0;p<4;++p)*world+=resolved[p].button;
    return nativeNetplayMatch_EndFrame(*frame,frame,ending);
}
static void predict(uint64_t count)
{
    uint64_t frame=netplay.checkpoint.cursor;
    while(frame<count){CHECK(nativeNetplayMatch_BeginFrame(&frame));CHECK(!netplay.checkpoint.replaying);
        CHECK(finish(&frame,false)==NativeConfirmedCheckpoint_Continue);}
}
static void queue(uint64_t first,uint64_t end){CHECK(!queued);queue_first=first;queue_end=end;queued=true;}
static void catch_up(uint64_t* frame)
{
    while(netplay.checkpoint.replaying){CHECK(nativeNetplayMatch_BeginFrame(frame));
        NativeConfirmedCheckpointStep step=finish(frame,false);
        CHECK(step==NativeConfirmedCheckpoint_Replaying||step==NativeConfirmedCheckpoint_CaughtUp);}
}
int main(void)
{
    initialize();predict(6);queue(0,1);uint64_t frame=6;
    CHECK(nativeNetplayMatch_BeginFrame(&frame)&&netplay.checkpoint.replaying&&frame==0);
    CHECK(finish(&frame,false)==NativeConfirmedCheckpoint_Replaying);
    // A newer batch arrives while replay is in progress; it remains queued.
    queue(1,2);catch_up(&frame);CHECK(frame==6&&netplay.visible_tick_pending);
    unsigned before=received,prior_reconcile=reconciled;
    CHECK(nativeNetplayMatch_BeginFrame(&frame));
    CHECK(!netplay.checkpoint.replaying&&!netplay.visible_tick_pending);
    CHECK(received==before&&reconciled==prior_reconcile&&queued);
    // The real normal input filter still receives packets before preparing input.
    CHECK(receive_packets());CHECK(finish(&frame,false)==NativeConfirmedCheckpoint_Replaying);
    catch_up(&frame);CHECK(frame==7&&netplay.checkpoint.boundary==2);
    CHECK(*world==14*PAD_BUTTON_A);CHECK(!queued);

    // A queued visible tick cannot bypass the maximum prediction window.
    initialize();predict(15);queue(5,6);frame=15;
    CHECK(nativeNetplayMatch_BeginFrame(&frame)&&netplay.checkpoint.replaying);
    CHECK(finish(&frame,false)==NativeConfirmedCheckpoint_Replaying);catch_up(&frame);
    CHECK(netplay.visible_tick_pending&&netplay.checkpoint.boundary==0);
    before=received;queue(0,1);
    CHECK(nativeNetplayMatch_BeginFrame(&frame)&&netplay.checkpoint.replaying&&frame==0);
    CHECK(received>before&&netplay.input_stall_count==1);
    CHECK(finish(&frame,false)==NativeConfirmedCheckpoint_Replaying);catch_up(&frame);
    CHECK(netplay.checkpoint.boundary==1&&frame==15);
    CHECK(nativeNetplayMatch_BeginFrame(&frame)&&!netplay.checkpoint.replaying);
    CHECK(finish(&frame,false)==NativeConfirmedCheckpoint_Continue);
    CHECK(netplay.checkpoint.cursor-netplay.checkpoint.boundary==15);

    // Finishing still flushes delayed inputs and confirms before scene exit.
    initialize();predict(5);frame=5;CHECK(nativeNetplayMatch_BeginFrame(&frame));
    exchange.artificial_delay_frames=14;queue(0,6);
    CHECK(finish(&frame,true)==NativeConfirmedCheckpoint_Replaying);
    CHECK(!queued&&sent>=2&&exchange.artificial_delay_frames==14);
    CHECK(!netplay.visible_tick_pending&&netplay.checkpoint.replaying);
    while(frame<5){CHECK(nativeNetplayMatch_BeginFrame(&frame));CHECK(finish(&frame,false)==NativeConfirmedCheckpoint_Replaying);}
    CHECK(nativeNetplayMatch_BeginFrame(&frame));
    CHECK(finish(&frame,true)==NativeConfirmedCheckpoint_CaughtUp);
    CHECK(!netplay.visible_tick_pending&&!netplay.checkpoint.replaying&&netplay.checkpoint.boundary==6);
    CHECK(*world==12*PAD_BUTTON_A);

    // Transport failures and a timeout remain terminal; no tick is admitted.
    initialize();predict(15);netplay.visible_tick_pending=true;frame=15;receive_failed=true;
    CHECK(!nativeNetplayMatch_BeginFrame(&frame)&&!netplay.frame_active&&received>0);
    initialize();predict(15);netplay.visible_tick_pending=true;frame=15;send_failed=true;
    CHECK(!nativeNetplayMatch_BeginFrame(&frame)&&!netplay.frame_active&&sent>0);
    initialize();predict(15);netplay.visible_tick_pending=true;frame=15;
    CHECK(!nativeNetplayMatch_BeginFrame(&frame)&&netplay.failed&&clock_ms>=NETPLAY_INPUT_TIMEOUT_MS);
    CHECK(!netplay.frame_active&&netplay.checkpoint.cursor==15);
    initialize();predict(6);netplay.visible_tick_pending=true;frame=6;++arena.lifetime;
    CHECK(!nativeNetplayMatch_BeginFrame(&frame)&&netplay.failed&&!netplay.frame_active);
    printf("cadence checks=%u; actual Begin/End bodies, checkpoint/input/arena/audio; controlled transport/gameplay; no performance claim\n",checks);
    return 0;
}
