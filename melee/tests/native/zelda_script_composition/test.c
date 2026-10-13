/* Complete extracted phase bodies vs the actual generated module. Allocation,
 * motion/animation, stage probes, recipient dispatch and rendering are explicit
 * controls. Private stock unions are projected into named script state only. */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/cm/camera.h>
#include <melee/db/db.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftZelda/types.h>
#include <melee/ft/kinds/ftZelda/forward.h>
#include <melee/it/it_3F14.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspecialn.h>
#include <melee/ft/kinds/ftZelda/ftzeldaspeciallw.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itCharItems.h>
#include <melee/it/itcoll.h>
#include <melee/it/item.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itzeldadinfire.h>
#include <melee/it/kinds/itzeldadinfireexplode.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>

static unsigned checks, pairs, safety;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"pair%u safety%u line%d %s\n",pairs,safety,__LINE__,#x); exit(1); } } while(0)
typedef struct ZdEvent { int op, a, b; float x,y,z,w; } ZdEvent;
enum { Motion, Start, GroundPhys, ReflectCreate, ReflectUpdate, ProbeGround, ProbeAir,
       GroundToAir, AirToGround, Particle, Destroy, ItemState, DisableHits,
       Rumble, Quake, Debug, Advance, ChildCreate, Interact, Group, Pickup, Drop,
       Show, Hide, KnockEnter, KnockExit, CopyAttrs, ReloadAttrs };
static ZdEvent events[64], expected_events[64]; static unsigned event_count, expected_count;
static void record(int op,int a,int b,float x,float y,float z,float w) {
    CHECK(event_count<64); events[event_count++]=(ZdEvent){op,a,b,x,y,z,w};
}
static void unexpected_boundary(void) { CHECK(!"unexecuted Host boundary was called"); }
static HSD_GObj source, foreign, object, child, *heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static Fighter fighter, other_fighter;
static Item item; static HSD_JObj joint, child_joint, sibling_joint; static FighterBone bones[4];
static ftZelda_DatAttrs zd_attrs; static float din_attrs[12], explosion_attrs[5];
static Article article;
static ftCommonData common; ftCommonData* p_ftCommonData=&common;
static ItemCommonData item_common; ItemCommonData* it_804D6D28=&item_common;
static bool remaining, ground_result, air_result, scripted;
static bool held_heavy;static Item_HoldKinds held_kind;
static int invalidation;
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic* din_logic,*explosion_logic;
bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*id) { (void)g;*id=0;return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj*g,uint32_t id) { (void)g;(void)id;return NULL; }
void __assert(char* file,u32 line,char* message) { fprintf(stderr,"%s:%d %s\n",file,(unsigned)line,message);exit(1); }
void HSD_JObjSetMtxDirtySub(HSD_JObj*j) { j->flags|=JOBJ_MTX_DIRTY; }
void HSD_JObjClearFlagsAll(HSD_JObj*j,u32 flags) { if(!j)return;j->flags&=~flags;for(HSD_JObj*c=j->child;c;c=c->next)HSD_JObjClearFlagsAll(c,flags); }
void HSD_JObjSetFlagsAll(HSD_JObj*j,u32 flags) { if(!j)return;j->flags|=flags;for(HSD_JObj*c=j->child;c;c=c->next)HSD_JObjSetFlagsAll(c,flags); }
static void pause_effects(HSD_GObj*g) { (void)g; }
static void resume_effects(HSD_GObj*g) { (void)g; }
void efLib_PauseAll(HSD_GObj*g) { pause_effects(g); }
void efLib_ResumeAll(HSD_GObj*g) { resume_effects(g); }
void efLib_DestroyAll(HSD_GObj*g) { CHECK(g==&object||g==&source);record(Destroy,0,0,0,0,0,0); }
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj*g,...) { va_list a;va_start(a,g);HSD_JObj*j=va_arg(a,HSD_JObj*);va_end(a);CHECK(j==&joint);record(Particle,(int)effect,0,0,0,0,0);return (EF_SpawnResult){0}; }
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj*donor) {
    CHECK(g==&source && donor==NULL);record(Motion,motion,(int)flags,frame,rate,blend,0);fighter.motion_id=motion;fighter.cur_anim_frame=frame;
    if(invalidation==1)source.user_data=NULL;else if(invalidation==2)fighter.x8_spawnNum++;
}
void ftAnim_8006EBA4(Fighter_GObj*g) { CHECK(g==&source);record(Start,0,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj*g) { CHECK(g==&source);return remaining; }
void ft_8008A2BC(Fighter_GObj*g) { CHECK(g==&source);fighter.motion_id=14;record(Motion,14,0,0,0,0,0); }
void ftCo_Fall_Enter(Fighter_GObj*g) { CHECK(g==&source);fighter.motion_id=29;record(Motion,29,0,0,0,0,0); }
void ft_80084F3C(Fighter_GObj*g) { CHECK(g==&source);record(GroundPhys,0,0,0,0,0,0); }
void ftColl_CreateReflectHit(Fighter_GObj*g,ReflectDesc*d,HSD_GObjEvent hit) {
    CHECK(g==&source);record(ReflectCreate,(int)d->x0_bone_id,(int)d->x4_max_damage,d->x8_offset.x,d->x8_offset.y,d->x8_offset.z,d->x14_size);
    fighter.reflecting=true;fighter.reflect_hit_cb=hit;
    CHECK(d->x0_bone_id==zd_attrs.x84.x0_bone_id&&d->x4_max_damage==zd_attrs.x84.x4_max_damage);
    CHECK(d->x8_offset.x==zd_attrs.x84.x8_offset.x&&d->x8_offset.y==zd_attrs.x84.x8_offset.y&&d->x8_offset.z==zd_attrs.x84.x8_offset.z);
    CHECK(d->x14_size==zd_attrs.x84.x14_size&&d->x18_damage_mul==zd_attrs.x84.x18_damage_mul&&d->x1C_speed_mul==zd_attrs.x84.x1C_speed_mul&&d->x20_behavior==zd_attrs.x84.x20_behavior);
}
void ftColl_8007AEF8(Fighter_GObj*g) { CHECK(g==&source);record(ReflectUpdate,0,0,0,0,0,0); }
bool ft_80082708(Fighter_GObj*g) { CHECK(g==&source);record(ProbeGround,0,0,0,0,0,0);return ground_result; }
GroundOrAir ft_80081D0C(Fighter_GObj*g) { CHECK(g==&source);record(ProbeAir,0,0,0,0,0,0);return air_result; }
void ftCommon_8007D5D4(Fighter*f) { CHECK(f==&fighter);f->ground_or_air=GA_Air;record(GroundToAir,0,0,0,0,0,0); }
void ftCommon_8007D7FC(Fighter*f) { CHECK(f==&fighter);f->ground_or_air=GA_Ground;record(AirToGround,0,0,0,0,0,0); }
void it_8026B3A8(Item_GObj*g) { GET_ITEM(g)->xDC8_word.flags.x15=0; }
void Item_80268E5C(Item_GObj*g,enum_t state,Item_StateChangeFlags flags) { CHECK(g==&object);item.msid=state;record(ItemState,(int)state,flags,0,0,0,0);if(invalidation==3)object.user_data=NULL;else if(invalidation==4)item.x1C++; }
void it_802725D4(Item_GObj*g) { CHECK(g==&object);record(DisableHits,0,0,0,0,0,0); }
bool it_8026DA08(Item_GObj*g) { CHECK(g==&object);record(ProbeGround,0,0,0,0,0,0);return false; }
bool it_8026D9A0(Item_GObj*g) { (void)g;CHECK(false);return false; }
void it_80272460(HitCapsule*h,u32 damage,Item_GObj*g) { CHECK(g==&object && h==&item.x5D4_hitboxes[0].hit);h->damage=damage; }
void it_80273598(Item_GObj*g,s32 a,s32 b) { CHECK(g==&object);record(Rumble,(int)a,(int)b,0,0,0,0); }
void Camera_RequestQuake(CmQuakeKind kind,Vec3*p) { record(Quake,kind,0,p->x,p->y,p->z,0); }
void db_80225DD8(Item_GObj*g,Fighter_GObj*owner) { CHECK(g==&object);record(Debug,owner==&source?1:owner==&foreign?2:0,0,0,0,0,0); }
void Item_802694CC(Item_GObj*g) { CHECK(g==&object);record(Advance,0,0,0,0,0,0); }
void ftLib_800865D8(Fighter_GObj*g,float*x,float*y) { CHECK(g==&source);*x=fighter.input.lstick[0].x;*y=fighter.input.lstick[0].y; }
Item_GObj* it_802C4580(Item_GObj*parent,HSD_GObj*secondary,Vec3*p,f32 facing,f32 charge) {
    CHECK(parent==&source && secondary==&object);record(ChildCreate,0,0,p->x,p->y,facing,charge);return &child;
}
bool it_8026B894(Item_GObj*g,Item_GObj*other) { CHECK(g==&object && other==&child);record(Interact,0,0,0,0,0,0);return false; }
void ftParts_80074A4C(Fighter_GObj*g,int group,int value) { CHECK(g==&source);record(Group,group,value,0,0,0,0); }
bool itIsHeavy(Item_GObj*g) { CHECK(g==&object);return held_heavy; }
Item_HoldKinds itGetHoldKind(Item_GObj*g) { CHECK(g==&object);return held_kind; }
void ftAnim_80070FB4(Fighter_GObj*g,s32 flag,s32 motion) { CHECK(g==&source);record(Pickup,(int)flag,(int)motion,0,0,0,0); }
void ftAnim_80070C48(Fighter_GObj*g,s32 flag) { CHECK(g==&source);record(Show,(int)flag,0,0,0,0,0); }
void ftAnim_80070CC4(Fighter_GObj*g,int flag) { CHECK(g==&source);record(Hide,flag,0,0,0,0,0); }
void ftAnim_800704F0(Fighter_GObj*g,int part,float frame) { CHECK(g==&source);record(KnockEnter,part,0,frame,0,0,0); }

#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
static unsigned fighter_field(const char*name) { for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(definition->state_fields[i].name,name))return definition->state_fields[i].offset;CHECK(false);return 0; }
static unsigned article_field(const NativeFighterArticleLogic*l,const char*name) { for(unsigned i=0;i<l->state_field_count;++i)if(!strcmp(l->state_fields[i].name,name))return l->state_fields[i].offset;CHECK(false);return 0; }
static void put_int(unsigned offset,int32_t v) { memcpy(fighter.native_module_state+offset,&v,4); }
static int32_t get_int(unsigned offset) { int32_t v;memcpy(&v,fighter.native_module_state+offset,4);return v; }
static void put_float(const NativeFighterArticleLogic*l,const char*name,float v) { memcpy(item.xDD4_itemVar.native_article.state+article_field(l,name),&v,4); }
static float get_float(const NativeFighterArticleLogic*l,const char*name) { float v;memcpy(&v,item.xDD4_itemVar.native_article.state+article_field(l,name),4);return v; }
static void put_bool(const NativeFighterArticleLogic*l,const char*name,bool v) { item.xDD4_itemVar.native_article.state[article_field(l,name)]=v; }
static bool get_bool(const NativeFighterArticleLogic*l,const char*name) { return item.xDD4_itemVar.native_article.state[article_field(l,name)]!=0; }
static uint32_t hash(const char*s) { uint32_t h=2166136261u;while(*s)h=(h^(uint8_t)*s++)*16777619u;return h; }
static const NativeFighterCallbackExport* exported(uint32_t h) { for(unsigned i=0;i<definition->callback_export_count;++i)if(definition->callback_exports[i].name_hash==h)return &definition->callback_exports[i];CHECK(false);return NULL; }
static bool target_bool(uint32_t id,uint32_t h) { CHECK(id==(uint32_t)fighter.x8_spawnNum);return exported(h)->bool_callback(&source); }
static HSD_GObj* creator(Item_GObj*g) {
    CHECK(g==&object);HSD_GObj*c=item.xDD4_itemVar.native_article.attachment_owner;
    return nativeCallbackSource_Alive(c,item.xDD4_itemVar.native_article.creator_spawn_id,NativeCallbackSource_Fighter)?c:NULL;
}
static uint32_t owner_id(Item_GObj*g) { CHECK(g==&object);uint32_t id;return nativeCallbackSource_Snapshot(item.owner,NativeCallbackSource_Fighter,&id)?id:0; }
static bool signal_article(HSD_GObj*g,uint32_t id,uint32_t name,float value) { CHECK(g==&source);if(id!=item.x1C||!nativeCallbackSource_Alive(&object,id,NativeCallbackSource_Item))return false;for(unsigned i=0;i<din_logic->signal_count;++i)if(din_logic->signals[i].name_hash==name){din_logic->signals[i].callback(&object,value);return true;}return false; }
static int32_t create_child(Item_GObj*g,ItemKind kind,Vec3*p,float facing,uint32_t owner,uint32_t group,float value,unsigned backlink) { CHECK(kind==It_Kind_Zelda_DinFire_Explode&&owner==(uint32_t)fighter.x8_spawnNum&&group==0&&backlink==0);it_802C4580(&source,g,p,facing,value);return (int32_t)UINT32_C(0xFABCDE01); }
static Fighter_Part resolve_bone(Fighter*f,Fighter_Part part) { CHECK(f==&fighter&&part==FtPart_TransN);return 1; }
static void item_scale(Item_GObj*g,float value) { CHECK(g==&object);Vec3 scale={value,value,value};HSD_JObjSetScale(&joint,&scale); }
static void item_joint(Item_GObj*g,unsigned model,const char*path,unsigned channel,float v) { CHECK(g==&object&&model==0);if(!strcmp(path,"0")){CHECK(channel==10);if(v)HSD_JObjClearFlagsAll(&child_joint,JOBJ_HIDDEN);else HSD_JObjSetFlagsAll(&child_joint,JOBJ_HIDDEN);}else{CHECK(!*path&&channel==4);HSD_JObjSetRotationY(&joint,v);} }
static void copy_attrs(Fighter_GObj*g) { CHECK(g==&source);record(CopyAttrs,0,0,0,0,0,0); }
static void reload_attrs(Fighter_GObj*g) { CHECK(g==&source);record(ReloadAttrs,0,0,0,0,0,0); }
static void initialize(bool generated_path) {
    scripted=generated_path;memset(&fighter,0,sizeof fighter);memset(&item,0,sizeof item);memset(&joint,0,sizeof joint);memset(&child_joint,0,sizeof child_joint);memset(&sibling_joint,0,sizeof sibling_joint);
    memset(&source,0,sizeof source);memset(&foreign,0,sizeof foreign);memset(&object,0,sizeof object);memset(heads,0,sizeof heads);
    source.user_data=&fighter;source.classifier=HSD_GOBJ_CLASS_FIGHTER;source.next=&foreign;foreign.user_data=&other_fighter;foreign.classifier=HSD_GOBJ_CLASS_FIGHTER;
    object.user_data=&item;object.hsd_obj=&joint;object.classifier=HSD_GOBJ_CLASS_ITEM;heads[HSD_GOBJ_PLINK_FIGHTER]=&source;heads[HSD_GOBJ_PLINK_ITEM]=&object;
    fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);other_fighter.x8_spawnNum=UINT32_C(0x89ABCDEF);fighter.kind=Ft_Kind_Zelda;fighter.x5AC.model_num=2;fighter.dat_attrs=&zd_attrs;fighter.parts=bones;
    for(unsigned i=0;i<4;++i)bones[i].joint=&joint;
    fighter.facing_dir=1;fighter.motion_id=341;fighter.cur_anim_frame=17.5f;fighter.co_attrs.terminal_velocity=2.0f;fighter.co_attrs.air_drift_max=1;fighter.co_attrs.aerial_friction=.05f;
    common.aerial_friction_oob=.08f;item_common.x4C_float=.5f;item.entity=&object;item.x1C=UINT32_C(0x80000001);item.owner=&source;item.facing_dir=1;item.xC4_article_data=&article;
    article.x4_specialAttributes=din_attrs;joint.child=&child_joint;child_joint.flags=JOBJ_HIDDEN;child_joint.next=&sibling_joint;sibling_joint.flags=JOBJ_HIDDEN;item.xDC8_word.flags.x15=1;item.xD44_lifeTimer=20;item.xD48_halfLifeTimer=10;
    remaining=true;ground_result=true;air_result=false;event_count=0;invalidation=0;held_heavy=false;held_kind=ITEM_HOLD_0;
    if(scripted) { item.xDD4_itemVar.native_article.attachment_owner=&source;item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;put_int(fighter_field("din_handle"),(int32_t)item.x1C); }
    else { fighter.u.zd.x222C=&object;item.xDD4_itemVar.zeldadinfire.xDE0=&source; }
}
typedef struct Observation {
    int motion,command0,command1,command2,command3,air,timer,retained,item_motion;
    float vx,vy,vz,accel,charge,angle,base_angle,speed,life,half_life,base_radius,hit_radius,damage;
    Vec3 scale;float rotation;unsigned reflected,effect,owner_bound,manual_lifetime,owner_present,control_present,child_hidden,sibling_hidden,reflect_active,fighter_effect,pre_hitlag,post_hitlag,death_bound,damage_bound,accessory_bound;
} Observation;
static Observation expected;
static Observation observe(int domain) {
    Observation o={0};o.motion=fighter.motion_id;o.command0=fighter.cmd_vars[0];o.command1=fighter.cmd_vars[1];o.command2=fighter.cmd_vars[2];o.command3=fighter.cmd_vars[3];o.air=fighter.ground_or_air;o.item_motion=item.msid;
    o.timer=scripted?get_int(fighter_field("neutral_gravity_timer")):fighter.mv.zd.specialn.x0;
    o.retained=scripted?(get_int(fighter_field("din_handle"))!=-1):(fighter.u.zd.x222C!=NULL);
    o.vx=domain?item.x40_vel.x:fighter.self_vel.x;o.vy=domain?item.x40_vel.y:fighter.self_vel.y;o.vz=domain?item.x40_vel.z:fighter.self_vel.z;o.accel=fighter.x74_self_accel.x;
    if(domain==1){o.charge=scripted?get_float(din_logic,"charge"):item.xDD4_itemVar.zeldadinfire.xDD8;o.angle=scripted?get_float(din_logic,"angle"):item.xDD4_itemVar.zeldadinfire.xDE8;
      o.base_angle=scripted?get_float(din_logic,"base_angle"):item.xDD4_itemVar.zeldadinfire.xDEC;o.speed=scripted?get_float(din_logic,"speed"):item.xDD4_itemVar.zeldadinfire.xDF0;
      o.reflected=scripted?get_bool(din_logic,"reflected"):(item.xDD4_itemVar.zeldadinfire.xDDC!=0);o.effect=scripted?get_bool(din_logic,"effect_active"):(item.xDD4_itemVar.zeldadinfire.xDF4!=0);
      o.control_present=scripted?(item.xDD4_itemVar.native_article.attachment_owner!=NULL):(item.xDD4_itemVar.zeldadinfire.xDE0!=NULL);}
    if(domain==2){o.charge=scripted?get_float(explosion_logic,"charge"):item.xDD4_itemVar.zeldadinfireexplode.xDD4;o.base_radius=scripted?get_float(explosion_logic,"base_hitbox_radius"):item.xDD4_itemVar.zeldadinfireexplode.xDD8;
      o.effect=scripted?get_bool(explosion_logic,"effect_active"):(item.xDD4_itemVar.zeldadinfireexplode.xDE0!=0);}
    o.life=item.xD44_lifeTimer;o.half_life=item.xD48_halfLifeTimer;o.damage=item.x5D4_hitboxes[0].hit.damage;o.hit_radius=item.x5D4_hitboxes[0].hit.scale;o.scale=joint.scale;o.rotation=joint.rotate.y;
    o.owner_bound=item.xDC8_word.flags.x13;o.manual_lifetime=item.xDC8_word.flags.x15;o.owner_present=item.owner!=NULL;o.child_hidden=(child_joint.flags&JOBJ_HIDDEN)!=0;o.sibling_hidden=(sibling_joint.flags&JOBJ_HIDDEN)!=0;o.reflect_active=fighter.reflecting;o.fighter_effect=fighter.x2219_b0;o.pre_hitlag=fighter.pre_hitlag_cb!=NULL;o.post_hitlag=fighter.post_hitlag_cb!=NULL;o.death_bound=fighter.death2_cb!=NULL;o.damage_bound=fighter.take_dmg_cb!=NULL;o.accessory_bound=fighter.accessory4_cb!=NULL;
    return o;
}
static void save(int domain) { expected=observe(domain);expected_count=event_count;memcpy(expected_events,events,event_count*sizeof(*events)); }
static uint32_t float_bits(float value) { uint32_t bits;memcpy(&bits,&value,4);return bits; }
static void compare(int domain) {
    Observation actual=observe(domain);CHECK(event_count==expected_count);
    for(unsigned i=0;i<event_count;++i) {
        CHECK(events[i].op==expected_events[i].op&&events[i].a==expected_events[i].a&&events[i].b==expected_events[i].b);
        CHECK(float_bits(events[i].x)==float_bits(expected_events[i].x)&&float_bits(events[i].y)==float_bits(expected_events[i].y)&&float_bits(events[i].z)==float_bits(expected_events[i].z)&&float_bits(events[i].w)==float_bits(expected_events[i].w));
    }
#define MATCH(field) do { if(actual.field!=expected.field)fprintf(stderr,"domain%d field %s actual%.9g expected%.9g\n",domain,#field,(double)actual.field,(double)expected.field);CHECK(actual.field==expected.field); } while(0)
#define MATCH_FLOAT(field) do { if(float_bits(actual.field)!=float_bits(expected.field))fprintf(stderr,"domain%d field %s actual%.9g expected%.9g\n",domain,#field,(double)actual.field,(double)expected.field);CHECK(float_bits(actual.field)==float_bits(expected.field)); } while(0)
    MATCH(motion);MATCH(command0);MATCH(command1);MATCH(command2);MATCH(command3);MATCH(air);MATCH(timer);MATCH(retained);MATCH(item_motion);
    MATCH_FLOAT(vx);MATCH_FLOAT(vy);MATCH_FLOAT(vz);MATCH_FLOAT(accel);MATCH_FLOAT(charge);MATCH_FLOAT(angle);MATCH_FLOAT(base_angle);MATCH_FLOAT(speed);MATCH_FLOAT(life);MATCH_FLOAT(half_life);MATCH_FLOAT(base_radius);MATCH_FLOAT(hit_radius);MATCH_FLOAT(damage);
    MATCH_FLOAT(scale.x);MATCH_FLOAT(scale.y);MATCH_FLOAT(scale.z);MATCH_FLOAT(rotation);
    MATCH(reflected);MATCH(effect);MATCH(owner_bound);MATCH(manual_lifetime);MATCH(owner_present);MATCH(control_present);MATCH(child_hidden);MATCH(sibling_hidden);MATCH(reflect_active);MATCH(fighter_effect);MATCH(pre_hitlag);MATCH(post_hitlag);MATCH(death_bound);MATCH(damage_bound);MATCH(accessory_bound);
#undef MATCH
#undef MATCH_FLOAT
    ++pairs;
}
static void seed_din(float charge,float angle,float speed,bool reflected,bool effect,int owner,int motion,int retained,float stick,float life) {
    fighter.motion_id=motion;fighter.cmd_vars[1]=1;fighter.input.lstick[0].y=stick;item.owner=owner==0?NULL:owner==1?&source:&foreign;item.xD44_lifeTimer=life;
    if(scripted){put_int(fighter_field("din_handle"),retained?(int32_t)item.x1C:-1);put_float(din_logic,"charge",charge);put_float(din_logic,"angle",angle);put_float(din_logic,"base_angle",0);put_float(din_logic,"speed",speed);put_bool(din_logic,"reflected",reflected);put_bool(din_logic,"effect_active",effect);}
    else{fighter.u.zd.x222C=retained?&object:NULL;item.xDD4_itemVar.zeldadinfire.xDD8=charge;item.xDD4_itemVar.zeldadinfire.xDE8=angle;item.xDD4_itemVar.zeldadinfire.xDEC=0;item.xDD4_itemVar.zeldadinfire.xDF0=speed;item.xDD4_itemVar.zeldadinfire.xDDC=reflected;item.xDD4_itemVar.zeldadinfire.xDF4=effect;}
}
int main(void) {
#include "attributes.inc"
    CHECK(offsetof(ftZelda_DatAttrs,x84)==132);CHECK(zd_attrs.x4==4);CHECK(din_attrs[0]==65&&din_attrs[1]==60);
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/4,parameter_schema));
    NativeFighterHost host={0};
#include "hosts.inc"
    host.callback_source_alive=nativeCallbackSource_Alive;host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.animation_frames_remaining=ftAnim_IsFramesRemaining;
    host.enter_fall=ftCo_Fall_Enter;host.finish_ground_action=ft_8008A2BC;host.ground_friction=ft_80084F3C;host.create_reflector=ftColl_CreateReflectHit;host.update_reflector=ftColl_8007AEF8;
    host.ground_collision=ft_80082708;host.air_collision=ft_80081D0C;host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;host.fall=ftCommon_Fall;host.quick_air_decel=ftCommon_CalcSelfAccel_DeaccelQuickAir;
    host.resolve_bone=resolve_bone;host.spawn_effect=efSync_Spawn;host.pause_effects=pause_effects;host.resume_effects=resume_effects;host.destroy_effects=efLib_DestroyAll;
    host.item_creator=creator;host.item_current_owner=owner_id;host.target_call_bool=target_bool;host.signal_article=signal_article;host.item_create_child=create_child;
    host.sin_float=sinf;host.cos_float=cosf;host.abs_float=fabsf;host.item_set_lifetime=it_80275158;host.item_use_manual_lifetime=it_8026B3A8;host.item_expire_tick=it_802751D8;
    host.item_change_state_flags=Item_80268E5C;host.item_set_uniform_scale=item_scale;host.item_set_joint_value=item_joint;host.item_set_debug_owner=db_80225DD8;host.item_advance_animation=Item_802694CC;
    host.item_stage_collision_land=it_8026DA08;host.item_stage_collision=it_8026D9A0;host.item_disable_hitboxes=it_802725D4;host.item_set_hitbox_damage=it_80272460;host.item_rumble=it_80273598;host.camera_quake=Camera_RequestQuake;host.item_interact=it_8026B894;
    host.set_visibility_group=ftParts_80074A4C;host.fighter_item_pickup=Fighter_OnItemPickup;host.fighter_item_drop=Fighter_OnItemDrop;host.fighter_item_visible=Fighter_OnItemVisible;host.fighter_item_invisible=Fighter_OnItemInvisible;
    host.fighter_knockback_enter=Fighter_OnKnockbackEnter;host.fighter_knockback_exit=Fighter_OnKnockbackExit;host.copy_ext_attributes=copy_attrs;host.reload_ext_attributes=reload_attrs;
    MotionState motions[18]={0};NativeFighterDefinition base={.fighter_root="ftDataZelda",.source_kind=Ft_Kind_Zelda,.motion_state_first=341,.motion_state_count=18,.motion_states=motions};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->article_logic_count==2);din_logic=&definition->article_logics[0];explosion_logic=&definition->article_logics[1];
    CHECK(din_logic->on_created&&explosion_logic->on_created);
    /* Neutral complete entry, animation, physics, collision and accessory. */
    const float velocities[]={-3,-.03f,0,.04f,3};const int commands[]={0,1,2,3};const int timers[]={-1,0,1,4};
    for(unsigned air=0;air<2;++air)for(unsigned v=0;v<5;++v) {
        initialize(false);fighter.self_vel=(Vec3){velocities[v],-1.1f,.2f};if(air)ftZd_SpecialAirN_Enter(&source);else ftZd_SpecialN_Enter(&source);save(0);
        initialize(true);fighter.self_vel=(Vec3){velocities[v],-1.1f,.2f};if(air)definition->special_air_n(&source);else definition->special_n(&source);compare(0);
    }
    for(unsigned air=0;air<2;++air)for(unsigned c=0;c<4;++c)for(unsigned frames=0;frames<2;++frames) {
        initialize(false);remaining=frames;fighter.cmd_vars[0]=commands[c];if(air)ftZd_SpecialAirN_Anim(&source);else ftZd_SpecialN_Anim(&source);save(0);
        initialize(true);remaining=frames;fighter.cmd_vars[0]=commands[c];definition->motion_states[air].anim_cb(&source);compare(0);
    }
    for(unsigned air=0;air<2;++air)for(unsigned t=0;t<4;++t)for(unsigned v=0;v<5;++v) {
        initialize(false);fighter.mv.zd.specialn.x0=timers[t];fighter.self_vel=(Vec3){velocities[v],-1.97f,0};if(air)ftZd_SpecialAirN_Phys(&source);else ftZd_SpecialN_Phys(&source);save(0);
        initialize(true);put_int(fighter_field("neutral_gravity_timer"),timers[t]);fighter.self_vel=(Vec3){velocities[v],-1.97f,0};definition->motion_states[air].phys_cb(&source);compare(0);
    }
    for(unsigned air=0;air<2;++air)for(unsigned c=0;c<4;++c)for(unsigned hit=0;hit<2;++hit) {
        initialize(false);fighter.cmd_vars[0]=commands[c];ground_result=hit;air_result=hit;if(air)ftZd_SpecialAirN_Coll(&source);else ftZd_SpecialN_Coll(&source);save(0);
        initialize(true);fighter.cmd_vars[0]=commands[c];ground_result=hit;air_result=hit;definition->motion_states[air].coll_cb(&source);compare(0);
    }
    for(unsigned air=0;air<2;++air)for(unsigned effect=0;effect<2;++effect) {
        initialize(false);fighter.x2219_b0=effect;if(air)ftZd_SpecialAirN_Enter(&source);else ftZd_SpecialN_Enter(&source);event_count=0;if(air)ftZd_SpecialN_8013A8AC(&source);else ftZd_SpecialN_8013A830(&source);save(0);
        initialize(true);fighter.x2219_b0=effect;definition->special_n(&source);event_count=0;if(air){definition->special_air_n(&source);event_count=0;}fighter.accessory4_cb(&source);compare(0);
    }
    /* Initialized article phases: exact steering gates, charge, duplicate burst,
     * reflected/changed damage owner and timer ordering. */
    const float charges[]={0,1,59,60,70};const float sticks[]={-1,-.21f,0,.21f,1};const int motion_ids[]={344,345,347,348,14};
    for(unsigned mode=0;mode<3;++mode)for(unsigned r=0;r<2;++r)for(unsigned owner=0;owner<3;++owner)for(unsigned m=0;m<5;++m)for(unsigned n=0;n<5;++n) {
        float life=n==0?.5f:20;float angle=sticks[n];bool result;
        initialize(false);seed_din(charges[n],angle,.8f,r,true,owner,motion_ids[m],true,sticks[n],life);
        result=mode==0?itZeldadinfire_UnkMotion0_Anim(&object):mode==1?(itZeldadinfire_UnkMotion0_Phys(&object),false):itZeldadinfire_UnkMotion1_Anim(&object);save(1);
        initialize(true);seed_din(charges[n],angle,.8f,r,true,owner,motion_ids[m],true,sticks[n],life);
        ItemStateTable*state=&din_logic->logic->states[mode==2?1:0];bool observed=mode==1?(state->physics_updated(&object),false):state->animated(&object);CHECK(observed==result);compare(1);
    }
    for(unsigned mode=0;mode<3;++mode)for(unsigned r=0;r<2;++r)for(unsigned n=0;n<5;++n) {
        initialize(false);seed_din(charges[n],sticks[n],.8f,r,true,1,345,true,sticks[n],n==0?.5f:20);item.xDD4_itemVar.zeldadinfire.xDE0=NULL;
        bool result=mode==0?itZeldadinfire_UnkMotion0_Anim(&object):mode==1?(itZeldadinfire_UnkMotion0_Phys(&object),false):itZeldadinfire_UnkMotion1_Anim(&object);save(1);
        initialize(true);seed_din(charges[n],sticks[n],.8f,r,true,1,345,true,sticks[n],n==0?.5f:20);item.xDD4_itemVar.native_article.attachment_owner=NULL;
        ItemStateTable*state=&din_logic->logic->states[mode==2?1:0];bool observed=mode==1?(state->physics_updated(&object),false):state->animated(&object);CHECK(observed==result);compare(1);
    }
    const u32 masks[]={0,Collide_FloorMask,Collide_CeilingMask,Collide_LeftWallMask,Collide_RightWallMask,Collide_FloorMask|Collide_RightWallMask};
    for(unsigned x=0;x<3;++x)for(unsigned y=0;y<3;++y)for(unsigned m=0;m<6;++m) {
        initialize(false);seed_din(20,0,1,false,true,1,344,true,0,20);item.x40_vel=(Vec3){(float)x-1,(float)y-1,7};item.x378_itemColl.env_flags=masks[m];CHECK(!itZeldadinfire_UnkMotion0_Coll(&object));save(1);
        initialize(true);seed_din(20,0,1,false,true,1,344,true,0,20);item.x40_vel=(Vec3){(float)x-1,(float)y-1,7};item.x378_itemColl.env_flags=masks[m];CHECK(!din_logic->logic->states[0].collided(&object));compare(1);
    }
    for(unsigned facing=0;facing<2;++facing)for(unsigned n=0;n<5;++n) {
        initialize(false);seed_din(20,.2f,.7f,false,true,1,344,true,0,20);item.facing_dir=facing?1:-1;item.x40_vel=(Vec3){velocities[n],1,7};CHECK(!itZeldaDinFire_Logic65_Reflected(&object));save(1);
        initialize(true);seed_din(20,.2f,.7f,false,true,1,344,true,0,20);item.facing_dir=facing?1:-1;item.x40_vel=(Vec3){velocities[n],1,7};CHECK(!din_logic->logic->reflected(&object));compare(1);
    }
    for(unsigned c=0;c<5;++c)for(unsigned active=0;active<2;++active)for(unsigned radius=0;radius<2;++radius)for(unsigned expire=0;expire<2;++expire) {
        bool result;float charge=charges[c],base_radius=radius?2:0;
        initialize(false);memset(&item.xDD4_itemVar,0,sizeof item.xDD4_itemVar);article.x4_specialAttributes=explosion_attrs;item.xDD4_itemVar.zeldadinfireexplode.xDD4=charge;item.xDD4_itemVar.zeldadinfireexplode.xDD8=base_radius;item.x5D4_hitboxes[0].hit.state=active?HitCapsule_Enabled:HitCapsule_Disabled;item.x5D4_hitboxes[0].hit.scale=3;item.xD44_lifeTimer=expire?.5f:20;result=itZeldadinfireexplode_UnkMotion0_Anim(&object);save(2);
        initialize(true);article.x4_specialAttributes=explosion_attrs;put_float(explosion_logic,"charge",charge);put_float(explosion_logic,"base_hitbox_radius",base_radius);item.x5D4_hitboxes[0].hit.state=active?HitCapsule_Enabled:HitCapsule_Disabled;item.x5D4_hitboxes[0].hit.scale=3;item.xD44_lifeTimer=expire?.5f:20;CHECK(explosion_logic->logic->states[0].animated(&object)==result);compare(2);
    }
    /* Initializers run after controlled allocation/creator snapshots. Din's
     * constructor prefix is reproduced as its real pre-init writes/timer call.
     * Full model allocation and fighter hand-particle creation are not claimed. */
    for(unsigned facing=0;facing<2;++facing) {
        initialize(false);item.facing_dir=facing?1:-1;it_80275158(&object,din_attrs[0]);it_802C3D74(&object);save(1);
        initialize(true);item.facing_dir=facing?1:-1;din_logic->on_created(&object);compare(1);
    }
    const float explosion_charges[]={0,26.249f,26.25f,37.499f,37.5f,48.749f,48.75f,60};
    for(unsigned c=0;c<8;++c)for(unsigned active=0;active<2;++active) {
        float value=explosion_charges[c];initialize(false);memset(&item.xDD4_itemVar,0,sizeof item.xDD4_itemVar);article.x4_specialAttributes=explosion_attrs;item.xDD4_itemVar.zeldadinfireexplode.xDD4=value;item.x5D4_hitboxes[0].hit.state=active?HitCapsule_Enabled:HitCapsule_Disabled;item.x5D4_hitboxes[0].hit.scale=3;it_802C46C4(&object,&item);save(2);
        initialize(true);article.x4_specialAttributes=explosion_attrs;item.xDD4_itemVar.native_article.creation_value=value;item.x5D4_hitboxes[0].hit.state=active?HitCapsule_Enabled:HitCapsule_Disabled;item.x5D4_hitboxes[0].hit.scale=3;explosion_logic->on_created(&object);compare(2);
    }
    for(unsigned domain=1;domain<=2;++domain)for(unsigned owner=0;owner<3;++owner)for(unsigned effect=0;effect<2;++effect) {
        initialize(false);if(domain==1)seed_din(0,0,0,false,effect,owner,344,true,0,20);else{memset(&item.xDD4_itemVar,0,sizeof item.xDD4_itemVar);item.owner=owner==0?NULL:owner==1?&source:&foreign;item.xDD4_itemVar.zeldadinfireexplode.xDE0=effect;}if(domain==1)itZeldaDinFire_Logic65_Destroyed(&object);else itZeldaDinFireExplode_Logic66_Destroyed(&object);save(domain);
        initialize(true);if(domain==1)seed_din(0,0,0,false,effect,owner,344,true,0,20);else{item.owner=owner==0?NULL:owner==1?&source:&foreign;put_bool(explosion_logic,"effect_active",effect);}if(domain==1)din_logic->logic->destroyed(&object);else explosion_logic->logic->destroyed(&object);compare(domain);
    }
    /* Secondary article phases/callbacks and detach/cleanup use the same real
     * callback tables. There is no constructor or projectile tick surrogate. */
    for(unsigned domain=1;domain<=2;++domain)for(unsigned phase=0;phase<2;++phase)for(unsigned v=0;v<5;++v) {
        initialize(false);if(domain==2)memset(&item.xDD4_itemVar,0,sizeof item.xDD4_itemVar);item.x40_vel=(Vec3){velocities[v],2,3};
        bool old_result=false;if(phase==0){if(domain==1)itZeldadinfire_UnkMotion1_Phys(&object);else itZeldadinfireexplode_UnkMotion0_Phys(&object);}else old_result=domain==1?itZeldadinfire_UnkMotion1_Coll(&object):itZeldadinfireexplode_UnkMotion0_Coll(&object);save(domain);
        initialize(true);item.x40_vel=(Vec3){velocities[v],2,3};ItemStateTable*state=&(domain==1?din_logic:explosion_logic)->logic->states[domain==1?1:0];
        if(phase==0)state->physics_updated(&object);else CHECK(state->collided(&object)==old_result);compare(domain);
    }
    for(unsigned owner=0;owner<3;++owner)for(unsigned retained=0;retained<2;++retained) {
        initialize(false);seed_din(20,.2f,.7f,false,true,owner,344,retained,0,20);fighter.death2_cb=ftZd_SpecialLw_8013B5EC;fighter.take_dmg_cb=ftZd_SpecialLw_8013B5EC;it_802C3D44(&object);save(1);
        initialize(true);seed_din(20,.2f,.7f,false,true,owner,344,retained,0,20);fighter.death2_cb=script_fn_cleanup_din;fighter.take_dmg_cb=script_fn_cleanup_din;signal_article(&source,item.x1C,hash("detach_control"),0);compare(1);
        initialize(false);seed_din(20,.2f,.7f,false,true,owner,344,retained,0,20);fighter.death2_cb=ftZd_SpecialLw_8013B5EC;fighter.take_dmg_cb=ftZd_SpecialLw_8013B5EC;ftZd_SpecialLw_8013B5EC(&source);save(1);
        initialize(true);seed_din(20,.2f,.7f,false,true,owner,344,retained,0,20);fighter.death2_cb=script_fn_cleanup_din;fighter.take_dmg_cb=script_fn_cleanup_din;script_fn_cleanup_din(&source);compare(1);
    }
    for(unsigned domain=1;domain<=2;++domain) {
        initialize(false);bool old_clank=domain==1?itZeldaDinFire_Logic65_Clanked(&object):itZeldaDinFireExplode_Logic66_Clanked(&object);
        bool old_absorb=domain==1?itZeldaDinFire_Logic65_Absorbed(&object):itZeldaDinFireExplode_Logic66_Absorbed(&object);
        if(domain==1)itZeldaDinFire_Logic65_EvtUnk(&object,&child);else{itZeldaDinFireExplode_Logic66_EvtUnk(&object,&child);CHECK(itZeldaDinFireExplode_Logic66_ShieldBounced(&object));CHECK(itZeldaDinFireExplode_Logic66_HitShield(&object));memset(&item.xDD4_itemVar,0,sizeof item.xDD4_itemVar);}save(domain);
        initialize(true);const ItemLogicTable*l=(domain==1?din_logic:explosion_logic)->logic;CHECK(l->clanked(&object)==old_clank);CHECK(l->absorbed(&object)==old_absorb);l->evt_unk(&object,&child);if(domain==2){CHECK(l->shield_bounced(&object));CHECK(l->hit_shield(&object));}compare(domain);
    }
    /* Actual stock generic held-item/knockback callbacks and death reset. */
    for(unsigned call=0;call<7;++call)for(unsigned flag=0;flag<2;++flag)for(unsigned heavy=0;heavy<2;++heavy)for(unsigned kind=0;kind<3;++kind) {
        initialize(false);fighter.item_gobj=&object;held_heavy=heavy;held_kind=(Item_HoldKinds)kind;
        switch(call){case 0:ftZd_Init_OnItemPickup(&source,flag);break;case 1:ftZd_Init_OnItemDrop(&source,flag);break;case 2:ftZd_Init_OnItemVisible(&source);break;case 3:ftZd_Init_OnItemInvisible(&source);break;case 4:ftZd_Init_OnKnockbackEnter(&source);break;case 5:ftZd_Init_OnKnockbackExit(&source);break;case 6:ftZd_Init_OnDeath(&source);break;}save(0);
        initialize(true);fighter.item_gobj=&object;held_heavy=heavy;held_kind=(Item_HoldKinds)kind;
        switch(call){case 0:definition->on_item_pickup(&source,flag);break;case 1:definition->on_item_drop(&source,flag);break;case 2:definition->on_item_visible(&source);break;case 3:definition->on_item_invisible(&source);break;case 4:definition->on_knockback_enter(&source);break;case 5:definition->on_knockback_exit(&source);break;case 6:definition->on_death(&source);break;}compare(0);
    }
    for(unsigned air=0;air<2;++air) {
        initialize(false);if(air)ftZd_SpecialAirN_IASA(&source);else ftZd_SpecialN_IASA(&source);save(0);
        initialize(true);definition->motion_states[air].input_cb(&source);compare(0);
    }
    initialize(false);fighter.cmd_vars[0]=1;ftZd_SpecialN_Anim(&source);event_count=0;fighter.reflect_hit_cb(&source);save(0);initialize(true);fighter.cmd_vars[0]=1;definition->motion_states[0].anim_cb(&source);event_count=0;fighter.reflect_hit_cb(&source);compare(0);
    /* Owned callback controls stop the generated initializer/entry tail on
     * source retirement/recycling; do not run the old body through stale data. */
    for(int change=1;change<=2;++change) { initialize(true);invalidation=change;definition->special_n(&source);CHECK(event_count==1&&events[0].op==Motion);++safety; }
    for(int change=3;change<=4;++change)for(unsigned domain=1;domain<=2;++domain) { initialize(true);invalidation=change;(domain==1?din_logic:explosion_logic)->on_created(&object);CHECK(event_count==1&&events[0].op==ItemState);++safety; }
    printf("PASS %u original/generated pairs, %u checks, %u authored safety cases\n",pairs,checks,safety);return 0;
}
