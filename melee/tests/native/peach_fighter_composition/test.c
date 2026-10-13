#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftDonkey/forward.h>
#include <melee/ft/kinds/ftKirby/forward.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ef/native_model_effect.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/lb_00F9.h>
#include <melee/mp/mplib.h>
#include <melee/it/it_26B1.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,pairs;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"pair %u line %d: %s\n",pairs,__LINE__,#x);exit(1);} } while(0)
typedef struct PolicyEvent { int kind,a,b;float value[4]; } PolicyEvent;
static PolicyEvent events[64],expected[64];static unsigned count,expected_count;
static void record(int kind,int a,int b,float x,float y,float z,float w)
{CHECK(count<64);events[count++]=(PolicyEvent){kind,a,b,{x,y,z,w}};}
static void save_events(void){expected_count=count;memcpy(expected,events,sizeof(events));count=0;}
static void compare_events(void){CHECK(count==expected_count);CHECK(memcmp(events,expected,count*sizeof(events[0]))==0);}
static Fighter fighter,target;static HSD_GObj source,*heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static ftData data;static ftPe_DatAttrs attributes,backup;static Article* assets[5];
static ftCommonData common;ftCommonData* p_ftCommonData=&common;
static bool heavy,segment_hit,floor_hit,swinging,rebound_allowed;static Item_HoldKinds hold_kind;
static float clip_frames[2];static int jump_motion;static Vec3 jump_velocity;
static const NativeFighterDefinition* definition;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return definition;}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* id){(void)g;*id=0;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t id){(void)g;(void)id;return NULL;}
static void unexpected_boundary(void){fprintf(stderr,"Uncontrolled composition boundary invoked\n");exit(1);}
void ftParts_80074A4C(HSD_GObj* g,int group,int variant){CHECK(g==&source);record(1,group,variant,0,0,0,0);}
FigaTree* ftData_80085E50(Fighter* f,enum_t index){CHECK(f==&fighter&&index>=18&&index<=19);return (FigaTree*)(uintptr_t)(index+1);}
float lbAnim_8001E8F8(FigaTree* clip){int index=(int)(uintptr_t)clip-1;CHECK(index==18||index==19);record(2,index,0,clip_frames[index-18],0,0,0);return clip_frames[index-18];}
void it_8026B3F8(Article* asset,s32 kind){int i;for(i=0;i<5&&asset!=assets[i];i++) {}CHECK(i<5);record(3,i,(int)kind,0,0,0,0);}
bool itIsHeavy(Item_GObj* g){(void)g;return heavy;}
Item_HoldKinds itGetHoldKind(Item_GObj* g){(void)g;return hold_kind;}
void ftAnim_80070FB4(Fighter_GObj* g,s32 flag,s32 type){CHECK(g==&source);record(4,(int)flag,(int)type,0,0,0,0);}
void ftAnim_80070C48(Fighter_GObj* g,s32 flag){CHECK(g==&source);record(5,(int)flag,0,0,0,0,0);}
void ftAnim_80070CC4(Fighter_GObj* g,int flag){CHECK(g==&source);record(6,flag,0,0,0,0,0);}
void ftAnim_800704F0(Fighter_GObj* g,int index,float frame){CHECK(g==&source);record(7,index,0,frame,0,0,0);}
void ftCommon_8007D5D4(Fighter* f){CHECK(f==&fighter);f->ground_or_air=GA_Air;record(8,0,0,0,0,0,0);}
void ftCo_JumpAerial_Begin(Fighter_GObj* g,FtMotionId motion,Vec3* velocity,bool a,bool b,bool c)
{CHECK(g==&source);CHECK(fighter.cmd_vars[0]==1);jump_motion=motion;jump_velocity=*velocity;record(9,motion,a|(b<<1)|(c<<2),velocity->x,velocity->y,velocity->z,0);}
bool mpCheckAll(Vec3* pos,int* line,u32* flags,Vec3* normal,int skip,int only,float x0,float y0,float x1,float y1)
{(void)pos;(void)line;(void)flags;(void)normal;CHECK(skip==-1&&only==-1);record(10,0,0,x0,y0,x1,y1);return segment_hit;}
bool ftCo_800A0FB0(Vec3* pos,int* line,u32* flags,Vec3* normal,int a,int b,int c,float x0,float y0,float x1,float y1,float extra)
{(void)pos;(void)line;(void)flags;(void)normal;CHECK(a==-1&&b==-1&&c==-1&&extra==0);record(11,0,0,x0,y0,x1,y1);return floor_hit;}
bool ftCo_800A5A90(Fighter* f){CHECK(f==&fighter);return swinging;}
void lb_8001044C(DynamicsDesc* desc,UNK_T a,int b,float pos,bool arg,Fighter_Part part,int start,bool ret)
{(void)desc;(void)a;(void)b;record(12,part,start,pos,arg,ret,0);}
#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
static void copy_attributes(Fighter_GObj* g){CHECK(g==&source);PUSH_ATTRS(&fighter,ftPe_DatAttrs);}
static void reload_attributes(Fighter_GObj* g){ftPe_Init_LoadSpecialAttrs(g);}
static float common_attribute(NativeFighterCommonAttribute kind){CHECK(kind==NativeFighterCommon_AerialJumpReverse);return common.x78;}
static const NativeFighterStateField* field(const char* name)
{for(unsigned i=0;i<definition->state_field_count;i++)if(strcmp(definition->state_fields[i].name,name)==0)return &definition->state_fields[i];CHECK(false);return NULL;}
static int32_t read_int(const char* name){int32_t value=0;const NativeFighterStateField* f=field(name);memcpy(&value,fighter.native_module_state+f->offset,f->type==NativeFighterState_Bool?1:4);return value;}
static float read_float(const char* name){float value;memcpy(&value,fighter.native_module_state+field(name)->offset,4);return value;}
static void reset(void)
{memset(&fighter,0,sizeof(fighter));fighter.x5AC.model_num=7;fighter.gobj=&source;fighter.kind=Ft_Kind_Peach;fighter.x8_spawnNum=UINT32_C(0xfedcba98);source.user_data=&fighter;heads[HSD_GOBJ_PLINK_FIGHTER]=&source;fighter.ft_data=&data;fighter.dat_attrs_backup=&backup;fighter.dat_attrs=&backup;data.ext_attr=&attributes;data.x48_items=(UNK_T*)assets;count=0;}
int main(void)
{
    MotionState motions[30]={0};NativeFighterDefinition base={.source_kind=Ft_Kind_Peach,.fighter_root="ftDataPeach",.motion_state_first=341,.motion_state_count=30,.motion_states=motions,.joint_count=114};NativeFighterHost host={0};
#include "hosts.inc"
    host.callback_source_alive=nativeCallbackSource_Alive;host.animation_clip=ftData_80085E50;host.clip_length=lbAnim_8001E8F8;host.copy_ext_attributes=copy_attributes;host.reload_ext_attributes=reload_attributes;host.set_visibility_group=ftParts_80074A4C;
    host.fighter_item_pickup=Fighter_OnItemPickup;host.fighter_item_drop=Fighter_OnItemDrop;host.fighter_item_visible=Fighter_OnItemVisible;host.fighter_item_invisible=Fighter_OnItemInvisible;host.fighter_knockback_enter=Fighter_OnKnockbackEnter;host.fighter_knockback_exit=Fighter_OnKnockbackExit;
    host.ground_to_air=ftCommon_8007D5D4;host.begin_aerial_jump=ftCo_JumpAerial_Begin;host.common_attribute=common_attribute;host.cpu_ranged_unsafe=ftCo_CpuRangedAttackUnsafe;host.cpu_add_candidate=ftCo_CpuAddAttackCandidate;
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/4,parameter_schema));definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);CHECK(definition->source_kind==Ft_Kind_Peach);CHECK(definition->state_bytes==57&&definition->state_field_count==14);CHECK(definition->motion_state_count==30);CHECK(definition->article_logic_count==3);CHECK(definition->callback_export_count==7);
    for(unsigned i=0;i<30;i++)CHECK(memcmp(&definition->motion_states[i],&motions[i],sizeof(MotionState))==0);
    CHECK(!FreakMelee_SetFighterParameters(parameter_bits,39,parameter_schema));CHECK(!FreakMelee_SetFighterParameters(parameter_bits,40,parameter_schema+1));
    NativeFighterHost missing=host;missing.cpu_ranged_unsafe=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    NativeFighterDefinition invalid=base;invalid.motion_state_count=1;CHECK(!FreakMelee_GetFighterModule(&invalid,&host));invalid=base;invalid.state_bytes=200;CHECK(!FreakMelee_GetFighterModule(&invalid,&host));
    CHECK(It_Kind_Peach_Explode==98&&It_Kind_Peach_Turnip==99&&It_Kind_Peach_Parasol==103&&It_Kind_Peach_Toad==104&&It_Kind_Peach_ToadSpore==111);
    for(unsigned i=0;i<definition->state_field_count;i++) { const NativeFighterStateField* f=&definition->state_fields[i];CHECK(f->count==1&&!f->array&&f->offset+f->stride<=definition->state_bytes);for(unsigned j=0;j<i;j++)CHECK(strcmp(f->name,definition->state_fields[j].name)!=0); }
    const NativeFighterArticleLogic* turnip=NULL;for(unsigned i=0;i<definition->article_logic_count;i++)if(definition->article_logics[i].kind==It_Kind_Peach_Turnip)turnip=&definition->article_logics[i];CHECK(turnip&&turnip->state_count==5);
    for(unsigned i=0;i<5;i++) {CHECK(turnip->logic->states[i].animated);CHECK(turnip->logic->states[i].physics_updated);CHECK((turnip->logic->states[i].collided!=NULL)==(i==2||i==3));}
    // Actual reset vs owned variable projection, including costume-specific group order.
    for(int costume=0;costume<5;costume++)for(int prior=0;prior<2;prior++){
        reset();fighter.x619_costume_id=costume;fighter.u.pe.has_float=prior;fighter.u.pe.specialairn_used=prior;fighter.u.pe.attacks4_motion_id=369;ftPe_Init_OnDeath(&source);save_events();definition->on_death(&source);compare_events();
        CHECK(read_int("has_float")==fighter.u.pe.has_float);CHECK(read_int("last_smash_motion")==fighter.u.pe.attacks4_motion_id);CHECK(read_int("toad_air_boost_used")==fighter.u.pe.specialairn_used);CHECK(read_int("parasol_handle")==-1&&read_int("saved_handle")==-1&&read_int("toad_handle")==-1&&read_int("vegetable_handle")==-1);pairs++;
    }
    for(int value=0;value<8;value++){
        reset();clip_frames[0]=13.5f+value;clip_frames[1]=31.75f-value;for(int i=0;i<5;i++)assets[i]=(Article*)(uintptr_t)(128+i);ftPe_Init_OnLoad(&source);CHECK(count==7);CHECK(events[0].a==18&&events[1].a==19);float first=backup.floatfallf_anim_start,second=backup.floatfallb_anim_start;count=0;definition->on_load(&source);CHECK(count==2);CHECK(events[0].a==18&&events[1].a==19);CHECK(read_float("float_fall_forward_end")==first&&read_float("float_fall_backward_end")==second);CHECK(fighter.dat_attrs==&backup);pairs++;
    }
    for(int heavy_value=0;heavy_value<2;heavy_value++)for(int hold=0;hold<=5;hold++)for(int flag=0;flag<2;flag++){
        reset();heavy=heavy_value;hold_kind=(Item_HoldKinds)hold;
#define PRESENT(old,member,arg) do { count=0;old(&source,arg);save_events();definition->member(&source,arg);compare_events();pairs++; } while(0)
        PRESENT(ftPe_Init_OnItemPickup,on_item_pickup,flag);PRESENT(ftPe_Init_OnItemPickup,on_item_pickup_ext,flag);PRESENT(ftPe_Init_OnItemDrop,on_item_drop,flag);PRESENT(ftPe_Init_OnItemDrop,on_item_drop_ext,flag);
#define VISUAL(old,member) do { count=0;old(&source);save_events();definition->member(&source);compare_events();pairs++; } while(0)
        VISUAL(ftPe_Init_OnItemInvisible,on_item_invisible);VISUAL(ftPe_Init_OnItemVisible,on_item_visible);VISUAL(ftPe_Init_OnKnockbackEnter,on_knockback_enter);VISUAL(ftPe_Init_OnKnockbackExit,on_knockback_exit);
    }
    const float sticks[]={-1,-0.3f,-0.25f,0,0.25f,0.3f,1};
    for(int face=-1;face<=1;face+=2)for(unsigned i=0;i<sizeof(sticks)/sizeof(sticks[0]);i++)for(int scale=0;scale<3;scale++){
        reset();fighter.facing_dir=face;fighter.input.lstick[0].x=sticks[i];fighter.co_attrs.air_jump_h_multiplier=0.75f+scale;common.x78=0.25f;ftPe_JumpAerial_Enter(&source);int old_motion=jump_motion;Vec3 old_velocity=jump_velocity;save_events();fighter.cmd_vars[0]=0;fighter.ground_or_air=GA_Ground;definition->on_aerial_jump(&source);compare_events();CHECK(jump_motion==old_motion&&memcmp(&old_velocity,&jump_velocity,sizeof(Vec3))==0);CHECK(fighter.ground_or_air==GA_Air);pairs++;
    }
    for(int motion=0;motion<=380;motion++){
        reset();fighter.motion_id=motion;bool attacking=ftCo_800B630C(&fighter);if(motion<341)continue;CHECK(definition->on_cpu_attack_motion(&source)==attacking);pairs++;
    }
    for(int motion=0;motion<=380;motion++){reset();fighter.motion_id=motion;original_peach_rebound(&fighter);CHECK(definition->on_body_rebound(&source)==rebound_allowed);pairs++;}
    for(int face=-1;face<=1;face+=2)for(int has_target=0;has_target<2;has_target++)for(int segment=0;segment<2;segment++)for(int floor=0;floor<2;floor++)for(int queue=0;queue<=8;queue++){
        reset();fighter.cur_pos=(Vec3){12.5f,-24.5f,0};fighter.facing_dir=face;target.cur_pos=(Vec3){-17,80,0};fighter.cpu.x44=has_target?&target:NULL;fighter.cpu.xEC=queue;segment_hit=segment;floor_hit=floor;original_peach_ranged(&fighter);u8 length=fighter.cpu.xEC;unsigned char candidates[sizeof(fighter.cpu.xCC_array)];memcpy(candidates,fighter.cpu.xCC_array,sizeof(candidates));save_events();fighter.cpu.xEC=queue;memset(fighter.cpu.xCC_array,0,sizeof(fighter.cpu.xCC_array));definition->on_cpu_ranged_candidates(&source);compare_events();CHECK(fighter.cpu.xEC==length&&memcmp(candidates,fighter.cpu.xCC_array,sizeof(candidates))==0);pairs++;
    }
    for(unsigned n=1;n<=Ft_Dynamics_NumMax;n++){
        reset();for(unsigned i=0;i<n;i++)fighter.dynamic_bone_sets[i].bone_id=i;original_peach_dynamics(&fighter,fighter.dynamic_bone_sets,n,true,true);save_events();for(unsigned i=0;i<n;i++)lb_8001044C(&fighter.dynamic_bone_sets[i].dyn_desc,&fighter.x1670,fighter.x166C,fighter.cur_pos.y,true,i,definition->on_dynamics_interaction_start(&source,i,n),true);compare_events();pairs++;
    }
    printf("%u original/generated comparisons, %u checks\n",pairs,checks);return 0;
}
