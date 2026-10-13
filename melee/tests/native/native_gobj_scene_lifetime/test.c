#include <melee/gm/types.h>
#include <melee/gm/gmvsmode.h>
#include <melee/gm/gmvsmelee.h>
#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gm_1A45.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_netplay_match.h>
#include <melee/gm/native_match_object_census.h>
#include <melee/gm/native_snapshot_census.h>
#include <melee/gm/native_replay.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/it/native_article_resource_scene.h>
#include <melee/it/native_article_resource_animation.h>
#include <melee/it/native_article_resource_model.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/gm/gm_1601.h>
#include <melee/lb/types.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lbheap.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routing.inc"
static struct stateMachine state_machine;
bool gmMain_ExitRequested;
MatchExitInfo gmVsMelee_VsExitInfo,gmVsMelee_SuddenDeathExitInfo;
static PreloadedGameModeState preload;
static HSD_GObj* proxy;
static NativeMatchPoolReference retained;
static GameSceneInfo* active_info;
static unsigned checks,transitions,phase,scene_id,requested_next,mode_id,cleanups;
static unsigned proc_calls,draw_calls,prepared_sudden_death;
static uint64_t last_epoch;
static bool tied,have_retained;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u phase%u scene%u: %s\n",(unsigned)__LINE__,phase,scene_id,#x);assert(x);}}while(0)
void OSReport(char* text,...){(void)text;}
void nativeSnapshotCensus_Abort(const char* reason){(void)reason;abort();}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason){fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
void HSD_LObjRemoveAll(HSD_LObj* p){(void)p;abort();}
void HSD_JObjRemoveAll(HSD_JObj* p){(void)p;abort();}
void HSD_LObj_803668EC(HSD_LObj* p){(void)p;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* p){(void)p;abort();}
void HSD_JObjDispAll(HSD_JObj* p,Mtx m,u32 f,u32 r){(void)p;(void)m;(void)f;(void)r;abort();}
void HSD_FogSet(HSD_Fog* p){(void)p;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* p){(void)p;abort();}
void HSD_CObjEndCurrent(void){abort();}

const NativeNetplayLaunchConfig* nativeMatchLaunch_Netplay(void){return NULL;}
bool nativeMatchLaunch_ObjectCensus(void){return false;}
bool nativeMatchLaunch_TraceFighterState(void){return false;}
bool nativeNetplayMatch_Prepare(bool sudden){(void)sudden;abort();}
bool nativeNetplayMatch_HasFailed(void){return false;}
void nativeNetplayMatch_Abort(const char* reason){(void)reason;abort();}
void nativeNetplayMatch_Shutdown(bool keep,const MatchEnd* result){(void)keep;(void)result;abort();}
bool nativeMatchLaunch_PrepareSuddenDeathFighters(void){CHECK(phase==5&&nativeGObjPool.lifetime==last_epoch);++prepared_sudden_death;return true;}
void nativeMatchObjectCensus_BeginScene(uint32_t scene){(void)scene;abort();}
void nativeMatchObjectCensus_Sample(uint64_t frame,uint64_t sample,bool replaying){(void)frame;(void)sample;(void)replaying;abort();}
void nativeMatchObjectCensus_FinishScene(void){abort();}
void nativeMatchObjectCensus_Abort(const char* reason){(void)reason;abort();}
uint64_t nativeReplay_SampleCount(void){return 0;}
// This scene routing fixture constructs no articles or interpreted actors.
bool nativeArticleResourceScene_Retire(void){return true;}
bool nativeArticleResourceAnimation_Retire(void){return true;}
bool nativeArticleResourceModel_Retire(void){return true;}
bool nativeArticleModelScene_Retire(void){return true;}
void nativeScriptFighter_Report(void){}
bool gm_WasMatchCanceled(MatchOutcome outcome){(void)outcome;return false;}
bool gm_MatchHasMultipleWinners(MatchEnd* result){(void)result;return tied;}

void lbDvd_80018CF4(int kind)
{
    CHECK(kind==3&&phase==0);phase=1;
    CHECK(nativeGObjPool.lifetime==0&&nativeGObjProcPool.lifetime==0);
    CHECK(GObj_Create(999,0,0)==NULL);
    if(have_retained)CHECK(nativeMatchPool_Resolve(retained)==NULL);
}
void HSD_SisLib_803A6048(u32 size){CHECK(phase==1&&size>=0x2400);}
PreloadedGameModeState* lbDvd_GetPreloadCacheScene(void){return &preload;}
LbHeapStatus lbHeap_80015BB8(int index){CHECK(index==2||index==3);return LbHeapStatus_Create;}
void lbDvd_80018254(void){CHECK(phase==1&&nativeGObjPool.lifetime==0);}
void Toy_803127D4(void){CHECK(phase==1);}
void tyDisplay_8031C8B8(void){CHECK(phase==1);}
void lbMthp_8001F800(void){CHECK(phase==6);phase=7;}

static void mode_enter(GameModeState* state)
{
    CHECK(phase==1&&state->id==mode_id);phase=2;
    // Audited mode callbacks only prepare data/home flags. This controlled
    // boundary deliberately performs no model/actor creation before HSD init.
    CHECK(nativeGObjPool.lifetime==0&&GObj_Create(998,0,0)==NULL);
}
void gm_801A4BD4(void)
{
    CHECK(phase==2);phase=3;
    HSD_GObjLibInitDataType config;HSD_GObj_803912E0(&config);config.gproc_pri_max=24;
    HSD_GObj_80391304(&config);
    CHECK(nativeGObjPool.lifetime==last_epoch+1);
    last_epoch=nativeGObjPool.lifetime;
    CHECK(nativeGObjPool.used==0&&nativeGObjProcPool.used==0);
    if(have_retained)CHECK(nativeMatchPool_Resolve(retained)==NULL);
}
void gm_801A4B88(GameSceneInfo* info){CHECK(phase==3);active_info=info;}
static void proc(HSD_GObj* object){CHECK(object==proxy);++proc_calls;}
static void drawing(HSD_GObj* object,int pass){CHECK(object==proxy&&pass==0);++draw_calls;}
static void cleanup(void* object){CHECK(object==proxy);++cleanups;}
static void scene_enter(void* data)
{
    (void)data;CHECK(phase==3&&active_info->scene_kind==scene_id);phase=4;
    proxy=GObj_Create((u16)(100+mode_id),1,0);CHECK(proxy);
    proxy->x34_unk=proxy;GObj_InitUserData(proxy,6,cleanup,proxy);
    HSD_GObj_SetupProc(proxy,proc,0);GObj_SetupGXLinkMax(proxy,drawing,0);
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,proxy,&retained));have_retained=true;
}
static void frame_callback(void){CHECK(phase==4&&nativeMatchPool_Resolve(retained)==proxy);}
void gm_801A4D34(void(*frame)(void),GameSceneInfo* info)
{
    CHECK(info==active_info&&phase==4);if(frame)frame();
    HSD_GObj_80390CFC();HSD_GObj_80390FC0();
    CHECK(nativeGObjPool.used==1&&nativeGObjProcPool.used==1);
}
static void scene_exit(void* data)
{
    (void)data;CHECK(phase==4);phase=5;
    CHECK(nativeGObjPool.lifetime==last_epoch&&nativeMatchPool_Resolve(retained)==proxy);
}
static void mode_exit(GameModeState* state)
{
    CHECK(phase==5&&state->id==mode_id);phase=6;
    CHECK(nativeMatchPool_Resolve(retained)==proxy);
    // Routing is supplied at the audited native-home boundary, not a claim
    // that CSS UI, match result rules or challenge selection ran here.
    gm_SetNextGameModeStateId((u8)requested_next);
}
static GameScene scenes[]={
    {GS_CSS,frame_callback,scene_enter,scene_exit,NULL},
    {GS_SSS,frame_callback,scene_enter,scene_exit,NULL},
    {GS_VS,frame_callback,scene_enter,scene_exit,NULL},
    {GS_SUDDEN_DEATH,frame_callback,scene_enter,scene_exit,NULL},
    {GS_RESULTS,frame_callback,scene_enter,scene_exit,NULL},
};
GameScene* gm_FindGameSceneHandler(u8 kind){for(unsigned i=0;i<5;++i)if(scenes[i].kind==kind)return &scenes[i];abort();}

#include "pipeline.inc"

static void route(const unsigned* ids,unsigned count)
{
    GameModeState states[]={
        {gmVsMode_State_Css,3,0,mode_enter,mode_exit,{GS_CSS,NULL,NULL}},
        {gmVsMode_State_Sss,3,0,mode_enter,mode_exit,{GS_SSS,NULL,NULL}},
        {gmVsMode_State_Vs,3,0,mode_enter,mode_exit,{GS_VS,NULL,NULL}},
        {gmVsMode_State_SuddenDeath,3,0,mode_enter,mode_exit,{GS_SUDDEN_DEATH,NULL,NULL}},
        {gmVsMode_State_Results,3,0,mode_enter,mode_exit,{GS_RESULTS,NULL,NULL}},
        {gmVsMode_State_Practice,3,0,mode_enter,mode_exit,{GS_VS,NULL,NULL}},
        {GM_GAMEMODESTATE_TERMINATE},
    };
    GameMode mode={.kind=GM_VS,.states=states};
    for(unsigned index=0;index<count;++index){
        mode_id=ids[index];scene_id=states[mode_id].info.scene_kind;
        requested_next=index+1<count?ids[index+1]:gmVsMode_State_Css;
        state_machine.routing.curr_state_id=(u8)mode_id;
        phase=0;tied=mode_id==gmVsMode_State_Vs&&requested_next==gmVsMode_State_SuddenDeath;
        gm_801A4014(&mode);
        CHECK(phase==7&&state_machine.routing.curr_state_id==requested_next);
        CHECK(cleanups==0&&nativeGObjPool.used==1);++transitions;
    }
}
int main(void)
{
    const unsigned match[]={gmVsMode_State_Css,gmVsMode_State_Practice,gmVsMode_State_Sss,gmVsMode_State_Vs,gmVsMode_State_Results,gmVsMode_State_Css};
    const unsigned home[]={gmVsMode_State_Css,gmVsMode_State_Practice,gmVsMode_State_Css};
    const unsigned sudden[]={gmVsMode_State_Vs,gmVsMode_State_SuddenDeath,gmVsMode_State_Results,gmVsMode_State_Css};
    route(match,6);route(home,3);route(sudden,4);
    CHECK(proc_calls==transitions&&draw_calls==transitions&&prepared_sudden_death==1);
    CHECK(nativeGObjPool_RetireScene()&&nativeMatchPool_Resolve(retained)==NULL&&cleanups==0);
    printf("{\"transitions\":%u,\"checks\":%u,\"proc_ticks\":%u,\"draw_callbacks\":%u,\"sudden_death_preparations\":%u,\"retirement_destructors\":%u}\n",
        transitions,checks,proc_calls,draw_calls,prepared_sudden_death,cleanups);
    return 0;
}
