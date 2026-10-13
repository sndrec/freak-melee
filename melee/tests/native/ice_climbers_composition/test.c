/* Actual complete original bodies and generated callbacks. Model/motion/stage
 * kernels and allocation are explicit boundaries; no stock callback forwarding. */
#include <math.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/db/db.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftPopo/ftpopo.h>
#include <melee/ft/kinds/ftPopo/ftpopospecialn.h>
#include <melee/ft/kinds/ftPopo/ftpopospeciallw.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/it/inlines.h>
#include <melee/it/item.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itcoll.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itclimbersice.h>
#include <melee/it/kinds/itclimbersblizzard.h>
#include <melee/it/kinds/types.h>
#include <melee/it/native_article.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbvector.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/random.h>

static unsigned checks, pairs, safety;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"pair%u safety%u line%d %s\n",pairs,safety,__LINE__,#x); exit(1); } } while(0)
typedef struct Trace { int op,a,b; float x,y,z; } Trace;
enum { Change, Start, GroundPhysics, AirPhysics, GroundProbe, AirProbe,
       ToAir, ToGround, Clamp, Particle, Destroy, ItemChange, Debug, Refresh,
       Allocate, JointPosition, Sound, Remove, Follow, FloorAlign, BounceGround,
       StageSweep, ReflectShield, BounceVictim, Spin, Interact, Visibility };
static Trace trace[96], expected_trace[96];
static unsigned trace_count, expected_count;
static void event(int op,int a,int b,float x,float y,float z) {
    CHECK(trace_count<96); trace[trace_count++]=(Trace){op,a,b,x,y,z};
}
static void unexpected(void) { CHECK(!"unexpected Host boundary"); }
static HSD_GObj source, foreign, object, *heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static Fighter fighter, foreign_fighter;
static Item item;
static HSD_JObj joints[49], article_root, article_child;
static FighterBone bones[49];
static ftIceClimberAttributes fighter_attrs;
static itClimbersIceAttributes ice_attrs;
static itClimbersBlizzardAttributes blizzard_attrs;
static ItemAttr item_attrs;
static Article article;
static ItemCommonData item_common;
ItemCommonData* it_804D6D28=&item_common;
static bool scripted, remaining, ground, landed, allocation_success, stage_callback;
static unsigned domain, invalidation;
static float random_value;
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic *ice_logic, *blizzard_logic, *active_logic;
static const NativeFighterDefinition* active_definition;
#define GET_ATTRS(ip) ((itClimbersBlizzardAttributes*)(ip)->xC4_article_data->x4_specialAttributes)
#define M_TAU 6.283185307179586

bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*n) { (void)g;*n=0;return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj*g,uint32_t n) { (void)g;(void)n;return NULL; }
void __assert(char*f,u32 line,char*m) { fprintf(stderr,"%s:%lu %s\n",f,line,m);exit(1); }
void HSD_JObjSetMtxDirtySub(HSD_JObj*j) { j->flags|=JOBJ_MTX_DIRTY; }
static unsigned field(const NativeFighterStateField*fields,unsigned count,const char*name) {
    for(unsigned i=0;i<count;++i) if(!strcmp(fields[i].name,name))return fields[i].offset;
    CHECK(false);return 0;
}
static unsigned ff(const char*n) { return field(definition->state_fields,definition->state_field_count,n); }
static unsigned af(const char*n) { return field(active_logic->state_fields,active_logic->state_field_count,n); }
static int32_t read_int(const void*p,unsigned at) { int32_t v;memcpy(&v,(const char*)p+at,4);return v; }
static float read_float(const void*p,unsigned at) { float v;memcpy(&v,(const char*)p+at,4);return v; }
static void put_int(void*p,unsigned at,int32_t v) { memcpy((char*)p+at,&v,4); }
static void put_float(void*p,unsigned at,float v) { memcpy((char*)p+at,&v,4); }
static void fi(const char*n,int32_t v) { put_int(fighter.native_module_state,ff(n),v); }
static void fb(const char*n,bool v) { fighter.native_module_state[ff(n)]=v; }
static void fl(const char*n,float v) { put_float(fighter.native_module_state,ff(n),v); }
static void ai(const char*n,int32_t v) { put_int(item.xDD4_itemVar.native_article.state,af(n),v); }
static void ab(const char*n,bool v) { item.xDD4_itemVar.native_article.state[af(n)]=v; }
static void al(const char*n,float v) { put_float(item.xDD4_itemVar.native_article.state,af(n),v); }

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind) {
    return kind==Ft_Kind_CustomFirst?active_definition:NULL;
}
Article* nativeFighterRuntime_ArticleAssets(FighterKind kind,ItemKind article_kind) {
    CHECK(kind==Ft_Kind_CustomFirst);CHECK(article_kind==It_Kind_IceClimber_Ice||article_kind==It_Kind_IceClimber_Blizzard);return &article;
}
uint32_t nativeFighterTarget_Id(Fighter_GObj*g) { return g&&g->user_data?(uint32_t)((Fighter*)g->user_data)->x8_spawnNum:0; }
static Fighter_GObj* creator(Item_GObj*g) {
    CHECK(g==&object);
    if(source.user_data==&fighter&&item.xDD4_itemVar.native_article.attachment_owner==&source&&
       item.xDD4_itemVar.native_article.creator_spawn_id==fighter.x8_spawnNum)return &source;
    return NULL;
}
static uint32_t owner_id(Item_GObj*g) { CHECK(g==&object);return nativeFighterTarget_Id(item.owner); }
static void invalidate(HSD_GObj*g,unsigned point) {
    if(invalidation==point)g->user_data=NULL;
    else if(invalidation==point+10) { if(g==&source)fighter.x8_spawnNum++;else item.x1C++; }
}
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId id,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj*donor) {
    CHECK(g==&source&&donor==NULL);event(Change,id,(int)flags,frame,rate,blend);fighter.motion_id=id;fighter.cur_anim_frame=frame;invalidate(g,1);
}
void ftAnim_8006EBA4(Fighter_GObj*g) { CHECK(g==&source);event(Start,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj*g) { CHECK(g==&source);return remaining; }
void ft_8008A2BC(Fighter_GObj*g) { CHECK(g==&source);fighter.motion_id=14;event(Change,14,0,0,0,0); }
void ftCo_Fall_Enter(Fighter_GObj*g) { CHECK(g==&source);fighter.motion_id=29;event(Change,29,0,0,0,0); }
void ft_80084F3C(Fighter_GObj*g) { CHECK(g==&source);event(GroundPhysics,0,0,0,0,0);fighter.self_vel.x*=.8f; }
void ft_80084EEC(Fighter_GObj*g) { CHECK(g==&source);event(AirPhysics,0,0,0,0,0);fighter.self_vel.y-=.1f; }
bool ft_80082708(Fighter_GObj*g) { CHECK(g==&source);event(GroundProbe,0,0,0,0,0);return ground; }
GroundOrAir ft_80081D0C(Fighter_GObj*g) { CHECK(g==&source);event(AirProbe,0,0,0,0,0);return landed?GA_Air:GA_Ground; }
void ft_80082C74(Fighter_GObj*g,HSD_GObjEvent cb) { CHECK(g==&source);event(AirProbe,0,0,0,0,0);if(landed)cb(g); }
void ftCommon_8007D5D4(Fighter*f) { CHECK(f==&fighter);event(ToAir,0,0,0,0,0);f->ground_or_air=GA_Air; }
void ftCommon_8007D7FC(Fighter*f) { CHECK(f==&fighter);event(ToGround,0,0,0,0,0);f->ground_or_air=GA_Ground; }
void ftCommon_ClampAirDrift(Fighter*f) { CHECK(f==&fighter);event(Clamp,0,0,0,0,0);f->self_vel.x=fmaxf(-2,fminf(2,f->self_vel.x)); }
void ftCo_LandingFallSpecial_Enter(Fighter_GObj*g,bool b,float lag) { CHECK(g==&source);event(Change,43,b,lag,0,0);fighter.motion_id=43; }
void ftPartSetRotX(Fighter*f,int index,float angle) { CHECK(f==&fighter&&index==0);HSD_JObjSetRotationX(f->parts[index].joint,angle); }
void lb_8000B1CC(HSD_JObj*j,Vec3*offset,Vec3*out) {
    CHECK(offset==NULL);unsigned index=(unsigned)(j-joints);CHECK(index==0||index==26);
    *out=j->translate;event(JointPosition,index,0,out->x,out->y,out->z);
}
void ft_PlaySFX(Fighter*f,enum_t id,u8 volume,u8 pan) { CHECK(f==&fighter);event(Sound,(int)id,0,volume,pan,0); }
void ft_800881D8(Fighter*f,int id,u8 volume,u8 pan) { CHECK(f==&fighter);event(Sound,id,2,volume,pan,0); }
void ft_80088510(Fighter*f,int id,u8 volume,u8 pan) { CHECK(f==&fighter);event(Sound,id,3,volume,pan,0); }
void efLib_DestroyAll(HSD_GObj*g) { CHECK(g==&source||g==&object);event(Destroy,g==&source?0:1,0,0,0,0); }
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj*g,...) {
    CHECK(g==&source);va_list va;va_start(va,g);HSD_JObj*j=va_arg(va,HSD_JObj*);va_end(va);CHECK(j==&joints[29]);event(Particle,(int)id,0,29,0,0);return (EF_SpawnResult){0};
}
void efAsync_Spawn(HSD_GObj*g,void*queue,u32 mode,u32 id,HSD_JObj*j,...) {
    CHECK(g==&object&&queue==&item.xBC0&&j==&article_child);float facing=0;
    if(mode==3){va_list va;va_start(va,j);float*direction=va_arg(va,float*);CHECK(direction==&item.facing_dir);facing=*direction;va_end(va);}else CHECK(mode==0);
    event(Particle,(int)id,(int)mode,facing,0,0);
}
static void async_joint(Item_GObj*g,int effect,const char*path,unsigned mode) {
    CHECK(!strcmp(path,"0"));if(mode==3)efAsync_Spawn(g,&item.xBC0,3,effect,&article_child,&item.facing_dir);else efAsync_Spawn(g,&item.xBC0,0,effect,&article_child);
}
static void spawn_part(Fighter_GObj*g,s32 effect,s32 part) { CHECK(part==29);efSync_Spawn(effect,g,fighter.parts[part].joint); }
void Item_80268E5C(HSD_GObj*g,enum_t state,Item_StateChangeFlags flags) {
    CHECK(g==&object);event(ItemChange,(int)state,flags,0,0,0);item.msid=state;invalidate(g,2);
}
void db_80225DD8(Item_GObj*g,Fighter_GObj*owner) { CHECK(g==&object);event(Debug,owner==&source?1:owner==&foreign?2:0,0,0,0,0); }
void it_8026B3A8(Item_GObj*g) { CHECK(g==&object);item.xDC8_word.flags.x15=0; }
void it_8026BD6C(HSD_GObj*g) { CHECK(g==&object);item.xDCD_flag.b3=true; }
void it_8026BD84(HSD_GObj*g) { CHECK(g==&object);item.xDCD_flag.b4=true; }
void it_802750F8(Item_GObj*g) { CHECK(g==&object);event(Refresh,0,0,0,0,0);invalidate(g,3); }
void it_8026BB68(HSD_GObj*g,Vec3*p) { CHECK(g==&source);*p=(Vec3){11,17,.25f}; }
Item_GObj* Item_80268B18(SpawnItem*s) {
    event(Allocate,s->kind,(int)s->x40,s->prev_pos.x,s->prev_pos.y,s->prev_pos.z);
    CHECK(s->x0_parent_gobj==&source&&s->x4_parent_gobj2==&source&&s->x44_flag.b0&&s->x3C_damage==0);
    CHECK(s->pos.x==11&&s->pos.y==17&&s->pos.z==.25f&&s->vel.x==0&&s->vel.y==0&&s->vel.z==0);
    if(!allocation_success)return NULL;item.kind=s->kind;item.owner=&source;item.pos=s->pos;item.facing_dir=s->facing_dir;return &object;
}
Item_GObj* Item_CreateAuthored(SpawnItem*s,FighterKind author) {
    CHECK(author==Ft_Kind_CustomFirst);Item_GObj*g=Item_80268B18(s);if(g)item.native_article_author_kind=author;return g;
}
void Item_8026A8EC(Item_GObj*g) {
    CHECK(g==&object);event(Remove,0,0,0,0,0);
    if(scripted)active_logic->logic->destroyed(g);else it_2725_Logic90_Destroyed(g);
    object.user_data=NULL;heads[HSD_GOBJ_PLINK_ITEM]=NULL;
}
void ftPp_Init_8011F060(HSD_GObj*g) { (void)g;CHECK(!"full Squall/Belay cleanup is outside this component"); }
void ftKb_SpecialNIc_80108CE8(HSD_GObj*g,Item_GObj*i) { (void)g;(void)i;CHECK(!"Kirby excluded"); }
void ftParts_80074A4C(Fighter_GObj*g,int group,int value) { CHECK(g==&source);event(Visibility,group,value,0,0,0); }
void it_8026D62C(HSD_GObj*g,HSD_GObjEvent cb) { CHECK(g==&object);event(Follow,0,0,0,0,0);if(stage_callback)cb(g); }
void it_8026E15C(Item_GObj*g,HSD_GObjEvent cb) { CHECK(g==&object);event(BounceGround,0,0,0,0,0);if(stage_callback)cb(g); }
void it_80276CB8(Item_GObj*g) { CHECK(g==&object);event(FloorAlign,0,0,0,0,0); }
bool it_8026D9A0(Item_GObj*g) { CHECK(g==&object);event(StageSweep,0,0,0,0,0);return false; }
s32 it_8026DAA8(Item_GObj*g) { CHECK(g==&object);event(StageSweep,1,0,0,0,0);return (item.x378_itemColl.env_flags&Collide_FloorMask?1:0)|it_80276308(g)|it_802763E0(g); }
void mpCollSetFacingDir(CollData*c,int direction) { CHECK(c==&item.x378_itemColl);c->facing_dir=direction; }
void lbVector_Mirror(Vec3*v,Vec3*n) { float dot=v->x*n->x+v->y*n->y+v->z*n->z;v->x-=2*dot*n->x;v->y-=2*dot*n->y;v->z-=2*dot*n->z; }
int it_80274658(Item_GObj*g,f32 scale) { CHECK(g==&object);event(Spin,0,0,scale,0,0);return 0; }
bool itColl_BounceOffShield(Item_GObj*g) { CHECK(g==&object);event(ReflectShield,0,0,0,0,0);return false; }
void itColl_BounceOffVictim(Item_GObj*g) { CHECK(g==&object);event(BounceVictim,0,0,0,0,0);item.x40_vel.x=-item.x40_vel.x; }
bool it_8026B894(Item_GObj*g,Item_GObj*other) { CHECK(g==&object);event(Interact,other==&source?1:other==&foreign?2:0,0,0,0,0);return false; }
void it_80272460(HitCapsule*h,u32 damage,Item_GObj*g) { CHECK(g==&object&&h==&item.x5D4_hitboxes[0].hit);h->damage=damage; }
float HSD_Randf(void) { event(Allocate,-1,0,random_value,0,0);return random_value; }
static bool expire(Item_GObj*g) { return Item_TickLifetime(GET_ITEM(g)); }
static void gravity(Item_GObj*g) { Item_ApplyFallingPhysics(g); }
static unsigned bone_count(Fighter*f) { CHECK(f==&fighter);return 49; }
static void joint_write(Item_GObj*g,unsigned model,const char*path,unsigned channel,float value) {
    CHECK(g==&object&&model==0&&!strcmp(path,"0")&&channel>=6&&channel<=8);(&article_child.scale.x)[channel-6]=value;HSD_JObjSetMtxDirtySub(&article_child);
}
static float joint_read(Item_GObj*g,unsigned model,const char*path,unsigned channel) { CHECK(g==&object&&model==0&&!strcmp(path,"0"));return channel==9?1:0; }

void ftPp_SpecialN_8011F500(Fighter_GObj*);
#include "actual.inc"
#include "shared.inc"
#include "generated.inc"
#include "parameters.inc"
#include "fighter_cases.inc"
#include "article_cases.inc"

int main(void) {
    NativeFighterHost host={0};
#include "hosts.inc"
    static NativeFighterSoundChannel channels[7];
    for(unsigned i=0;i<7;++i)channels[i]=(NativeFighterSoundChannel)unexpected;
    channels[2]=ft_800881D8;channels[3]=ft_80088510;
    host.sound_channels=channels;host.callback_source_alive=nativeCallbackSource_Alive;
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/sizeof(*parameter_bits),parameter_schema));
    host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.animation_frames_remaining=ftAnim_IsFramesRemaining;
    host.finish_ground_action=ft_8008A2BC;host.enter_fall=ftCo_Fall_Enter;host.enter_landing_fall_special=ftCo_LandingFallSpecial_Enter;
    host.ground_friction=ft_80084F3C;host.air_gravity_and_friction=ft_80084EEC;host.ground_collision=ft_80082708;host.air_collision=ft_80081D0C;
    host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;host.clamp_air_drift=ftCommon_ClampAirDrift;host.air_aerial_landing_collision=ft_80082C74;
    host.bone_count=bone_count;host.joint_position=lb_8000B1CC;host.set_root_rotation_x=ftPartSetRotX;host.angle_atan2_float=atan2f;
    host.create_article_at=nativeArticle_Create;host.signal_article=signal;host.spawn_effect_on_part=spawn_part;host.play_sound=ft_PlaySFX;host.loop_sound=ft_80088510;
    host.destroy_effects=efLib_DestroyAll;host.set_visibility_group=ftParts_80074A4C;
    host.random_float=HSD_Randf;host.sin_float=sinf;host.cos_float=cosf;host.abs_float=fabsf;
    host.item_creator=creator;host.item_current_owner=owner_id;host.item_change_state_flags=Item_80268E5C;host.item_set_lifetime=it_80275158;
    host.item_use_manual_lifetime=it_8026B3A8;host.item_refresh_physics=it_802750F8;host.item_set_debug_owner=db_80225DD8;
    host.item_spawn_async_joint_effect=async_joint;host.item_remove=Item_8026A8EC;host.item_align_facing=it_80272980;host.item_gravity=gravity;host.item_fall=it_80272860;
    host.item_follow_ground=it_8026D62C;host.item_bounce_stage_to_ground=it_8026E15C;host.item_align_to_floor=it_80276CB8;
    host.item_wall_contact_flags=it_80276308;host.item_ceiling_contact_flags=it_802763E0;host.item_reflect_from_wall=it_8027770C;host.item_sweep_stage_flags=it_8026DAA8;
    host.item_stage_collision=it_8026D9A0;host.item_bounce=itColl_BounceOffVictim;host.item_reflect=it_80273030;host.item_shield_bounce=itColl_BounceOffShield;
    host.item_set_hitbox_damage=it_80272460;host.item_set_joint_value=joint_write;host.item_joint_value=joint_read;host.item_interact=it_8026B894;
    static MotionState motions[26];NativeFighterDefinition base={.fighter_root=FORM_NANA?"ftDataNana":"ftDataPopo",.source_kind=FORM_NANA?Ft_Kind_Nana:Ft_Kind_Popo,.motion_state_first=341,.motion_state_count=26,.motion_states=motions};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->article_logic_count==2);active_definition=definition;
    ice_logic=&definition->article_logics[0];blizzard_logic=&definition->article_logics[1];CHECK(ice_logic->kind==It_Kind_IceClimber_Ice&&blizzard_logic->kind==It_Kind_IceClimber_Blizzard);
    memset(&fighter_attrs,0,sizeof(fighter_attrs));memset(&ice_attrs,0,sizeof(ice_attrs));memset(&blizzard_attrs,0,sizeof(blizzard_attrs));
#include "attributes.inc"
    fighter_cases();article_cases();
    printf("PASS %u original/generated pairs, %u checks, %u authored safety cases\n",pairs,checks,safety);
    return 0;
}
