/* Full original callback bodies versus actual generated code. Named semantic
 * projection; controlled motion/animation/stage/allocation/render boundaries. */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/db/db.h>
#include <melee/ef/efsync.h>
#include <melee/ef/eflib.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_Pass.h>
#include <melee/ft/kinds/ftSeak/types.h>
#include <melee/ft/kinds/ftSeak/forward.h>
#include <melee/ft/kinds/ftSeak/ftseakspecialhi.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/item.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itseakvanish.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>

static unsigned checks,pairs,safety;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"pair%u safety%u line%d %s\n",pairs,safety,__LINE__,#x);exit(1);}}while(0)
typedef struct SkEvent {int op,a,b;float x,y,z,w;} SkEvent;
enum {Motion,Start,GroundPhys,GroundProbe,AirProbe,GroundToAir,AirToGround,
    Particle,BoneRead,CliffProbe,SoftFloor,LedgeProbe,Rate,BodyStatus,
    Freefall,LandingSpecial,Create,Hide,Debug,ItemState,Interact};
static SkEvent events[128],expected_events[128];static unsigned event_count,expected_count;
static void record(int op,int a,int b,float x,float y,float z,float w){CHECK(event_count<128);events[event_count++]=(SkEvent){op,a,b,x,y,z,w};}
static void unexpected_boundary(void){CHECK(!"unexpected engine boundary");}
static HSD_GObj source,object,*heads[32];HSD_GObj**HSD_GObjPLinkHead=heads;
static Fighter fighter;static Item item;static HSD_JObj root,raw_joints[58];static FighterBone bones[58];
static ftSeakAttributes sk_attrs;static Article article;
static ftCommonData common;ftCommonData*p_ftCommonData=&common;
static ItemCommonData item_common;ItemCommonData*it_804D6D28=&item_common;
static bool scripted,remaining,ground_result,air_result,soft_floor,cliff_result,create_success;
static int invalidation;static const NativeFighterDefinition*definition;
static const NativeFighterArticleLogic*vanish_logic;
bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*id){(void)g;*id=0;return false;}
EF_Effect*nativeModelEffect_Resolve(HSD_GObj*g,uint32_t id){(void)g;(void)id;return NULL;}
void __assert(char*f,u32 l,char*m){fprintf(stderr,"%s:%u %s\n",f,(unsigned)l,m);exit(1);}
void HSD_JObjSetMtxDirtySub(HSD_JObj*j){j->flags|=JOBJ_MTX_DIRTY;}
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj*donor){
    CHECK(g==&source&&!donor);record(Motion,motion,(int)flags,frame,rate,blend,0);fighter.motion_id=motion;fighter.cur_anim_frame=frame;
    if(invalidation==1)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;else if(invalidation==2)fighter.x8_spawnNum++;
}
void ftAnim_8006EBA4(Fighter_GObj*g){CHECK(g==&source);record(Start,0,0,0,0,0,0);}
bool ftAnim_IsFramesRemaining(Fighter_GObj*g){CHECK(g==&source);return remaining;}
void ft_8008A2BC(Fighter_GObj*g){CHECK(g==&source);fighter.motion_id=14;record(Motion,14,0,0,0,0,0);}
void ft_80084F3C(Fighter_GObj*g){CHECK(g==&source);record(GroundPhys,0,0,0,0,0,0);}
bool ft_80082708(Fighter_GObj*g){CHECK(g==&source);record(GroundProbe,0,0,0,0,0,0);return ground_result;}
bool ft_800827A0(Fighter_GObj*g){return ft_80082708(g);}
void ftCommon_8007D5D4(Fighter*f){CHECK(f==&fighter);f->ground_or_air=GA_Air;record(GroundToAir,0,0,0,0,0,0);}
void ftCommon_8007D7FC(Fighter*f){CHECK(f==&fighter);f->ground_or_air=GA_Ground;record(AirToGround,0,0,0,0,0,0);}
bool ft_CheckGroundAndLedge(Fighter_GObj*g,int facing){CHECK(g==&source);record(LedgeProbe,facing,0,0,0,0,0);return air_result;}
bool ftCliffCommon_80081298(Fighter_GObj*g){CHECK(g==&source);record(CliffProbe,0,0,0,0,0,0);return cliff_result;}
bool ftCo_8009A134(Fighter_GObj*g){CHECK(g==&source);record(SoftFloor,0,0,0,0,0,0);return soft_floor;}
void ftCo_80096900(Fighter_GObj*g,int a,int b,bool interrupt,float mobility,float lag){CHECK(g==&source);fighter.motion_id=35;record(Freefall,a,b,interrupt,mobility,lag,0);}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj*g,bool interrupt,float lag){CHECK(g==&source);fighter.motion_id=43;record(LandingSpecial,interrupt,0,lag,0,0,0);}
void ftAnim_SetAnimRate(Fighter_GObj*g,float rate){CHECK(g==&source);fighter.frame_speed_mul=rate;record(Rate,0,0,rate,0,0,0);}
void ftColl_8007B62C(Fighter_GObj*g,enum_t status){CHECK(g==&source);fighter.x1988=status;record(BodyStatus,(int)status,0,0,0,0,0);}
static void pause_effects(HSD_GObj*g){CHECK(g==&source);}
static void resume_effects(HSD_GObj*g){CHECK(g==&source);}
void efLib_PauseAll(HSD_GObj*g){pause_effects(g);}
void efLib_ResumeAll(HSD_GObj*g){resume_effects(g);}
void lb_8000B1CC(HSD_JObj*j,Vec3*offset,Vec3*out){CHECK(!offset);int index=(int)(j-raw_joints);CHECK(index>=0&&index<58);*out=j->translate;record(BoneRead,index,0,out->x,out->y,out->z,0);}
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj*g,...){CHECK(g==&source);va_list a;va_start(a,g);Vec3*p=va_arg(a,Vec3*);record(Particle,(int)effect,0,p->x,p->y,p->z,0);va_end(a);return(EF_SpawnResult){0};}
void ft_PlaySFX(Fighter*f,enum_t sound,u8 volume,u8 pan){CHECK(f==&fighter);record(Particle,(int)sound,volume,pan,0,0,0);}
Item_GObj*Item_80268B18(SpawnItem*spawn){
    CHECK(spawn->kind==It_Kind_Seak_Vanish&&spawn->x0_parent_gobj==&source&&spawn->x4_parent_gobj2==&source);
    CHECK(spawn->prev_pos.x==spawn->pos.x&&spawn->prev_pos.y==spawn->pos.y&&spawn->prev_pos.z==spawn->pos.z);
    CHECK(spawn->x3C_damage==0&&spawn->vel.x==0&&spawn->vel.y==0&&spawn->vel.z==0&&spawn->x40==0&&spawn->x44_flag.b0);
    record(Create,create_success,0,spawn->pos.x,spawn->pos.y,spawn->pos.z,spawn->facing_dir);return create_success?&object:NULL;
}
void it_8026BB44(Item_GObj*g){CHECK(g==&object);root.flags|=JOBJ_HIDDEN;record(Hide,0,0,0,0,0,0);}
void Item_80268E5C(Item_GObj*g,enum_t state,Item_StateChangeFlags flags){CHECK(g==&object);item.msid=state;record(ItemState,(int)state,flags,0,0,0,0);if(invalidation==3)heads[HSD_GOBJ_PLINK_ITEM]=NULL;else if(invalidation==4)item.x1C++;}
void db_80225DD8(Item_GObj*g,Fighter_GObj*owner){CHECK(g==&object&&owner==&source);record(Debug,1,0,0,0,0,0);}
bool it_8026B894(Item_GObj*g,Item_GObj*other){CHECK(g==&object&&other==&object);record(Interact,0,0,0,0,0,0);return false;}
void pl_8003FC44(int player,bool flag){(void)player;(void)flag;}
float ft_GetGroundFrictionMultiplier(Fighter*f){CHECK(f==&fighter);return .8f;}

#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
static unsigned field(const char*name){for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(definition->state_fields[i].name,name))return definition->state_fields[i].offset;CHECK(false);return 0;}
static void put_int(const char*n,int32_t v){memcpy(fighter.native_module_state+field(n),&v,4);}
static int32_t get_int(const char*n){int32_t v;memcpy(&v,fighter.native_module_state+field(n),4);return v;}
static void put_float(const char*n,float v){memcpy(fighter.native_module_state+field(n),&v,4);}
static float get_float(const char*n){float v;memcpy(&v,fighter.native_module_state+field(n),4);return v;}
static unsigned bone_count(Fighter*f){CHECK(f==&fighter);return 58;}
static int32_t create_vanish(Fighter_GObj*g,ItemKind kind,Vec3*p,Vec3*v,float facing,bool at_owner,uint32_t group,float value,bool collision){
    CHECK(g==&source&&kind==It_Kind_Seak_Vanish&&!at_owner&&group==0&&value==0&&collision&&v->x==0&&v->y==0&&v->z==0);
    SpawnItem spawn={0};spawn.kind=kind;Item_InitSpawn(&spawn,g,p,facing);
    if(!Item_80268B18(&spawn))return -1;vanish_logic->on_created(&object);return(int32_t)item.x1C;
}
static HSD_GObj*creator(Item_GObj*g){CHECK(g==&object);HSD_GObj*c=item.xDD4_itemVar.native_article.attachment_owner;return nativeCallbackSource_Alive(c,item.xDD4_itemVar.native_article.creator_spawn_id,NativeCallbackSource_Fighter)?c:NULL;}
static void initialize(bool generated,int motion,int timer){
    scripted=generated;memset(&fighter,0,sizeof fighter);memset(&item,0,sizeof item);memset(&source,0,sizeof source);memset(&object,0,sizeof object);memset(&root,0,sizeof root);memset(heads,0,sizeof heads);
    source.user_data=&fighter;source.classifier=HSD_GOBJ_CLASS_FIGHTER;object.user_data=&item;object.classifier=HSD_GOBJ_CLASS_ITEM;object.hsd_obj=&root;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&source;heads[HSD_GOBJ_PLINK_ITEM]=&object;
    fighter.kind=generated?Ft_Kind_CustomFirst:Ft_Kind_Seak;fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);fighter.dat_attrs=&sk_attrs;fighter.parts=bones;fighter.gobj=&source;
    for(unsigned i=0;i<58;++i){memset(&raw_joints[i],0,sizeof raw_joints[i]);raw_joints[i].translate=(Vec3){i,.25f*i,-.5f*i};bones[i].joint=&raw_joints[i];}
    fighter.motion_id=motion;fighter.ground_or_air=motion>=358;fighter.self_vel=(Vec3){1.2f,-.8f,.3f};fighter.gr_vel=2.3f;fighter.frame_speed_mul=.7f;fighter.cur_anim_frame=17.5f;
    fighter.xE4_ground_accel_1=.5f;fighter.input.lstick[0]=(Vec2){.75f,.25f};fighter.facing_dir=1;fighter.co_attrs.max_jumps=2;
    fighter.co_attrs.gravity=.13f;fighter.co_attrs.terminal_velocity=2;fighter.co_attrs.air_drift_max=1;fighter.co_attrs.air_drift_stick_mul=.03f;fighter.co_attrs.aerial_drift_base=.015f;fighter.co_attrs.aerial_friction=.05f;
    fighter.coll_data.floor.normal=(Vec3){0,1,0};fighter.coll_data.ceiling.normal=(Vec3){0,-1,0};fighter.coll_data.left_facing_wall.normal=(Vec3){1,0,0};fighter.coll_data.right_facing_wall.normal=(Vec3){-1,0,0};
    common.horizontal_stick_deadzone=.2f;common.aerial_friction_oob=.08f;item_common.xF8=29;
    item.entity=&object;item.x1C=UINT32_C(0x80000001);item.kind=It_Kind_Seak_Vanish;item.owner=&source;item.facing_dir=1;item.xC4_article_data=&article;item.xDC8_word.flags.x15=1;item.xDC8_word.flags.x1A=1;item.xD44_lifeTimer=20;
    if(generated){item.native_article_author_kind=Ft_Kind_CustomFirst;item.xB8_itemLogicTable=(ItemLogicTable*)vanish_logic->logic;item.xDD4_itemVar.native_article.attachment_owner=&source;item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;put_int("vanish_remaining",timer);put_int("vanish_collision_age",timer);}
    else{fighter.mv.sk.specialhi.x0=timer;fighter.mv.sk.specialhi.xC=timer;}
    event_count=0;remaining=true;ground_result=true;air_result=false;soft_floor=false;cliff_result=false;create_success=true;invalidation=0;
}
typedef struct Observation{int32_t i[32];float f[24];}Observation;
static Observation expected;
static Observation observe(void){Observation o={0};unsigned n=0,m=0;
#define I(v) o.i[n++]=(int32_t)(v)
#define F(v) o.f[m++]=(v)
    I(fighter.motion_id);I(fighter.ground_or_air);I(fighter.cmd_vars[0]);I(fighter.x1968_jumpsUsed);I(fighter.invisible);I(fighter.x2223_b4);I(fighter.x1988);I(fighter.ecb_lock);I(fighter.coll_data.x130_flags);I(fighter.accessory4_cb!=NULL);I(fighter.x2219_b0);I(fighter.pre_hitlag_cb!=NULL);I(fighter.post_hitlag_cb!=NULL);
    I(scripted?get_int("vanish_remaining"):fighter.mv.sk.specialhi.x0);I(scripted?get_int("vanish_collision_age"):fighter.mv.sk.specialhi.xC);
    I(item.msid);I(item.owner!=NULL);I(item.xDC8_word.flags.x15);I(item.xDC8_word.flags.x1A);I(item.xDD0_flag.b3);I(item.xDCF_flag.b2);I(item.xDD1_flag.b0);I((root.flags&JOBJ_HIDDEN)!=0);
    F(fighter.self_vel.x);F(fighter.self_vel.y);F(fighter.self_vel.z);F(fighter.x74_self_accel.x);F(fighter.x74_self_accel.y);F(fighter.gr_vel);F(fighter.facing_dir);F(fighter.cur_anim_frame);F(fighter.frame_speed_mul);F(fighter.xE4_ground_accel_1);
    F(scripted?get_float("vanish_stick_x"):fighter.mv.sk.specialhi.vel.x);F(scripted?get_float("vanish_stick_y"):fighter.mv.sk.specialhi.vel.y);
    F(scripted?get_float("vanish_saved_x"):fighter.mv.co.walk.fast_anim_frame);F(scripted?get_float("vanish_saved_y"):fighter.mv.co.common.x14);F(scripted?get_float("vanish_saved_ground"):fighter.mv.co.common.x18);F(item.xD44_lifeTimer);
#undef I
#undef F
    return o;}
static uint32_t bits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static void save(void){expected=observe();expected_count=event_count;memcpy(expected_events,events,event_count*sizeof*events);}
static void compare(void){Observation o=observe();CHECK(event_count==expected_count);for(unsigned i=0;i<event_count;++i){CHECK(events[i].op==expected_events[i].op);CHECK(events[i].a==expected_events[i].a);CHECK(events[i].b==expected_events[i].b);CHECK(bits(events[i].x)==bits(expected_events[i].x));CHECK(bits(events[i].y)==bits(expected_events[i].y));CHECK(bits(events[i].z)==bits(expected_events[i].z));CHECK(bits(events[i].w)==bits(expected_events[i].w));}
    for(unsigned i=0;i<32;++i){if(o.i[i]!=expected.i[i])fprintf(stderr,"integer%u actual%d expected%d\n",i,o.i[i],expected.i[i]);CHECK(o.i[i]==expected.i[i]);}
    for(unsigned i=0;i<24;++i){if(bits(o.f[i])!=bits(expected.f[i]))fprintf(stderr,"float%u actual%.9g expected%.9g\n",i,o.f[i],expected.f[i]);CHECK(bits(o.f[i])==bits(expected.f[i]));}++pairs;}
static HSD_GObjEvent original[6][4]={
 {ftSk_SpecialHiStart_0_Anim,ftSk_SpecialHiStart_0_IASA,ftSk_SpecialHiStart_0_Phys,ftSk_SpecialHiStart_0_Coll},
 {ftSk_SpecialHiStart_1_Anim,ftSk_SpecialHiStart_1_IASA,ftSk_SpecialHiStart_1_Phys,ftSk_SpecialHiStart_1_Coll},
 {ftSk_SpecialHi_Anim,ftSk_SpecialHi_IASA,ftSk_SpecialHi_Phys,ftSk_SpecialHi_Coll},
 {ftSk_SpecialAirHiStart_0_Anim,ftSk_SpecialAirHiStart_0_IASA,ftSk_SpecialAirHiStart_0_Phys,ftSk_SpecialAirHiStart_0_Coll},
 {ftSk_SpecialAirHiStart_1_Anim,ftSk_SpecialAirHiStart_1_IASA,ftSk_SpecialAirHiStart_1_Phys,ftSk_SpecialAirHiStart_1_Coll},
 {ftSk_SpecialAirHi_Anim,ftSk_SpecialAirHi_IASA,ftSk_SpecialAirHi_Phys,ftSk_SpecialAirHi_Coll}};
static void phase(int motion,int which,bool generated){if(!generated){original[motion-355][which](&source);return;}const MotionState*s=&definition->motion_states[motion-341];HSD_GObjEvent f=which==0?s->anim_cb:which==1?s->input_cb:which==2?s->phys_cb:s->coll_cb;CHECK(f);f(&source);}
int main(void){
#include "attributes.inc"
    CHECK(offsetof(ftSeakAttributes,x38)==56&&sizeof(sk_attrs.x38)==4);CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/4,parameter_schema));
    NativeFighterHost host={0};
#include "hosts.inc"
    host.callback_source_alive=nativeCallbackSource_Alive;host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.finish_ground_action=ft_8008A2BC;host.ground_friction=ft_80084F3C;
    host.ground_collision=ft_80082708;host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;host.fall=ftCommon_Fall;host.air_accel_normal=ftCommon_CalcSelfAccel_Drift;
    host.air_decel_normal=ftCommon_CalcSelfAccel_DeaccelAir;host.fall_basic=ftCommon_FallBasic;host.force_air_and_use_jumps=ftCommon_8007D60C;host.ground_to_self_movement=ftCommon_SetSelfMovementFromGroundedMovement;
    host.sin_float=sinf;host.cos_float=cosf;host.abs_float=fabsf;host.sqrt_float=sqrtf;host.angle_atan2_float=atan2f;host.update_facing=ftCommon_UpdateFacing;host.update_facing_threshold=ftCommon_8007DA24;host.vector_angle_xy=lbVector_AngleXY;
    host.skip_soft_floor=ftCo_8009A134;host.set_body_status=ftColl_8007B62C;host.check_ground_and_ledge=ft_CheckGroundAndLedge;host.check_cliff_catch=ftCliffCommon_80081298;host.special_ground_collision=ft_800827A0;
    host.enter_freefall=ftCo_80096900;host.enter_landing_fall_special=ftCo_LandingFallSpecial_Enter;host.set_animation_rate=ftAnim_SetAnimRate;host.bone_count=bone_count;host.joint_position=lb_8000B1CC;
    host.spawn_effect=efSync_Spawn;host.pause_effects=pause_effects;host.resume_effects=resume_effects;host.play_sound=ft_PlaySFX;host.create_article_at=create_vanish;
    host.item_creator=creator;host.item_hide_model=it_8026BB44;host.item_begin_expiry=it_8027518C;host.item_expire_tick=it_802751D8;host.item_use_manual_lifetime=it_8026B3A8;host.item_change_state_flags=Item_80268E5C;host.item_set_debug_owner=db_80225DD8;host.item_interact=it_8026B894;
    MotionState motions[24]={0};NativeFighterDefinition base={.fighter_root="ftDataSeak",.source_kind=Ft_Kind_Seak,.motion_state_first=341,.motion_state_count=24,.motion_states=motions};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->article_logic_count==1);vanish_logic=&definition->article_logics[0];CHECK(vanish_logic->kind==It_Kind_Seak_Vanish);
    const int32_t timers[]={-1,0,1,4,16777217,-16777217};const float speed[]={-3,-.03f,0,.04f,3};
    for(int air=0;air<2;++air)for(int v=0;v<5;++v)for(int generated=0;generated<2;++generated){initialize(generated,air?358:355,4);fighter.self_vel.x=speed[v];fighter.cmd_vars[0]=3;
        (generated?(air?definition->special_air_hi:definition->special_hi):(air?ftSk_SpecialAirHi_Enter:ftSk_SpecialHi_Enter))(&source);if(generated)compare();else save();}
    const Vec2 sticks[]={{0,0},{.001f,0},{-.001f,0},{.49999997f,0},{.5f,0},{-.5f,0},{0,.5f},{0,-.5f},{.75f,.25f},{1,1},{-1,-1},{.3f,.4f}};
    for(int motion=355;motion<=360;++motion)for(unsigned stick=0;stick<12;++stick)for(int facing=0;facing<2;++facing)for(int soft=0;soft<2;++soft)for(int frames=0;frames<2;++frames)for(unsigned t=0;t<6;++t)for(int generated=0;generated<2;++generated){
        initialize(generated,motion,timers[t]);fighter.input.lstick[0]=sticks[stick];fighter.facing_dir=facing?1:-1;soft_floor=soft;remaining=frames;phase(motion,0,generated);if(generated)compare();else save();}
    for(int motion=355;motion<=360;++motion)for(int v=0;v<5;++v)for(int command=0;command<2;++command)for(int generated=0;generated<2;++generated){initialize(generated,motion,4);fighter.self_vel.x=speed[v];fighter.self_vel.y=v&1?-3:.2f;fighter.cmd_vars[0]=command;phase(motion,2,generated);if(generated)compare();else save();}
    for(int motion=355;motion<=360;++motion)for(int land=0;land<2;++land)for(int cliff=0;cliff<2;++cliff)for(int soft=0;soft<2;++soft)for(int mask=0;mask<8;++mask)for(unsigned t=0;t<6;++t)for(int generated=0;generated<2;++generated){
        initialize(generated,motion,timers[t]);ground_result=land;air_result=land;cliff_result=cliff;soft_floor=soft;fighter.self_vel=(Vec3){-2,3,0};fighter.coll_data.env_flags=(mask&1?Collide_CeilingMask:0)|(mask&2?Collide_LeftWallMask:0)|(mask&4?Collide_RightWallMask:0);phase(motion,3,generated);if(generated)compare();else save();}
    for(int motion=355;motion<=360;++motion)for(int generated=0;generated<2;++generated){initialize(generated,motion,4);phase(motion,1,generated);if(generated)compare();else save();}
    for(int end=0;end<2;++end)for(int active=0;active<2;++active)for(int allocation=0;allocation<2;++allocation)for(int generated=0;generated<2;++generated){initialize(generated,358,4);fighter.x2219_b0=active;create_success=allocation;
        (generated?(end?script_fn_vanish_end_effect:script_fn_vanish_start_effect):(end?fn_80113038:fn_80112ED8))(&source);if(generated)compare();else save();}
    for(int generated=0;generated<2;++generated){initialize(generated,355,4);if(generated)vanish_logic->on_created(&object);else {it_802B1D40(&object,&source);db_80225DD8(&object,&source);}if(generated)compare();else save();}
    const float life[]={-1,0,.5f,1,2,60};for(unsigned l=0;l<6;++l){bool result;initialize(false,355,4);item.xD44_lifeTimer=life[l];result=itSeakvanish_UnkMotion0_Anim(&object);save();initialize(true,355,4);item.xD44_lifeTimer=life[l];CHECK(vanish_logic->logic->states[0].animated(&object)==result);compare();}
    for(int generated=0;generated<2;++generated){initialize(generated,355,4);if(generated){CHECK(!vanish_logic->logic->dmg_dealt(&object));vanish_logic->logic->evt_unk(&object,&object);}else{CHECK(!itSeakVanish_Logic42_DmgDealt(&object));it_802B1DCC(&object,&object);}if(generated)compare();else save();}
    /* Healthy constructor and callback cases above stay separate from authored
     * registry invalidation; no old stale-pointer body is called below. */
    for(int air=0;air<2;++air)for(int change=1;change<=2;++change){initialize(true,air?358:355,4);invalidation=change;(air?definition->special_air_hi:definition->special_hi)(&source);CHECK(event_count==1&&events[0].op==Motion);++safety;}
    for(int change=3;change<=4;++change){initialize(true,355,4);invalidation=change;vanish_logic->on_created(&object);CHECK(event_count==2&&events[1].op==ItemState);++safety;}
    for(unsigned m=0;m<24;++m){const MotionState*s=&definition->motion_states[m];if(m>=14&&m<20)CHECK(s->anim_cb&&s->input_cb&&s->phys_cb&&s->coll_cb);else CHECK(!s->anim_cb&&!s->input_cb&&!s->phys_cb&&!s->coll_cb);}
    printf("PASS %u original/generated pairs, %u checks, %u authored safety cases\n",pairs,checks,safety);return 0;
}
