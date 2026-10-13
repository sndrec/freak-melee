#include <melee/gm/native_confirmed_checkpoint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
typedef struct Actor { int position,velocity,damage,charge; } Actor;
typedef struct World { uint32_t seed,tick; Actor actors[8]; } World;
static NativeMatchArena arenas[2];
static _Alignas(64) uint8_t live[2][2048];
static _Alignas(8) uint8_t saved[8192];
static NativeConfirmedCheckpoint checkpoint;
static NativeRollbackInputHistory inputs;
static World* world;
static struct {uint32_t count;int script_state[8];} script;
static uint32_t articles[16];
static PADStatus local[4];
static World reference;
static uint64_t frame;

static PADStatus actual(uint64_t at,unsigned port)
{
    return (PADStatus){.button=(uint16_t)((at+port)%3 ? PAD_BUTTON_A : PAD_BUTTON_B),
        .stickX=(int8_t)(at*7+port),.err=PAD_ERR_NONE};
}
static void tick(World* state,const PADStatus pads[4])
{
    state->seed=state->seed*1664525U+1013904223U;
    for(unsigned i=0;i<8;++i) {
        Actor* actor=&state->actors[i]; const PADStatus* pad=&pads[i/2];
        actor->velocity=pad->stickX;
        actor->position+=actor->velocity;
        actor->damage+=(pad->button&PAD_BUTTON_B)!=0;
        actor->charge+=(int)((state->seed>>(i*3))&7);
    }
    ++state->tick;
}
static void initialize(void)
{
    CHECK(nativeMatchArena_Begin(&arenas[0],live[0],sizeof(live[0]),11));
    CHECK(nativeMatchArena_Begin(&arenas[1],live[1],sizeof(live[1]),17));
    world=nativeMatchArena_Allocate(&arenas[0],sizeof(*world),8);CHECK(world);
    *world=(World){.seed=1234}; reference=*world;
    memset(&script,0,sizeof(script));memset(articles,0,sizeof(articles));
    CHECK(nativeMatchArena_AddRoot(&arenas[1],&script,sizeof(script)));
    CHECK(nativeMatchArena_AddCountedRoot(&arenas[1],articles,sizeof(*articles),16,&script.count));
    NativeMatchArena* owners[]={&arenas[0],&arenas[1]};
    CHECK(nativeConfirmedCheckpoint_Bytes(owners,2)<sizeof(saved));
    CHECK(nativeConfirmedCheckpoint_Bind(&checkpoint,owners,2,saved,sizeof(saved),29));
    CHECK(checkpoint.boundary==0 && checkpoint.captures==1 && checkpoint.valid);
    CHECK(nativeRollbackInput_Init(&inputs,5));
    memset(local,0,sizeof(local));frame=0;
}
static NativeConfirmedCheckpointStep simulate(void)
{
    CHECK(nativeConfirmedCheckpoint_CanAdvance(&checkpoint,29));
    PADStatus pads[4];
    if(checkpoint.replaying) CHECK(nativeRollbackInput_GetFrame(&inputs,frame,pads));
    else {
        local[0]=actual(frame,0);local[2]=actual(frame,2);
        CHECK(nativeRollbackInput_PrepareFrame(&inputs,frame,local,pads));
    }
    tick(world,pads);
    for(unsigned i=0;i<8;++i)script.script_state[i]=world->actors[i].charge;
    script.count=(uint32_t)(world->tick%16);
    for(unsigned i=0;i<script.count;++i) articles[i]=world->seed+i;
    uint64_t next;
    NativeConfirmedCheckpointStep step=nativeConfirmedCheckpoint_EndFrame(&checkpoint,&inputs,frame,29,&next);
    CHECK(step!=NativeConfirmedCheckpoint_Failed);frame=next;
    return step;
}
static void confirm(uint64_t at,unsigned port,bool matching)
{
    PADStatus value=matching ? (PADStatus){.err=PAD_ERR_NONE} : actual(at,port);
    CHECK(nativeRollbackInput_SubmitRemote(&inputs,at,port,&value));
}
static NativeConfirmedCheckpointStep reconcile(void)
{
    uint64_t next=UINT64_MAX;
    NativeConfirmedCheckpointStep step=nativeConfirmedCheckpoint_Reconcile(&checkpoint,&inputs,29,&next);
    CHECK(step!=NativeConfirmedCheckpoint_Failed);frame=next;return step;
}
static void replay(void)
{
    while(checkpoint.replaying) simulate();
    CHECK(frame==world->tick);
    reference=(World){.seed=1234};
    for(uint64_t at=0;at<frame;++at) {
        PADStatus pads[4]; CHECK(nativeRollbackInput_GetFrame(&inputs,at,pads));tick(&reference,pads);
    }
    CHECK(!memcmp(world,&reference,sizeof(reference)));
    for(unsigned i=0;i<8;++i) CHECK(script.script_state[i]==world->actors[i].charge);
}
static void delayed_network(uint8_t local_mask)
{
    enum { Frames=64 };
    World actual_states[Frames+1]; bool delivered[Frames][4]={0};
    unsigned delay[4]={3,2,4,7};
    initialize();CHECK(nativeRollbackInput_Init(&inputs,local_mask));
    actual_states[0]=*world;
    for(unsigned at=0;at<Frames;++at) {
        PADStatus pads[4];for(unsigned p=0;p<4;++p)pads[p]=actual(at,p);
        actual_states[at+1]=actual_states[at];tick(&actual_states[at+1],pads);
    }
    while(frame<Frames) {
        uint64_t live_boundary=frame;
        for(unsigned at=0;at<=live_boundary && at<Frames;++at)
            for(unsigned port=0;port<4;++port)
                if(!(local_mask&(1U<<port)) && !delivered[at][port] &&
                    at+delay[port]+(at%2 ? 0 : 3)<=live_boundary) {
                    confirm(at,port,false);delivered[at][port]=true;
                }
        reconcile();
        while(checkpoint.replaying)simulate();
        CHECK(frame==live_boundary);
        // Check the actual saved checkpoint against an independent all-actual
        // reference through multiple input ring wraps, not just the live world.
        size_t world_offset=(uint8_t*)world-arenas[0].memory;
        CHECK(!memcmp(checkpoint.spans[0].memory+world_offset,
            &actual_states[checkpoint.boundary],sizeof(World)));
        World expected=actual_states[checkpoint.boundary];
        for(uint64_t at=checkpoint.boundary;at<frame;++at) {
            PADStatus pads[4];CHECK(nativeRollbackInput_GetFrame(&inputs,at,pads));tick(&expected,pads);
        }
        CHECK(!memcmp(world,&expected,sizeof(expected)));
        for(unsigned port=0;port<4;++port)local[port]=actual(frame,port);
        // simulate() supplies fixed shared local ports; prepare all selected
        // local ports here so masks 0, 1 and 15 use their own recorded inputs.
        PADStatus pads[4];CHECK(nativeRollbackInput_PrepareFrame(&inputs,frame,local,pads));
        simulate();
    }
    for(unsigned at=0;at<Frames;++at)for(unsigned port=0;port<4;++port)
        if(!(local_mask&(1U<<port))&&!delivered[at][port])confirm(at,port,false);
    reconcile();while(checkpoint.replaying)simulate();
    CHECK(checkpoint.boundary==Frames);
    CHECK(!memcmp(world,&actual_states[Frames],sizeof(World)));
    CHECK(checkpoint.captures<=Frames+1);
}
int main(void)
{
    initialize();
    // Confirming frame five does not bridge the still-predicted frames 0..4.
    for(unsigned i=0;i<6;++i)simulate();
    CHECK(checkpoint.captures==1);
    confirm(5,1,false);confirm(5,3,false);
    uint64_t boundary=99;
    CHECK(nativeRollbackInput_ConfirmedBoundary(&inputs,0,6,0xF,&boundary)&&boundary==0);
    CHECK(reconcile()==NativeConfirmedCheckpoint_Replaying&&frame==0);
    replay();CHECK(checkpoint.boundary==0&&checkpoint.captures==1);
    // One reconstruction save at four, then two predicted replay ticks.
    for(unsigned i=0;i<4;++i){confirm(i,1,false);confirm(i,3,false);}
    CHECK(reconcile()==NativeConfirmedCheckpoint_Replaying&&frame==0);
    replay();CHECK(checkpoint.boundary==4&&checkpoint.captures==2);
    CHECK(checkpoint.restores==2&&checkpoint.replay_ticks==12);
    // Mismatching input four reconstructs from the confirmed state at four.
    confirm(4,1,false);confirm(4,3,false);
    CHECK(reconcile()==NativeConfirmedCheckpoint_Replaying&&frame==4);
    replay();CHECK(checkpoint.boundary==6&&checkpoint.captures==3);
    CHECK(checkpoint.replay_ticks==14);
    CHECK(reconcile()==NativeConfirmedCheckpoint_Continue&&checkpoint.captures==3);
    uint64_t unchanged=checkpoint.boundary;
    CHECK(nativeConfirmedCheckpoint_Reconcile(&checkpoint,&inputs,30,&boundary)==NativeConfirmedCheckpoint_Failed);
    CHECK(checkpoint.boundary==unchanged);
    World prior=*world;arenas[1].lifetime=18;
    inputs.has_correction=true;inputs.correction_frame=6;
    CHECK(nativeConfirmedCheckpoint_Reconcile(&checkpoint,&inputs,29,&boundary)==NativeConfirmedCheckpoint_Failed);
    CHECK(!memcmp(world,&prior,sizeof(prior)));

    initialize();
    for(unsigned i=0;i<6;++i)simulate();
    // All predictions match, but reconstructing a historical frontier is needed.
    for(unsigned i=0;i<4;++i){confirm(i,1,true);confirm(i,3,true);}
    CHECK(!inputs.has_correction);
    CHECK(reconcile()==NativeConfirmedCheckpoint_Replaying);
    replay();CHECK(checkpoint.boundary==4&&checkpoint.captures==2&&checkpoint.restores==1);
    for(unsigned i=4;i<6;++i){confirm(i,1,true);confirm(i,3,true);}
    // Fully confirmed live state can be saved directly without replay.
    CHECK(reconcile()==NativeConfirmedCheckpoint_Continue);
    CHECK(checkpoint.boundary==6&&checkpoint.captures==3&&checkpoint.restores==1);

    initialize();
    for(unsigned i=0;i<15;++i)simulate();
    CHECK(!nativeConfirmedCheckpoint_CanAdvance(&checkpoint,29));
    CHECK(nativeRollbackInput_GetFrame(&inputs,0,local));
    for(unsigned i=0;i<15;++i){confirm(i,1,false);confirm(i,3,false);}
    CHECK(reconcile()==NativeConfirmedCheckpoint_Replaying);
    replay();CHECK(checkpoint.boundary==15&&checkpoint.captures==2&&checkpoint.replay_ticks==15);
    CHECK(nativeConfirmedCheckpoint_CanAdvance(&checkpoint,29));
    // Wrong-identity replay input is a failure, never a falsely confirmed prefix.
    inputs.frames[15%NATIVE_ROLLBACK_INPUT_CAPACITY].valid=false;
    CHECK(!nativeRollbackInput_ConfirmedBoundary(&inputs,15,16,0xF,&boundary));
    CHECK(!nativeRollbackInput_ConfirmedBoundary(&inputs,16,15,0xF,&boundary));
    CHECK(!nativeRollbackInput_ConfirmedBoundary(&inputs,0,17,0xF,&boundary));
    CHECK(!nativeRollbackInput_ConfirmedBoundary(&inputs,0,0,0,&boundary));
    delayed_network(0);delayed_network(1);delayed_network(5);delayed_network(15);
    printf("confirmed checkpoint checks=%u; shared-PC, holes, corrections, matching predictions, frontier jumps, stalls, counted roots, input ring wraps and lifetime\n",checks);
    return 0;
}
