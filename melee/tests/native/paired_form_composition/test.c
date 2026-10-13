// Complete Transform originals execute against the real generated assembled
// module. Common presentation/transition/transfer calls are ordered boundaries.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftSeak/types.h>
#include <melee/ft/kinds/ftSeak/ftseakspeciallw.h>
#include <melee/ft/kinds/ftZelda/types.h>
#include <melee/ft/kinds/ftZelda/forward.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspeciallw.h>
#include <melee/ef/efsync.h>
#include <melee/ef/eflib.h>
#include <melee/ef/native_model_effect.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#undef NDEBUG
#include <assert.h>

#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"check %s:%d %s\n",__FILE__,__LINE__,#x); abort(); } } while(0)
static unsigned checks, pairs, lifetime_cases;
static unsigned pending_pairs;
static unsigned recovery_pairs;
static HSD_GObj object, *heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static Fighter* fighter;
static FighterBone parts[118];
static HSD_JObj joints[118];
static bool remaining, ground_ok;
static GroundOrAir air_result;
static unsigned mutation_event, mutation;
static unsigned bone_total;
static NativeFighterHost host;
static NativeFighterDefinition base;
static MotionState motions[24];
static const NativeFighterDefinition* definition;
#if MEMBER_SHEIK
static ftSeakAttributes special_attributes;
#define FIRST 361
#define ROOT_NAME "ftDataSeak"
#define PHASE_COUNT 24
#define STOCK_ENTRY ftSk_SpecialLw_Enter
#define STOCK_AIR_ENTRY ftSk_SpecialAirLw_Enter
#define STOCK_FINISH ftSk_SpecialLw_80114758
#else
static ftZelda_DatAttrs special_attributes;
#define FIRST 355
#define ROOT_NAME "ftDataZelda"
#define PHASE_COUNT 18
#define STOCK_ENTRY ftZd_SpecialLw_Enter
#define STOCK_AIR_ENTRY ftZd_SpecialAirLw_Enter
#define STOCK_FINISH ftZd_SpecialLw_8013B4D8
#endif

typedef struct TransformEvent { int id, a, b; float f[4]; } TransformEvent;
static TransformEvent events[16];static unsigned event_count;
static void record(TransformEvent event)
{
    CHECK(event_count<16);events[event_count++]=event;
    if (event.id != (int)mutation_event) return;
    if (mutation == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
        DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_NOACCESS,&old));
    } else if (mutation == 2) ++fighter->x8_spawnNum;
}
static void unexpected(void) { CHECK(!"unexecuted component crossed controlled boundary"); }
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* generation)
{ (void)g;(void)generation;unexpected();return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t generation)
{ (void)g;(void)generation;unexpected();return NULL; }
void __assert(char* file,u32 line,char* message)
{ fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,message);abort(); }
void OSReport(char* fmt,...) { va_list args;va_start(args,fmt);vfprintf(stderr,fmt,args);va_end(args); }
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId motion,MotionFlags flags,
                              float frame,float rate,float blend,Fighter_GObj* donor)
{
    CHECK(g==&object);CHECK(donor==NULL);CHECK(blend==0);
    Fighter* f=g->user_data;f->motion_id=motion;f->cur_anim_frame=frame;
    f->frame_speed_mul=rate;f->accessory4_cb=NULL;
    record((TransformEvent){1,motion,(int)flags,{frame,rate,blend,0}});
}
void ftAnim_8006EBA4(Fighter_GObj* g)
{ CHECK(g==&object);record((TransformEvent){2}); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* g)
{ CHECK(g==&object);record((TransformEvent){3,remaining});return remaining; }
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result)
{
    CHECK(offset==NULL);CHECK(joint>=joints&&joint<joints+bone_total);
    *result=(Vec3){11,-12,13};record((TransformEvent){4,(int)(joint-joints)});
}
void lb_800119DC(Vec3* pos,int frames,float strength,float decay,float frequency)
{ record((TransformEvent){5,frames,0,{pos->x,pos->y,pos->z,frequency}});CHECK(strength==0.4f&&decay==0.003f); }
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj* g,...)
{
    CHECK(g==&object);va_list args;va_start(args,g);HSD_JObj* joint=va_arg(args,HSD_JObj*);va_end(args);
    CHECK(joint>=joints&&joint<joints+bone_total);
    record((TransformEvent){6,effect,(int)(joint-joints)});return (EF_SpawnResult){0};
}
static void spawn_on_part(Fighter_GObj* g,s32 effect,s32 part)
{ CHECK(part>=0&&(unsigned)part<bone_total);efSync_Spawn(effect,g,parts[part].joint); }
static unsigned count_parts(Fighter* f) { CHECK(f==fighter);return bone_total; }
void ftCo_800B46B8(Fighter* f,cmd_t command,arg_t arg)
{ CHECK(f==fighter);record((TransformEvent){16,command,arg}); }
void ftCo_800B463C(Fighter* f,cmd_t command)
{ CHECK(f==fighter);record((TransformEvent){17,command}); }
static void run_cpu_script(Fighter* f,int index)
{ CHECK(f==fighter);record((TransformEvent){15,index}); }
void efLib_PauseAll(HSD_GObj* g) { CHECK(g==&object); }
void efLib_ResumeAll(HSD_GObj* g) { CHECK(g==&object); }
void ft_80084F3C(Fighter_GObj* g)
{ CHECK(g==&object);fighter->gr_vel*=0.5f;record((TransformEvent){7}); }
void ft_8008A2BC(Fighter_GObj* g)
{ CHECK(g==&object);fighter->motion_id=14;record((TransformEvent){8}); }
void ftCo_Fall_Enter(Fighter_GObj* g)
{ CHECK(g==&object);fighter->motion_id=29;record((TransformEvent){9}); }
bool ft_80082708(Fighter_GObj* g)
{ CHECK(g==&object);record((TransformEvent){10,ground_ok});return ground_ok; }
GroundOrAir ft_80081D0C(Fighter_GObj* g)
{ CHECK(g==&object);record((TransformEvent){11,air_result});return air_result; }
void ftCommon_8007D5D4(Fighter* f)
{ CHECK(f==fighter);f->ground_or_air=GA_Air;record((TransformEvent){12}); }
void ftCommon_8007D7FC(Fighter* f)
{ CHECK(f==fighter);f->ground_or_air=GA_Ground;record((TransformEvent){13}); }
bool ftCommon_ActivatePartner(Fighter_GObj* g,HSD_GObjEvent finish)
{
    CHECK(g==&object);record((TransformEvent){14,finish!=NULL});return true;
}
#if MEMBER_SHEIK
void ftZd_SpecialLw_8013B4D8(Fighter_GObj* g) { (void)g;unexpected(); }
#else
void ftSk_SpecialLw_80114758(Fighter_GObj* g) { (void)g;unexpected(); }
#endif

#include "original.inc"
#include "generated.inc"
#include "parameters.inc"

static void setup(unsigned variant)
{
    DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
    memset(fighter,0,sizeof(*fighter));memset(&object,0,sizeof(object));
    memset(heads,0,sizeof(heads));heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    object.user_data=fighter;fighter->gobj=&object;fighter->x8_spawnNum=0x80000123;
    fighter->kind=Ft_Kind_CustomFirst;fighter->dat_attrs=&special_attributes;
    fighter->parts=parts;fighter->ground_or_air=variant&1?GA_Air:GA_Ground;
    fighter->motion_id=FIRST;fighter->cur_anim_frame=(variant%5)*3.25f;
    fighter->self_vel=(Vec3){((int)(variant%9)-4)*0.375f,((int)(variant%11)-5)*2.5f,9};
    fighter->gr_vel=((int)(variant%7)-3)*1.25f;fighter->cmd_vars[0]=9;
    fighter->co_attrs.aerial_friction=(variant%4)*0.25f;
    fighter->x2219_b0=(variant&2)!=0;
    remaining=(variant&4)!=0;ground_ok=(variant&8)!=0;
    air_result=variant&16?GA_Air:GA_Ground;
    event_count=mutation_event=mutation=0;
}
typedef struct Result {
    int motion,air,command,active,accessory,hitlag;
    float frame,rate,ground,velocity[3],acceleration[3];
    unsigned count;TransformEvent events[16];
} Result;
static Result result(void)
{
    Result r={0};r.motion=fighter->motion_id;r.air=fighter->ground_or_air;
    r.command=fighter->cmd_vars[0];r.active=fighter->x2219_b0;
    r.accessory=fighter->accessory4_cb!=NULL;
    r.hitlag=(fighter->pre_hitlag_cb!=NULL)*1+(fighter->post_hitlag_cb!=NULL)*2;
    r.frame=fighter->cur_anim_frame;r.rate=fighter->frame_speed_mul;r.ground=fighter->gr_vel;
    memcpy(r.velocity,&fighter->self_vel,sizeof(r.velocity));
    memcpy(r.acceleration,&fighter->x74_self_accel,sizeof(r.acceleration));
    r.count=event_count;memcpy(r.events,events,event_count*sizeof(TransformEvent));
    // The retail transfer passes a stock finish pointer; the generated policy
    // passes NULL and invokes the destination's authored hook in the real core.
    // This fixture compares invocation timing, not that separately tested core.
    for(unsigned i=0;i<r.count;++i) if(r.events[i].id==14) r.events[i].a=0;
    return r;
}
static void compare(HSD_GObjEvent original,HSD_GObjEvent authored,unsigned variant,bool accessory)
{
    setup(variant);original(&object);
    if(accessory&&fighter->accessory4_cb) fighter->accessory4_cb(&object);
    Result expected=result();setup(variant);authored(&object);
    if(accessory&&fighter->accessory4_cb) fighter->accessory4_cb(&object);
    Result actual=result();
    if(memcmp(&expected,&actual,sizeof(actual))) {
        fprintf(stderr,"variant %u accessory %u expected motion %d flags %d actual %d flags %d\n",variant,accessory,expected.motion,expected.events[expected.count-1].b,actual.motion,actual.events[actual.count-1].b);
        for(unsigned i=0;i<expected.count||i<actual.count;++i) fprintf(stderr,"event%u old%d/%d/%d new%d/%d/%d\n",i,expected.events[i].id,expected.events[i].a,expected.events[i].b,actual.events[i].id,actual.events[i].a,actual.events[i].b);
    }
    CHECK(memcmp(&expected,&actual,sizeof(actual))==0);++pairs;
}
static void configure_tuning(unsigned profile)
{
    uint32_t values[sizeof(parameter_bits)/4];memcpy(values,parameter_bits,sizeof(values));
    if(profile) {
#include "tuning.inc"
    }
#include "attributes.inc"
    CHECK(FreakMelee_SetFighterParameters(values,sizeof(values)/4,parameter_schema));
}
int main(void)
{
    fighter=VirtualAlloc(NULL,sizeof(*fighter),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(fighter);
    bone_total=MEMBER_SHEIK?58:118;
    for(unsigned i=0;i<bone_total;++i) parts[i].joint=&joints[i];
    base.fighter_root=ROOT_NAME;base.motion_state_first=341;base.motion_state_count=PHASE_COUNT;
    base.source_kind=MEMBER_SHEIK?Ft_Kind_Seak:Ft_Kind_Zelda;
    base.animation_source_kind=base.source_kind;base.joint_count=bone_total;
    base.motion_states=motions;
    base.special_ground_state[3]=FIRST;base.special_air_state[3]=FIRST+2;
#include "hosts.inc"
    host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;
    host.animation_frames_remaining=ftAnim_IsFramesRemaining;
    host.joint_position=lb_8000B1CC;host.bone_count=count_parts;
    host.spawn_wind=lb_800119DC;host.spawn_effect_on_part=spawn_on_part;
    host.pause_effects=efLib_PauseAll;host.resume_effects=efLib_ResumeAll;
    host.ground_friction=ft_80084F3C;host.finish_ground_action=ft_8008A2BC;host.enter_fall=ftCo_Fall_Enter;
    host.fall=ftCommon_Fall;host.air_decel_normal=ftCommon_CalcSelfAccel_DeaccelAir;
    host.ground_collision=ft_80082708;host.air_collision=ft_80081D0C;
    host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;
    host.activate_partner=ftCommon_ActivatePartner;host.callback_source_alive=nativeCallbackSource_Alive;
    host.cpu_run_script=run_cpu_script;
    host.cpu_command=ftCo_800B463C;host.cpu_command_arg=ftCo_800B46B8;host.abs_float=fabsf;
    configure_tuning(0);
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
#if MEMBER_SHEIK
    HSD_GObjEvent originals[]={ftSk_SpecialLw_Anim,ftSk_SpecialLw_IASA,ftSk_SpecialLw_Phys,ftSk_SpecialLw_Coll,
        ftSk_SpecialLw2_Anim,ftSk_SpecialLw2_IASA,ftSk_SpecialLw2_Phys,ftSk_SpecialLw2_Coll,
        ftSk_SpecialAirLw_Anim,ftSk_SpecialAirLw_IASA,ftSk_SpecialAirLw_Phys,ftSk_SpecialAirLw_Coll,
        ftSk_SpecialAirLw2_Anim,ftSk_SpecialAirLw2_IASA,ftSk_SpecialAirLw2_Phys,ftSk_SpecialAirLw2_Coll};
#else
    HSD_GObjEvent originals[]={ftZd_SpecialLw_Anim,ftZd_SpecialLw_IASA,ftZd_SpecialLw_Phys,ftZd_SpecialLw_Coll,
        ftZd_SpecialLw2_Anim,ftZd_SpecialLw2_IASA,ftZd_SpecialLw2_Phys,ftZd_SpecialLw2_Coll,
        ftZd_SpecialAirLw_Anim,ftZd_SpecialAirLw_IASA,ftZd_SpecialAirLw_Phys,ftZd_SpecialAirLw_Coll,
        ftZd_SpecialAirLw2_Anim,ftZd_SpecialAirLw2_IASA,ftZd_SpecialAirLw2_Phys,ftZd_SpecialAirLw2_Coll};
#endif
    for(unsigned profile=0;profile<2;++profile) {
    configure_tuning(profile);
    for(unsigned v=0;v<128;++v) {
        compare(STOCK_ENTRY,definition->special_lw,v,true);
        compare(STOCK_AIR_ENTRY,definition->special_air_lw,v,true);
        compare(STOCK_FINISH,definition->on_form_activated,v,true);
        for(unsigned phase=0;phase<16;++phase) {
            const MotionState* ms=&definition->motion_states[FIRST-341+phase/4];
            HSD_GObjEvent authored=phase%4==0?ms->anim_cb:phase%4==1?ms->input_cb:phase%4==2?ms->phys_cb:ms->coll_cb;
            CHECK(authored);compare(originals[phase],authored,v,false);compare(originals[phase],authored,v,true);
        }
    }
    }
    for(unsigned mode=1;mode<=2;++mode) for(unsigned event=1;event<=2;++event) {
        setup(0);mutation=mode;mutation_event=event;definition->special_lw(&object);
        CHECK(event_count==event);++lifetime_cases;
    }
    for(unsigned mode=1;mode<=2;++mode) {
        // Both conversion directions stop after a destructive motion change,
        // before the new accessory pointer can be assigned to retired userdata.
        for(unsigned air=0;air<2;++air) {
            setup(air?16:0);mutation=mode;mutation_event=1;
            definition->motion_states[FIRST-341+air*2].coll_cb(&object);
            CHECK(event_count==3);++lifetime_cases;
        }
        setup(0);mutation=mode;mutation_event=1;definition->on_form_activated(&object);
        CHECK(event_count==1);++lifetime_cases;
        setup(0);definition->motion_states[FIRST-341].anim_cb(&object);
        CHECK(fighter->accessory4_cb);mutation=mode;mutation_event=14;
        fighter->accessory4_cb(&object);CHECK(event_count==2);++lifetime_cases;
    }
    for(unsigned pending=0;pending<2;++pending) {
        setup(0);fighter->cpu.xF8_b5=pending;
        CHECK(definition->on_cpu_pending_action);
        bool handled=definition->on_cpu_pending_action(&object);
        CHECK(handled==pending);CHECK(!fighter->cpu.xF8_b5);
        CHECK(event_count==pending);
        if(pending) CHECK(events[0].id==15&&events[0].a==38);
        ++pending_pairs;
    }
#if !MEMBER_SHEIK
    const float distances[]={-50,-30.000002f,-30,-29.999998f,0,29.999998f,30,30.000002f,50};
    for(unsigned d=0;d<9;++d) for(unsigned level=1;level<=9;++level) for(unsigned unused=0;unused<2;++unused) {
        setup(0);fighter->cpu.level=level;fighter->cpu.x54.x=distances[d];
        ftCo_800A949C(fighter,unused);Result expected=result();
        setup(0);fighter->cpu.level=level;fighter->cpu.x54.x=distances[d];
        CHECK(definition->on_cpu_recovery);definition->on_cpu_recovery(&object);
        Result actual=result();CHECK(!memcmp(&expected,&actual,sizeof(actual)));++recovery_pairs;
    }
#endif
    printf("{\"checks\":%u,\"pairs\":%u,\"lifetime_cases\":%u,\"transform_phases\":16,\"pending_pairs\":%u,\"recovery_pairs\":%u}\n",checks,pairs,lifetime_cases,pending_pairs,recovery_pairs);
    return 0;
}
