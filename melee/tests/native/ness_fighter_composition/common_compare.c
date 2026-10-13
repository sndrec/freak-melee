// Original healthy common entry bodies versus generated policy, and actual46
// presentation dispatch. Allocation/render/audio are ordered explicit boundaries.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftFox/types.h>
#include <melee/ft/kinds/ftMario/ftmariospecialn.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/it/kinds/itnessbat.h>
#include <melee/it/kinds/itnessyoyo.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/it/kinds/itfoxblaster.h>
#include <melee/it/kinds/itdrmariopill.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define GET_ITEM(o) ((Item*)(o)->user_data)

static unsigned checks, jump_pairs, presentation_pairs, constructor_pairs, log_count;
static Fighter fighter;
static Item bat;
static HSD_GObj source, object;
static bool allocation_fails, generated_path, removed_source;
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic* bat_logic;
static ftCommonData common;
ftCommonData* p_ftCommonData=&common;
static struct Fighter_804D6520_t bunny;
struct Fighter_804D6520_t* Fighter_804D6520=&bunny;
static FtSFX sfx={.x14=42};
static ftData data={.x4C_sfx=&sfx};
typedef struct Call { unsigned operation; int value; float args[3]; } Call;
static Call log_calls[64];
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"%s:%d: %s\n", \
    __FILE__,__LINE__,#c); exit(1); } } while (0)
static void record(unsigned op,int value,float x,float y,float z)
{ CHECK(log_count<64); log_calls[log_count++]=(Call){op,value,{x,y,z}}; }
static void unexpected(void) { CHECK(false); }
static bool alive(HSD_GObj* o,uint32_t generation,uint32_t kind)
{
    if(kind==NativeCallbackSource_Fighter)
        return !removed_source && o==&source && o->user_data==&fighter &&
            generation==(uint32_t)fighter.x8_spawnNum;
    return o==&object && o->user_data==&bat && generation==bat.x1C;
}
bool nativeCallbackSource_Snapshot(HSD_GObj* o,uint32_t kind,uint32_t* generation)
{
    if(!generation || o!=&source || kind!=NativeCallbackSource_Fighter) return false;
    *generation=(uint32_t)fighter.x8_spawnNum; return alive(o,*generation,kind);
}
bool nativeCallbackSource_Alive(HSD_GObj* o,uint32_t generation,uint32_t kind)
{ return alive(o,generation,kind); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_Ness ? definition : NULL; }
void Fighter_ChangeMotionState(HSD_GObj* o,FtMotionId motion,MotionFlags flags,
                              float start,float rate,float blend,HSD_GObj* donor)
{
    CHECK(o==&source && donor==NULL); record(1,flags,start,rate,blend);
    fighter.motion_id=motion;
}
void ftCommon_8007D5D4(Fighter* f)
{ CHECK(f==&fighter); record(2,0,0,0,0); f->ground_or_air=GA_Air; }
void ft_80088770(Fighter* f) { CHECK(f==&fighter); record(3,0,0,0,0); }
void ft_80088328(Fighter* f,int id,u8 volume,u8 pan)
{ CHECK(f==&fighter); record(4,id,volume,pan,0); }
void ft_PlaySFX(Fighter* f,int id,u8 volume,u8 pan)
{ CHECK(f==&fighter); record(5,id,volume,pan,0); }
void ftCo_JumpAerial_Phys_Cb(HSD_GObj* o) { (void)o; CHECK(false); }
void ftNs_JumpAerial_Phys_Cb(HSD_GObj* o);
void ftCommon_CalcSelfAccel_DriftFrom(Fighter*,float);
void ftCommon_CalcSelfAccel_AccelToVelClampedFrom(Fighter*,float,float,float,float);
void it_2725_Logic58_PickedUp(HSD_GObj*);
#include "jump_original.h"
// An empty placeholder can be folded with an authored pass callback by ICF.
// A retained donor callback must fail if called, and retain distinct identity.
static void stock_slot(HSD_GObj* o) { (void)o; CHECK(false); }
#include "base.h"
#include "behavior.generated.c"
#include "empty_original.h"
static float common_attribute(NativeFighterCommonAttribute attr)
{ CHECK(attr==NativeFighterCommon_AerialJumpReverse); return common.x78; }
static unsigned field(const char* name)
{
    for(unsigned i=0;i<definition->state_field_count;++i)
        if(!strcmp(definition->state_fields[i].name,name)) return definition->state_fields[i].offset;
    CHECK(false); return 0;
}
static void reset(void)
{
    memset(&fighter,0,sizeof(fighter)); memset(&bat,0,sizeof(bat));
    source=(HSD_GObj){.user_data=&fighter,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    object=(HSD_GObj){.user_data=&bat,.classifier=HSD_GOBJ_CLASS_ITEM};
    fighter.gobj=&source; fighter.kind=Ft_Kind_Ness; fighter.x8_spawnNum=0x80000099u;
    fighter.ft_data=&data; fighter.facing_dir=1; fighter.cur_pos=(Vec3){4,7,11};
    fighter.co_attrs=(ftCo_DatAttrs){.air_jump_h_multiplier=1.3f,
        .air_drift_stick_mul=.02f,.aerial_drift_base=.01f,.air_drift_max=1.2f,
        .aerial_friction=.04f,.air_max_horizontal_velocity=1.4f,.model_scaling=1.5f};
    fighter.x34_scale.y=1.25f; fighter.x1968_jumpsUsed=1;
    bat.x1C=0x80000007u; bat.scl=1; bat.entity=&object;
    common.x78=.3f; bunny.x14=1.75f;
    log_count=0; memset(log_calls,0,sizeof(log_calls)); removed_source=false;
}
static void jump_matrix(void)
{
    const float sticks[]={-1,-.30001f,-.3f,-.29999f,0,.29999f,.3f,.30001f,1};
    const float velocities[]={-2,-1.2f,-.01f,0,.01f,1.2f,2};
    for(unsigned f=0;f<2;++f) for(unsigned s=0;s<9;++s)
      for(unsigned worn=0;worn<2;++worn) {
        reset(); fighter.facing_dir=f?-1:1; fighter.input.lstick[0].x=sticks[s];
        fighter.x197C=worn?&object:NULL;
        ftNs_JumpAerial_Enter(&source);
        int motion=fighter.motion_id; Vec3 velocity=fighter.self_vel;
        unsigned commands=fighter.cmd_vars[0], jumps=fighter.x1968_jumpsUsed;
        int timer=fighter.x671_timer_lstick_tilt_y;
        float accumulator=fighter.mv.co.jumpaerial.init_h_vel;
        Call expected[64]; memcpy(expected,log_calls,sizeof(expected)); unsigned count=log_count;
        reset(); fighter.facing_dir=f?-1:1; fighter.input.lstick[0].x=sticks[s];
        fighter.x197C=worn?&object:NULL; definition->on_aerial_jump(&source);
        CHECK(fighter.motion_id==motion && !memcmp(&velocity,&fighter.self_vel,sizeof(velocity)));
        CHECK(fighter.cmd_vars[0]==commands && fighter.x1968_jumpsUsed==jumps);
        CHECK(fighter.x671_timer_lstick_tilt_y==timer && fighter.x2221_b7);
        CHECK(script_state_get_float(&fighter,field("jump_horizontal_accumulator"))==accumulator);
        CHECK(fighter.phys_cb==script_fn_JumpPhysics);
        CHECK(log_count==count && !memcmp(log_calls,expected,sizeof(expected))); ++jump_pairs;
        for(unsigned v=0;v<7;++v) for(unsigned frame=0;frame<3;++frame) {
            fighter.mv.co.jumpaerial.init_h_vel=velocities[v];
            fighter.x6A4_transNOffset=(Vec3){.4f,(float)frame-.6f,.15f};
            fighter.self_vel=(Vec3){-3,4,5}; fighter.x74_self_accel=(Vec3){7,8,9};
            ftNs_JumpAerial_Phys_Cb(&source);
            Vec3 expected_vel=fighter.self_vel, expected_accel=fighter.x74_self_accel;
            float expected_accumulator=fighter.mv.co.jumpaerial.init_h_vel;
            fighter.self_vel=(Vec3){-3,4,5}; fighter.x74_self_accel=(Vec3){7,8,9};
            script_state_set_float(&fighter,field("jump_horizontal_accumulator"),velocities[v]);
            fighter.phys_cb(&source);
            CHECK(!memcmp(&expected_vel,&fighter.self_vel,sizeof(Vec3)));
            CHECK(!memcmp(&expected_accel,&fighter.x74_self_accel,sizeof(Vec3)));
            CHECK(script_state_get_float(&fighter,field("jump_horizontal_accumulator"))==expected_accumulator);
            ++jump_pairs;
        }
      }
}
// Common presentation constructor boundary records every SpawnItem field actually
// initialized by the original. Padding/uninitialized fields are never compared.
static SpawnItem last_spawn;
Item_GObj* Item_80268B18(SpawnItem* spawn)
{
    CHECK(spawn->kind==It_Kind_Ness_Bat || spawn->kind==It_Kind_Ness_Yoyo); last_spawn=*spawn;
    record(6,spawn->kind,spawn->pos.x,spawn->pos.y,spawn->pos.z);
    if(allocation_fails) return NULL;
    bat.pos=spawn->pos; bat.x40_vel=spawn->vel; bat.facing_dir=spawn->facing_dir;
    bat.owner=spawn->x0_parent_gobj; return &object;
}
float ftLib_800865C0(HSD_GObj* o) { CHECK(o==&source); return fighter.facing_dir; }
void ftLib_80086644(HSD_GObj* o,Vec3* pos) { CHECK(o==&source); *pos=fighter.cur_pos; }
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part part)
{ CHECK(f==&fighter && part==FtPart_RThumbNb); return 42; }
void Item_80268E5C(HSD_GObj* o,enum_t state,Item_StateChangeFlags flags)
{ CHECK(o==&object); bat.msid=state; record(8,state,flags,0,0); }
void Item_802694CC(HSD_GObj* o) { CHECK(o==&object); record(9,0,0,0,0); }
void Item_8026AB54(HSD_GObj* o,HSD_GObj* creator,Fighter_Part part)
{
    CHECK(o==&object && creator==&source && part==42); record(7,part,0,0,0);
    if(generated_path) bat_logic->logic->picked_up(o); else it_2725_Logic58_PickedUp(o);
}
void it_8026BAE8(HSD_GObj* o,float scale)
{
    if(!o) return; CHECK(o==&object); bat.scl=scale*.75f; record(10,0,bat.scl,0,0);
}
#include "presentation_bat_original.h"
void it_80275158(Item_GObj* o,float lifetime)
{ (void)o; (void)lifetime; CHECK(false); }
#include "constructors_original.h"
Article* nativeFighterRuntime_ArticleAssets(FighterKind author,ItemKind kind)
{
    CHECK(author==Ft_Kind_Ness && (kind==It_Kind_Ness_Bat || kind==It_Kind_Ness_Yoyo));
    static Article asset; return &asset;
}
uint32_t nativeFighterTarget_Id(HSD_GObj* o)
{ CHECK(o==&source); return fighter.x8_spawnNum; }
Item_GObj* Item_CreateAuthored(SpawnItem* spawn,FighterKind author)
{ CHECK(author==Ft_Kind_Ness); return Item_80268B18(spawn); }
#include "constructor_current.h"
// Unused other-fighter branches remain compiled but must not execute.
Item_GObj* it_802AE994(HSD_GObj* o,Fighter_Part part,ItemKind kind)
{ (void)o;(void)part;(void)kind;CHECK(false);return NULL; }
int ftMr_SpecialN_VitaminRandom(HSD_GObj* o) { (void)o;CHECK(false);return 0; }
Item_GObj* itDrMarioPill_802C09C4(HSD_GObj* o,Vec3* p,s32 v,ItemKind k,s32 mode,
                                Fighter_Part part,float face)
{ (void)o;(void)p;(void)v;(void)k;(void)mode;(void)part;(void)face;CHECK(false);return NULL; }
#define setupInitialState old_setup
#define FoxHelper old_fox
#define DocHelper old_doc
#define ftCo_800BECB0 old_down
#define ftCo_800BED88 old_right
#define ftCo_800BEF04 old_star
#define ftCo_800BF034 old_camera
#define ftCo_800BF108 old_sleep
#include "presentation_original.h"
#undef setupInitialState
#undef FoxHelper
#undef DocHelper
#undef ftCo_800BECB0
#undef ftCo_800BED88
#undef ftCo_800BEF04
#undef ftCo_800BF034
#undef ftCo_800BF108
#include "presentation_current.h"
static unsigned bone_count(Fighter* f) { CHECK(f==&fighter); return 63; }
static void manual(Item_GObj* o) { CHECK(o==&object); bat.xDCC_flag.b3=false; }
static int32_t create(HSD_GObj* o,ItemKind kind,Vec3* pos,Vec3* vel,float face,
                      bool owner_origin,uint32_t group,float value,bool collision)
{
    CHECK(o==&source && !owner_origin && !collision && group==0 && value==1);
    SpawnItem spawn={0}; spawn.kind=kind; spawn.pos=spawn.prev_pos=*pos;
    spawn.vel=*vel; spawn.facing_dir=face; spawn.x0_parent_gobj=spawn.x4_parent_gobj2=o;
    Item_GObj* created=Item_80268B18(&spawn); if(!created) return -1;
    bat.xDD4_itemVar.native_article.creation_value=value;
    bat.xDD4_itemVar.native_article.attachment_owner=o;
    bat_logic->on_created(created); return (int32_t)bat.x1C;
}
static bool bind(HSD_GObj* o,uint32_t id,bool auxiliary)
{ CHECK(o==&source && id==bat.x1C && !auxiliary); fighter.item_gobj=&object; return true; }
static bool clear(HSD_GObj* o,bool auxiliary)
{
    CHECK(o==&source);
    if(auxiliary) fighter.x1984_heldItemSpec=NULL; else fighter.item_gobj=NULL;
    return true;
}
static bool signal(HSD_GObj* o,uint32_t id,uint32_t hash,float value)
{
    CHECK(o==&source && id==bat.x1C);
    for(unsigned i=0;i<bat_logic->signal_count;++i) if(bat_logic->signals[i].name_hash==hash) {
        bat_logic->signals[i].callback(&object,value); return true;
    } CHECK(false); return false;
}
static float model_scale(HSD_GObj* o) { CHECK(o==&source); return fighter.co_attrs.model_scaling; }
static HSD_GObj* creator(HSD_GObj* o) { CHECK(o==&object); return &source; }
static Vec3 attachment;
static FighterBone parts[63];
static void joint_position(HSD_JObj* joint,Vec3* offset,Vec3* position)
{ CHECK(joint==(HSD_JObj*)(uintptr_t)0x9876 && !offset); *position=attachment; }
static void constructor_matrix(NativeFighterHost* host)
{
    host->create_article_at=nativeArticle_Create; host->joint_position=joint_position;
    for(unsigned kind=0;kind<2;++kind) for(unsigned face=0;face<2;++face)
      for(unsigned height=0;height<3;++height) for(unsigned z=0;z<3;++z) {
        reset(); allocation_fails=true;
        fighter.facing_dir=face?-1:1; fighter.cur_pos=(Vec3){4,7,(float)z-1};
        fighter.coll_data.ecb.top.y=2+height; fighter.coll_data.ecb.bottom.y=height;
        attachment=(Vec3){-6,12,(float)z+3};
        fighter.parts=parts; parts[42].joint=(HSD_JObj*)(uintptr_t)0x9876;
        if(kind) CHECK(!it_802BE9D8(&source,fighter.facing_dir,&attachment,345));
        else CHECK(!it_802AD478(&source,&fighter.cur_pos,42,fighter.facing_dir));
        SpawnItem original=last_spawn; log_count=0;
        fighter.pre_hitlag_cb=NULL; fighter.post_hitlag_cb=NULL;
        fighter.x1984_heldItemSpec=&object;
        if(kind) {
            fighter.motion_id=345; script_fn_YoyoCreate(&source);
            CHECK(fighter.pre_hitlag_cb==script_fn_YoyoPause &&
                  fighter.post_hitlag_cb==script_fn_YoyoResume);
            CHECK(!fighter.x1984_heldItemSpec);
        } else script_fn_BatCreate(&source);
        CHECK(original.kind==last_spawn.kind && original.facing_dir==last_spawn.facing_dir);
        CHECK(original.x0_parent_gobj==last_spawn.x0_parent_gobj &&
              original.x4_parent_gobj2==last_spawn.x4_parent_gobj2);
        CHECK(!memcmp(&original.pos,&last_spawn.pos,sizeof(Vec3)) &&
              !memcmp(&original.prev_pos,&last_spawn.prev_pos,sizeof(Vec3)) &&
              !memcmp(&original.vel,&last_spawn.vel,sizeof(Vec3)));
        CHECK(original.x3C_damage==last_spawn.x3C_damage && original.x40==last_spawn.x40 &&
              original.x44_flag.b0==last_spawn.x44_flag.b0);
        // Positive control: owner-origin really differs for these source inputs.
        Vec3 origin=original.pos, zero={0};
        CHECK(nativeArticle_Create(&source,original.kind,&origin,&zero,
            fighter.facing_dir,true,0,0,!kind?false:true)==-1);
        CHECK(memcmp(&original.pos,&last_spawn.pos,sizeof(Vec3))!=0);
        ++constructor_pairs;
      }
    host->create_article_at=create;
}
static void presentation_matrix(void)
{
    HSD_GObjEvent old[]={old_down,old_right,old_star,old_camera,old_sleep};
    HSD_GObjEvent now[]={ftCo_800BECB0,ftCo_800BED88,ftCo_800BEF04,ftCo_800BF034,ftCo_800BF108};
    for(unsigned mode=0;mode<5;++mode) for(unsigned fail=0;fail<2;++fail)
      for(unsigned facing=0;facing<2;++facing) for(unsigned scale=0;scale<3;++scale) {
        reset(); allocation_fails=fail; generated_path=false;
        fighter.facing_dir=facing?-1:1; fighter.x34_scale.y=.5f+scale;
        fighter.item_gobj=(HSD_GObj*)(uintptr_t)0x1234;
        fighter.x1984_heldItemSpec=(HSD_GObj*)(uintptr_t)0x5678;
        old[mode](&source);
        int motion=fighter.motion_id; bool held=fighter.item_gobj==&object;
        HSD_GObj* auxiliary=fighter.x1984_heldItemSpec;
        unsigned flags=fighter.x2219_b2|(fighter.x2219_b1<<1)|(fighter.x2228_b1<<2);
        int commands[]={bat.xDAC_itcmd_var0,bat.xDB0_itcmd_var1,bat.xDB4_itcmd_var2,bat.xDB8_itcmd_var3};
        float scl=bat.scl; Vec3 position=bat.pos, velocity=bat.x40_vel;
        unsigned state=bat.msid, count=log_count; Call expected[64]; memcpy(expected,log_calls,sizeof(expected));
        reset(); generated_path=true; fighter.facing_dir=facing?-1:1; fighter.x34_scale.y=.5f+scale;
        fighter.item_gobj=(HSD_GObj*)(uintptr_t)0x1234;
        fighter.x1984_heldItemSpec=(HSD_GObj*)(uintptr_t)0x5678;
        now[mode](&source);
        CHECK(fighter.motion_id==motion && (fighter.item_gobj==&object)==held);
        CHECK(fighter.x1984_heldItemSpec==auxiliary);
        CHECK((fighter.x2219_b2|(fighter.x2219_b1<<1)|(fighter.x2228_b1<<2))==flags);
        CHECK(commands[0]==bat.xDAC_itcmd_var0 && commands[1]==bat.xDB0_itcmd_var1 &&
              commands[2]==bat.xDB4_itcmd_var2 && commands[3]==bat.xDB8_itcmd_var3);
        CHECK(bat.scl==scl && bat.msid==state);
        CHECK(!memcmp(&position,&bat.pos,sizeof(Vec3)) && !memcmp(&velocity,&bat.x40_vel,sizeof(Vec3)));
        CHECK(log_count==count && !memcmp(expected,log_calls,sizeof(expected))); ++presentation_pairs;
      }
}
static void ownership(void)
{
    const NativeFighterDefinition* base=&ftNessClone_Definition;
    CHECK(definition->motion_state_first==341 && definition->motion_state_count==36);
    unsigned phases=0;
    for(unsigned i=0;i<base->motion_state_count;++i) {
        const MotionState* original=&base->motion_states[i];
        const MotionState* owned=&definition->motion_states[i];
        HSD_GObjEvent a[]={original->anim_cb,original->input_cb,original->phys_cb,original->coll_cb};
        HSD_GObjEvent b[]={owned->anim_cb,owned->input_cb,owned->phys_cb,owned->coll_cb};
        for(unsigned p=0;p<4;++p) if(a[p]) {
            if(!b[p] || b[p]==a[p]) fprintf(stderr,"Unowned motion%d phase%u\n",base->motion_state_first+i,p);
            CHECK(b[p] && b[p]!=a[p]); ++phases;
        }
        CHECK(owned->cam_cb==original->cam_cb);
    }
    CHECK(phases==142 && definition->article_logic_count==11);
    const int empty[]={348,352,358,362,367,372};
    const HSD_GObjEvent empty_original[]={ftNs_SpecialNStart_IASA,ftNs_SpecialAirNStart_IASA,
        ftNs_SpecialHiStart_IASA,ftNs_SpecialAirHiStart_IASA,
        ftNs_SpecialLwStart_IASA,ftNs_SpecialAirLwStart_IASA};
    for(unsigned i=0;i<6;++i) {
        reset(); Fighter original=fighter;
        empty_original[i](&source); CHECK(!memcmp(&original,&fighter,sizeof(fighter)));
        definition->motion_states[empty[i]-341].input_cb(&source);
        CHECK(!memcmp(&original,&fighter,sizeof(fighter)));
    }
    // The whole-special authoring form requires these two empty callbacks over
    // original NULL slots. Assert their no-op behavior, not pointer equivalence.
    for(unsigned i=356;i<=357;++i) {
        CHECK(!base->motion_states[i-341].input_cb);
        reset(); Fighter original=fighter;
        CHECK(definition->motion_states[i-341].input_cb);
        definition->motion_states[i-341].input_cb(&source);
        CHECK(!memcmp(&original,&fighter,sizeof(fighter)));
    }
    HSD_GObjEvent entries[]={definition->special_n,definition->special_air_n,
        definition->special_s,definition->special_air_s,definition->special_hi,
        definition->special_air_hi,definition->special_lw,definition->special_air_lw,
        definition->on_load,definition->on_load_special_attrs,definition->on_death,
        definition->on_absorb,definition->on_item_invisible,definition->on_item_visible,
        definition->on_knockback_enter,definition->on_knockback_exit};
    for(unsigned i=0;i<sizeof(entries)/sizeof(*entries);++i) CHECK(entries[i] && entries[i]!=stock_slot);
    CHECK(definition->on_item_pickup_ext && definition->on_item_drop_ext &&
          definition->on_item_pickup && definition->on_item_drop);
    CHECK(!definition->on_death1 && !definition->on_user_data_remove && !definition->neutral_article);
    CHECK(definition->bone_count==51 && definition->joint_count==63 && definition->article_asset_count==11);
    CHECK(definition->demo_motion_names==base->demo_motion_names && definition->demo_count==base->demo_count);
    printf("Ness ownership: %u non-NULL stock gameplay phases replaced; all8 entries and original non-NULL lifecycle slots supplied\n",phases);
}
int main(int argc,char** argv)
{
    CHECK(argc==2); NativeFighterHost host={0};
#include "required_host.h"
    host.callback_source_alive=alive; host.ground_to_air=ftCommon_8007D5D4;
    host.begin_aerial_jump=ftCo_JumpAerial_Begin; host.common_attribute=common_attribute;
    host.air_accel_from=ftCommon_CalcSelfAccel_DriftFrom;
    host.animation_velocity_y_adjusted=ft_800851D0;
    host.create_article_at=create; host.bind_fighter_item=bind; host.clear_fighter_item=clear;
    host.signal_article=signal; host.fighter_model_scale=model_scale; host.bone_count=bone_count;
    host.item_use_manual_lifetime=manual; host.item_attach_to_owner=Item_8026AB54;
    host.item_change_state_flags=Item_80268E5C; host.item_advance_animation=Item_802694CC;
    host.item_scale_by_attribute=it_8026BAE8; host.item_creator=creator;
    definition=FreakMelee_GetFighterModule(&ftNessClone_Definition,&host); CHECK(definition);
    for(unsigned i=0;i<definition->article_logic_count;++i)
        if(definition->article_logics[i].kind==It_Kind_Ness_Bat) bat_logic=&definition->article_logics[i];
    CHECK(bat_logic);
    FILE* file=fopen(argv[1],"rb"); CHECK(file);
    char text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY+1]; size_t size=fread(text,1,sizeof(text)-1,file);
    CHECK(feof(file)&&!ferror(file)); fclose(file); NativeFighterParameters p;
    CHECK(nativeFighterParameters_Parse(text,size,&p,NULL)); uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for(unsigned i=0;i<p.count;++i) values[i]=p.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(values,p.count,p.schema_hash));
    ownership(); jump_matrix(); presentation_matrix(); constructor_matrix(&host);
    printf("Ness composed common: %u checks, 0 failures, %u jump pairs, %u presentation pairs, %u constructor pairs\n",
           checks,jump_pairs,presentation_pairs,constructor_pairs); return 0;
}
