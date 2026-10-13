#include <melee/lb/native_dynamics_snapshot.h>
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/jobj.h>
#include <melee/lb/lbvector.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,rejections,matrix_calls;
static bool idle=true,script_idle=true;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %u: %s\n",(unsigned)__LINE__,#x);exit(1);}}while(0)
bool nativeGmLoop_SnapshotBoundary(void) {return idle;}
bool nativeScriptRuntime_Idle(void) {return script_idle;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) {joint->flags|=JOBJ_MTX_DIRTY;}
void HSD_JObjSetupMatrixSub(HSD_JObj* joint) {joint->flags&=~JOBJ_MTX_DIRTY;++matrix_calls;}
void __assert(char* file,u32 line,char* reason) {(void)file;(void)line;(void)reason;abort();}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "consumer.inc"
#pragma clang diagnostic pop
static NativeDynamicsSnapshot saved,bad,after,endpoint,repeat;
static HSD_JObj joints[4];
static void roundtrip(void)
{
    size_t bytes=0;CHECK(nativeDynamics_Save(&saved,sizeof(saved),&bytes));CHECK(bytes==sizeof(saved));
    CHECK(nativeDynamics_Validate(&saved,bytes));
    memset(&dynamics_state,0xA5,sizeof(dynamics_state));CHECK(nativeDynamics_Load(&saved,bytes));
    CHECK(nativeDynamics_Save(&after,sizeof(after),&bytes));CHECK(!memcmp(&after,&saved,sizeof(saved)));
}
static void reject(void)
{
    NativeDynamicsState before=dynamics_state;
    CHECK(!nativeDynamics_Validate(&bad,sizeof(bad)));CHECK(!nativeDynamics_Load(&bad,sizeof(bad)));
    CHECK(!memcmp(&before,&dynamics_state,sizeof(before)));++rejections;
}
int main(void)
{
    size_t bytes=0;CHECK(!nativeDynamics_Save(&saved,sizeof(saved),&bytes));lbDynamics_Reset();roundtrip();
    CHECK(dynamics_state.free_nodes==&dynamics_state.nodes[0]);CHECK(dynamics_state.free_wind==&dynamics_state.wind[0]);
    NativeWindReference wind_ref;
    CHECK(nativeDynamics_WindReference(&dynamics_state.wind[7],&wind_ref));
    CHECK(nativeDynamics_ResolveWind(wind_ref)==&dynamics_state.wind[7]);
    CHECK(!nativeDynamics_WindReference((void*)1,&wind_ref));
    CHECK(!nativeDynamics_WindReference((void*)((u8*)&dynamics_state.wind[0]+1),&wind_ref));
    CHECK(!nativeDynamics_WindReference(&dynamics_state.wind[0],(void*)&dynamics_state));
    NativeWindReference wrong=wind_ref;wrong.reserved=1;CHECK(!nativeDynamics_ResolveWind(wrong));
    wrong=wind_ref;wrong.slot=WIND_FIELD_COUNT;CHECK(!nativeDynamics_ResolveWind(wrong));
    wrong=wind_ref;++wrong.epoch;CHECK(!nativeDynamics_ResolveWind(wrong));
    DynamicsDesc desc={0};
    for(unsigned i=0;i<4;++i) {joints[i].child=i<3 ? &joints[i+1] : NULL;joints[i].scale=(Vec3){1,1,1};joints[i].translate.x=(float)i;joints[i].mtx[0][3]=(float)i;}
    lb_8000FD48(joints,&desc,4);CHECK(desc.count==4 && desc.data==&dynamics_state.nodes[0]);CHECK(matrix_calls==4);
    CHECK(dynamics_state.free_nodes==&dynamics_state.nodes[4]);CHECK(desc.data->next==&dynamics_state.nodes[1]);roundtrip();
    DynamicsDesc companion=desc;lb_8000FD18(&desc);CHECK(!desc.data);CHECK(dynamics_state.free_nodes==&dynamics_state.nodes[3]);
    CHECK(dynamics_state.nodes[3].desc.lb_unk0.jobj==&joints[3]);roundtrip();
    // Retained free payload is preserved, not a live joint ownership claim.
    dynamics_state.nodes[3].desc.lb_unk0.jobj=(void*)42;roundtrip();
    lbDynamics_Reset();lb_8000FD48(joints,&desc,4);companion=desc;
    Vec3 direction={1,0,0};
    struct lb_80011A50_t* first=lb_80011A50(&direction,-1,0.5f,0,0.1f,-10000,10000,10000,-10000);
    CHECK(first==&dynamics_state.wind[0]);CHECK(dynamics_state.active_wind==first);
    lb_800119DC(&direction,3,0.3f,0.05f,0.2f);roundtrip();
    for(unsigned pass=0;pass<2;++pass) {
        if(pass) {CHECK(nativeDynamics_Load(&saved,sizeof(saved)));desc=companion;}
        for(unsigned frame=0;frame<24;++frame) {
            lb_800115F4();
            // Simulated caller writes demonstrate complete dynamics payload
            // ownership; the actual full bone physics loop is tested live.
            desc.data->desc.lb_unk0.unk_44+=(float)frame/7;
            if(frame==7)lb_800119DC(&direction,6,0.25f,0.01f,0.3f);
        }
        CHECK(nativeDynamics_Save(pass ? &repeat : &endpoint,sizeof(endpoint),&bytes));
    }
    CHECK(!memcmp(&endpoint,&repeat,sizeof(endpoint)));CHECK(nativeDynamics_Load(&saved,sizeof(saved)));
#define REJECT(change) do {bad=saved;change;reject();}while(0)
    REJECT(++bad.epoch);REJECT(bad.state.free_nodes=(void*)1);
    REJECT(bad.state.nodes[4].next=&dynamics_state.nodes[4]);
    REJECT(bad.state.nodes[0].next=&dynamics_state.nodes[4]);
    REJECT(bad.state.nodes[3].next=&dynamics_state.nodes[0]);
    REJECT(bad.state.nodes[2].next=&dynamics_state.nodes[1]);
    REJECT(bad.state.active_wind=(void*)1);REJECT(bad.state.free_wind=bad.state.active_wind);
    REJECT(bad.state.wind[0].next=&dynamics_state.wind[0]);
    REJECT(bad.state.wind[7].next=(void*)1);REJECT(bad.state.wind_status=3);
#undef REJECT
    CHECK(!nativeDynamics_Load(&saved,sizeof(saved)-1));
    CHECK(!nativeDynamics_Save(&dynamics_state,sizeof(saved),&bytes));
    CHECK(!nativeDynamics_Save(&after,sizeof(after),(void*)&dynamics_state));
    CHECK(!nativeDynamics_Save(&after,sizeof(after),(void*)&after));
    CHECK(!nativeDynamics_Save(&after,sizeof(after)-1,&bytes));
    idle=false;CHECK(!nativeDynamics_Load(&saved,sizeof(saved)));idle=true;
    script_idle=false;CHECK(!nativeDynamics_Save(&after,sizeof(after),&bytes));script_idle=true;
    // Fill wind capacity; original replacement preserves chain topology.
    lbDynamics_Reset();
    for(unsigned i=0;i<WIND_FIELD_COUNT;++i)CHECK(lb_80011A50(&direction,1,0.5f,0,0,-1,1,1,-1)==&dynamics_state.wind[i]);
    CHECK(!dynamics_state.free_wind);roundtrip();
    CHECK(lb_80011A50(&direction,1,0.5f,0,0,-1,1,1,-1)==dynamics_state.active_wind);roundtrip();
    lb_800115F4();CHECK(!dynamics_state.active_wind);CHECK(lb_80011ABC()==1);roundtrip();
    struct lb_80011A50_t* next=lb_80011A50(&direction,2,0.25f,0.01f,0,-1,1,1,-1);CHECK(next==&dynamics_state.wind[0]);
    CHECK(nativeDynamics_Load(&saved,sizeof(saved)));CHECK(lb_80011A50(&direction,2,0.25f,0.01f,0,-1,1,1,-1)==next);
    // Full node reservation has no lower capacity than the original engine.
    lbDynamics_Reset();DynamicsDesc many[DYNAMICS_NODE_COUNT];
    for(unsigned i=0;i<DYNAMICS_NODE_COUNT;++i) {many[i]=(DynamicsDesc){0};lb_8000FD48(joints,&many[i],1);CHECK(many[i].data==&dynamics_state.nodes[i]);}
    CHECK(!dynamics_state.free_nodes);roundtrip();
    DynamicsDesc overflow={0};lb_8000FD48(joints,&overflow,1);CHECK(!overflow.data && overflow.count==0);
    lb_8000FD18(&many[99]);CHECK(dynamics_state.free_nodes==&dynamics_state.nodes[99]);roundtrip();
    lb_8000FD48(joints,&overflow,1);CHECK(overflow.data==&dynamics_state.nodes[99]);
    CHECK(nativeDynamics_Load(&saved,sizeof(saved)));overflow=(DynamicsDesc){0};lb_8000FD48(joints,&overflow,1);CHECK(overflow.data==&dynamics_state.nodes[99]);
    // Malformed live topology must leave output and its length untouched.
    dynamics_state.free_wind=(void*)1;memset(&after,0xA5,sizeof(after));bad=after;bytes=313;
    CHECK(!nativeDynamics_Save(&after,sizeof(after),&bytes));CHECK(bytes==313 && !memcmp(&after,&bad,sizeof(after)));
    CHECK(nativeDynamics_Load(&saved,sizeof(saved)));nativeDynamics_EndScene();CHECK(!nativeDynamics_Load(&saved,sizeof(saved)));
    CHECK(!nativeDynamics_ResolveWind(wind_ref));
    lbDynamics_Reset();CHECK(!nativeDynamics_Load(&saved,sizeof(saved)));
    CHECK(!nativeDynamics_ResolveWind(wind_ref));
    printf("{\"checks\":%u,\"malformed_candidates\":%u,\"snapshot_bytes\":%zu,\"nodes\":%u,\"wind_fields\":%u,\"replayed_wind_ticks\":24,\"controlled_matrix_calls\":%u}\n",checks,rejections,sizeof(saved),DYNAMICS_NODE_COUNT,WIND_FIELD_COUNT,matrix_calls);
    return 0;
}
