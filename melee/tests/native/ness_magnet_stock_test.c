#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_damage.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/ft/kinds/ftNess/ftnessspeciallw.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ef/efasync.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* Bind)(const uint32_t*, unsigned, uint64_t);
enum { Motion=1, Animation, GroundPhysics, FallPhysics, Deaccel, GroundTest,
       AirTest, GroundToAir, AirToGround, Clamp, Create, Effect, Destroy,
       Sound, RotateY, ReadRotY, FinishWait, FinishFall, PlayerHP, Credit };
typedef struct MagnetEvent { int op,a,b,c; float x,y,z,w; } MagnetEvent;
typedef struct MagnetTrace { MagnetEvent events[64]; unsigned count; } MagnetTrace;
static MagnetTrace traces[2];
static unsigned lane, checks, failures;
static Fighter fighters[2];
static HSD_GObj objects[2];
static HSD_JObj joints[2][63];
static FighterBone parts[2][63];
static ftNessAttributes attrs;
static const NativeFighterDefinition* module;
static bool remaining, contact;
#include "ness_magnet_raw.generated.h"

void __assert(char* file,u32 line,char* message)
{ fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,message);abort(); }
static void unused_host(void) { fputs("Unexpected host operation\n",stderr);abort(); }
void Fighter_TakeDamage_8006CC7C(Fighter* fighter,float amount)
{ (void)fighter;(void)amount;unused_host(); }
static void event(int op,int a,int b,int c,float x,float y,float z,float w)
{
    MagnetTrace* trace=&traces[lane];if(trace->count==64)abort();
    trace->events[trace->count++]=(MagnetEvent){op,a,b,c,x,y,z,w};
}
static void check(bool valid,const char* name)
{ ++checks;if(!valid){++failures;fprintf(stderr,"%s differs\n",name);} }
static void equal_float(float a,float b,const char* name)
{ check(memcmp(&a,&b,4)==0,name); }
static const NativeFighterStateField* field(const char* name)
{
    for(unsigned i=0;i<module->state_field_count;++i)
        if(strcmp(module->state_fields[i].name,name)==0)return &module->state_fields[i];
    fprintf(stderr,"Missing state metadata: %s\n",name);abort();
}
static void state_set(const char* name,int32_t value)
{
    const NativeFighterStateField* f=field(name);
    if(f->count!=1||f->offset+f->stride>sizeof(fighters[1].native_module_state))abort();
    if(f->stride==1)fighters[1].native_module_state[f->offset]=(uint8_t)value;
    else memcpy(fighters[1].native_module_state+f->offset,&value,4);
}
static int32_t state_get(const char* name)
{
    const NativeFighterStateField* f=field(name);int32_t value=0;
    if(f->stride==1)return fighters[1].native_module_state[f->offset];
    memcpy(&value,fighters[1].native_module_state+f->offset,4);return value;
}
static void set_counters(int lag,bool release,int gravity,int turn,int sound)
{
    fighters[0].mv.ns.speciallw.releaseLag=lag;
    fighters[0].mv.ns.speciallw.isRelease=release;
    fighters[0].mv.ns.speciallw.gravityDelay=gravity;
    fighters[0].mv.ns.speciallw.turnFrames=turn;
    fighters[0].mv.ns.speciallw.x10=sound;
    state_set("magnet_release_lag",lag);state_set("magnet_release",release);
    state_set("magnet_gravity_delay",gravity);state_set("magnet_turn_frames",turn);
    state_set("magnet_sound_timer",sound);
}
static void compare(void)
{
    check(traces[0].count==traces[1].count&&memcmp(traces[0].events,traces[1].events,
          traces[0].count*sizeof(MagnetEvent))==0,"ordered common operations");
    check(fighters[0].motion_id==fighters[1].motion_id,"motion");
    check(fighters[0].ground_or_air==fighters[1].ground_or_air,"ground/air");
    check(fighters[0].mv.ns.speciallw.releaseLag==state_get("magnet_release_lag"),"release lag");
    check(!!fighters[0].mv.ns.speciallw.isRelease==!!state_get("magnet_release"),"release latch");
    check(fighters[0].mv.ns.speciallw.gravityDelay==state_get("magnet_gravity_delay"),"gravity delay");
    check(fighters[0].mv.ns.speciallw.turnFrames==state_get("magnet_turn_frames"),"turn frames");
    check(fighters[0].mv.ns.speciallw.x10==state_get("magnet_sound_timer"),"sound timer");
    check(fighters[0].cmd_vars[0]==fighters[1].cmd_vars[0],"turn command");
    check(fighters[0].x2219_b0==fighters[1].x2219_b0,"effect active");
    check(fighters[0].x2218_b6==fighters[1].x2218_b6&&fighters[0].x2218_b7==fighters[1].x2218_b7,"absorber flags");
    check(fighters[0].absorb_hit.skip_update_pos==fighters[1].absorb_hit.skip_update_pos,"absorber update flag");
    check(fighters[0].pre_hitlag_cb==fighters[1].pre_hitlag_cb&&
          fighters[0].post_hitlag_cb==fighters[1].post_hitlag_cb,"effect hitlag callbacks");
    check(memcmp(&fighters[0].self_vel,&fighters[1].self_vel,sizeof(Vec3))==0,"velocity");
    equal_float(fighters[0].facing_dir,fighters[1].facing_dir,"facing");
    equal_float(fighters[0].dmg.x1830_percent,fighters[1].dmg.x1830_percent,"percent");
    equal_float(joints[0][0].rotate.y,joints[1][0].rotate.y,"turn rotation");
}
void Fighter_ChangeMotionState(Fighter_GObj* gobj,FtMotionId motion,MotionFlags flags,
                              float start,float speed,float blend,Fighter_GObj* donor)
{ (void)donor;Fighter* f=gobj->user_data;f->motion_id=motion;f->cur_anim_frame=start;event(Motion,motion,flags,0,start,speed,blend,0); }
void ftAnim_8006EBA4(Fighter_GObj* gobj) { (void)gobj;event(Animation,0,0,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* gobj) { (void)gobj;return remaining; }
void ft_80084F3C(Fighter_GObj* gobj) { (void)gobj;event(GroundPhysics,0,0,0,0,0,0,0); }
void ftCommon_Fall(Fighter* f,float gravity,float terminal)
{ f->self_vel.y-=gravity;if(f->self_vel.y<-terminal)f->self_vel.y=-terminal;event(FallPhysics,0,0,0,gravity,terminal,0,0); }
bool ftCommon_CalcSelfAccel_DeaccelQuickAir(Fighter* f) { (void)f;event(Deaccel,0,0,0,0,0,0,0);return false; }
bool ft_80082708(Fighter_GObj* gobj) { (void)gobj;event(GroundTest,0,0,0,0,0,0,0);return contact; }
GroundOrAir ft_80081D0C(Fighter_GObj* gobj) { (void)gobj;event(AirTest,0,0,0,0,0,0,0);return contact?GA_Air:GA_Ground; }
void ftCommon_8007D5D4(Fighter* f) { f->ground_or_air=GA_Air;event(GroundToAir,0,0,0,0,0,0,0); }
void ftCommon_8007D7FC(Fighter* f) { f->ground_or_air=GA_Ground;event(AirToGround,0,0,0,0,0,0,0); }
void ftCommon_ClampAirDrift(Fighter* f) { (void)f;event(Clamp,0,0,0,0,0,0,0); }
void ftColl_8007AF10(Fighter_GObj* gobj) { ((Fighter*)gobj->user_data)->absorb_hit.skip_update_pos=false; }
void ftColl_CreateAbsorbHit(Fighter_GObj* gobj,AbsorbDesc* desc)
{
    Fighter* f=gobj->user_data;f->x2218_b6=true;f->x2218_b7=false;
    f->absorb_hit.bone=f->parts[desc->x0_bone_id].joint;f->absorb_hit.offset=desc->x4_offset;
    f->absorb_hit.size=desc->x10_size;
    event(Create,desc->x0_bone_id,0,0,desc->x4_offset.x,desc->x4_offset.y,desc->x4_offset.z,desc->x10_size);
}
void efAsync_Spawn(HSD_GObj* gobj,void* storage,u32 flags,u32 effect,HSD_JObj* joint,...)
{
    (void)gobj;(void)storage;
    event(Effect,effect,flags,(int)(joint-joints[lane]),0,0,0,0);
}
void efLib_PauseAll(Fighter_GObj* gobj) { (void)gobj; }
void efLib_ResumeAll(Fighter_GObj* gobj) { (void)gobj; }
void ftCommon_8007DB24(Fighter_GObj* gobj)
{ ((Fighter*)gobj->user_data)->x2219_b0=false;event(Destroy,0,0,0,0,0,0,0); }
void ft_8008A2BC(Fighter_GObj* gobj) { ((Fighter*)gobj->user_data)->motion_id=14;event(FinishWait,0,0,0,0,0,0,0); }
void ftCo_Fall_Enter(Fighter_GObj* gobj) { ((Fighter*)gobj->user_data)->motion_id=29;event(FinishFall,0,0,0,0,0,0,0); }
void ftCommon_8007D92C(Fighter_GObj* gobj)
{ if(((Fighter*)gobj->user_data)->ground_or_air==GA_Air)ftCo_Fall_Enter(gobj);else ft_8008A2BC(gobj); }
void ftPartSetRotY(Fighter* f,int joint,float value) { (void)f;joints[lane][joint].rotate.y=value;event(RotateY,joint,0,0,value,0,0,0); }
float ftPartGetRotY(Fighter* f,int joint) { (void)f;event(ReadRotY,joint,0,0,0,0,0,0);return joints[lane][joint].rotate.y; }
float ftNs_Init_GetAbsorbHeal(Fighter* f) { return ((ftNessAttributes*)f->dat_attrs)->x94_PSI_MAGNET_HEAL_MUL; }
void Player_SetHPByIndex(s32 player,s32 secondary,s32 percent) { event(PlayerHP,player,secondary,percent,0,0,0,0); }
void pl_80040B8C(int player,int secondary,int amount) { event(Credit,player,secondary,amount,0,0,0,0); }
void ft_80088478(Fighter* f,int sound,u8 volume,u8 pan)
{ (void)f;event(Sound,sound,volume,pan,0,0,0,0); }
static unsigned bone_count(Fighter* f) { (void)f;return 63; }
static void reset(unsigned state,bool air,float facing)
{
    memset(fighters,0,sizeof(fighters));memset(traces,0,sizeof(traces));memset(joints,0,sizeof(joints));
    for(unsigned i=0;i<2;++i){objects[i].user_data=&fighters[i];fighters[i].dat_attrs=&attrs;
        fighters[i].parts=parts[i];fighters[i].motion_id=state;fighters[i].ground_or_air=air?GA_Air:GA_Ground;
        fighters[i].facing_dir=facing;fighters[i].cur_anim_frame=4.5f;fighters[i].self_vel=(Vec3){2.75f,-.125f,0};
        fighters[i].co_attrs.terminal_velocity=1.9f;fighters[i].absorb_hit.skip_update_pos=true;
        fighters[i].player_id=1;fighters[i].x221F_b4=true;joints[i][0].rotate.y=.375f;
        for(unsigned j=0;j<63;++j)parts[i][j].joint=&joints[i][j];}
    set_counters(1,false,1,3,1);
}
static char* read_file(const char* path,size_t* size)
{ FILE* f=fopen(path,"rb");if(!f)abort();fseek(f,0,SEEK_END);*size=ftell(f);rewind(f);char* s=malloc(*size+1);if(!s||fread(s,1,*size,f)!=*size)abort();s[*size]=0;fclose(f);return s; }
int main(int argc,char** argv)
{
    if(argc!=3)return 2;
    size_t size;char* text=read_file(argv[2],&size);NativeFighterParameters parameters;unsigned line;
    if(!nativeFighterParameters_Parse(text,size,&parameters,&line))return 2;free(text);
    HMODULE library=LoadLibraryA(argv[1]);if(!library)return 2;
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");
    Bind bind=(Bind)GetProcAddress(library,"FreakMelee_SetFighterParameters");
    uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<parameters.count;++i)bits[i]=parameters.fields[i].bits;
    if(!entry||!bind||!bind(bits,parameters.count,parameters.schema_hash))return 2;
    MotionState motions[36]={0};NativeFighterDefinition base={.code_key="ness_clone",.source_kind=Ft_Kind_Ness,
        .animation_source_kind=Ft_Kind_Ness,.fighter_root="ftDataNess",.costume_joint="PlyNess5K_Share_joint",
        .costume_matanim="PlyNess5K_Share_matanim_joint",.action_count=326,.demo_count=14,.joint_count=63,
        .motion_state_first=341,.motion_state_count=36,.motion_states=motions};
    base.special_ground_state[NativeFighterSpecial_Side]=356;base.special_air_state[NativeFighterSpecial_Side]=357;
    base.special_ground_state[NativeFighterSpecial_Down]=367;base.special_air_state[NativeFighterSpecial_Down]=372;
    NativeFighterHost host={0};
#include "ness_magnet_required_host.generated.h"
    host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;
    host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.ground_friction=ft_80084F3C;
    host.fall=ftCommon_Fall;host.quick_air_decel=ftCommon_CalcSelfAccel_DeaccelQuickAir;
    host.ground_collision=ft_80082708;host.air_collision=ft_80081D0C;
    host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;host.clamp_air_drift=ftCommon_ClampAirDrift;
    host.bone_count=bone_count;host.create_absorber=ftColl_CreateAbsorbHit;host.heal_damage=nativeFighterDamage_Heal;
    host.spawn_async_effect=efAsync_Spawn;host.pause_effects=efLib_PauseAll;host.resume_effects=efLib_ResumeAll;
    host.destroy_effects=ftCommon_8007DB24;host.play_sound=ft_80088478;
    host.set_root_rotation_y=ftPartSetRotY;host.get_bone_rotation_y=ftPartGetRotY;
    host.finish_ground_action=ft_8008A2BC;host.enter_fall=ftCo_Fall_Enter;
    module=entry(&base,&host);if(!module||!module->on_absorb)return 2;
    memset(&attrs,0,sizeof(attrs));
    for(unsigned i=0;i<sizeof(raw_attrs)/sizeof(raw_attrs[0]);++i)
        memcpy((unsigned char*)&attrs+raw_attrs[i].offset,&raw_attrs[i].bits,4);
    check(FtPart_L1stNb==23,"raw effect joint ordinal");
    HSD_GObjEvent anim[]={ftNs_SpecialLwStart_Anim,ftNs_SpecialLwHold_Anim,ftNs_SpecialLwHit_Anim,
        ftNs_SpecialLwEnd_Anim,ftNs_SpecialLwTurn_Anim,ftNs_SpecialAirLwStart_Anim,ftNs_SpecialAirLwHold_Anim,
        ftNs_SpecialAirLwHit_Anim,ftNs_SpecialAirLwEnd_Anim,ftNs_SpecialAirLwTurn_Anim};
    HSD_GObjEvent phys[]={ftNs_SpecialLwStart_Phys,ftNs_SpecialLwHold_Phys,ftNs_SpecialLwHit_Phys,
        ftNs_SpecialLwEnd_Phys,ftNs_SpecialLwTurn_Phys,ftNs_SpecialAirLwStart_Phys,ftNs_SpecialAirLwHold_Phys,
        ftNs_SpecialAirLwHit_Phys,ftNs_SpecialAirLwEnd_Phys,ftNs_SpecialAirLwTurn_Phys};
    HSD_GObjEvent coll[]={ftNs_SpecialLwStart_Coll,ftNs_SpecialLwHold_Coll,ftNs_SpecialLwHit_Coll,
        ftNs_SpecialLwEnd_Coll,ftNs_SpecialLwTurn_Coll,ftNs_SpecialAirLwStart_Coll,ftNs_SpecialAirLwHold_Coll,
        ftNs_SpecialAirLwHit_Coll,ftNs_SpecialAirLwEnd_Coll,ftNs_SpecialAirLwTurn_Coll};
    HSD_GObjEvent interrupt[]={ftNs_SpecialLwStart_IASA,ftNs_SpecialLwHold_IASA,ftNs_SpecialLwHit_IASA,
        ftNs_SpecialLwEnd_IASA,ftNs_SpecialLwTurn_IASA,ftNs_SpecialAirLwStart_IASA,ftNs_SpecialAirLwHold_IASA,
        ftNs_SpecialAirLwHit_IASA,ftNs_SpecialAirLwEnd_IASA,ftNs_SpecialAirLwTurn_IASA};
    for(unsigned air=0;air<2;++air)for(unsigned face=0;face<2;++face){
        reset(367+5*air,air,face?-1:1);lane=0;if(air)ftNs_SpecialAirLwStart_Enter(&objects[0]);else ftNs_SpecialLwStart_Enter(&objects[0]);
        lane=1;if(air)module->special_air_lw(&objects[1]);else module->special_lw(&objects[1]);compare();
    }
    for(unsigned state=0;state<10;++state)for(unsigned hold=0;hold<2;++hold)
    for(int lag=-1;lag<=2;++lag)for(unsigned end=0;end<2;++end)for(unsigned active=0;active<2;++active)
    for(unsigned command=0;command<2;++command){
        reset(367+state,state>=5,hold?-1:1);remaining=!end;
        set_counters(lag,hold,1,end?-1:8,active?1:0);
        for(unsigned i=0;i<2;++i){fighters[i].input.held_buttons[0]=hold?HSD_PAD_B:0;
            fighters[i].x2219_b0=active;fighters[i].cmd_vars[0]=command;}
        lane=0;anim[state](&objects[0]);lane=1;module->motion_states[26+state].anim_cb(&objects[1]);compare();
    }
    for(unsigned state=0;state<10;++state)for(int delay=-1;delay<=1;++delay)for(unsigned touch=0;touch<2;++touch){
        reset(367+state,state>=5,-1);contact=touch;set_counters(1,false,delay,3,1);
        lane=0;phys[state](&objects[0]);coll[state](&objects[0]);interrupt[state](&objects[0]);
        lane=1;module->motion_states[26+state].phys_cb(&objects[1]);module->motion_states[26+state].coll_cb(&objects[1]);
        module->motion_states[26+state].input_cb(&objects[1]);compare();
    }
    for(unsigned air=0;air<2;++air)for(unsigned already_hit=0;already_hit<2;++already_hit)
    for(unsigned late=0;late<2;++late)for(unsigned hp=0;hp<7;++hp)for(unsigned damage=0;damage<10;++damage){
        reset((already_hit?369:368)+5*air,air,1);
        for(unsigned i=0;i<2;++i){fighters[i].dmg.x1830_percent=hp*1.25f;
            fighters[i].AbsorbAttr.x1A44_damageTaken=damage*.7f;fighters[i].AbsorbAttr.x1A40_absorbHitDirection=-1;
            fighters[i].cur_anim_frame=attrs.x7C_PSI_MAGNET_UNK2+(late?.25f:0);}
        lane=0;ftNs_AbsorbThink_DecideAction(&objects[0]);lane=1;module->on_absorb(&objects[1]);compare();
    }
    printf("Ness Magnet stock comparison: %u checks, %u failures\n",checks,failures);
    FreeLibrary(library);return failures?1:0;
}
