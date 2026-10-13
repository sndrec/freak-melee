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
#include <melee/ft/kinds/ftSeak/ftseakspecialn.h>
#include <melee/ft/kinds/ftSeak/ftseakspecials.h>
#include <melee/ft/kinds/ftSeak/ftseak.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/item.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itseakneedleheld.h>
#include <melee/it/kinds/itseakneedlethrown.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/lb/native_model_joint.h>
#include <melee/lb/lbcollision.h>
#include <sysdolphin/baselib/random.h>
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
    Freefall,LandingSpecial,Create,Hide,Debug,ItemState,Interact,Color,Random,Attach,PickupSound,AttachTail,Center,DisableHitbox,Recount,Sweep,LineExists,BindLine,LineSpeed,LineNormal};
static SkEvent events[512],expected_events[512];static unsigned event_count,expected_count;
static void record(int op,int a,int b,float x,float y,float z,float w){CHECK(event_count<512);events[event_count++]=(SkEvent){op,a,b,x,y,z,w};}
static void unexpected_boundary(void){CHECK(!"unexpected engine boundary");}
static HSD_GObj source,object,foreign,*heads[32];HSD_GObj**HSD_GObjPLinkHead=heads;
static Fighter fighter,foreign_fighter;static Item item;static HSD_JObj root,child,needles[6],raw_joints[58];static FighterBone bones[58];
static ftSeakAttributes sk_attrs;static Article article;static itSeakNeedleThrownAttributes throw_attrs;
static ftCommonData common;ftCommonData*p_ftCommonData=&common;
static ItemCommonData item_common;ItemCommonData*it_804D6D28=&item_common;
static bool scripted,remaining,ground_result,air_result,soft_floor,cliff_result,create_success;
static int invalidation;static const NativeFighterDefinition*definition;
static const NativeFighterArticleLogic*held_logic,*thrown_logic,*active_logic;
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
static ItemLogicTable stock_held,stock_thrown;
static Vec3 center={7,11,-3};
Item_GObj*Item_80268B18(SpawnItem*spawn){
    CHECK((spawn->kind==It_Kind_Seak_NeedleHeld||spawn->kind==It_Kind_Seak_NeedleThrow)&&spawn->x0_parent_gobj==&source&&spawn->x4_parent_gobj2==&source);
    CHECK(spawn->x3C_damage==0&&spawn->vel.x==0&&spawn->vel.y==0&&spawn->vel.z==0&&spawn->x40==0&&spawn->x44_flag.b0);
    record(Create,create_success,spawn->kind,spawn->pos.x,spawn->pos.y,spawn->pos.z,spawn->facing_dir);
    record(Create,-1,0,spawn->prev_pos.x,spawn->prev_pos.y,spawn->prev_pos.z,0);
    if(!create_success)return NULL;
    item.pos=spawn->pos;item.kind=spawn->kind;item.owner=&source;item.facing_dir=spawn->facing_dir;
    active_logic=spawn->kind==It_Kind_Seak_NeedleHeld?held_logic:thrown_logic;
    if(scripted){item.native_article_author_kind=fighter.kind;item.xB8_itemLogicTable=active_logic->logic;item.xDD4_itemVar.native_article.attachment_owner=&source;item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;}
    else item.xB8_itemLogicTable=spawn->kind==It_Kind_Seak_NeedleHeld?&stock_held:&stock_thrown;
    return &object;
}
void it_8026BB44(Item_GObj*g){CHECK(g==&object);root.flags|=JOBJ_HIDDEN;record(Hide,0,0,0,0,0,0);}
void Item_80268E5C(Item_GObj*g,enum_t state,Item_StateChangeFlags flags){CHECK(g==&object);item.msid=state;record(ItemState,(int)state,flags,0,0,0,0);if(invalidation==3)heads[HSD_GOBJ_PLINK_ITEM]=NULL;else if(invalidation==4)item.x1C++;}
void db_80225DD8(Item_GObj*g,Fighter_GObj*owner){CHECK(g==&object&&owner==&source);record(Debug,1,0,0,0,0,0);}
bool it_8026B894(Item_GObj*g,Item_GObj*other){CHECK(g==&object&&other==&object);record(Interact,0,0,0,0,0,0);return false;}
void pl_8003FC44(int player,bool flag){(void)player;(void)flag;}
float ft_GetGroundFrictionMultiplier(Fighter*f){CHECK(f==&fighter);return .8f;}


static unsigned random_seed,random_calls;
s32 HSD_Randi(s32 bound){CHECK(bound>0);unsigned value=(random_seed+random_calls*7)%((unsigned)bound);++random_calls;record(Random,(int)bound,(int)value,0,0,0,0);return value;}
void it_8026BB68(HSD_GObj*g,Vec3*p){CHECK(g==&source);*p=center;record(Center,0,0,p->x,p->y,p->z,0);}
void HSD_JObjSetFlags(HSD_JObj*j,u32 flags){if(j)j->flags|=flags;}
void HSD_JObjClearFlags(HSD_JObj*j,u32 flags){if(j)j->flags&=~flags;}
void HSD_JObjSetFlagsAll(HSD_JObj*j,u32 flags){if(j){j->flags|=flags;for(HSD_JObj*c=j->child;c;c=c->next)HSD_JObjSetFlagsAll(c,flags);}}
void HSD_JObjClearFlagsAll(HSD_JObj*j,u32 flags){if(j){j->flags&=~flags;for(HSD_JObj*c=j->child;c;c=c->next)HSD_JObjClearFlagsAll(c,flags);}}
bool ftCo_800BFFD0(Fighter*f,enum_t color,bool force){CHECK(f==&fighter);record(Color,color,force,0,0,0,0);return true;}
GroundOrAir ft_80081D0C(Fighter_GObj*g){CHECK(g==&source);record(AirProbe,air_result,0,0,0,0,0);return air_result?GA_Air:GA_Ground;}
void ftCo_Fall_Enter(Fighter_GObj*g){CHECK(g==&source);fighter.motion_id=29;record(Motion,29,0,0,0,0,0);}
void ftCo_Landing_Enter_Basic(Fighter_GObj*g){CHECK(g==&source);fighter.motion_id=42;record(Motion,42,0,0,0,0,0);}
void ftSk_SpecialS_CheckAndDestroyChain(HSD_GObj*g){CHECK(g==&source&&!fighter.u.sk.x8);/* Explicit no-active-Chain boundary. */}
bool ftKb_SpecialNSk_80105FF0(HSD_GObj*g){(void)g;CHECK(!"Kirby excluded");return false;}
int ftKb_SpecialNSk_80106020(HSD_GObj*g){(void)g;CHECK(!"Kirby excluded");return 0;}
f32 ftLib_800869D4(Fighter_GObj*g){CHECK(g==&source);return 1.375f;}
s32 ftLib_800865CC(Fighter_GObj*g){CHECK(g==&source);return fighter.ground_or_air==GA_Air;}
void it_80273168(Item_GObj*g){CHECK(g==&object);record(PickupSound,0,0,0,0,0,0);}
void it_802742F4(Item_GObj*g,Fighter_GObj*owner,Fighter_Part part){CHECK(g==&object&&owner==&source);item.owner=owner;item.xDC4=part;record(Attach,part,0,0,0,0,0);}
void Item_8026B074(Item*ip){CHECK(ip==&item);record(AttachTail,0,0,0,0,0,0);}
void lbColl_80008428(HitCapsule*hit){CHECK(hit==&item.x5D4_hitboxes[0].hit);hit->state=0;record(DisableHitbox,0,0,0,0,0,0);}
void it_8027129C_by_4(Item_GObj*g){CHECK(g==&object);record(Recount,0,0,0,0,0,0);}
void Item_8026AE84(Item*ip,enum_t sound,u8 a,u8 b){CHECK(ip==&item);record(Particle,sound,a,b,0,0,0);}
static bool sweep_hit,line_exists;static Vec3 line_normal={.6f,.8f,0},line_velocity={.25f,.5f,0};
bool it_8026EA20(HSD_GObj*g,Vec3*previous,Vec3*current,Vec3*normal,int*line,u32*flags){CHECK(g==&object&&current==&item.pos);record(Sweep,0,0,previous->x,previous->y,previous->z,0);if(sweep_hit){*normal=line_normal;*line=7;*flags=0x10000;}return sweep_hit;}
bool mpLib_80054ED8(int line){record(LineExists,line,line_exists,0,0,0,0);return line_exists;}
void mpColl_80043558(CollData*c,int line){CHECK(c==&item.x378_itemColl);record(BindLine,line,0,0,0,0,0);}
bool mpGetSpeed(int line,Vec3*p,Vec3*v){CHECK(p==&item.pos&&v==&item.x40_vel);*v=line_velocity;record(LineSpeed,line,0,v->x,v->y,v->z,0);return true;}
Vec3*mpLineGetNormal(int line,Vec3*p){*p=line_normal;record(LineNormal,line,0,p->x,p->y,p->z,0);return p;}
#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
static unsigned bone_count(Fighter*f){CHECK(f==&fighter);return 58;}
static unsigned field(const char*name){for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(definition->state_fields[i].name,name))return definition->state_fields[i].offset;CHECK(false);return 0;}
static void put_int(const char*n,int32_t v){memcpy(fighter.native_module_state+field(n),&v,4);}
static int32_t get_int(const char*n){int32_t v;memcpy(&v,fighter.native_module_state+field(n),4);return v;}
static void put_bool(const char*n,bool v){fighter.native_module_state[field(n)]=v;}
static bool get_bool(const char*n){return fighter.native_module_state[field(n)]!=0;}
static unsigned item_field(const char*name){for(unsigned i=0;i<active_logic->state_field_count;++i)if(!strcmp(active_logic->state_fields[i].name,name))return active_logic->state_fields[i].offset;CHECK(false);return 0;}
static void item_put_float(const char*n,float v){memcpy(item.xDD4_itemVar.native_article.state+item_field(n),&v,4);}
static void item_put_int(const char*n,int32_t v){memcpy(item.xDD4_itemVar.native_article.state+item_field(n),&v,4);}
static float item_get_float(const char*n){float v;memcpy(&v,item.xDD4_itemVar.native_article.state+item_field(n),4);return v;}
static int32_t item_get_int(const char*n){int32_t v;memcpy(&v,item.xDD4_itemVar.native_article.state+item_field(n),4);return v;}
static void signal(const char*name){uint32_t hash=2166136261u;for(const unsigned char*p=(const unsigned char*)name;*p;++p)hash=(hash^*p)*16777619u;for(unsigned i=0;i<active_logic->signal_count;++i)if(active_logic->signals[i].name_hash==hash){active_logic->signals[i].callback(&object,0);return;}CHECK(false);}
static bool signal_by_id(HSD_GObj*caller,uint32_t id,uint32_t hash,float value){CHECK(caller==&source&&id==item.x1C);for(unsigned i=0;i<active_logic->signal_count;++i)if(active_logic->signals[i].name_hash==hash){active_logic->signals[i].callback(&object,value);return true;}CHECK(false);return false;}
static int32_t create_needle(Fighter_GObj*g,ItemKind kind,Vec3*p,Vec3*v,float facing,bool at_owner,uint32_t group,float value,bool collision){
    CHECK(g==&source&&group==0&&value==0&&collision&&v->x==0&&v->y==0&&v->z==0);
    CHECK(at_owner==(kind==It_Kind_Seak_NeedleThrow));SpawnItem spawn={0};spawn.kind=kind;Item_InitSpawn(&spawn,g,p,facing);
    if(at_owner)it_8026BB68(g,&spawn.pos);
    if(!Item_80268B18(&spawn))return -1;
    active_logic->on_created(&object);return(int32_t)item.x1C;
}
static HSD_GObj*creator(Item_GObj*g){CHECK(g==&object);HSD_GObj*c=item.xDD4_itemVar.native_article.attachment_owner;return nativeCallbackSource_Alive(c,item.xDD4_itemVar.native_article.creator_spawn_id,NativeCallbackSource_Fighter)?c:NULL;}
static uint32_t current_owner(Item_GObj*g){CHECK(g==&object);uint32_t generation;return nativeCallbackSource_Snapshot(item.owner,NativeCallbackSource_Fighter,&generation)?generation:0;}
static bool target_airborne(uint32_t id){CHECK(id==fighter.x8_spawnNum);return fighter.ground_or_air==GA_Air;}
static const NativeFighterCallbackExport*target_export(uint32_t id,uint32_t hash){CHECK(id==fighter.x8_spawnNum);for(unsigned i=0;i<definition->callback_export_count;++i)if(definition->callback_exports[i].name_hash==hash)return &definition->callback_exports[i];CHECK(false);return NULL;}
static bool target_bool(uint32_t id,uint32_t hash){const NativeFighterCallbackExport*e=target_export(id,hash);CHECK(e->result_type==NativeFighterReturn_Bool);return e->bool_callback(&source);}
static int32_t target_int(uint32_t id,uint32_t hash){const NativeFighterCallbackExport*e=target_export(id,hash);CHECK(e->result_type==NativeFighterReturn_Int);return e->int_callback(&source);}
static float joint_value(Item_GObj*g,unsigned model,const char*path,unsigned channel){CHECK(g==&object&&model==0);return nativeModelJoint_Read(nativeModelJoint_Resolve(&root,path),channel);}
static void set_joint_value(Item_GObj*g,unsigned model,const char*path,unsigned channel,float value){CHECK(g==&object&&model==0);nativeModelJoint_Write(nativeModelJoint_Resolve(&root,path),channel,value);}
static void initialize(bool generated,int motion,unsigned seed,bool held){
    scripted=generated;memset(&fighter,0,sizeof fighter);memset(&foreign_fighter,0,sizeof foreign_fighter);memset(&item,0,sizeof item);memset(&source,0,sizeof source);memset(&foreign,0,sizeof foreign);memset(&object,0,sizeof object);memset(&root,0,sizeof root);memset(&child,0,sizeof child);memset(needles,0,sizeof needles);memset(heads,0,sizeof heads);
    source.user_data=&fighter;source.classifier=HSD_GOBJ_CLASS_FIGHTER;foreign.user_data=&foreign_fighter;foreign.classifier=HSD_GOBJ_CLASS_FIGHTER;source.next=&foreign;foreign_fighter.x8_spawnNum=0x80000002;
    object.user_data=&item;object.classifier=HSD_GOBJ_CLASS_ITEM;object.hsd_obj=&root;heads[HSD_GOBJ_PLINK_FIGHTER]=&source;heads[HSD_GOBJ_PLINK_ITEM]=&object;
    root.child=&child;child.child=needles;root.scale=child.scale=(Vec3){1,1,1};for(unsigned i=0;i<6;++i){needles[i].next=i==5?NULL:&needles[i+1];needles[i].flags=JOBJ_HIDDEN;}
    fighter.kind=generated?Ft_Kind_CustomFirst:Ft_Kind_Seak;fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);fighter.dat_attrs=&sk_attrs;fighter.parts=bones;fighter.gobj=&source;fighter.motion_id=motion;fighter.ground_or_air=motion>=345;
    fighter.self_vel=(Vec3){1.2f,-.8f,.3f};fighter.gr_vel=2.3f;fighter.x74_self_accel=(Vec3){.3f,.4f,.5f};fighter.frame_speed_mul=.7f;fighter.cur_anim_frame=17.5f;fighter.x34_scale=(Vec3){.7f,1.25f,.9f};fighter.cur_pos=(Vec3){3,5,9};fighter.facing_dir=1;
    fighter.input.lstick[0]=(Vec2){.75f,.25f};fighter.co_attrs.gravity=.13f;fighter.co_attrs.terminal_velocity=2;fighter.co_attrs.air_drift_max=1;fighter.co_attrs.air_drift_stick_mul=.03f;fighter.co_attrs.aerial_drift_base=.015f;fighter.co_attrs.aerial_friction=.05f;
    common.horizontal_stick_deadzone=.2f;common.aerial_friction_oob=.08f;item_common.x4C_float=.5f;
    item.entity=&object;item.x1C=UINT32_C(0x80000001);item.kind=held?It_Kind_Seak_NeedleHeld:It_Kind_Seak_NeedleThrow;item.owner=&source;item.facing_dir=1;item.pos=(Vec3){2,4,6};item.x40_vel=(Vec3){1,-2,.5f};item.xC4_article_data=&article;article.x4_specialAttributes=&throw_attrs;
    item.xDC8_word.flags.x15=1;item.xDC8_word.flags.x13=1;item.xDC8_word.flags.x1F=1;item.xD44_lifeTimer=20;item.xD48_halfLifeTimer=10;item.xDAA_flag.b2=true;item.x5D4_hitboxes[0].hit.state=1;
    active_logic=held?held_logic:thrown_logic;
    if(generated){item.native_article_author_kind=fighter.kind;item.xB8_itemLogicTable=active_logic->logic;item.xDD4_itemVar.native_article.attachment_owner=&source;item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;put_int("needle_count",3);put_int("needle_held",(int32_t)item.x1C);put_int("needle_shot_tick",4);put_bool("needle_shot_pending",true);put_int("needle_charge_tick",0);
        if(!held){active_logic->on_created(&object);item_put_float("spin",.17f);item_put_float("horizontal",.4f);item_put_float("terminal",-2.2f);item_put_float("gravity",-.14f);item_put_float("previous_x",-1);item_put_float("previous_y",2);item_put_float("previous_z",0);item_put_float("direction",.7f);item_put_int("surface_line",7);item_put_float("current_surface_angle",.2f);item_put_float("previous_surface_angle",.1f);}}
    else{item.xB8_itemLogicTable=held?&stock_held:&stock_thrown;fighter.u.sk.x0=3;fighter.u.sk.x4=&object;fighter.mv.sk.specialn.x0=4;fighter.mv.sk.specialn.x4=true;fighter.mv.sk.specialn.x8=0;
        if(held)item.xDD4_itemVar.seakneedleheld.owner=&source;else item.xDD4_itemVar.seakneedlethrown=(itSeakNeedleThrown_ItemVars){.xDD4=.17f,.xDD8=.4f,.xDDC=-2.2f,.xDE0=-.14f,.xDE4={-1,2,0},.xDF0=.7f,.xDF4=7,.xDF8=.2f,.xDFC=.1f};}
    /* Reset baseline effects after table initialization. */
    item.xD44_lifeTimer=20;item.xD48_halfLifeTimer=10;item.xDCD_flag.b3=false;
    event_count=random_calls=0;random_seed=seed;remaining=true;ground_result=true;air_result=false;create_success=true;sweep_hit=false;line_exists=true;invalidation=0;
}
typedef struct Observation {int32_t i[48];float f[48];} Observation;
static Observation expected;
static Observation observe(void){Observation o={0};unsigned n=0,m=0;
#define I(v) o.i[n++]=(int32_t)(v)
#define F(v) o.f[m++]=(v)
    I(fighter.motion_id);I(fighter.ground_or_air);I(fighter.cmd_vars[0]);I(fighter.cmd_vars[1]);I(fighter.cmd_vars[2]);I(fighter.cmd_vars[3]);I(fighter.throw_flags_b0);I(fighter.take_dmg_cb!=NULL);I(fighter.death2_cb!=NULL);I(fighter.accessory4_cb!=NULL);
    I(scripted?get_int("needle_count"):fighter.u.sk.x0);I(scripted?get_int("needle_held")!=-1:fighter.u.sk.x4!=NULL);I(scripted?get_int("needle_shot_tick"):fighter.mv.sk.specialn.x0);I(scripted?get_bool("needle_shot_pending"):fighter.mv.sk.specialn.x4);I(scripted?get_int("needle_charge_tick"):fighter.mv.sk.specialn.x8);
    I(item.kind);I(item.msid);I(item.owner==&source?1:item.owner==&foreign?2:0);I(item.xDC8_word.flags.x15);I(item.xDC8_word.flags.x13);I(item.xDC8_word.flags.x1F);I(item.xDCD_flag.b3);I(item.xDCC_flag.b3);I(item.xDAA_flag.b2);I(item.x5D4_hitboxes[0].hit.state);I(item.xDC4);I(random_calls);
    I(item.xDAC_itcmd_var0);I(item.xDB0_itcmd_var1);I(item.xDB4_itcmd_var2);I(item.xDB8_itcmd_var3);
    for(unsigned i=0;i<6;++i)I((needles[i].flags&JOBJ_HIDDEN)!=0);
    F(fighter.self_vel.x);F(fighter.self_vel.y);F(fighter.self_vel.z);F(fighter.x74_self_accel.x);F(fighter.x74_self_accel.y);F(fighter.gr_vel);F(fighter.cur_anim_frame);F(fighter.frame_speed_mul);F(fighter.facing_dir);
    F(item.pos.x);F(item.pos.y);F(item.pos.z);F(item.x40_vel.x);F(item.x40_vel.y);F(item.x40_vel.z);F(item.facing_dir);F(item.xD44_lifeTimer);F(item.xD48_halfLifeTimer);
    F(root.rotate.x);F(root.rotate.y);F(root.rotate.z);F(child.rotate.x);F(child.rotate.y);F(child.rotate.z);F(child.scale.x);F(child.scale.y);F(child.scale.z);
    if(item.kind==It_Kind_Seak_NeedleThrow){
#define T(name,member) F(scripted?item_get_float(name):item.xDD4_itemVar.seakneedlethrown.member)
        T("spin",xDD4);T("horizontal",xDD8);T("terminal",xDDC);T("gravity",xDE0);T("previous_x",xDE4.x);T("previous_y",xDE4.y);T("previous_z",xDE4.z);T("direction",xDF0);I(scripted?item_get_int("surface_line"):item.xDD4_itemVar.seakneedlethrown.xDF4);T("current_surface_angle",xDF8);T("previous_surface_angle",xDFC);
#undef T
    }
#undef I
#undef F
    CHECK(n<=48&&m<=48);return o;}
static uint32_t bits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static void save(void){expected=observe();expected_count=event_count;memcpy(expected_events,events,event_count*sizeof*events);}
static void compare(void){Observation o=observe();if(event_count!=expected_count)fprintf(stderr,"events actual%u expected%u\n",event_count,expected_count);if(event_count!=expected_count){for(unsigned j=0;j<expected_count||j<event_count;++j)fprintf(stderr,"%u actual%d,%d,%d expected%d,%d,%d\n",j,j<event_count?events[j].op:-1,j<event_count?events[j].a:0,j<event_count?events[j].b:0,j<expected_count?expected_events[j].op:-1,j<expected_count?expected_events[j].a:0,j<expected_count?expected_events[j].b:0);}CHECK(event_count==expected_count);for(unsigned i=0;i<event_count;++i){if(memcmp(&events[i],&expected_events[i],sizeof events[i]))fprintf(stderr,"event%u op%d/%d a%d/%d b%d/%d values %.9g/%.9g\n",i,events[i].op,expected_events[i].op,events[i].a,expected_events[i].a,events[i].b,expected_events[i].b,events[i].x,expected_events[i].x);CHECK(events[i].op==expected_events[i].op);CHECK(events[i].a==expected_events[i].a);CHECK(events[i].b==expected_events[i].b);CHECK(bits(events[i].x)==bits(expected_events[i].x));CHECK(bits(events[i].y)==bits(expected_events[i].y));CHECK(bits(events[i].z)==bits(expected_events[i].z));CHECK(bits(events[i].w)==bits(expected_events[i].w));}
    for(unsigned i=0;i<48;++i){if(o.i[i]!=expected.i[i])fprintf(stderr,"integer%u actual%d expected%d\n",i,o.i[i],expected.i[i]);CHECK(o.i[i]==expected.i[i]);}
    for(unsigned i=0;i<48;++i){if(bits(o.f[i])!=bits(expected.f[i]))fprintf(stderr,"float%u actual%.9g expected%.9g\n",i,o.f[i],expected.f[i]);CHECK(bits(o.f[i])==bits(expected.f[i]));}++pairs;}
static HSD_GObjEvent original[8][4]={
 {ftSk_SpecialNStart_Anim,ftSk_SpecialNStart_IASA,ftSk_SpecialNStart_Phys,ftSk_SpecialNStart_Coll},
 {ftSk_SpecialNLoop_Anim,ftSk_SpecialNLoop_IASA,ftSk_SpecialNLoop_Phys,ftSk_SpecialNLoop_Coll},
 {ftSk_SpecialNCancel_Anim,ftSk_SpecialNCancel_IASA,ftSk_SpecialNCancel_Phys,ftSk_SpecialNCancel_Coll},
 {ftSk_SpecialNEnd_Anim,ftSk_SpecialNEnd_IASA,ftSk_SpecialNEnd_Phys,ftSk_SpecialNEnd_Coll},
 {ftSk_SpecialAirNStart_Anim,ftSk_SpecialAirNStart_IASA,ftSk_SpecialAirNStart_Phys,ftSk_SpecialAirNStart_Coll},
 {ftSk_SpecialAirNLoop_Anim,ftSk_SpecialAirNLoop_IASA,ftSk_SpecialAirNLoop_Phys,ftSk_SpecialAirNLoop_Coll},
 {ftSk_SpecialAirNCancel_Anim,ftSk_SpecialAirNCancel_IASA,ftSk_SpecialAirNCancel_Phys,ftSk_SpecialAirNCancel_Coll},
 {ftSk_SpecialAirNEnd_Anim,ftSk_SpecialAirNEnd_IASA,ftSk_SpecialAirNEnd_Phys,ftSk_SpecialAirNEnd_Coll}};
static void phase(int motion,int which,bool generated){if(!generated){original[motion-341][which](&source);return;}const MotionState*s=&definition->motion_states[motion-341];HSD_GObjEvent f=which==0?s->anim_cb:which==1?s->input_cb:which==2?s->phys_cb:s->coll_cb;CHECK(f);f(&source);}
static void item_sound(Item_GObj*g,s32 sound,u8 a,u8 b){CHECK(g==&object);(void)Item_8026AE84(&item,(u32)sound,a,b);}
int main(void){
#include "attributes.inc"
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/4,parameter_schema));NativeFighterHost host={0};
#include "hosts.inc"
    host.callback_source_alive=nativeCallbackSource_Alive;host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.finish_ground_action=ft_8008A2BC;host.ground_friction=ft_80084F3C;
    host.ground_collision=ft_80082708;host.air_collision=ft_80081D0C;host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;host.fall=ftCommon_Fall;host.air_decel_normal=ftCommon_CalcSelfAccel_DeaccelAir;host.air_accel_normal=ftCommon_CalcSelfAccel_Drift;
    host.air_drift=ft_80084EEC;host.enter_fall=ftCo_Fall_Enter;host.enter_freefall=ftCo_80096900;host.enter_landing_basic=ftCo_Landing_Enter_Basic;host.start_color=ftCo_800BFFD0;
    host.sin_float=sinf;host.cos_float=cosf;host.abs_float=fabsf;host.sqrt_float=sqrtf;host.angle_atan2_float=atan2f;host.random_int=HSD_Randi;host.vector_mirror=lbVector_Mirror;
    host.spawn_effect=efSync_Spawn;host.play_sound=ft_PlaySFX;host.create_article_at=create_needle;host.signal_article=signal_by_id;
    host.bone_count=bone_count;host.item_creator=creator;host.item_current_owner=current_owner;host.target_call_bool=target_bool;host.target_call_int=target_int;host.target_airborne=target_airborne;host.fighter_model_scale=ftLib_800869D4;
    host.item_expire_tick=it_80273130;host.item_use_manual_lifetime=it_8026B3A8;host.item_change_state_flags=Item_80268E5C;host.item_set_debug_owner=db_80225DD8;host.item_interact=it_8026B894;host.item_set_lifetime=it_80275158;
    host.item_joint_value=joint_value;host.item_set_joint_value=set_joint_value;host.item_attach_to_owner=Item_8026AB54;host.item_set_airborne=it_802762BC;host.item_play_sound=item_sound;host.item_disable_hitbox=it_80272560;
    host.item_segment_sweep=it_8026EA20;host.stage_line_exists=mpLib_80054ED8;host.set_collision_line=mpColl_80043558;host.stage_line_speed=mpGetSpeed;host.stage_line_normal=mpLineGetNormal;
    MotionState motions[24]={0};NativeFighterDefinition base={.fighter_root="ftDataSeak",.source_kind=Ft_Kind_Seak,.motion_state_first=341,.motion_state_count=24,.motion_states=motions};definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->article_logic_count==(
#ifdef SHEIK_COMPOSED_VANISH
3
#else
2
#endif
));
    for(unsigned i=0;i<definition->article_logic_count;++i){const NativeFighterArticleLogic*l=&definition->article_logics[i];if(l->kind==It_Kind_Seak_NeedleHeld)held_logic=l;else if(l->kind==It_Kind_Seak_NeedleThrow)thrown_logic=l;else CHECK(l->kind==It_Kind_Seak_Vanish);}CHECK(held_logic&&thrown_logic);CHECK(thrown_logic->state_count==5&&thrown_logic->local_state_bytes<=512&&held_logic->state_count==1);
    stock_held.picked_up=itSeakNeedleHeld_Logic110_PickedUp;
    for(int air=0;air<2;++air)for(int count=0;count<=6;++count)for(int generated=0;generated<2;++generated){initialize(generated,air?345:341,0,true);fighter.cmd_vars[0]=fighter.cmd_vars[1]=fighter.cmd_vars[2]=fighter.cmd_vars[3]=9;fighter.throw_flags_b0=true;if(generated)put_int("needle_count",count);else fighter.u.sk.x0=count;(generated?(air?definition->special_air_n:definition->special_n):(air?ftSk_SpecialAirN_Enter:ftSk_SpecialN_Enter))(&source);if(generated)compare();else save();}
    for(int motion=341;motion<=348;++motion)for(int count=0;count<=7;++count)for(int tick=0;tick<=20;++tick)for(int remaining_value=0;remaining_value<2;++remaining_value)for(int generated=0;generated<2;++generated){initialize(generated,motion,0,true);remaining=remaining_value;fighter.cur_anim_frame=tick%2?0:17.5f;if(generated){put_int("needle_count",count);put_int("needle_shot_tick",tick);put_int("needle_charge_tick",tick%3);}else{fighter.u.sk.x0=count;fighter.mv.sk.specialn.x0=tick;fighter.mv.sk.specialn.x8=tick%3;}phase(motion,0,generated);if(generated)compare();else save();}
    for(int motion=341;motion<=348;++motion)for(unsigned buttons=0;buttons<4;++buttons)for(int generated=0;generated<2;++generated){initialize(generated,motion,0,true);fighter.input.held_buttons[0]=buttons&1?HSD_PAD_B:0;fighter.input.pressed_buttons=buttons&2?HSD_PAD_LR:0;phase(motion,1,generated);if(generated)compare();else save();}
    for(int motion=341;motion<=348;++motion)for(unsigned variant=0;variant<8;++variant)for(int generated=0;generated<2;++generated){initialize(generated,motion,0,true);fighter.self_vel.x=(variant-4.f)*.5f;fighter.self_vel.y=(variant-2.f)*.3f;fighter.input.lstick[0].x=(variant-4.f)*.25f;phase(motion,2,generated);if(generated)compare();else save();}
    const float lags[]={0,sk_attrs.x10};for(int motion=341;motion<=348;++motion)for(int collided=0;collided<2;++collided)for(unsigned lag=0;lag<2;++lag)for(int generated=0;generated<2;++generated){initialize(generated,motion,0,true);ground_result=!collided;air_result=collided;sk_attrs.x10=lags[lag];/* Inline generated tuning is immutable; alternate zero policy is tested only when retail equals zero. */if(lag==0&&lags[1]!=0)continue;phase(motion,3,generated);if(generated)compare();else save();}sk_attrs.x10=lags[1];
    for(int air=0;air<2;++air)for(int count=0;count<=6;++count)for(int pending=0;pending<2;++pending)for(unsigned seed=0;seed<9;++seed)for(int allocation=0;allocation<2;++allocation)for(int generated=0;generated<2;++generated){initialize(generated,air?348:344,seed,false);create_success=allocation;fighter.ground_or_air=air;if(generated){put_int("needle_count",count);put_bool("needle_shot_pending",pending);}else{fighter.u.sk.x0=count;fighter.mv.sk.specialn.x4=pending;}(generated?script_fn_needle_fire:shootNeedles)(&source);if(generated)compare();else save();}
    for(int air=0;air<2;++air)for(int count=0;count<=6;++count)for(int allocation=0;allocation<2;++allocation)for(int held=0;held<2;++held)for(int generated=0;generated<2;++generated){initialize(generated,air?348:344,3,false);create_success=allocation;fighter.ground_or_air=air;fighter.death2_cb=generated?script_fn_needle_cleanup:ftSk_Init_80110198;fighter.take_dmg_cb=fighter.death2_cb;if(generated){put_int("needle_count",count);put_int("needle_held",held?(int32_t)item.x1C:-1);}else{fighter.u.sk.x0=count;fighter.u.sk.x4=held?&object:NULL;}(generated?script_fn_needle_cleanup:ftSk_Init_80110198)(&source);if(generated)compare();else save();}
    for(int held=0;held<2;++held)for(int allocation=0;allocation<2;++allocation)for(int generated=0;generated<2;++generated){initialize(generated,341,1,held);create_success=allocation;Vec3 pos=fighter.cur_pos;pos.z=0;if(generated){Vec3 vel={0};(void)create_needle(&source,held?It_Kind_Seak_NeedleHeld:It_Kind_Seak_NeedleThrow,&pos,&vel,1,!held,0,0,true);}else if(held)(void)it_802B19AC(&source,&fighter.cur_pos,23,It_Kind_Seak_NeedleHeld,1);else(void)it_802AFD8C(&source,&pos,It_Kind_Seak_NeedleThrow,1);if(generated)compare();else save();}
    for(int count=0;count<=7;++count)for(int owner=0;owner<3;++owner)for(int missing=0;missing<2;++missing){bool result;for(int generated=0;generated<2;++generated){initialize(generated,342,0,true);item.owner=owner==0?NULL:owner==1?&source:&foreign;if(generated){put_int("needle_count",count);put_int("needle_held",missing?-1:(int32_t)item.x1C);}else{fighter.u.sk.x0=count;fighter.u.sk.x4=missing?NULL:&object;}bool actual=generated?held_logic->logic->states[0].animated(&object):itSeakneedleheld_UnkMotion0_Anim(&object);if(generated){CHECK(actual==result);compare();}else{result=actual;save();}}}
    /* Defined original missing-creator return and all Held no-op/interaction phases. */
    for(int generated=0;generated<2;++generated){initialize(generated,342,0,true);if(generated)item.xDD4_itemVar.native_article.attachment_owner=NULL;else item.xDD4_itemVar.seakneedleheld.owner=NULL;bool result=generated?held_logic->logic->states[0].animated(&object):itSeakneedleheld_UnkMotion0_Anim(&object);CHECK(result);if(generated)compare();else save();}
    for(int generated=0;generated<2;++generated){initialize(generated,342,0,true);(generated?held_logic->logic->states[0].physics_updated:itSeakneedleheld_UnkMotion0_Phys)(&object);CHECK(!(generated?held_logic->logic->states[0].collided:itSeakneedleheld_UnkMotion0_Coll)(&object));(generated?held_logic->logic->evt_unk:itSeakNeedleHeld_Logic110_EvtUnk)(&object,&object);if(generated)compare();else save();}
    for(int mode=0;mode<2;++mode)for(int air=0;air<2;++air)for(int facing=0;facing<2;++facing)for(unsigned seed=0;seed<16;++seed)for(int generated=0;generated<2;++generated){initialize(generated,341,seed,false);fighter.ground_or_air=air;item.facing_dir=facing?1:-1;if(generated)signal(mode?"initialize_drop":"initialize_shot");else it_802AFEA8(&object,&source,mode);if(generated)compare();else save();}
    const float life[]={-1,0,.5f,1,2,60};for(int state=0;state<5;++state)for(unsigned l=0;l<6;++l){bool result;for(int generated=0;generated<2;++generated){initialize(generated,341,2,false);item.xD44_lifeTimer=life[l];bool actual=generated?thrown_logic->logic->states[state].animated(&object):it_803F6F50[state].animated(&object);if(generated){CHECK(actual==result);compare();}else{result=actual;save();}}}
    for(int state=0;state<5;++state)for(unsigned v=0;v<9;++v)for(int generated=0;generated<2;++generated){initialize(generated,341,2,false);item.x40_vel.y=-.25f*v;item.x40_vel.x=.3f*v;(generated?thrown_logic->logic->states[state].physics_updated:it_803F6F50[state].physics_updated)(&object);if(generated)compare();else save();}
    for(int state=0;state<5;++state)for(int hit=0;hit<2;++hit)for(int line=0;line<2;++line)for(unsigned seed=0;seed<32;++seed){bool result;for(int generated=0;generated<2;++generated){initialize(generated,341,seed,false);sweep_hit=hit;line_exists=line;bool actual=generated?thrown_logic->logic->states[state].collided(&object):it_803F6F50[state].collided(&object);if(generated){CHECK(actual==result);compare();}else{result=actual;save();}}}
    bool(*contacts[4])(Item_GObj*)={it_2725_Logic109_DmgDealt,it_2725_Logic109_Clanked,it_2725_Logic109_DmgReceived,it_2725_Logic109_HitShield};
    bool(*generated_contacts[4])(Item_GObj*)={thrown_logic->logic->dmg_dealt,thrown_logic->logic->clanked,thrown_logic->logic->dmg_received,thrown_logic->logic->hit_shield};
    for(unsigned contact=0;contact<4;++contact)for(unsigned seed=0;seed<32;++seed){bool result;for(int generated=0;generated<2;++generated){initialize(generated,341,seed,false);bool actual=(generated?generated_contacts[contact]:contacts[contact])(&object);if(generated){CHECK(actual==result);compare();}else{result=actual;save();}}}
    for(int bounce=0;bounce<2;++bounce)for(int facing=0;facing<2;++facing)for(unsigned v=0;v<8;++v){bool result;for(int generated=0;generated<2;++generated){initialize(generated,341,2,false);item.facing_dir=facing?1:-1;item.x40_vel=(Vec3){(v-4.f)*.4f,(v-2.f)*.3f,.75f};item.xC58=(Vec3){.6f,.8f,0};bool actual=generated?(bounce?thrown_logic->logic->shield_bounced:thrown_logic->logic->reflected)(&object):(bounce?it_2725_Logic109_ShieldBounced:it_2725_Logic109_Reflected)(&object);if(generated){CHECK(actual==result);compare();}else{result=actual;save();}}}
    for(int generated=0;generated<2;++generated){initialize(generated,341,0,false);(generated?thrown_logic->logic->destroyed:itSeakNeedleThrown_Logic109_Destroyed)(&object);(generated?thrown_logic->logic->evt_unk:itSeakNeedleThrown_Logic109_EvtUnk)(&object,&object);if(generated)compare();else save();}
    for(int air=0;air<2;++air)for(int change=1;change<=2;++change){initialize(true,air?345:341,0,true);invalidation=change;(air?definition->special_air_n:definition->special_n)(&source);CHECK(event_count==1&&events[0].op==Motion);++safety;}
    for(int mode=0;mode<2;++mode)for(int change=3;change<=4;++change){initialize(true,341,0,false);invalidation=change;signal(mode?"initialize_drop":"initialize_shot");CHECK(event_count==1&&events[0].op==ItemState);++safety;}
    for(int change=3;change<=4;++change){initialize(true,341,0,true);invalidation=change;held_logic->on_created(&object);CHECK(event_count==3&&events[0].op==PickupSound&&events[1].op==Attach&&events[2].op==ItemState);++safety;}
    for(int change=1;change<=2;++change){initialize(true,342,0,true);if(change==1)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;else ++fighter.x8_spawnNum;CHECK(held_logic->logic->states[0].animated(&object));CHECK(event_count==0);++safety;}
    for(unsigned m=0;m<24;++m){const MotionState*s=&definition->motion_states[m];if(m<8
#ifdef SHEIK_COMPOSED_VANISH
||(m>=14&&m<20)
#endif
)CHECK(s->anim_cb&&s->input_cb&&s->phys_cb&&s->coll_cb);else CHECK(!s->anim_cb&&!s->input_cb&&!s->phys_cb&&!s->coll_cb);}
    printf("PASS %u original/generated pairs, %u checks, %u authored safety cases\n",pairs,checks,safety);return 0;
}
