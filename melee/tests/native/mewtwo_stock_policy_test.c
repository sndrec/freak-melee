#include "../../tools/native_fighter_script_state.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include "native_fighter_script_code.h"
#include <melee/ft/kinds/ftMewtwo/ftmewtwospecialn.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwo.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/kinds/ftMewtwo/types.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/it/item.h>
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/itmewtwoshadowball.h>
#include <melee/it/kinds/inlines.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* Bind)(const uint32_t*, unsigned, uint64_t);
enum { Joint=1, Origin, Spawn, Change, Lifetime, Scale, Manual, Sound, Random, Attach, Queue, QueueArg, Knockback, AirTransition, MotionChange, JumpNotify, JumpVoice, ArticleRegister, TurnRoot };
typedef struct ShadowEvent { int op,a,b; float x,y,z; } ShadowEvent;
typedef struct Trace { ShadowEvent events[64]; unsigned count; } Trace;
static Trace traces[2];
static unsigned lane, comparisons, failures, random_cursor;
static Fighter fighters[2];
static HSD_GObj owners[2], objects[2];
static Item items[2];
static FighterBone parts[2][68];
static HSD_JObj joints[2][68];
static ftMewtwoAttributes fighter_attributes;
static itMewtwoShadowball_DatAttrs item_attributes;
#include "mewtwo_stock_parts.generated.h"
static FighterPartsTable stock_parts;
static FighterPartsTable* parts_tables[32];
FighterPartsTable** ftPartsTable=parts_tables;
static Article article;
static ftData fighter_data;
static FtSFX fighter_sfx;
static ftCommonData common_data;
ftCommonData* p_ftCommonData=&common_data;
static unsigned char attributes_backup[2][FT_CHARACTER_EXT_ATTR_CAPACITY];
static Article* item_list[2];
void stock_original_screw(Fighter_GObj*,Vec3*);
void stock_original_multijump(Fighter_GObj*);
void ft_800D2E7C(Fighter_GObj*,Vec3*);
void ftCo_800D74A4(Fighter_GObj*);
void stock_original_mt_jump(Fighter_GObj*);
void stock_original_aerial_begin(Fighter_GObj*,FtMotionId,Vec3*,bool);
static bool spawn_success=true;
static float model_scale=1;
static const NativeFighterArticleLogic* logic;
static ScriptStateTable fighter_layout={.bytes=1}, article_layout;
static const NativeFighterDefinition* module;
static const float random_values[]={.125f,.875f};

void __assert(char* file,u32 line,char* message)
{ fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,message);abort(); }
static void unused_host(void) { fputs("Unexpected host operation\n",stderr);abort(); }
static void record(int op,int a,int b,float x,float y,float z)
{ if(traces[lane].count==64) abort();traces[lane].events[traces[lane].count++]=(ShadowEvent){op,a,b,x,y,z}; }
static int field_offset(const ScriptStateTable* table,const char* name)
{ const ScriptStateField* field=nativeFighterScript_StateField(table,name);if(!field) abort();return field->offset; }
static void set_fighter_int(const char* name,int value)
{ memcpy(fighters[1].native_module_state+field_offset(&fighter_layout,name),&value,4); }
static void set_fighter_bool(const char* name,bool value)
{ fighters[1].native_module_state[field_offset(&fighter_layout,name)]=(uint8_t)value; }
static int article_int(const char* name)
{ int value;memcpy(&value,items[1].xDD4_itemVar.native_article.state+field_offset(&article_layout,name),4);return value; }
static float article_float(const char* name)
{ float value;memcpy(&value,items[1].xDD4_itemVar.native_article.state+field_offset(&article_layout,name),4);return value; }
static bool article_bool(const char* name)
{ return items[1].xDD4_itemVar.native_article.state[field_offset(&article_layout,name)]!=0; }
static void check(bool valid,const char* phase,const char* property)
{ ++comparisons;if(!valid){++failures;fprintf(stderr,"%s: %s differs\n",phase,property);} }
static bool equal_float(float a,float b) { return memcmp(&a,&b,4)==0; }
static void same_float(const char* phase,const char* name,float value)
{ check(equal_float(value,article_float(name)),phase,name); }
static void same_int(const char* phase,const char* name,int value)
{ check(value==article_int(name),phase,name); }
static void compare_trace(const char* phase)
{
    bool same=traces[0].count==traces[1].count && memcmp(traces[0].events,traces[1].events,traces[0].count*sizeof(ShadowEvent))==0;
    check(same,phase,"ordered engine events");
    if(!same) for(unsigned s=0;s<2;++s)for(unsigned i=0;i<traces[s].count;++i){ShadowEvent* e=&traces[s].events[i];fprintf(stderr,"lane%u event%u op%d %d %d %.9g %.9g %.9g\n",s,i,e->op,e->a,e->b,e->x,e->y,e->z);}
}
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result)
{ (void)offset;*result=joint->translate;record(Joint,(int)(joint-joints[lane]),0,result->x,result->y,result->z); }
void it_8026BB68(HSD_GObj* owner,Vec3* result)
{ *result=((Fighter*)owner->user_data)->cur_pos;record(Origin,0,0,result->x,result->y,result->z); }
Item_GObj* Item_80268B18(SpawnItem* spawn)
{
    record(Spawn,spawn->kind,spawn->x40,spawn->prev_pos.x,spawn->prev_pos.y,spawn->prev_pos.z);
    check(spawn->x0_parent_gobj==&owners[lane] && spawn->x4_parent_gobj2==&owners[lane] && spawn->x44_flag.b0 && spawn->x3C_damage==0,"constructor","parent/flags");
    check(spawn->vel.x==0 && spawn->vel.y==0 && spawn->vel.z==0,"constructor","spawn velocity");
    if(!spawn_success) return NULL;
    Item* item=&items[lane];item->owner=&owners[lane];item->pos=spawn->pos;item->x40_vel=spawn->vel;item->facing_dir=spawn->facing_dir;
    item->kind=spawn->kind;item->x1C=73;item->xC4_article_data=&article;
    return &objects[lane];
}
void Item_80268E5C(HSD_GObj* object,enum_t state,Item_StateChangeFlags flags)
{ ((Item*)object->user_data)->msid=state;record(Change,state,flags,0,0,0); }
void it_80275158(HSD_GObj* object,float value)
{ ((Item*)object->user_data)->xD44_lifeTimer=value;record(Lifetime,0,0,value,0,0); }
float ftLib_800869D4(HSD_GObj* owner)
{ (void)owner;record(Scale,0,0,model_scale,0,0);return model_scale; }
void it_8026B3A8(HSD_GObj* object)
{ ((Item*)object->user_data)->xDC8_word.flags.x15=false;record(Manual,0,0,0,0,0); }
void ft_PlaySFX(Fighter* fighter,enum_t sound,u8 volume,u8 pan)
{ (void)fighter;record(Sound,sound,volume,(float)pan,0,0); }
float HSD_Randf(void)
{ if(random_cursor==2)abort();float value=random_values[random_cursor++];record(Random,0,0,value,0,0);return value; }
void Item_8026AB54(HSD_GObj* object,HSD_GObj* owner,Fighter_Part part)
{ Item* item=object->user_data;item->owner=owner;item->xDC8_word.flags.x13=true;record(Attach,part,0,0,0,0); }
static unsigned bone_count(Fighter* fighter) { (void)fighter;return 68; }
static Fighter_GObj* item_creator(Item_GObj* object)
{ if(object!=&objects[lane])abort();return &owners[lane]; }
static void change_state(HSD_GObj* object,int state) { Item_80268E5C(object,state,ITEM_ANIM_UPDATE); }
static int32_t create_article(HSD_GObj* owner,ItemKind kind,Vec3* previous,Vec3* velocity,float facing,bool origin_at_owner,uint32_t group,float value,bool initial_collision)
{
    SpawnItem spawn={0};spawn.kind=kind;Item_InitSpawn(&spawn,owner,previous,facing);spawn.x40=group;spawn.x44_flag.b0=initial_collision;
    spawn.vel=*velocity;
    if(origin_at_owner)it_8026BB68(owner,&spawn.pos);
    HSD_GObj* object=Item_80268B18(&spawn);if(!object)return -1;
    Item* item=object->user_data;item->xDD4_itemVar.native_article.attachment_owner=owner;
    item->xDD4_itemVar.native_article.creation_value=value;item->xDD4_itemVar.native_article.attachment_facing=facing;
    item->xDD4_itemVar.native_article.launch_origin=spawn.pos;
    logic->on_created(object);return item->x1C;
}
void ftCo_800B463C(Fighter* fighter,u8 command)
{ (void)fighter;record(Queue,command,0,0,0,0); }
void ftCo_800B46B8(Fighter* fighter,u8 command,u8 arg)
{ (void)fighter;record(QueueArg,command,arg,0,0,0); }
void ftAnim_800704F0(HSD_GObj* owner,int part,float frame)
{ (void)owner;record(Knockback,part,0,frame,0,0); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ (void)kind;return kind==Ft_Kind_CustomFirst?module:NULL; }
#include "mewtwo_cpu_reference.generated.h"

void stock_ground_to_air(Fighter*);
void ftCommon_8007D5D4(Fighter* fighter)
{ stock_ground_to_air(fighter);record(AirTransition,0,0,0,0,0); }
void ftCo_JumpAerial_Phys_Cb(Fighter_GObj* gobj) { (void)gobj; }
void Fighter_ChangeMotionState(Fighter_GObj* gobj,FtMotionId motion,MotionFlags flags,float start,float speed,float blend,Fighter_GObj* donor)
{ Fighter* fighter=gobj->user_data;fighter->motion_id=motion;fighter->phys_cb=NULL;record(MotionChange,motion,flags,start,speed,blend);check(donor==NULL,"aerial jump","no animation donor"); }
void ft_80088770(Fighter* fighter)
{ record(JumpNotify,fighter->x1968_jumpsUsed,fighter->x671_timer_lstick_tilt_y,fighter->self_vel.x,fighter->self_vel.y,fighter->self_vel.z); }
void ft_80088328(Fighter* fighter,int sound,u8 volume,u8 pan)
{ (void)fighter;record(JumpVoice,sound,volume,pan,0,0); }
static float common_attribute(NativeFighterCommonAttribute attribute)
{ if(attribute!=NativeFighterCommon_AerialJumpReverse)abort();return common_data.x78; }

void it_8026B3F8(Article* asset,s32 kind)
{ record(ArticleRegister,kind,(asset==item_list[1]),0,0,0); }
static void copy_ext_attributes(Fighter_GObj* gobj)
{ Fighter* fighter=gobj->user_data;memcpy(fighter->dat_attrs_backup,fighter->ft_data->ext_attr,FT_CHARACTER_EXT_ATTR_CAPACITY);fighter->dat_attrs=fighter->dat_attrs_backup; }
static void reload_ext_attributes(Fighter_GObj* gobj)
{ Fighter* fighter=gobj->user_data;memcpy(fighter->dat_attrs,fighter->ft_data->ext_attr,FT_CHARACTER_EXT_ATTR_CAPACITY); }
s32 ftKb_SpecialN_800F1CD8(Fighter_GObj* gobj) { (void)gobj;abort(); }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint)
{ joint->flags|=JOBJ_MTX_DIRTY;record(TurnRoot,0,0,joint->rotate.x,joint->rotate.y,joint->rotate.z); }
static void compare_jump(const char* phase)
{
    Fighter* stock=&fighters[0];Fighter* script=&fighters[1];compare_trace(phase);
    check(stock->motion_id==script->motion_id,phase,"selected motion");
    check(memcmp(&stock->self_vel,&script->self_vel,sizeof(Vec3))==0,phase,"exact velocity");
    check(stock->ground_or_air==script->ground_or_air,phase,"air transition");
    check(stock->cmd_vars[0]==script->cmd_vars[0],phase,"command 0");
    check(stock->x2221_b7==script->x2221_b7,phase,"aerial animation flag");
    check(stock->x671_timer_lstick_tilt_y==script->x671_timer_lstick_tilt_y,phase,"input timer");
    check(stock->x1968_jumpsUsed==script->x1968_jumpsUsed,phase,"jump count");
    check(stock->phys_cb==script->phys_cb,phase,"animation physics callback");
}
static void reset(float facing)
{
    memset(traces,0,sizeof(traces));memset(fighters,0,sizeof(fighters));memset(items,0,sizeof(items));
    for(unsigned i=0;i<2;++i){owners[i].user_data=&fighters[i];objects[i].user_data=&items[i];
        fighters[i].gobj=&owners[i];fighters[i].ft_data=&fighter_data;fighters[i].dat_attrs_backup=attributes_backup[i];fighters[i].kind=Ft_Kind_Mewtwo;fighters[i].parts=parts[i];fighters[i].dat_attrs=&fighter_attributes;
        fighters[i].facing_dir=facing;fighters[i].cur_pos=(Vec3){-30,14,2};fighters[i].x34_scale.y=model_scale;items[i].entity=&objects[i];
        for(unsigned j=0;j<68;++j){parts[i][j].joint=&joints[i][j];joints[i][j].flags=0;joints[i][j].rotate=(Quaternion){0};}items[i].xDC8_word.flags.x15=true;}
}
static void compare_constructor(const char* phase)
{
    Item* stock=&items[0];Item* script=&items[1];itMewtwoShadowball_ItemVars* vars=&stock->xDD4_itemVar.mewtwoshadowball;
    check(stock->msid==script->msid,phase,"article state");check(memcmp(&stock->pos,&script->pos,sizeof(Vec3))==0,phase,"origin position");
    check(memcmp(&stock->x40_vel,&script->x40_vel,sizeof(Vec3))==0,phase,"launch velocity");
    check(stock->facing_dir==script->facing_dir,phase,"facing");check(stock->xD44_lifeTimer==script->xD44_lifeTimer,phase,"lifetime");
    check(stock->xDC8_word.flags.x15==script->xDC8_word.flags.x15,phase,"manual lifetime");
    check((vars->x14!=0)==article_bool("fired"),phase,"fired");
    same_float(phase,"angle",vars->x4.x);same_float(phase,"speed",vars->x4.y);same_float(phase,"size",vars->x10);
    same_int(phase,"charge",vars->x18);same_int(phase,"maximum",vars->x1C);same_int(phase,"accessory_step",vars->x20);same_int(phase,"damage",vars->x24);
    same_float(phase,"wobble_y",vars->x30.y);same_float(phase,"wobble_z",vars->x30.z);
    same_float(phase,"wobble_angle",vars->x3C);same_float(phase,"wobble_speed",vars->x40);same_float(phase,"wobble_acceleration",vars->x44);
    same_int(phase,"wobble_timer",vars->x48);same_int(phase,"wobble_steps",vars->x4C);same_float(phase,"charge_fraction",vars->x50);same_float(phase,"base_scale",vars->x54);
    same_float(phase,"rest_x",vars->x58.x);same_float(phase,"rest_y",vars->x58.y);same_float(phase,"rest_z",vars->x58.z);
}
static char* read_file(const char* path,size_t* size)
{ FILE* file=fopen(path,"rb");if(!file)abort();fseek(file,0,SEEK_END);long length=ftell(file);rewind(file);char* data=malloc(length+1);if(!data)abort();*size=fread(data,1,length,file);data[*size]=0;fclose(file);return data; }
static void parse_layout(char* source)
{
    bool shadow=false;char* cursor=source;
    while(*cursor){char* end=strchr(cursor,'\n');if(end){*end=0;if(end>cursor&&end[-1]=='\r')end[-1]=0;}
        if(strncmp(cursor,"article ",8)==0)shadow=strncmp(cursor,"article It_Kind_Mewtwo_ShadowBall ",33)==0;
        if(strncmp(cursor,"state bool ",11)==0 || strncmp(cursor,"state int ",10)==0 || strncmp(cursor,"state float ",12)==0){if(!nativeFighterScript_AddStateField(&fighter_layout,cursor))abort();}
        else if(shadow && (strncmp(cursor,"    state bool ",15)==0 || strncmp(cursor,"    state int ",14)==0 || strncmp(cursor,"    state float ",16)==0)){if(!nativeFighterScript_AddStateField(&article_layout,cursor+4))abort();}
        if(!end)break;cursor=end+1;
    }
}
static float parameter_float(const NativeFighterParameters* parameters,const char* name)
{ for(unsigned i=0;i<parameters->count;++i)if(strcmp(parameters->fields[i].name,name)==0){float result;memcpy(&result,&parameters->fields[i].bits,4);return result;}abort(); }
static int parameter_int(const NativeFighterParameters* parameters,const char* name)
{ for(unsigned i=0;i<parameters->count;++i)if(strcmp(parameters->fields[i].name,name)==0)return parameters->fields[i].bits;abort(); }
int main(int argc,char** argv)
{
    if(argc!=4)return 2;size_t size;char* text=read_file(argv[3],&size);parse_layout(text);free(text);
    text=read_file(argv[2],&size);NativeFighterParameters parameters;unsigned line;
    if(!nativeFighterParameters_Parse(text,size,&parameters,&line))return 2;free(text);
    HMODULE library=LoadLibraryA(argv[1]);if(!library)return 2;
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");Bind bind=(Bind)GetProcAddress(library,"FreakMelee_SetFighterParameters");
    uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<parameters.count;++i)bits[i]=parameters.fields[i].bits;
    if(!entry||!bind||!bind(bits,parameters.count,parameters.schema_hash))return 2;
    MotionState motions[20]={0};NativeFighterDefinition base={.code_key="mewtwo_clone",.source_kind=Ft_Kind_Mewtwo,.animation_source_kind=Ft_Kind_Mewtwo,.fighter_root="ftDataMewtwo",.costume_joint="PlyMewtwo5K_Share_joint",.costume_matanim="PlyMewtwo5K_Share_matanim_joint",.action_count=314,.demo_count=14,.joint_count=68,.motion_state_first=341,.motion_state_count=20,.motion_states=motions};
    const int ground[]={341,351,353,359},air[]={346,352,356,360};
    for(unsigned i=0;i<4;++i){base.special_ground_state[i]=ground[i];base.special_air_state[i]=air[i];}
    NativeFighterHost host={0};
#include "mewtwo_required_host.generated.h"
    host.joint_position=lb_8000B1CC;host.bone_count=bone_count;host.create_article_at=create_article;
    host.item_creator=item_creator;
    host.item_change_state=change_state;host.item_change_state_flags=Item_80268E5C;host.item_set_lifetime=it_80275158;
    host.fighter_model_scale=ftLib_800869D4;host.item_use_manual_lifetime=it_8026B3A8;host.item_attach_to_owner=Item_8026AB54;
    host.play_sound=ft_PlaySFX;host.random_float=HSD_Randf;host.angle_atan2_float=atan2f;host.cos_float=cosf;host.sin_float=sinf;
    host.begin_aerial_jump=ftCo_JumpAerial_Begin;host.common_attribute=common_attribute;host.ground_to_air=ftCommon_8007D5D4;host.resolve_bone=ftParts_GetBoneIndex;host.copy_ext_attributes=copy_ext_attributes;host.reload_ext_attributes=reload_ext_attributes;
    host.cpu_command=ftCo_800B463C;host.cpu_command_arg=ftCo_800B46B8;host.fighter_knockback_enter=Fighter_OnKnockbackEnter;host.fighter_knockback_exit=Fighter_OnKnockbackExit;
    stock_parts=(FighterPartsTable){stock_joint_map,stock_part_map,sizeof(stock_joint_map)};parts_tables[Ft_Kind_Mewtwo]=&stock_parts;
    module=entry(&base,&host);if(!module){fputs("Comparison host rejected\n",stderr);return 2;}
    for(unsigned i=0;i<module->article_logic_count;++i)if(module->article_logics[i].kind==It_Kind_Mewtwo_ShadowBall)logic=&module->article_logics[i];if(!logic)return 2;
    fighter_attributes.x0_MEWTWO_SHADOWBALL_CHARGE_CYCLES=parameter_float(&parameters,"shadow_max_charge");
    item_attributes=(itMewtwoShadowball_DatAttrs){.x0=parameter_float(&parameters,"It_Kind_Mewtwo_ShadowBall.lifetime"),.x10=parameter_float(&parameters,"It_Kind_Mewtwo_ShadowBall.damage_min"),.x14=parameter_float(&parameters,"It_Kind_Mewtwo_ShadowBall.damage_max"),.x18=parameter_float(&parameters,"It_Kind_Mewtwo_ShadowBall.scale_min"),.x1C=parameter_float(&parameters,"It_Kind_Mewtwo_ShadowBall.scale_max"),.x20=parameter_int(&parameters,"It_Kind_Mewtwo_ShadowBall.wobble_period"),.x2C=parameter_float(&parameters,"It_Kind_Mewtwo_ShadowBall.throw_speed")};
    article.x4_specialAttributes=&item_attributes;
    for(unsigned facing=0;facing<2;++facing)for(unsigned scale=0;scale<3;++scale)for(unsigned angle=0;angle<16;++angle)for(unsigned allocation=0;allocation<2;++allocation){
        model_scale=(float[]){.5f,1,1.75f}[scale];spawn_success=allocation!=0;reset(facing?-1:1);
        float theta=(angle-8.0f)*.39269908169872415f;
        for(unsigned i=0;i<2;++i){fighters[i].motion_id=219;fighters[i].cmd_vars[3]=1;joints[i][35].translate=(Vec3){3+cosf(theta),10+sinf(theta),7};joints[i][32].translate=(Vec3){3,10,-5};}
        lane=0;random_cursor=0;ftMt_SpecialN_Shoot(&owners[0]);unsigned stock_random=random_cursor;
        lane=1;random_cursor=0;module->on_throw_animation(&owners[1]);
        compare_trace("forward throw");check(stock_random==0&&random_cursor==0,"forward throw","zero RNG draws");check(fighters[0].cmd_vars[3]==fighters[1].cmd_vars[3] && fighters[1].cmd_vars[3]==0,"forward throw","cue consumed");
        check(!fighters[1].native_module_state[field_offset(&fighter_layout,"shadow_throw_creating")],"forward throw","creation mode restored");
        if(spawn_success)compare_constructor("forward throw");
        memset(traces,0,sizeof(traces));lane=0;ftMt_SpecialN_Shoot(&owners[0]);lane=1;module->on_throw_animation(&owners[1]);compare_trace("cue consumed once");check(traces[0].count==0,"cue consumed once","no repeated launch");
    }
    for(unsigned motion=220;motion<=222;++motion){reset(1);fighters[1].motion_id=motion;fighters[1].cmd_vars[3]=1;lane=1;module->on_throw_animation(&owners[1]);check(traces[1].count==0,"other throw","no forward-throw article cue");check(fighters[1].cmd_vars[3]==1,"other throw","unrelated cue retained");}
    spawn_success=true;
    for(unsigned facing=0;facing<2;++facing){reset(facing?-1:1);Vec3 position={-4,12,0};
        lane=0;random_cursor=0;it_802C5000(&owners[0],&position,35,It_Kind_Mewtwo_ShadowBall,fighters[0].facing_dir);unsigned stock_random=random_cursor;
        lane=1;random_cursor=0;set_fighter_bool("shadow_throw_creating",false);create_article(&owners[1],It_Kind_Mewtwo_ShadowBall,&position,&(Vec3){0},fighters[1].facing_dir,true,0,0,true);
        compare_trace("ordinary held constructor");check(stock_random==2&&random_cursor==2,"ordinary held constructor","two RNG draws retained");compare_constructor("ordinary held constructor");
    }
    for(int motion=0;motion<361;++motion)for(int charge=-1;charge<=(int)fighter_attributes.x0_MEWTWO_SHADOWBALL_CHARGE_CYCLES+1;++charge){reset(1);
        fighters[0].motion_id=fighters[1].motion_id=motion;fighters[0].u.mt.x2234_shadowBallCharge=charge;set_fighter_int("shadow_charge",charge);
        lane=0;stock_cpu_charge_decision(&fighters[0]);bool expected=traces[0].count!=0;lane=1;bool actual=module->on_cpu_charge_decision(&owners[1]);compare_trace("CPU charge decision");check(expected==actual,"CPU charge decision","interception result");
        memset(traces,0,sizeof(traces));lane=0;stock_cpu_charge_cancel(&fighters[0]);lane=1;module->on_cpu_charge_cancel(&owners[1]);compare_trace("CPU charge cancellation");
    }
    for(int motion=0;motion<361;++motion){reset(1);bool angled=ft_80089914(&owners[0],motion);check(module->on_motion_is_angled(&owners[1],motion)==angled,"angled motion","stock statistic selection");}
    for(unsigned air=0;air<2;++air)for(unsigned scale=0;scale<3;++scale)for(int motion=0;motion<361;++motion){model_scale=(float[]){.5f,1,1.75f}[scale];reset(1);fighters[0].ground_or_air=fighters[1].ground_or_air=air?GA_Air:GA_Ground;fighters[0].motion_id=fighters[1].motion_id=motion;bool expected=stock_dynamics_ground(&fighters[0]);check(module->on_dynamics_ground_collision(&owners[1])==expected,"dynamics ground policy","stock predicate");}
    reset(1);check(module->on_idle_variants(&owners[1]),"idle","variants allowed");fighters[1].item_gobj=&objects[1];check(module->on_idle_variants(&owners[1]),"idle with item","variants allowed");
    lane=0;ftMt_Init_OnKnockbackEnter(&owners[0]);ftMt_Init_OnKnockbackExit(&owners[0]);lane=1;module->on_knockback_enter(&owners[1]);module->on_knockback_exit(&owners[1]);compare_trace("knockback part mode");
    for(unsigned flag=0;flag<2;++flag){reset(1);lane=0;ftMt_Init_OnItemPickup(&owners[0],flag);ftMt_Init_OnItemDrop(&owners[0],flag);ftMt_Init_OnItemInvisible(&owners[0]);ftMt_Init_OnItemVisible(&owners[0]);lane=1;module->on_item_pickup(&owners[1],flag);module->on_item_drop(&owners[1],flag);module->on_item_invisible(&owners[1]);module->on_item_visible(&owners[1]);compare_trace("ordinary item no-op callbacks");}
    fighter_sfx.x14=0x2345;fighter_data.x4C_sfx=&fighter_sfx;common_data.x78=.4f;
    const float sticks[]={-1,-.5f,-.4f,nextafterf(-.4f,-INFINITY),nextafterf(-.4f,INFINITY),0,.4f,nextafterf(.4f,-INFINITY),nextafterf(.4f,INFINITY),.5f,1};
    for(unsigned facing=0;facing<2;++facing)for(unsigned stick=0;stick<sizeof(sticks)/sizeof(*sticks);++stick)for(unsigned mult=0;mult<3;++mult)for(unsigned hood=0;hood<2;++hood)for(unsigned count=0;count<3;++count){
        reset(facing?-1:1);for(unsigned i=0;i<2;++i){fighters[i].input.lstick[0].x=sticks[stick];fighters[i].co_attrs.air_jump_h_multiplier=(float[]){0,.7f,2.25f}[mult];fighters[i].x197C=hood?&objects[i]:NULL;fighters[i].x1968_jumpsUsed=count;fighters[i].x671_timer_lstick_tilt_y=37;fighters[i].self_vel=(Vec3){8,9,10};}
        lane=0;stock_original_mt_jump(&owners[0]);lane=1;module->on_aerial_jump(&owners[1]);compare_jump("authored aerial jump");
    }
    for(unsigned kind=0;kind<3;++kind)for(unsigned hood=0;hood<2;++hood)for(unsigned reset_timer=0;reset_timer<2;++reset_timer)for(unsigned motion=27;motion<=28;++motion){
        reset(1);for(unsigned i=0;i<2;++i){fighters[i].kind=(FighterKind[]){Ft_Kind_Mewtwo,Ft_Kind_Ness,Ft_Kind_Mario}[kind];fighters[i].x197C=hood?&objects[i]:NULL;fighters[i].x1968_jumpsUsed=2;fighters[i].x671_timer_lstick_tilt_y=37;}Vec3 velocity={-.2f,3,0};
        lane=0;stock_original_aerial_begin(&owners[0],motion,&velocity,reset_timer);lane=1;ftCo_JumpAerial_Begin(&owners[1],motion,&velocity,reset_timer,false,kind==2);compare_jump("refactored common aerial core");
    }
    for(int sets=1;sets<=8;++sets)for(int set=0;set<sets;++set){reset(1);check(module->on_dynamics_interaction_start(&owners[1],set,sets)==8,"dynamics interaction","stock first active part 8");}
    unsigned char source_attrs[FT_CHARACTER_EXT_ATTR_CAPACITY];for(unsigned i=0;i<sizeof(source_attrs);++i)source_attrs[i]=(unsigned char)(i*37+13);fighter_data.ext_attr=source_attrs;item_list[0]=&article;item_list[1]=(Article*)&objects[0];fighter_data.x48_items=(void**)item_list;
    reset(1);lane=0;ftMt_Init_OnLoad(&owners[0]);lane=1;module->on_load(&owners[1]);
    check(fighters[0].parts[FtPart_TransN].flags_b4==fighters[1].parts[FtPart_TransN].flags_b4,"load","direct animation flag");check(fighters[0].x2221_b2==fighters[1].x2221_b2,"load","hip root compensation");
    check(memcmp(fighters[0].dat_attrs,fighters[1].dat_attrs,sizeof(ftMewtwoAttributes))==0,"load","stock ext attributes copy");check(fighters[1].dat_attrs==fighters[1].dat_attrs_backup,"load","owned attribute storage");
    check(traces[0].count==2&&traces[0].events[0].a==It_Kind_Mewtwo_Disable&&traces[0].events[1].a==It_Kind_Mewtwo_ShadowBall,"load","stock asset slots agree with copied-base declarations");
    memset(fighters[1].dat_attrs_backup,0,FT_CHARACTER_EXT_ATTR_CAPACITY);module->on_load_special_attrs(&owners[1]);check(memcmp(fighters[1].dat_attrs,source_attrs,FT_CHARACTER_EXT_ATTR_CAPACITY)==0,"reload","raw ext attribute reset");
    check(!module->on_worn_item_jump_sound(&owners[1]),"worn sound policy","suppression predicate");
    common_data.x800=.8f;
    for(unsigned hood=0;hood<2;++hood)for(unsigned facing=0;facing<2;++facing)for(unsigned reset_timer=0;reset_timer<3;++reset_timer){
        reset(facing?-1:1);fighters[1].kind=Ft_Kind_CustomFirst;for(unsigned i=0;i<2;++i){fighters[i].x197C=hood?&objects[i]:NULL;fighters[i].x1968_jumpsUsed=reset_timer;fighters[i].x671_timer_lstick_tilt_y=37;}Vec3 velocities[2]={{-.3f,2.7f,.125f},{-.3f,2.7f,.125f}};
        lane=0;stock_original_screw(&owners[0],&velocities[0]);lane=1;ft_800D2E7C(&owners[1],&velocities[1]);compare_jump("common Screw Attack worn sound");check(memcmp(&velocities[0],&velocities[1],sizeof(Vec3))==0,"common Screw Attack worn sound","incoming velocity scaling");
    }
    struct Fighter_x2D0_t multijump={.x0=6,.x4=.4f,.x8=.7f,.x14={2,1.8f,1.5f,1.1f,.8f},.x2C=155};
    for(unsigned hood=0;hood<2;++hood)for(unsigned facing=0;facing<2;++facing)for(unsigned stick=0;stick<3;++stick)for(unsigned count=1;count<=5;++count){
        reset(facing?-1:1);fighters[1].kind=Ft_Kind_CustomFirst;for(unsigned i=0;i<2;++i){fighters[i].x2D0=&multijump;fighters[i].input.lstick[0].x=(float[]){-.8f,0,.8f}[stick];fighters[i].x197C=hood?&objects[i]:NULL;fighters[i].x1968_jumpsUsed=count;fighters[i].x671_timer_lstick_tilt_y=37;}
        lane=0;stock_original_multijump(&owners[0]);lane=1;ftCo_800D74A4(&owners[1]);compare_jump("common multijump worn sound");check(fighters[0].mv.co.jumpaerial.x0==fighters[1].mv.co.jumpaerial.x0,"common multijump worn sound","turn countdown");check(equal_float(joints[0][0].rotate.y,joints[1][0].rotate.y),"common multijump worn sound","exact root rotation");check(fighters[0].facing_dir==fighters[1].facing_dir,"common multijump worn sound","turn facing");
    }
    FreeLibrary(library);printf("Mewtwo stock-policy comparison: %u checks, %u failures\n",comparisons,failures);return failures?1:0;
}
