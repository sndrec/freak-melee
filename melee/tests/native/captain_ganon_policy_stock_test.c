// Direct original-C comparison of complete generated Captain/Ganon policies.
// The standalone test loads matching compiled modules; it never starts a game.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/db/db.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <sysdolphin/baselib/jobj.h>
#include <setjmp.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/mp/mpisland.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DecisionEvent { int operation, a, b; float value; } DecisionEvent;
static DecisionEvent events[64], expected[64];
static unsigned event_count, expected_count, checks, rng_count, expected_rng, query_count;
static Fighter fighters[2], victims[2], target, alternate;
static HSD_GObj objects[2], victim_objects[2];
static const NativeFighterDefinition *modules[2], *active;
static uint32_t parameter_bits[2][NATIVE_FIGHTER_PARAMETER_CAPACITY];
static unsigned parameter_count[2], low_script_field=NATIVE_FIGHTER_PARAMETER_CAPACITY;
static uint64_t parameter_schema[2];
static HMODULE libraries[2];
static HSD_JObj root_joint;
static struct FighterBone parts[84];
static MotionState motions[23], common_motions[341];
static const unsigned original_phase_masks[23];
static bool expect_assert;
static jmp_buf assertion_jump;
static Vec3 world_position;
DbLKind DbLevel = 0;
static float random_values[3];
static bool far_unsafe, near_unsafe, redirect_target, grabbing;
static int expected_attempts;

static void require(bool condition, const char* detail)
{ if(!condition){fprintf(stderr,"%s at check %u\n",detail,checks);exit(1);} }
static void event(int operation,int a,int b,float value)
{ require(event_count<64,"Event capacity");events[event_count++]=(DecisionEvent){operation,a,b,value}; }
float HSD_Randf(void)
{ require(rng_count<3,"Unexpected random draw");float value=random_values[rng_count++];event(1,0,0,value);return value; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind >= Ft_Kind_CustomFirst ? active : NULL; }
void ftCo_800B4880(Fighter* fighter,int index)
{ (void)fighter;event(2,index,0,0); }
void ftCo_800B463C(Fighter* fighter,u8 command)
{ (void)fighter;event(3,command,0,0); }
void ftCo_800B46B8(Fighter* fighter,u8 command,u8 argument)
{ (void)fighter;event(4,command,argument,0); }
void ftCo_800A0C8C(Fighter* fighter)
{ (void)fighter;event(5,0,0,0); }
bool ftCo_IsGrabbing(Fighter* fighter) { (void)fighter;return grabbing; }
mp_UnkStruct0* mpIsland_8005AC14(Vec3* position,float depth)
{ (void)position;event(6,0,0,depth);return NULL; }
void mpIsland_8005ACE8(mp_UnkStruct0* island,Vec3* left,Vec3* right)
{ (void)island;(void)left;(void)right;require(false,"Unexpected island endpoints"); }
bool ftCo_CpuRangedAttackUnsafe(Fighter* fighter,Fighter* queried,float margin)
{
    event(7,queried==&target?1:queried==&alternate?2:0,0,margin);
    if(query_count++==0){if(redirect_target)fighter->cpu.x44=&alternate;return far_unsafe;}
    return near_unsafe;
}
void ftCo_CpuAddAttackCandidate(Fighter* fighter,int index)
{ if(index>0&&index<62&&fighter->cpu.xEC<8)fighter->cpu.xCC_array[fighter->cpu.xEC++]=index; }
void __assert(char* file,u32 line,char* expression)
{ if(expect_assert)longjmp(assertion_jump,1);fprintf(stderr,"%s:%lu: %s\n",file,(unsigned long)line,expression);abort(); }
void OSReport(char* format, ...) { (void)format; }
void ftCl_Init_80149318(Fighter_GObj* object) { (void)object;require(false,"Unexpected Young Link taunt"); }
void ftDr_Init_80149910(Fighter_GObj* object) { (void)object;require(false,"Unexpected Dr Mario taunt"); }
void ftPe_Init_8011B93C(Fighter_GObj* object) { (void)object;require(false,"Unexpected Peach taunt"); }
void ftZd_Init_801395C8(Fighter_GObj* object) { (void)object;require(false,"Unexpected Zelda taunt"); }
void ftKb_SpecialN_800F5D04(Fighter_GObj* object, bool flag)
{ (void)object;(void)flag;require(false,"Unexpected Kirby taunt"); }
void pl_80040120(int player,int flag) { event(10,player,flag,0); }
void ftCo_800DEAE8(Fighter_GObj* object,FtMotionId right,FtMotionId left)
{ ((Fighter*)object->user_data)->allow_interrupt=false;event(8,right,left,0); }
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result)
{ require(joint==&root_joint,"Top must resolve to raw part0");require(offset==NULL,"No offset for this wind");*result=world_position; }
void lb_800119DC(Vec3* position,int frames,float strength,float decay,float frequency)
{
    require(!memcmp(position,&world_position,sizeof(*position)),"Exact wind world position");
    event(9,frames,0,strength);event(9,0,0,decay);event(9,0,0,frequency);
}
static Fighter_Part resolve_bone(Fighter* fighter,Fighter_Part role)
{ (void)fighter;require(role==FtPart_TopN,"Unexpected semantic joint");return 0; }

static bool callback_source_alive(HSD_GObj* object,uint32_t generation,uint32_t source_class)
{
    if(source_class!=NativeCallbackSource_Fighter)return false;
    for(unsigned i=0;i<2;++i)if(object==&objects[i]&&object->user_data==&fighters[i])
        return (uint32_t)fighters[i].x8_spawnNum==generation;
    return false;
}
static void common_phase(HSD_GObj* object) { (void)object; }
static void unexpected_motion(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,
                              float frame,float rate,float blend,Fighter_GObj* donor)
{ (void)object;(void)motion;(void)flags;(void)frame;(void)rate;(void)blend;(void)donor;require(false,"Unexpected full motion transition"); }
static void unexpected_animation(Fighter_GObj* object)
{ (void)object;require(false,"Unexpected animation startup"); }
static void initialize_common_motions(void)
{
    for(unsigned i=0;i<23;++i){
        motions[i].anim_cb=(original_phase_masks[i]&1)?common_phase:NULL;
        motions[i].input_cb=(original_phase_masks[i]&2)?common_phase:NULL;
        motions[i].phys_cb=(original_phase_masks[i]&4)?common_phase:NULL;
        motions[i].coll_cb=(original_phase_masks[i]&8)?common_phase:NULL;
    }
    for(unsigned i=0;i<6;++i){
        common_motions[122+4*i]=(MotionState){.anim_cb=common_phase,.input_cb=common_phase,.phys_cb=common_phase,.coll_cb=common_phase};
        motions[i].anim_id=500+i;
        motions[i].x4_flags=0x12340000u+i;
        motions[i].cam_cb=common_phase;
    }
}

#include "captain_ganon_unused_host.inc"
#include "captain_ganon_original.inc"

static void reset(void)
{
    memset(fighters,0,sizeof(fighters));memset(victims,0,sizeof(victims));
    for(unsigned i=0;i<2;++i){
        objects[i]=(HSD_GObj){.user_data=&fighters[i]};
        victim_objects[i]=(HSD_GObj){.user_data=&victims[i]};
        fighters[i].gobj=&objects[i];fighters[i].victim_gobj=&victim_objects[i];
        fighters[i].cpu.x44=&target;fighters[i].cpu.x1C=9;
        fighters[i].motion_id=ftCo_MS_CatchWait;fighters[i].x8_spawnNum=0xfedcba98u;
        fighters[i].parts=parts;fighters[i].player_id=1;fighters[i].allow_interrupt=true;
    }
    fighters[1].kind=Ft_Kind_CustomFirst;
    fighters[0].behavior_kind=Ft_Kind_Captain;
    fighters[1].behavior_kind=Ft_Kind_CustomFirst;
    event_count=expected_count=rng_count=expected_rng=query_count=0;
    random_values[0]=random_values[1]=random_values[2]=0.999f;
    far_unsafe=near_unsafe=redirect_target=grabbing=false;
    active=NULL;
}
static void remember(void)
{
    expected_count=event_count;memcpy(expected,events,sizeof(events));
    expected_rng=rng_count;expected_attempts=fighters[0].cpu.x94;
    event_count=rng_count=query_count=0;
}
static void compare(void)
{
    ++checks;
    require(expected_count==event_count&&!memcmp(events,expected,event_count*sizeof(*events)),"Decision call order/result differs");
    require(expected_rng==rng_count,"Random draw count differs");
    require(expected_attempts==fighters[1].cpu.x94,"Grab attempt counter differs");
    require(fighters[0].cpu.x18==fighters[1].cpu.x18,"Command completion delay differs");
    require((fighters[0].cpu.x44==NULL)==(fighters[1].cpu.x44==NULL),"Target clearing differs");
    require(fighters[0].cpu.xEC==fighters[1].cpu.xEC&&!memcmp(fighters[0].cpu.xCC_array,fighters[1].cpu.xCC_array,8),"Candidate sequence differs");
}
typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
typedef int (__cdecl* Upload)(const uint32_t*,unsigned,uint64_t);
static Upload uploads[2];

static void load_module(unsigned family,const char* dll,const char* fmp,const NativeFighterHost* host)
{
    libraries[family]=LoadLibraryA(dll);require(libraries[family]!=NULL,"Module load");
    Entry entry=(Entry)GetProcAddress(libraries[family],"FreakMelee_GetFighterModule");
    Upload upload=(Upload)GetProcAddress(libraries[family],"FreakMelee_SetFighterParameters");
    require(entry&&upload,"Module exports");
    char source[8192];FILE* file=fopen(fmp,"rb");require(file!=NULL,"FMP open");
    size_t size=fread(source,1,sizeof(source),file);fclose(file);
    NativeFighterParameters parameters;unsigned line;
    require(nativeFighterParameters_Parse(source,size,&parameters,&line),"FMP parse");
    uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<parameters.count;++i)values[i]=parameters.fields[i].bits;
    uploads[family]=upload;parameter_count[family]=parameters.count;parameter_schema[family]=parameters.schema_hash;
    memcpy(parameter_bits[family],values,parameters.count*sizeof(*values));
    if(!family)for(unsigned i=0;i<parameters.count;++i)
        if(!strcmp(parameters.fields[i].name,"cpu_grab_low_damage_script"))low_script_field=i;
    require(family||low_script_field<parameters.count,"Named CPU script tuning exists");
    NativeFighterDefinition base={.code_key=family?"ganon_clone":"captain_clone",
        .source_kind=family?Ft_Kind_Ganon:Ft_Kind_Captain,
        .fighter_root=family?"ftDataGanon":"ftDataCaptain",
        .motion_state_first=341,.motion_state_count=23,.motion_states=motions,
        .special_ground_state={347,349,353,357},.special_air_state={348,351,354,359},
        .joint_count=family?84:63};
    NativeFighterHost missing=*host;missing.common_motion_states=NULL;
    require(entry(&base,&missing)==NULL,"Common motion Host is required");
    NativeFighterDefinition wrong=base;wrong.motion_state_first=342;
    require(entry(&wrong,host)==NULL,"Policy target must belong to owned motion table");
    modules[family]=entry(&base,host);require(modules[family]&&upload(values,parameters.count,parameters.schema_hash),"Generated policy/parameters");
    require(modules[family]->source_kind==Ft_Kind_None,"Independent candidate must clear stock behavior");
    common_motions[122].phys_cb=NULL;
    require(entry(&base,host)==NULL,"Missing required common phase must fail independent guard");
    common_motions[122].phys_cb=common_phase;
    modules[family]=entry(&base,host);require(modules[family]!=NULL,"Restore valid common phase table");
    require(modules[family]->on_cpu_capture_decision==(family?NULL:modules[family]->on_cpu_capture_decision),"Ganon must not gain a special capture draw");
    require(family||modules[family]->on_cpu_capture_decision,"Captain capture callback");
    for(unsigned i=0;i<6;++i){
        const MotionState* actual=&modules[family]->motion_states[i];
        require(actual->anim_cb==common_phase&&actual->input_cb==common_phase&&actual->phys_cb==common_phase&&actual->coll_cb==common_phase,"Common swing phases bound directly");
        require(actual->anim_id==motions[i].anim_id&&actual->x4_flags==motions[i].x4_flags&&actual->cam_cb==motions[i].cam_cb,"Motion policy must preserve owned animation/flags/camera");
    }
}

int main(int argc,char** argv)
{
    require(argc==5,"Expected Captain DLL/FMP and Ganon DLL/FMP");
    initialize_common_motions();
    NativeFighterHost host={.cpu_ranged_unsafe=ftCo_CpuRangedAttackUnsafe,
        .cpu_add_candidate=ftCo_CpuAddAttackCandidate,.cpu_run_script=ftCo_800B4880,
        .random_float=HSD_Randf,.callback_source_alive=callback_source_alive,
        .common_motion_states=common_motions,.resolve_bone=resolve_bone,
        .joint_position=lb_8000B1CC,.spawn_wind=lb_800119DC,.enter_taunt=ftCo_800DEAE8,
        .change_motion_state=unexpected_motion,.start_animation=unexpected_animation};
    supply_unused_host(&host);
    load_module(0,argv[1],argv[2],&host);load_module(1,argv[3],argv[4],&host);
    for(unsigned family=0;family<2;++family)for(unsigned far_result=0;far_result<2;++far_result)for(unsigned near_result=0;near_result<2;++near_result)
    for(unsigned count=0;count<=8;++count)for(unsigned redirect=0;redirect<2;++redirect){
        reset();fighters[0].kind=family?Ft_Kind_Ganon:Ft_Kind_Captain;
        far_unsafe=far_result;near_unsafe=near_result;redirect_target=redirect;
        for(unsigned i=0;i<2;++i){fighters[i].cpu.xEC=count;memset(fighters[i].cpu.xCC_array,0x65,8);}
        original_ranged(&fighters[0]);remember();modules[family]->on_cpu_ranged_candidates(&objects[1]);compare();
        require(query_count==2,"Both geometry probes must run with a full queue");
    }
    const float damage_values[]={0,99.99999f,100,100.00001f,999,NAN};
    for(unsigned family=0;family<2;++family)
    for(unsigned level=0;level<=9;++level)for(unsigned attempts=0;attempts<3;++attempts)
    for(unsigned draw=0;draw<5;++draw)for(unsigned damage=0;damage<6;++damage)for(unsigned grab=0;grab<2;++grab){
        reset();fighters[0].kind=family?Ft_Kind_Ganon:Ft_Kind_Captain;
        fighters[0].behavior_kind=fighters[0].kind;grabbing=grab;
        float threshold=0.05f*level;
        float choices[]={0,nextafterf(threshold,-INFINITY),threshold,nextafterf(threshold,INFINITY),0.999f};
        random_values[0]=choices[draw];random_values[1]=0.55f;random_values[2]=0.9f;
        for(unsigned i=0;i<2;++i){fighters[i].cpu.level=level;fighters[i].cpu.x94=attempts;victims[i].dmg.x1830_percent=damage_values[damage];}
        original_capture(&fighters[0]);remember();
        active=modules[family];production_capture(&fighters[1]);compare();
    }
    for(unsigned missing=0;missing<2;++missing){
        reset();fighters[0].kind=Ft_Kind_Captain;
        if(missing)for(unsigned i=0;i<2;++i)fighters[i].victim_gobj=NULL;
        original_capture(&fighters[0]);remember();active=modules[0];production_capture(&fighters[1]);compare();
    }
    parts[0].joint=&root_joint;
    for(unsigned facing=0;facing<2;++facing)for(unsigned position=0;position<4;++position){
        reset();fighters[0].kind=Ft_Kind_Ganon;fighters[0].behavior_kind=Ft_Kind_Ganon;
        fighters[0].facing_dir=fighters[1].facing_dir=facing?-1:1;
        world_position=(Vec3){position*7.25f,-(float)position*3.5f,position*0.25f};
        original_taunt(&objects[0]);remember();
        modules[1]->on_taunt(&objects[1]);pl_80040120(fighters[1].player_id,fighters[1].x221F_b4);compare();
        require(!fighters[0].allow_interrupt&&!fighters[1].allow_interrupt,"Common taunt interrupt reset");
        require(event_count==6&&events[3].operation==8&&events[4].operation==8&&events[5].operation==10,"Wind then two taunt entries then one notification");
    }
    for(unsigned family=0;family<2;++family)for(unsigned type=0;type<6;++type)
    for(unsigned variant=0;variant<5;++variant){
        reset();fighters[0].kind=family?Ft_Kind_Ganon:Ft_Kind_Captain;
        fighters[0].behavior_kind=fighters[0].kind;
        bool unsupported=family&&variant==3;
        expect_assert=unsupported;
        int expected_motion=-1,actual_motion=-1;
        if(!setjmp(assertion_jump)){expected_motion=original_get_anim_id(&fighters[0],type,variant);require(!unsupported,"Stock unsupported variant unexpectedly accepted");}
        else require(unsupported,"Unexpected stock swing assertion");
        active=modules[family];
        if(!setjmp(assertion_jump)){actual_motion=production_get_anim_id(&fighters[1],type,variant);require(!unsupported,"Ganon must not gain fourth held swing");}
        else require(unsupported,"Unexpected authored swing assertion");
        expect_assert=false;++checks;
        require(expected_motion==actual_motion,"Swing selector differs");
    }
    require(modules[0]->on_item_swing_motion!=NULL&&modules[1]->on_item_swing_motion==NULL,"Falcon-only fourth swing hook");
    const unsigned clock_values[]={0,1,599,600,601,1200};
    const float spacing_draws[]={0,0.0001f,0.125f,0.5f,0.875f,0.99999f,1};
    for(unsigned family=0;family<2;++family)for(unsigned clock=0;clock<6;++clock)
    for(unsigned draw=0;draw<7;++draw){
        reset();fighters[0].kind=family?Ft_Kind_Ganon:Ft_Kind_Captain;
        for(unsigned i=0;i<2;++i){fighters[i].cpu.x7C=clock_values[clock];fighters[i].cpu.x56C=-999;}
        random_values[0]=spacing_draws[draw];original_spacing(&fighters[0]);remember();
        active=modules[family];production_spacing(&fighters[1]);compare();
        require(!memcmp(&fighters[0].cpu.x56C,&fighters[1].cpu.x56C,sizeof(float)),"Spacing result bits differ");
    }
    for(unsigned family=0;family<2;++family){
        reset();modules[family]->on_cpu_initialize(&objects[1]);++checks;
        require(fighters[1].cpu.x56C==3.5f,"Named default initial spacing");
    }
    const int indices[]={1,61,0,-1,62};
    uint32_t original_script=parameter_bits[0][low_script_field];
    for(unsigned i=0;i<5;++i){
        reset();fighters[1].cpu.level=9;random_values[0]=0;
        parameter_bits[0][low_script_field]=(uint32_t)indices[i];
        require(uploads[0](parameter_bits[0],parameter_count[0],parameter_schema[0]),"Boundary parameter upload");
        require(modules[0]->on_cpu_capture_decision(&objects[1]),"Successful capture choice remains true");
        ++checks;require(rng_count==1,"One probability draw at script boundary");
        bool valid=indices[i]>0&&indices[i]<62;
        require(event_count==(valid?2:1),"Invalid CPU script must be ignored");
        if(valid)require(events[1].operation==2&&events[1].a==indices[i],"Full integer CPU script argument");
    }
    parameter_bits[0][low_script_field]=original_script;
    require(uploads[0](parameter_bits[0],parameter_count[0],parameter_schema[0]),"Restore stock tuning");
    printf("%u original-C policy comparisons pass: generated CPU/taunt/swing and common callers; gameplay pending\n",checks);
    FreeLibrary(libraries[0]);FreeLibrary(libraries[1]);return 0;
}
