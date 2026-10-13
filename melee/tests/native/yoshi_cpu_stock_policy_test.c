// Direct original-C decisions versus the authored module and actual common
// routing bodies extracted from the matching frozen SDK by the driver.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/ft/kinds/ftYoshi/forward.h>
#include <melee/ft/kinds/ftZelda/forward.h>
#include <melee/mp/mpisland.h>
#include <melee/lb/lb_00CE.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
typedef int (__cdecl* SetParameters)(const uint32_t*,unsigned,uint64_t);
typedef struct CpuPolicyEvent { int id,a,b; float x,y,z,w; } CpuPolicyEvent;
static CpuPolicyEvent events[128], expected[128];
static unsigned event_count, expected_count, comparisons, lookup_count;
static Fighter fighters[2], targets[2];
static HSD_GObj objects[2];
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object, uint32_t generation)
{ (void)object; (void)generation; return NULL; }
bool nativeModelEffect_Snapshot(HSD_GObj* object, uint32_t* generation)
{ (void)object; (void)generation; return false; }
static mp_UnkStruct0 islands[2], *lookups[2], *floor_islands[2];
static const NativeFighterDefinition* policy;
static bool blocked, ranged;
static float destination_angle;

static void require(bool condition,const char* message)
{ if(!condition){fprintf(stderr,"%s at comparison %u\n",message,comparisons);exit(1);} }
static void record(int id,int a,int b,float x,float y,float z,float w)
{ require(event_count<128,"Event capacity");events[event_count++]=(CpuPolicyEvent){id,a,b,x,y,z,w}; }
void ftCo_800B463C(Fighter* f,u8 command)
{ (void)f;record(1,command,0,0,0,0,0); }
void ftCo_800B46B8(Fighter* f,u8 command,u8 argument)
{ (void)f;record(2,command,argument,0,0,0,0); }
mp_UnkStruct0* mpIsland_8005AC14(Vec3* p,float depth)
{ record(3,0,0,p->x,p->y,p->z,depth);require(lookup_count<2,"Excess floor lookup");return lookups[lookup_count++]; }
mp_UnkStruct0* mpIsland_8005AB54(int line)
{ record(4,line,0,0,0,0,0);require(line==0||line==1,"Floor line fixture");return floor_islands[line]; }
void mpIsland_8005ACE8(mp_UnkStruct0* island,Vec3* left,Vec3* right)
{ *left=island->x8;*right=island->x14; }
bool ftCo_CpuRangedAttackUnsafe(Fighter* f,Fighter* target,float margin)
{ (void)f;record(5,target!=NULL,0,margin,0,0,0);return ranged; }
void ftCo_CpuAddAttackCandidate(Fighter* f,int command)
{ if(f->cpu.xEC<8U)f->cpu.xCC_array[f->cpu.xEC++]=command; }
bool ftCo_800A0FB0(Vec3* p,int* line,u32* flags,Vec3* normal,int a,int b,int c,float x,float y,float tx,float ty,float offset)
{ (void)p;(void)line;(void)flags;(void)normal;require(a==-1&&b==-1&&c==-1&&offset==0,"Stage ray arguments");record(6,0,0,x,y,tx,ty);return blocked; }
// Two generic primitives replace the exporter-only combined query. The
// original native callback still uses its separately controlled CPU helper.
bool mpCheckFloor(float x,float y,float tx,float ty,float offset,Vec3* p,int* line,u32* flags,Vec3* normal,int a,int b,int c,bool(*filter)(Fighter_GObj*,int),Fighter_GObj* owner)
{require(a==-1&&b==-1&&c==-1&&offset==0&&!filter&&!owner,"Stage floor arguments");record(6,0,0,x,y,tx,ty);*p=(Vec3){0};*line=0;*flags=0;*normal=(Vec3){0,1,0};return blocked;}
bool ftCo_800A1B38(enum_t line){require(line==0,"Stage floor exclusion line");return false;}
float lb_8000D008(float y,float x)
{ record(7,0,0,y,x,0,0);return destination_angle; }
void ftCo_800A0C8C(Fighter* f) { ftCo_800B463C(f,127); }
bool ftCo_800B630C(Fighter* f) { (void)f;return false; }
bool ftCo_800B9CBC(Fighter* f) { (void)f;return false; }
void ftCo_800B4880(Fighter* f,int command)
{ (void)f;record(8,command,0,0,0,0,0); }
void ftCo_800B920C(Fighter* f) { (void)f;require(false,"Ness policy used"); }
void ftCo_800B92D4(Fighter* f) { (void)f;require(false,"Ness policy used"); }
void ftCo_800B9504(Fighter* f) { (void)f;require(false,"Zelda policy used"); }
void ftCo_800B9340(Fighter* f) { (void)f;require(false,"Native travel fallback used"); }
void ftCo_800A0508(Fighter* f) { (void)f;require(false,"Native tiny-jump fallback used"); }
static void ftCo_CpuClearTargetAndFinish(Fighter* f)
{ f->cpu.x44=NULL;f->cpu.x18=f->cpu.x1C;ftCo_800A0C8C(f); }
static void ftCo_CpuSetNeutralStick(Fighter* f)
{ ftCo_800B46B8(f,CpuCmd_SetLstickX,0);ftCo_800B46B8(f,CpuCmd_SetLstickY,0); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_None?policy:NULL; }

#include "cpu_original.inc"
#include "cpu_production.inc"

static void initialize(void)
{
    memset(fighters,0,sizeof(fighters));memset(targets,0,sizeof(targets));
    memset(objects,0,sizeof(objects));memset(islands,0,sizeof(islands));
    for(unsigned i=0;i<2;++i){
        objects[i].user_data=&fighters[i];fighters[i].gobj=&objects[i];
        fighters[i].kind=i?Ft_Kind_None:Ft_Kind_Yoshi;
        fighters[i].cpu.x44=&targets[i];fighters[i].cpu.x18=3;fighters[i].cpu.x1C=5;
        fighters[i].facing_dir=1;fighters[i].x34_scale.y=1;
        fighters[i].coll_data.floor.index=0;targets[i].coll_data.floor.index=1;
        islands[i].x8.x=-100;islands[i].x14.x=100;
    }
    memset(heads,0,sizeof(heads));
    heads[HSD_GOBJ_PLINK_FIGHTER]=&objects[0]; objects[0].next=&objects[1];
    lookups[0]=lookups[1]=floor_islands[0]=floor_islands[1]=&islands[0];
    event_count=expected_count=lookup_count=0;blocked=ranged=false;destination_angle=0;
}
static void remember(void)
{ expected_count=event_count;memcpy(expected,events,sizeof(events));event_count=lookup_count=0; }
static void compare(void)
{
    ++comparisons;
    if(expected_count!=event_count||memcmp(expected,events,event_count*sizeof(*events))){
        fprintf(stderr,"Expected %u events, got %u\n",expected_count,event_count);
        for(unsigned i=0;i<expected_count||i<event_count;++i)
            fprintf(stderr,"%u: %d,%d,%d,%g,%g,%g,%g / %d,%d,%d,%g,%g,%g,%g\n",i,
                expected[i].id,expected[i].a,expected[i].b,expected[i].x,expected[i].y,expected[i].z,expected[i].w,
                events[i].id,events[i].a,events[i].b,events[i].x,events[i].y,events[i].z,events[i].w);
        require(false,"CPU call order/arguments differ");
    }
    struct CpuFighter *a=&fighters[0].cpu,*b=&fighters[1].cpu;
    require(a->x18==b->x18&&a->x90==b->x90&&a->xA4==b->xA4&&a->xEC==b->xEC,"CPU scalar effect differs");
    require((a->x44==NULL)==(b->x44==NULL),"CPU target ownership differs");
    require(!memcmp(a->xCC_array,b->xCC_array,sizeof(a->xCC_array)),"Candidate list differs");
}
static void unexpected(void) { require(false,"Unrelated engine operation reached"); }

int main(int argc,char** argv)
{
    require(argc==3,"Expected module and FMP");
    FILE* file=fopen(argv[2],"rb");require(file!=NULL,"FMP open");char text[32768];size_t size=fread(text,1,sizeof(text),file);fclose(file);
    NativeFighterParameters parameters;unsigned line;require(nativeFighterParameters_Parse(text,size,&parameters,&line),"FMP parse");
    HMODULE library=LoadLibraryA(argv[1]);require(library!=NULL,"Module load");
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");
    SetParameters upload=(SetParameters)GetProcAddress(library,"FreakMelee_SetFighterParameters");require(entry&&upload,"Module exports");
    MotionState motions[28]={0};NativeFighterDefinition base={.fighter_root="ftDataYoshi",.motion_state_first=341,.motion_state_count=28,.motion_states=motions};
    base.special_ground_state[0]=346;base.special_air_state[0]=351;base.special_ground_state[1]=356;base.special_air_state[1]=360;
    base.special_ground_state[2]=364;base.special_air_state[2]=365;base.special_ground_state[3]=366;base.special_air_state[3]=368;
    NativeFighterHost host={.cpu_command=ftCo_800B463C,.cpu_command_arg=ftCo_800B46B8,
        .callback_source_alive=nativeCallbackSource_Alive,
        .cpu_ranged_unsafe=ftCo_CpuRangedAttackUnsafe,.cpu_add_candidate=ftCo_CpuAddAttackCandidate,
        .find_floor_island=mpIsland_8005AC14,.island_bounds=mpIsland_8005ACE8,
        .fighters_share_floor=ftCo_800A2170,.stage_read_floor=mpCheckFloor,.stage_cpu_floor_excluded=ftCo_800A1B38};
    _Static_assert(sizeof(host)%sizeof(void*)==0,"Host pointer table");
    for(unsigned offset=0;offset<sizeof(host);offset+=sizeof(void*)){
        uintptr_t value;memcpy(&value,(char*)&host+offset,sizeof(value));
        if(!value){value=(uintptr_t)unexpected;memcpy((char*)&host+offset,&value,sizeof(value));}
    }
    policy=entry(&base,&host);require(policy!=NULL,"Candidate rejects matching SDK");
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<parameters.count;++i)words[i]=parameters.fields[i].bits;
    require(upload(words,parameters.count,parameters.schema_hash),"Owned parameter upload");

    for(unsigned air=0;air<2;++air)for(unsigned floor=0;floor<2;++floor)for(unsigned result=0;result<2;++result)for(unsigned count=0;count<9;++count){
        initialize();ranged=result;for(unsigned i=0;i<2;++i){fighters[i].ground_or_air=air;fighters[i].cpu.xFA_b5=floor;fighters[i].cpu.xEC=count;}
        original_ranged(&fighters[0]);remember();policy->on_cpu_ranged_candidates(&objects[1]);compare();
    }
    static const float positions[]={-100,-90.001f,-90,-89.999f,0,89.999f,90,90.001f,100};
    for(int motion=355;motion<=360;++motion)for(unsigned own=0;own<2;++own)for(unsigned target=0;target<2;++target)
    for(unsigned same=0;same<2;++same)for(unsigned p=0;p<sizeof(positions)/sizeof(*positions);++p){
        initialize();lookups[0]=own?&islands[0]:NULL;lookups[1]=same?&islands[0]:&islands[1];
        for(unsigned i=0;i<2;++i){fighters[i].motion_id=motion;fighters[i].cur_pos.x=positions[p];fighters[i].cur_pos.y=17.25f;targets[i].cur_pos.x=22.5f;targets[i].cur_pos.y=18;fighters[i].cpu.x90=7;if(!target)fighters[i].cpu.x44=NULL;}
        original_ftCo_800B9020(&fighters[0]);remember();production_ftCo_800B9020(&fighters[1]);compare();
        initialize();lookups[0]=own?&islands[0]:NULL;lookups[1]=same?&islands[0]:&islands[1];
        for(unsigned i=0;i<2;++i){fighters[i].motion_id=motion;fighters[i].cur_pos.x=positions[p];if(!target)fighters[i].cpu.x44=NULL;}
        bool handled=motion>=356&&motion<=359;if(handled)original_ftCo_800B9340(&fighters[0]);remember();
        require(policy->on_cpu_attack_decision(&objects[1])==handled,"First travel branch result");compare();
    }
    static const float cutoffs[]={-0.08726646192371845f,0.08726646192371845f,0.7853981573134661f,1.0471975430846214f,1.3089969288557768f};
    static const float distances[]={79.999f,80,80.001f};
    static const float target_x[]={-1.001f,-1,-0.999f,0,0.999f,1,1.001f};
    for(unsigned air=0;air<2;++air)for(unsigned target_air=0;target_air<2;++target_air)for(unsigned same=0;same<3;++same)
    for(unsigned facing=0;facing<2;++facing)for(unsigned x=0;x<7;++x)for(unsigned distance=0;distance<3;++distance)
    for(unsigned angle=0;angle<5;++angle)for(int edge=-1;edge<=1;++edge){
        initialize();floor_islands[0]=same==2?NULL:&islands[0];floor_islands[1]=same?&islands[1]:&islands[0];
        float value=edge<0?nextafterf(cutoffs[angle],-INFINITY):edge>0?nextafterf(cutoffs[angle],INFINITY):cutoffs[angle];
        for(unsigned i=0;i<2;++i){fighters[i].ground_or_air=air;fighters[i].facing_dir=facing?-1:1;targets[i].ground_or_air=target_air;targets[i].cur_pos.x=target_x[x];}
        bool result=original_ftCo_800B98C8(&fighters[0],value,distances[distance]);remember();
        bool selected=policy->on_cpu_special_candidate(&objects[1],value,distances[distance]);
        require(result==selected,"Special candidate eligibility differs");compare();
    }
    for(unsigned motion=365;motion<=369;++motion)for(unsigned ray=0;ray<2;++ray)for(int x=-5;x<=5;++x)for(unsigned width=0;width<3;++width){
        initialize();blocked=ray;for(unsigned i=0;i<2;++i){fighters[i].motion_id=motion;fighters[i].cur_pos.x=3.25f;fighters[i].cur_pos.y=35.75f;fighters[i].cpu.x564=width+0.25f;targets[i].cur_pos.x=3.25f+x;targets[i].cur_pos.y=1.125f;targets[i].cpu.x564=0.75f;}
        bool result=original_threat(&targets[0],&fighters[0]);remember();HSD_GObj target={.user_data=&targets[1]};
        require(policy->on_cpu_threat(&objects[1],&target)==result,"Threat geometry differs");compare();
    }
    static const float scales[]={0.5f,0.999f,1,1.001f,2};
    static const int elapsed[]={-1,0,899,900,901};
    for(unsigned scale=0;scale<5;++scale)for(unsigned age=0;age<5;++age)for(unsigned air=0;air<2;++air)
    for(unsigned falling=0;falling<2;++falling)for(unsigned forced=0;forced<2;++forced)for(unsigned throw_state=0;throw_state<2;++throw_state){
        initialize();for(unsigned i=0;i<2;++i){fighters[i].x34_scale.y=scales[scale];fighters[i].cpu.x7C=elapsed[age];fighters[i].ground_or_air=air;fighters[i].pos_delta.y=falling?-1:1;fighters[i].cpu.xFA_b6=forced;fighters[i].motion_id=throw_state?244:29;fighters[i].cpu.x54.y=10;}
        bool result=original_ftCo_800A3234(&fighters[0]);remember();require(production_ftCo_800A3234(&fighters[1])==result,"Recovery gate/order differs");compare();
    }
    static const float destination_x[]={-20.001f,-20,-19.999f,0,19.999f,20,20.001f};
    for(unsigned scale=0;scale<5;++scale)for(unsigned x=0;x<7;++x)for(unsigned forced=0;forced<2;++forced){
        initialize();for(unsigned i=0;i<2;++i){fighters[i].x34_scale.y=scales[scale];fighters[i].cpu.x54.x=destination_x[x];fighters[i].cpu.xFA_b6=forced;}
        original_ftCo_CpuUseAvailableJump(&fighters[0],&fighters[0].cpu);remember();production_ftCo_CpuUseAvailableJump(&fighters[1],&fighters[1].cpu);compare();
    }
    initialize();ftCo_800B463C(&fighters[0],CpuCmd_Done);remember();policy->on_cpu_recovery(&objects[1]);compare();
    printf("Passed %u exact original-C CPU comparisons\n",comparisons);FreeLibrary(library);return 0;
}
