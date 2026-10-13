/* Actual original Chain helpers against the actual generated assembled module.
 * This unit owns policy observations; constructor, pose and stage boundaries
 * are covered separately. No original body is rewritten to form an expected. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftNess/ftnessattackhi4.h>
#include <melee/ft/kinds/ftSeak/ftseak.h>
#include <melee/it/kinds/itseakchain.h>
#include <melee/it/kinds/itseakneedlethrown.h>
#include <melee/ft/kinds/ftSeak/ftseakspecialn.h>
#include <melee/lb/lb_00B0.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftSeak/types.h>
#include <melee/ft/kinds/ftSeak/forward.h>
#include <melee/ft/kinds/ftSeak/ftseakspecials.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/it/itCharItems.h>
#include <melee/lb/lbcollision.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/jobj.h>

static unsigned checks, pairs;
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr,"line%d pair%u %s\n",__LINE__,pairs,#v); exit(1); } } while(0)
static Fighter fighter;
static HSD_GObj source, object, *heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static Item item;
static Article article;
/* The original fighter reads the actual short attribute-prefix declaration;
 * the article owns the full declaration including model pointers. */
static union { itChainSegment prefix; itSeakChain_Attrs complete; } chain_storage;
#define chain_attributes chain_storage.prefix
static ftSeakAttributes attributes;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
struct Fighter_ShakeTable_t* Fighter_SmashChargeShakeTable;
static unsigned sounds[8], sound_count, victim_count, position_count;
typedef struct PoseEvent { int operation,part,graph;float a,b; } PoseEvent;
static PoseEvent pose_events[16];static unsigned pose_count;
static FighterBone bones[58];static HSD_JObj live, alternate;
static HSD_Joint descriptors[8];static void* imported_items[6];static ftData own_data;
static FigaTree clip;static int pose_invalidation;
static int graph_number(HSD_Joint* graph) { return graph==&descriptors[5]?1:graph==&descriptors[7]?2:0; }
static void phase_record(int op,int argument,float a,float b,float c);
static void pose_event(int operation,int part,HSD_Joint* graph,float a,float b)
{ CHECK(pose_count<16);pose_events[pose_count++]=(PoseEvent){operation,part,graph_number(graph),a,b};phase_record(50+operation,part,a,b,graph_number(graph)); }
void ftAnim_8006F4C8(Fighter* f,bool blend,FigaTree* c)
{ CHECK(f==&fighter&&blend&&c==&clip);pose_event(1,0,NULL,0,0); }
void ftAnim_80070710(HSD_JObj* root,float frame)
{ CHECK(root==&alternate);pose_event(2,0,NULL,frame,0); }
void ftAnim_8006FB88(Fighter* f,Fighter_Part part,HSD_Joint* graph)
{ CHECK(f==&fighter&&graph==&descriptors[1]);pose_event(3,part,graph,0,0); }
void HSD_JObjAnimAll(HSD_JObj* root)
{ CHECK(root==&alternate);pose_event(4,0,NULL,0,0);if(pose_invalidation==1)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;else if(pose_invalidation==2)++fighter.x8_spawnNum; }
void ftAnim_80070108(Fighter* f,Fighter_Part part,float a,float b,HSD_Joint* graph)
{ CHECK(f==&fighter);pose_event(5,part,graph,a,b); }
void ftAnim_8006FE9C(Fighter* f,Fighter_Part part,float a,float b)
{ CHECK(f==&fighter);pose_event(6,part,NULL,a,b); }
void ftAnim_8006FF74(Fighter* f,Fighter_Part part)
{ CHECK(f==&fighter);pose_event(7,part,NULL,0,0); }
void ftAnim_80070010(Fighter* f,Fighter_Part part,float a,float b,HSD_Joint* graph)
{ CHECK(f==&fighter);pose_event(8,part,graph,a,b); }
void ftAnim_8006FA58(Fighter* f,Fighter_Part part,HSD_Joint* graph)
{ CHECK(f==&fighter);pose_event(9,part,graph,0,0); }
FigaTree* ftData_80085E50(Fighter* f,int id) { CHECK(f==&fighter&&(id==305||id==308));return &clip; }
static FigaTree* owned_clip(Fighter_GObj* g,int32_t id) { CHECK(g==&source);return ftData_80085E50(&fighter,id); }
static HSD_Joint* owned_graph(Fighter_GObj* g,int32_t index)
{ CHECK(g==&source&&(index==0||index==1));return &descriptors[index?7:5]; }
static bool valid_pose(Fighter_GObj* g,NativeFighterPoseOperation op,int32_t dest,int32_t part,HSD_Joint* graph,FigaTree* c)
{ CHECK(g==&source&&dest>=0&&dest<=1&&part>=0&&part<58);(void)op;(void)graph;(void)c;return true; }
static HSD_Joint* costume_pose(Fighter* f,HSD_JObj* selected,HSD_Joint* costume)
{ CHECK(f==&fighter&&selected==&alternate&&costume==&descriptors[0]);return costume->child; }
bool nativeModelEffect_Snapshot(HSD_GObj* g, uint32_t* id)
{ (void)g; *id=0; return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g, uint32_t id)
{ (void)g; (void)id; return NULL; }
void ft_PlaySFX(Fighter* f, enum_t sound, u8 volume, u8 pan)
{ CHECK(f == &fighter && volume == 127 && pan == 64); CHECK(sound_count < 8); sounds[sound_count++] = sound; }
void lbColl_80008428(HitCapsule* hit) { hit->state = HitCapsule_Disabled; }
void lbColl_80008434(HitCapsule* hit) { hit->state = HitCapsule_Enabled; }
void lbColl_80008440(HitCapsule* hit)
{ CHECK(hit >= fighter.x914 && hit < fighter.x914 + 4); ++victim_count; memset(hit->victims_1, 0, sizeof(hit->victims_1)); memset(hit->victims_2, 0, sizeof(hit->victims_2)); hit->x44=hit->x45=0; }
void ftColl_8007B8A8(HitCapsule* hit, Vec3* position)
{ CHECK(hit >= fighter.x914 && hit < fighter.x914 + 4); ++position_count; hit->x4C = *position; }
static float read_article(HSD_GObj* g, uint32_t id, ItemKind kind, unsigned field, int32_t index)
{ CHECK(g == &source && id == item.x1C && kind == It_Kind_Seak_Chain && !index); CHECK(field == 15 || field == 16); return field == 15 ? chain_attributes.x48 : chain_attributes.x4C; }

#include "generated.inc"
#include "parameters.inc"
#include "native_fighter_module_actual.inc"
#include "fighter_policy_actual.inc"

/* Original phase callbacks remain unchanged. Shared physics, transitions,
 * article creation/dispatch and geometry are controlled ordered boundaries. */
typedef struct PhaseEvent { int op,argument;float a,b,c; } PhaseEvent;
static PhaseEvent events[64];static unsigned event_count;static bool phase_recording;
static bool ground_contact,air_contact,frames_remaining,spawn_success,blocked;
static bool needle_cleanup_probe;static int needle_creation_mutation;
static Vec3 hand={7,8,9};static int phase_invalidation;
static void phase_record(int op,int argument,float a,float b,float c)
{if(!phase_recording)return;CHECK(event_count<64);events[event_count++]=(PhaseEvent){op,argument,a,b,c};}
void Fighter_ChangeMotionState(HSD_GObj*g,FtMotionId id,MotionFlags flags,float frame,float rate,float blend,HSD_GObj* donor)
{CHECK(g==&source&&rate==1&&blend==0&&donor==NULL);phase_record(1,id,frame,(float)flags,0);fighter.motion_id=id;fighter.cur_anim_frame=frame;fighter.death2_cb=fighter.take_dmg_cb=NULL;if(phase_invalidation==1)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;else if(phase_invalidation==2)++fighter.x8_spawnNum;}
void ftAnim_8006EBA4(HSD_GObj*g){CHECK(g==&source);phase_record(4,0,0,0,0);}
void ftCommon_8007D5D4(Fighter*f){CHECK(f==&fighter);phase_record(2,0,0,0,0);f->ground_or_air=GA_Air;}
void ftCommon_8007D7FC(Fighter*f){CHECK(f==&fighter);phase_record(3,0,0,0,0);f->ground_or_air=GA_Ground;}
void ft_8008A2BC(HSD_GObj*g){CHECK(g==&source);phase_record(5,0,0,0,0);fighter.motion_id=14;}
void ftCo_Fall_Enter(HSD_GObj*g){CHECK(g==&source);phase_record(6,0,0,0,0);fighter.motion_id=29;}
bool ftAnim_IsFramesRemaining(HSD_GObj*g){CHECK(g==&source);phase_record(11,0,0,0,0);return frames_remaining;}
void ft_80084F3C(HSD_GObj*g){CHECK(g==&source);phase_record(7,0,0,0,0);}
void ft_80084EEC(HSD_GObj*g){CHECK(g==&source);phase_record(8,0,0,0,0);}
void ftCommon_Fall(Fighter*f,float gravity,float terminal){CHECK(f==&fighter);phase_record(9,0,gravity,terminal,0);}
void ftCommon_CalcSelfAccel_Deaccel(Fighter*f,float friction){CHECK(f==&fighter);phase_record(10,0,friction,0,0);}
bool ft_800827A0(HSD_GObj*g){CHECK(g==&source);phase_record(12,0,0,0,0);return ground_contact;}
GroundOrAir ft_80081D0C(HSD_GObj*g){CHECK(g==&source);phase_record(13,0,0,0,0);return air_contact?GA_Air:GA_Ground;}
void lb_8000B1CC(HSD_JObj*j,Vec3*offset,Vec3*result){CHECK(j==&live&&offset==NULL);phase_record(14,26,0,0,0);*result=hand;}
static unsigned bone_count(Fighter*f){CHECK(f==&fighter);return 58;}
Item_GObj* itSeakChain_Spawn(Fighter_GObj*g,Point3d*pos,float facing)
{CHECK(g==&source);phase_record(15,0,pos->x,pos->y,pos->z);CHECK(facing==fighter.facing_dir);return spawn_success?&object:NULL;}
static int32_t create_article(Fighter_GObj*g,ItemKind kind,Vec3*pos,Vec3*velocity,float facing,bool origin,uint32_t group,float value,bool initial)
{CHECK(velocity->x==0&&velocity->y==0&&velocity->z==0&&!group&&value==0&&initial);if(needle_cleanup_probe){CHECK(kind==It_Kind_Seak_NeedleThrow&&origin);phase_record(22,kind,pos->x,pos->y,pos->z);if(needle_creation_mutation==1)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;else if(needle_creation_mutation==2)++fighter.x8_spawnNum;return spawn_success?(int32_t)item.x1C:-1;}CHECK(kind==It_Kind_Seak_Chain&&!origin);return itSeakChain_Spawn(g,pos,facing)?(int32_t)item.x1C:-1;}
HSD_GObj*it_802AFD8C(HSD_GObj*g,Vec3*pos,u32 kind,float facing)
{CHECK(g==&source&&kind==It_Kind_Seak_NeedleThrow&&facing==fighter.facing_dir);phase_record(22,kind,pos->x,pos->y,pos->z);return spawn_success?&object:NULL;}
void it_802AFEA8(HSD_GObj*g,HSD_GObj*parent,u32 mode)
{CHECK(g==&object&&parent==&source&&mode==1);phase_record(23,0,0,0,0);}
static bool bind_item(Fighter_GObj*g,uint32_t id,bool auxiliary){CHECK(g==&source&&auxiliary);fighter.x1984_heldItemSpec=id==UINT32_MAX?NULL:&object;return id!=UINT32_MAX;}
void it_802BAEEC(HSD_GObj*g){CHECK(g==&object);phase_record(17,0,0,0,0);}
void it_802BAF0C(HSD_GObj*g){CHECK(g==&object);phase_record(18,0,0,0,0);}
void it_802BCF84(HSD_GObj*g){CHECK(g==&object);phase_record(19,0,0,0,0);}
void it_802BCFC4(HSD_GObj*g,Vec3*velocity){CHECK(g==&object);phase_record(20,0,velocity->x,velocity->y,velocity->z);}
void it_802BB20C(HSD_GObj*g){if(!g)return;CHECK(g==&object);phase_record(16,0,0,0,0);ftSk_SpecialS_80110E4C(&source);}
static uint32_t hash_name(const char*s){uint32_t hash=2166136261u;while(*s)hash=(hash^(uint8_t)*s++)*16777619u;return hash;}
static bool signal_article(HSD_GObj*g,uint32_t id,uint32_t name,float value)
{CHECK(g==&source&&value==0);if(id==UINT32_MAX)return false;CHECK(id==item.x1C);if(name==hash_name("initialize_drop")){CHECK(needle_cleanup_probe);it_802AFEA8(&object,&source,1);}else if(name==hash_name("launch")){Vec3 velocity={chain_attributes.x50*fighter.facing_dir,0,0};it_802BCFC4(&object,&velocity);}else if(name==hash_name("retract"))it_802BCF84(&object);else if(name==hash_name("pause"))it_802BAEEC(&object);else if(name==hash_name("resume"))it_802BAF0C(&object);else if(name==hash_name("destroy")){phase_record(16,0,0,0,0);script_fn_chain_lost(&source);}else CHECK(false);return true;}
static void init_collision(CollData*c){memset(c,0,sizeof *c);}
static bool sweep_box(CollData*c,ftCollisionBox*box)
{CHECK(c->x34_flags.b1234==5);phase_record(21,0,box->top,c->last_pos.y,c->cur_pos.y);c->env_flags=blocked?0x8000:0;return blocked;}
s32 ftNs_AttackHi4_YoyoCheckEnvColl(HSD_GObj*g,Vec3*from,Vec3*to,float radius)
{CHECK(g==&source);phase_record(21,0,radius*fighter.x34_scale.y,from->y,to->y);return blocked?32768:0;}
#include "fighter_phase_actual.inc"
static void ftSk_SpecialN_801120D4(Fighter_GObj*);
#include "fighter_cleanup_actual.inc"

static unsigned offset(const char* name)
{ for (unsigned i = 0; i < sizeof(authored_state_fields)/sizeof(*authored_state_fields); ++i) if (!strcmp(name, authored_state_fields[i].name)) return authored_state_fields[i].offset; CHECK(false); return 0; }
static void setf(const char* name, float value) { script_state_set_float(&fighter, offset(name), value); }
static float getf(const char* name) { return script_state_get_float(&fighter, offset(name)); }
static void seti(const char* name, int32_t value) { script_state_set_int(&fighter, offset(name), value); }
static int32_t geti(const char* name) { return script_state_get_int(&fighter, offset(name)); }
static void arrayf(const char* name, unsigned index, float value) { script_state_set_float(&fighter, offset(name)+4*index, value); }
static float getarray(const char* name, unsigned index) { return script_state_get_float(&fighter, offset(name)+4*index); }
static void reset(bool scripted)
{
    memset(&fighter,0,sizeof fighter); memset(&item,0,sizeof item); memset(&source,0,sizeof source); memset(&object,0,sizeof object); memset(heads,0,sizeof heads);
    source.user_data=&fighter; source.classifier=HSD_GOBJ_CLASS_FIGHTER; heads[HSD_GOBJ_PLINK_FIGHTER]=&source;
    object.user_data=&item; object.classifier=HSD_GOBJ_CLASS_ITEM; heads[HSD_GOBJ_PLINK_ITEM]=&object;
    fighter.gobj=&source; fighter.kind=scripted?Ft_Kind_CustomFirst:Ft_Kind_Seak; fighter.x8_spawnNum=UINT32_C(0xfedcba98); fighter.dat_attrs=&attributes;
    item.x1C=UINT32_C(0x80000001); item.xC4_article_data=&article; article.x4_specialAttributes=&chain_attributes;
    if(scripted) seti("chain_handle",(int32_t)item.x1C); else fighter.u.sk.x8=&object;
    fighter.ft_data=&own_data;own_data.x48_items=imported_items;imported_items[4]=&descriptors[4];imported_items[5]=&descriptors[6];descriptors[4].child=&descriptors[5];descriptors[6].child=&descriptors[7];descriptors[0].child=&descriptors[1];fighter.x108_costume_joint=&descriptors[0];fighter.parts=bones;bones[1].x4_jobj2=&alternate;fighter.x8AC_animSkeleton=&alternate;source.hsd_obj=&live;
    sound_count=victim_count=position_count=pose_count=0;pose_invalidation=0;
}
static float max_difference;
static void eq(float expected,float actual)
{ float delta=fabsf(expected-actual); if(delta>max_difference)max_difference=delta; CHECK(delta<=2e-6f*fmaxf(1,fmaxf(fabsf(expected),fabsf(actual)))); }
static void test_smoothing(void)
{
    const float input[]={-1,-.7f,-.2f,0,.2f,.7f,1};
    const float angles[]={0,4,90,179,180,181,359,360};
    const float magnitudes[]={0,.25f,.75f,1};
    const float smoothing[]={0,.125f,.5f,1};
    for(unsigned a=0;a<8;++a)for(unsigned m=0;m<4;++m)for(unsigned s=0;s<4;++s)for(unsigned x=0;x<7;++x)for(unsigned y=0;y<7;++y)for(int face=-1;face<=1;face+=2){
        reset(false); fighter.facing_dir=face; fighter.input.lstick[0]=(Vec2){input[x],input[y]}; common.x44C=smoothing[s];
        fighter.mv.sk.specials.x18=angles[a]; fighter.mv.sk.specials.x14=magnitudes[m];
        ftSk_SpecialS_80110490(&fighter); float angle=fighter.mv.sk.specials.x18, magnitude=fighter.mv.sk.specials.x14;
        reset(true); fighter.facing_dir=face; fighter.input.lstick[0]=(Vec2){input[x],input[y]}; setf("chain_angle",angles[a]); setf("chain_magnitude",magnitudes[m]);
        script_fn_chain_smooth(&source); eq(angle,getf("chain_angle")); eq(magnitude,getf("chain_magnitude")); ++pairs;
    }
}
static void test_input(void)
{
    const float values[]={-1,-.5f,-.3f,0,.3f,.5f,1};
    for(unsigned x=0;x<7;++x)for(unsigned y=0;y<7;++y)for(int face=-1;face<=1;face+=2)for(int timer=0;timer<3;++timer)for(int exists=0;exists<2;++exists){
        reset(false); fighter.facing_dir=face; fighter.input.lstick[0]=(Vec2){values[x],values[y]};fighter.input.lstick[1]=(Vec2){.1f,-.2f}; fighter.mv.sk.specials.x8=timer;fighter.mv.sk.specials.xC=timer;
        if(!exists)fighter.u.sk.x8=NULL; ftSk_SpecialS_80110788(&source);
        Vec2 delta={fighter.u.sk.lstick_delta.x,fighter.u.sk.lstick_delta.y};int forward=fighter.mv.sk.specials.x8,back=fighter.mv.sk.specials.xC; unsigned count=sound_count, expected[8];memcpy(expected,sounds,sizeof expected);
        reset(true);fighter.facing_dir=face;fighter.input.lstick[0]=(Vec2){values[x],values[y]};fighter.input.lstick[1]=(Vec2){.1f,-.2f};seti("chain_forward_sound_timer",timer);seti("chain_backward_sound_timer",timer);if(!exists)seti("chain_handle",-1);
        script_fn_chain_input(&source);eq(delta.x,getf("chain_delta_x"));eq(delta.y,getf("chain_delta_y"));CHECK(geti("chain_forward_sound_timer")==forward&&geti("chain_backward_sound_timer")==back);CHECK(count==sound_count);CHECK(!memcmp(expected,sounds,count*sizeof*expected));++pairs;
    }
}
static void test_pose(void)
{
    const float magnitudes[]={0,.25f,.75f,1,1.25f};const float phases[]={0,.25f,.75f,1,1.25f};
    for(int air=0;air<2;++air)for(unsigned m=0;m<5;++m)for(unsigned p=0;p<5;++p)for(int face=-1;face<=1;face+=2){
        reset(false);fighter.facing_dir=face;fighter.input.lstick[0]=(Vec2){.5f,-.25f};common.x44C=.125f;fighter.mv.sk.specials.x14=magnitudes[m];fighter.mv.sk.specials.x18=43;
        ftSk_SpecialS_80110610(&source,air?308:305,phases[p]);float angle=fighter.mv.sk.specials.x18,magnitude=fighter.mv.sk.specials.x14;unsigned count=pose_count;PoseEvent expected[16];memcpy(expected,pose_events,sizeof expected);
        reset(true);fighter.facing_dir=face;fighter.input.lstick[0]=(Vec2){.5f,-.25f};setf("chain_magnitude",magnitudes[m]);setf("chain_angle",43);script_fn_chain_pose(&source,air,phases[p]);eq(angle,getf("chain_angle"));eq(magnitude,getf("chain_magnitude"));CHECK(count==pose_count);
        for(unsigned i=0;i<count;++i){CHECK(expected[i].operation==pose_events[i].operation&&expected[i].part==pose_events[i].part&&expected[i].graph==pose_events[i].graph);eq(expected[i].a,pose_events[i].a);eq(expected[i].b,pose_events[i].b);}++pairs;
    }
    for(int retired=1;retired<=2;++retired){reset(true);setf("chain_magnitude",.5f);setf("chain_angle",43);pose_invalidation=retired;script_fn_chain_pose(&source,false,.5f);CHECK(pose_count==4);}
}
static void test_rehit(void)
{
    const float movement[]={0,.1f,.5f,1,2};
    for(int timer=0;timer<4;++timer)for(int grace=0;grace<3;++grace)for(unsigned moving=0;moving<5;++moving)for(int changed=0;changed<4;++changed){
        reset(false);fighter.mv.sk.specials.x1C=timer;fighter.mv.sk.specials.x20=grace;
        for(unsigned i=0;i<4;++i){fighter.u.sk.xC[i]=(Vec3){(i==changed?movement[moving]:0),0,7};fighter.u.sk.x3C[i]=(Vec3){0,0,9};fighter.x914[i].state=HitCapsule_Enabled;fighter.x914[i].x4C=(Vec3){2,3,4};fighter.x914[i].x58=(Vec3){5,6,7};}
        ftSk_SpecialS_80110BCC(&source);int expected_timer=fighter.mv.sk.specials.x1C,expected_grace=fighter.mv.sk.specials.x20;HitCapsule expected[4];memcpy(expected,fighter.x914,sizeof expected);bool latch=fighter.x2219_b3;unsigned victims=victim_count;
        reset(true);seti("chain_rehit_timer",timer);seti("chain_resume_timer",grace);
        for(unsigned i=0;i<4;++i){arrayf("chain_hit_x",i,i==changed?movement[moving]:0);arrayf("chain_hit_y",i,0);arrayf("chain_old_x",i,0);arrayf("chain_old_y",i,0);fighter.x914[i].state=HitCapsule_Enabled;fighter.x914[i].x4C=(Vec3){2,3,4};fighter.x914[i].x58=(Vec3){5,6,7};}
        script_fn_chain_rehit(&source);CHECK(expected_timer==geti("chain_rehit_timer")&&expected_grace==geti("chain_resume_timer"));CHECK(latch==fighter.x2219_b3&&victims==victim_count);
        for(unsigned i=0;i<4;++i){CHECK(expected[i].state==fighter.x914[i].state);CHECK(!memcmp(&expected[i].x4C,&fighter.x914[i].x4C,sizeof(Vec3)));CHECK(!memcmp(&expected[i].x58,&fighter.x914[i].x58,sizeof(Vec3)));eq(i==changed?movement[moving]:0,getarray("chain_old_x",i));}++pairs;
    }
}
static void test_endpoints(void)
{
    const Vec3 positions[]={{0,0,0},{0,0,2},{1,0,0},{0,2,0},{1,2,3}};
    for(int command=0;command<2;++command)for(int hit=0;hit<4;++hit)for(unsigned p=0;p<5;++p)for(int previous=0;previous<2;++previous){
        reset(false);fighter.cmd_vars[0]=command;fighter.x914[hit].x58=previous?(Vec3){5,6,7}:(Vec3){0};Vec3 position=positions[p];ftSk_SpecialS_UpdateHitboxes(&source,&position,hit);
        Vec3 expected=fighter.u.sk.xC[hit],cur=fighter.x914[hit].x4C,old=fighter.x914[hit].x58;unsigned writes=position_count;
        reset(true);fighter.cmd_vars[0]=command;fighter.x914[hit].x58=previous?(Vec3){5,6,7}:(Vec3){0};script_fn_chain_endpoint(&source,hit,position.x,position.y,position.z);
        eq(expected.x,getarray("chain_hit_x",hit));eq(expected.y,getarray("chain_hit_y",hit));eq(expected.z,getarray("chain_hit_z",hit));CHECK(!memcmp(&cur,&fighter.x914[hit].x4C,sizeof cur));CHECK(!memcmp(&old,&fighter.x914[hit].x58,sizeof old));CHECK(writes==position_count);++pairs;
    }
}
static void test_phases(void)
{
    HSD_GObjEvent originals[]={ftSk_SpecialS_Enter,ftSk_SpecialAirS_Enter,
        ftSk_SpecialSStart_Anim,ftSk_SpecialAirSStart_Anim,ftSk_SpecialS_Anim,ftSk_SpecialAirS_Anim,ftSk_SpecialSEnd_Anim,ftSk_SpecialAirSEnd_Anim,
        ftSk_SpecialSStart_IASA,ftSk_SpecialAirSStart_IASA,ftSk_SpecialS_IASA,ftSk_SpecialAirS_IASA,ftSk_SpecialSEnd_IASA,ftSk_SpecialAirSEnd_IASA,
        ftSk_SpecialSStart_Phys,ftSk_SpecialS_Phys,ftSk_SpecialSEnd_Phys,ftSk_SpecialAirSStart_Phys,ftSk_SpecialAirS_Phys,ftSk_SpecialAirSEnd_Phys,
        ftSk_SpecialSStart_Coll,ftSk_SpecialAirSStart_Coll,ftSk_SpecialS_Coll,ftSk_SpecialAirS_Coll,ftSk_SpecialSEnd_Coll,ftSk_SpecialAirSEnd_Coll};
    HSD_GObjEvent authored[]={authored_handler_34,authored_handler_35,authored_handler_36,authored_handler_37,authored_handler_38,authored_handler_39,authored_handler_40,authored_handler_41,authored_handler_42,authored_handler_43,authored_handler_44,authored_handler_45,authored_handler_46,authored_handler_47,authored_handler_48,authored_handler_49,authored_handler_50,authored_handler_51,authored_handler_52,authored_handler_53,authored_handler_54,authored_handler_55,authored_handler_56,authored_handler_57,authored_handler_58,authored_handler_59};
    int ticks[25]={0,1,2,3,5,7,10,15,20,30};unsigned tick_count=10;
    const float boundaries[]={attributes.x14,attributes.x1C,attributes.x20,attributes.x24,attributes.x28};
    for(unsigned b=0;b<5;++b)for(int delta=-1;delta<=1;++delta)ticks[tick_count++]=(int)boundaries[b]+delta;
    for(unsigned phase=0;phase<26;++phase)for(unsigned t=0;t<tick_count;++t)for(unsigned scenario=0;scenario<32;++scenario){
        bool exists=(scenario&1)!=0;
        /* The original launch dereferences the failed prior constructor on the
         * next tick; that undefined source path is separately safe in scripts. */
        if(phase>=2&&phase<4&&!exists&&ticks[t]+1==attributes.x1C+1)continue;
        if((phase==6||phase==7)&&!exists&&ticks[t]+1==attributes.x24)continue;
        Fighter expected;PhaseEvent expected_events[64];PoseEvent expected_pose[16];unsigned expected_event_count=0,expected_pose_count=0;
        for(int scripted=0;scripted<2;++scripted){
            reset(scripted);ground_contact=(scenario&2)!=0;air_contact=(scenario&2)!=0;frames_remaining=(scenario&4)!=0;blocked=(scenario&8)!=0;spawn_success=(scenario&16)!=0;phase_invalidation=0;phase_recording=true;event_count=0;
            fighter.ground_or_air=(phase%2)?GA_Air:GA_Ground;fighter.facing_dir=scenario&8?-1:1;fighter.cur_anim_frame=13.25f;fighter.self_vel=(Vec3){2,3,4};fighter.x34_scale.y=1.25f;fighter.cur_pos=(Vec3){2,3,4};fighter.coll_data.ecb.top.y=3;fighter.coll_data.ecb.bottom.y=-1;
            fighter.input.lstick[0]=(Vec2){.5f,-.25f};fighter.input.lstick[1]=(Vec2){.1f,.2f};fighter.input.held_buttons[0]=scenario&16?HSD_PAD_B:0;fighter.co_attrs.gravity=.09f;fighter.co_attrs.terminal_velocity=2.1f;fighter.co_attrs.aerial_friction=.03f;fighter.cmd_vars[0]=scenario&4?1:0;bones[26].joint=&live;common.x44C=.125f;item.facing_dir=fighter.facing_dir;
            if(scripted){seti("chain_tick",ticks[t]);seti("chain_rehit_timer",scenario&4?2:0);seti("chain_resume_timer",scenario&8?2:0);fighter.native_module_state[offset("chain_release_requested")]=(scenario&16)!=0;setf("chain_magnitude",.5f);setf("chain_angle",43);if(!exists)seti("chain_handle",-1);}
            else{fighter.mv.sk.specials.x0=ticks[t];fighter.mv.sk.specials.x1C=scenario&4?2:0;fighter.mv.sk.specials.x20=scenario&8?2:0;fighter.mv.sk.specials.x4=(scenario&16)!=0;fighter.mv.sk.specials.x14=.5f;fighter.mv.sk.specials.x18=43;if(!exists)fighter.u.sk.x8=NULL;}
            (scripted?authored[phase]:originals[phase])(&source);
            if(!scripted){expected=fighter;expected_event_count=event_count;memcpy(expected_events,events,sizeof events);expected_pose_count=pose_count;memcpy(expected_pose,pose_events,sizeof pose_events);}
            else{
                CHECK(expected.motion_id==fighter.motion_id&&expected.ground_or_air==fighter.ground_or_air);eq(expected.cur_anim_frame,fighter.cur_anim_frame);CHECK(!memcmp(&expected.self_vel,&fighter.self_vel,sizeof(Vec3)));CHECK(expected.cmd_vars[0]==fighter.cmd_vars[0]&&expected.x2222_b2==fighter.x2222_b2&&expected.x2219_b3==fighter.x2219_b3);
                CHECK(expected.mv.sk.specials.x0==geti("chain_tick")&&expected.mv.sk.specials.x1C==geti("chain_rehit_timer")&&expected.mv.sk.specials.x20==geti("chain_resume_timer"));CHECK(expected.mv.sk.specials.x4==(fighter.native_module_state[offset("chain_release_requested")]!=0));CHECK(expected.mv.sk.specials.x8==geti("chain_forward_sound_timer")&&expected.mv.sk.specials.xC==geti("chain_backward_sound_timer"));eq(expected.mv.sk.specials.x14,getf("chain_magnitude"));eq(expected.mv.sk.specials.x18,getf("chain_angle"));eq(expected.u.sk.lstick_delta.x,getf("chain_delta_x"));eq(expected.u.sk.lstick_delta.y,getf("chain_delta_y"));
                CHECK((expected.u.sk.x8!=NULL)==(geti("chain_handle")!=-1));CHECK(expected.x1984_heldItemSpec==fighter.x1984_heldItemSpec);
                CHECK((expected.death2_cb!=NULL)==(fighter.death2_cb!=NULL)&&(expected.take_dmg_cb!=NULL)==(fighter.take_dmg_cb!=NULL));if(fighter.death2_cb)CHECK(fighter.death2_cb==script_fn_sheik_resource_cleanup&&fighter.take_dmg_cb==script_fn_sheik_resource_cleanup);
                CHECK((expected.accessory4_cb!=NULL)==(fighter.accessory4_cb!=NULL));if(fighter.accessory4_cb)CHECK(fighter.accessory4_cb==script_fn_chain_accessory);
                CHECK((expected.pre_hitlag_cb!=NULL)==(fighter.pre_hitlag_cb!=NULL)&&(expected.post_hitlag_cb!=NULL)==(fighter.post_hitlag_cb!=NULL));if(fighter.pre_hitlag_cb)CHECK(fighter.pre_hitlag_cb==script_fn_chain_pause&&fighter.post_hitlag_cb==script_fn_chain_resume);
                for(unsigned i=0;i<4;++i){CHECK(expected.x914[i].state==fighter.x914[i].state);CHECK(!memcmp(&expected.x914[i].x4C,&fighter.x914[i].x4C,sizeof(Vec3)));CHECK(!memcmp(&expected.x914[i].x58,&fighter.x914[i].x58,sizeof(Vec3)));}
                CHECK(expected_event_count==event_count&&expected_pose_count==pose_count);for(unsigned i=0;i<event_count;++i){CHECK(expected_events[i].op==events[i].op&&expected_events[i].argument==events[i].argument);eq(expected_events[i].a,events[i].a);eq(expected_events[i].b,events[i].b);eq(expected_events[i].c,events[i].c);}for(unsigned i=0;i<pose_count;++i){CHECK(expected_pose[i].operation==pose_events[i].operation&&expected_pose[i].part==pose_events[i].part&&expected_pose[i].graph==pose_events[i].graph);eq(expected_pose[i].a,pose_events[i].a);eq(expected_pose[i].b,pose_events[i].b);}++pairs;
            }
            phase_recording=false;
        }
    }
    for(int retired=1;retired<=2;++retired){reset(true);phase_invalidation=retired;phase_recording=true;event_count=0;seti("chain_tick",77);authored_handler_34(&source);CHECK(event_count==1&&geti("chain_tick")==77);phase_recording=false;phase_invalidation=0;}
}
static void test_cleanup(void)
{
    needle_cleanup_probe=true;
    for(int needles=0;needles<=6;++needles)for(unsigned scenario=0;scenario<32;++scenario){
        Fighter expected;PhaseEvent expected_events[64];unsigned expected_count=0;
        for(int generated=0;generated<2;++generated){reset(generated);phase_recording=true;event_count=0;spawn_success=(scenario&16)!=0;fighter.ground_or_air=scenario&1?GA_Air:GA_Ground;fighter.facing_dir=scenario&2?-1:1;fighter.cur_pos=(Vec3){3,4,5};fighter.x34_scale.y=1.25f;
            bool held=(scenario&2)!=0,chain=(scenario&4)!=0,callbacks=(scenario&8)!=0;
            if(generated){seti("needle_count",needles);seti("needle_held",held?(int32_t)item.x1C:-1);if(!chain)seti("chain_handle",-1);if(callbacks)fighter.death2_cb=fighter.take_dmg_cb=script_fn_sheik_resource_cleanup;script_fn_sheik_resource_cleanup(&source);}
            else{fighter.u.sk.x0=needles;fighter.u.sk.x4=held?&object:NULL;fighter.u.sk.x8=chain?&object:NULL;if(callbacks)fighter.death2_cb=fighter.take_dmg_cb=ftSk_Init_80110198;ftSk_Init_80110198(&source);expected=fighter;expected_count=event_count;memcpy(expected_events,events,sizeof events);}
            if(generated){CHECK(expected.u.sk.x0==geti("needle_count")&&(expected.u.sk.x4!=NULL)==(geti("needle_held")!=-1)&&(expected.u.sk.x8!=NULL)==(geti("chain_handle")!=-1));CHECK((expected.death2_cb!=NULL)==(fighter.death2_cb!=NULL)&&(expected.take_dmg_cb!=NULL)==(fighter.take_dmg_cb!=NULL));CHECK(expected.mv.sk.specials.x20==geti("chain_resume_timer"));CHECK(expected_count==event_count);for(unsigned i=0;i<event_count;++i){CHECK(expected_events[i].op==events[i].op&&expected_events[i].argument==events[i].argument);eq(expected_events[i].a,events[i].a);eq(expected_events[i].b,events[i].b);eq(expected_events[i].c,events[i].c);}++pairs;}
            phase_recording=false;
        }
    }
    for(int invalidation=1;invalidation<=2;++invalidation){reset(true);phase_recording=true;event_count=0;spawn_success=true;needle_creation_mutation=invalidation;seti("needle_count",2);seti("needle_held",(int32_t)item.x1C);script_fn_sheik_resource_cleanup(&source);CHECK(event_count==1&&geti("needle_count")==2&&geti("needle_held")==-1&&geti("chain_handle")==((int32_t)item.x1C));phase_recording=false;needle_creation_mutation=0;}
    needle_cleanup_probe=false;
    for(int present=0;present<2;++present)for(int action=0;action<2;++action){unsigned original_events=0;PhaseEvent expected[64];int grace=0;
        for(int generated=0;generated<2;++generated){reset(generated);phase_recording=true;event_count=0;if(!present){if(generated)seti("chain_handle",-1);else fighter.u.sk.x8=NULL;}if(generated){if(action)script_fn_chain_resume(&source);else script_fn_chain_pause(&source);}else{if(action)ftSk_SpecialS_ChainSomething(&source);else ftSk_SpecialS_80110EE8(&source);original_events=event_count;memcpy(expected,events,sizeof events);grace=fighter.mv.sk.specials.x20;}if(generated){CHECK(original_events==event_count&&grace==geti("chain_resume_timer"));CHECK(!memcmp(expected,events,event_count*sizeof*events));++pairs;}phase_recording=false;}
    }
}
int main(void)
{
    NativeFighterHost host={0};host.common_attribute=common_attribute;host.abs_float=fabsf;host.sqrt_float=sqrtf;host.angle_atan2_float=atan2f;host.play_sound=ft_PlaySFX;host.article_ref_float=read_article;host.clear_hitbox_victims=lbColl_80008440;host.set_hitbox_position=ftColl_8007B8A8;host.callback_source_alive=nativeCallbackSource_Alive;engine=&host;
    host.pose_clip=owned_clip;host.pose_graph=owned_graph;host.pose_validate=valid_pose;host.find_costume_pose=costume_pose;host.load_pose_animation=ftAnim_8006F4C8;host.request_pose_frame=ftAnim_80070710;host.copy_blend_pose=ftAnim_8006FB88;host.model_advance_animation=HSD_JObjAnimAll;host.blend_graph_alternate=ftAnim_80070108;host.blend_pose_to_live=ftAnim_8006FE9C;host.copy_pose_to_live=ftAnim_8006FF74;host.blend_graph_live=ftAnim_80070010;host.copy_bone_pose=ftAnim_8006FA58;
    host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;host.finish_ground_action=ft_8008A2BC;host.enter_fall=ftCo_Fall_Enter;host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.ground_friction=ft_80084F3C;host.air_gravity_and_friction=ft_80084EEC;host.fall=ftCommon_Fall;host.air_decel=ftCommon_CalcSelfAccel_Deaccel;host.special_ground_collision=ft_800827A0;host.air_collision=ft_80081D0C;host.bone_count=bone_count;host.joint_position=lb_8000B1CC;host.create_article_at=create_article;host.bind_fighter_item=bind_item;host.signal_article=signal_article;host.initialize_collision=init_collision;host.sweep_collision_box=sweep_box;
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/sizeof(*parameter_bits),parameter_schema));
#include "policy_attributes.inc"
    test_smoothing();test_input();test_pose();test_rehit();test_endpoints();test_phases();test_cleanup();
    printf("PASS %u policy pairs, %u checks, maximum_float_difference %.9g\n",pairs,checks,max_difference);return 0;
}
