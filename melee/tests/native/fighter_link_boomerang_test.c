// Source-only acceptance: unchanged stock bodies versus actual generated C.
// No game process is launched. Undefined launch quaternion W is excluded for
// Euler joints; actual HSD model primitives have separate graph-level coverage.
#include <melee/ft/ftlib.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialhi.h>
#include <melee/ft/kinds/ftLink/ftlinkspecials.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itCharItems.h>
#include <melee/it/itdraw.h>
#include <melee/it/item.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boomerang_fixture.h"
#define abort() (fprintf(stderr,"fixture abort line %u active %u comparisons %u\n",(unsigned)__LINE__,active,comparisons),exit(2))

static Item items[2];
static Fighter fighters[2];
static HSD_GObj objects[2], creators[2], reflectors[2];
static HSD_JObj roots[2], children[2], trails[2][2], attachment[2];
static Article articles[2];
static itLinkBoomerangAttributes attributes[2];
static ftLk_DatAttrs fighter_attributes[2];
static HSD_Joint immutable[2];
static HSD_AnimJoint animations[2];
static unsigned active, comparisons, failures;
static int changes[2], removed[2], sounds[2], pickups[2], matrix_reads[2];
static int adds[2], requests[2], advances[2], removes_anim[2], destroys[2];
static unsigned change_flags[2];
static bool floor_sweep;
static Vec3 camera_target;
static int fighter_changes[2], fighter_starts[2], fighter_finishes[2], fighter_physics[2];
static unsigned interrupt_order[2], interrupt_accept;
static float model_rates[2][3];
static MotionFlags fighter_flags[2];
static bool frames_remaining, ground_contact, creation_fails;
static FighterBone fighter_parts[2][8];
static ItemLogicTable stock_logic={.thrown=it_802A0F88};
static ItemStateDesc animation_descriptors[4];
static ItemStateArray animation_table={animation_descriptors,4};
static SpawnItem spawns[2];
static struct ftCommonData common;
struct ftCommonData* p_ftCommonData = &common;

void __assert(char* file, u32 line, char* message)
{ fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,message);abort(); }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
#undef HSD_JObjSetMtxDirty
void HSD_JObjSetMtxDirty(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void HSD_JObjSetFlagsAll(HSD_JObj* joint, u32 flags)
{ for(;joint;joint=joint->next){joint->flags|=flags;if(joint->child)HSD_JObjSetFlagsAll(joint->child,flags);} }
void HSD_JObjClearFlagsAll(HSD_JObj* joint, u32 flags)
{ for(;joint;joint=joint->next){joint->flags&=~flags;if(joint->child)HSD_JObjClearFlagsAll(joint->child,flags);} }
void HSD_MtxGetRotation(Mtx matrix, Vec3* rotation)
{ ++matrix_reads[active];*rotation=(Vec3){matrix[0][0],matrix[1][0],matrix[2][0]}; }
void HSD_JObjAddAnimAll(HSD_JObj* root,HSD_AnimJoint* anim,HSD_MatAnimJoint* mat,HSD_ShapeAnimJoint* shape)
{ (void)root;(void)anim;(void)mat;(void)shape;++adds[active]; }
void HSD_JObjReqAnimAll(HSD_JObj* root,float frame)
{ (void)root;if(frame!=0)abort();++requests[active]; }
void HSD_JObjAnimAll(HSD_JObj* root) { (void)root;++advances[active]; }
void HSD_JObjRemoveAnimAll(HSD_JObj* root) { (void)root;++removes_anim[active]; }
void HSD_JObjRemoveAll(HSD_JObj* root) { (void)root;++destroys[active]; }
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* descriptor)
{ if(descriptor==&immutable[0])return &trails[active][0];if(descriptor==&immutable[1])return &trails[active][1];abort(); }
void lb_8000BA0C(HSD_JObj* joint,float rate)
{ if(joint==&children[active])model_rates[active][0]=rate;
  else if(joint==&trails[active][0])model_rates[active][1]=rate;
  else if(joint==&trails[active][1])model_rates[active][2]=rate;
  else abort(); }
void HSD_JObjDispAll(HSD_JObj* joint,MtxPtr matrix,u32 flags,u32 render_mode)
{ (void)joint;(void)matrix;(void)flags;(void)render_mode;abort(); }
u32 HSD_GObj_80390EB8(s32 value) { (void)value;abort(); }
void it_8026EECC(Item_GObj* object,int value) { (void)object;(void)value;abort(); }
void it_80272A18(HSD_JObj* root) { HSD_JObjClearFlagsAll(root,JOBJ_HIDDEN); }
void it_80272A3C(HSD_JObj* root) { HSD_JObjSetFlagsAll(root,JOBJ_HIDDEN); }
void Item_80268E5C(Item_GObj* object,enum_t state,Item_StateChangeFlags flags)
{
    ++changes[active];change_flags[active]=flags;((Item*)object->user_data)->msid=state;
    ((HSD_JObj*)object->hsd_obj)->rotate.z=.9875f;
    ((HSD_JObj*)object->hsd_obj)->child->rotate=(Quaternion){.x=.5f,.y=.25f,.z=-.125f,.w=.75f};
}
void it_80275158(Item_GObj* object,float life)
{ Item* item=object->user_data;item->xD44_lifeTimer=life;item->xD48_halfLifeTimer=life*.5f; }
void Item_8026AF0C(Item* item,enum_t sound,u8 pan,u8 volume)
{ if(sound!=(item->kind==It_Kind_Link_Boomerang?160091:70091)||pan!=127||volume!=64)abort();++sounds[active]; }
void it_802762BC(Item* item) { item->ground_or_air=GA_Air;item->xDC8_word.flags.x1F=0; }
void it_8026E0F4(Item_GObj* object) { (void)object;(void)floor_sweep; }










void Item_8026AB54(Item_GObj* object,HSD_GObj* owner,Fighter_Part part)
{ (void)owner;if(part!=7)abort();++pickups[active];((Item*)object->user_data)->xDC4=part;
 if(active==0)it_802A0E70(object);
 else scripted_article_logics[items[1].kind==It_Kind_CLink_Boomerang].logic->picked_up(object);
}
void Item_8026A8EC(Item_GObj* object)
{
    ++removed[active];
    // Item_8026A8EC invokes the logic-table destroyed notification before
    // freeing the item. The stable fixture keeps storage readable only to
    // compare the resulting state; it must not omit this synchronous callback.
    if(active==0)itLinkBoomerang_Logic18_Destroyed(object);
    else scripted_article_logics[items[1].kind==It_Kind_CLink_Boomerang].logic->destroyed(object);
    ((Item*)object->user_data)->owner=NULL;
}
bool it_8026B894(Item_GObj* object,HSD_GObj* other) { (void)object;(void)other;return false; }
void ftLib_800866DC(Fighter_GObj* object,Vec3* position) { (void)object;*position=camera_target; }
HSD_JObj* ftLib_80086630(Fighter_GObj* object,Fighter_Part part)
{ (void)object;if(part!=7)abort();return &attachment[active]; }
FighterKind ftLib_GetKind(Fighter_GObj* object) { return ((Fighter*)object->user_data)->kind; }
void it_8026BB68(Fighter_GObj* object,Vec3* position)
{ Fighter* f=object->user_data;*position=f->cur_pos;position->y+=.5f*(f->coll_data.ecb.top.y+f->coll_data.ecb.bottom.y); }
Item_GObj* Item_80268B18(SpawnItem* spawn)
{ Item* i=&items[active];if(creation_fails)return NULL;spawns[active]=*spawn;i->kind=spawn->kind;i->pos=spawn->pos;i->x40_vel=spawn->vel;i->facing_dir=spawn->facing_dir;i->owner=spawn->x0_parent_gobj;return &objects[active]; }
void it_8027429C(Item_GObj* object,Vec3* velocity) { ((Item*)object->user_data)->x40_vel=*velocity; }
void it_8026B3A8(Item_GObj* object) { ((Item*)object->user_data)->xDCD_flag.b4=1; }

static Fighter_GObj* creator(Item_GObj* object)
{ return ((Item*)object->user_data)->xDD4_itemVar.native_article.attachment_owner; }
static uint32_t current_owner(Item_GObj* object)
{ Item* item=object->user_data;return item->owner==&creators[active]?1:item->owner==&reflectors[active]?2:0; }
static uint32_t reflector(Item_GObj* object) { (void)object;return 2; }
static bool target_position(uint32_t target,Vec3* position)
{ if(!target)abort();*position=camera_target;return true; }
static bool attached_matrix(Item_GObj* object,Vec3* output)
{ HSD_MtxGetRotation(ftLib_80086630(creator(object),((Item*)object->user_data)->xDC4)->mtx,output);return true; }
static void add_descriptor(Item_GObj* object,unsigned index)
{ Item* item=object->user_data;item->xD0_itemStateDesc=&item->xC4_article_data->xC_itemStates->x0_itemStateDesc[index];HSD_JObjAddAnimAll(object->hsd_obj,item->xD0_itemStateDesc->x0_anim_joint,item->xD0_itemStateDesc->x4_matanim_joint,item->xD0_itemStateDesc->x8_parameters); }
static unsigned bones(Fighter* fighter) { (void)fighter;return 8; }

void ftLk_800EAF38(Fighter_GObj* object) { (void)object; abort(); }
void ftLk_800EAF58(Fighter_GObj* object) { (void)object; abort(); }
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* other)
{
 (void)blend;(void)other;Fighter* f=object->user_data;
 ++fighter_changes[active];fighter_flags[active]=flags;f->motion_id=motion;
 f->cur_anim_frame=frame;f->frame_speed_mul=rate;
 if(f->x21EC){f->x21EC(object);f->x21EC=NULL;}
}
void ftAnim_8006EBA4(Fighter_GObj* object) { (void)object;++fighter_starts[active]; }
bool ftAnim_IsFramesRemaining(Fighter_GObj* object) { (void)object;return frames_remaining; }
void ft_8008A2BC(Fighter_GObj* object) { ((Fighter*)object->user_data)->motion_id=14;++fighter_finishes[active]; }
void ftCo_Fall_Enter(Fighter_GObj* object) { ((Fighter*)object->user_data)->motion_id=29;++fighter_finishes[active]; }
void ftCo_Landing_Enter_Basic(Fighter_GObj* object) { ((Fighter*)object->user_data)->motion_id=42;++fighter_finishes[active]; }
void ft_80084F3C(Fighter_GObj* object) { (void)object;fighter_physics[active]+=1; }
void ft_80084EEC(Fighter_GObj* object) { (void)object;fighter_physics[active]+=2; }
bool ft_80082708(Fighter_GObj* object) { (void)object;return ground_contact; }
GroundOrAir ft_80081D0C(Fighter_GObj* object) { (void)object;return ground_contact?GA_Air:GA_Ground; }
void ftCommon_8007D5D4(Fighter* fighter) { fighter->ground_or_air=GA_Air; }
void ftCommon_8007D7FC(Fighter* fighter) { fighter->ground_or_air=GA_Ground; }
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter,Fighter_Part semantic)
{ (void)fighter;if(semantic!=FtPart_LThumbNb)abort();return 7; }
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* output)
{ (void)joint;if(offset)abort();*output=(Vec3){-2,8,5}; }
static bool input_check(unsigned index)
{ interrupt_order[active]=(interrupt_order[active]<<4)|(index+1);return interrupt_accept==index; }
bool ftCo_SpecialS_CheckInput(Fighter_GObj* object) { (void)object;return input_check(0); }
bool ftCo_Attack100_CheckInput(Fighter_GObj* object) { (void)object;return input_check(1); }
bool ftCo_800D6824(Fighter_GObj* object) { (void)object;return input_check(2); }
bool ftCo_800D68C0(Fighter_GObj* object) { (void)object;return input_check(3); }
bool ftCo_80091A4C(Fighter_GObj* object) { (void)object;return input_check(4); }
bool ftCo_Jump_CheckInput(Fighter_GObj* object) { (void)object;return input_check(5); }
bool ftCo_Dash_CheckInput(Fighter_GObj* object) { (void)object;return input_check(6); }
bool ftCo_SpecialAir_CheckInput(Fighter_GObj* object) { (void)object;return input_check(7); }
bool ftCo_800CB870(Fighter_GObj* object) { (void)object;return input_check(8); }
static bool owned_article(Fighter_GObj* object,s32 id)
{ (void)object;return id==77&&removed[active]==0; }
static bool signal_article(Fighter_GObj* object,s32 id,uint32_t name,float value)
{
 (void)object;if(!owned_article(object,id))return false;
 const NativeFighterArticleLogic* logic=&scripted_article_logics[items[active].kind==It_Kind_CLink_Boomerang];
 for(unsigned n=0;n<logic->signal_count;++n)if(logic->signals[n].name_hash==name){logic->signals[n].callback(&objects[active],value);return true;}
 abort();
}
static s32 spawn_article(Fighter_GObj* object,ItemKind kind,Fighter_Part semantic)
{
 if(semantic!=FtPart_LThumbNb)abort();if(creation_fails)return -1;
 Item* item=&items[active];item->kind=kind;item->owner=object;item->facing_dir=((Fighter*)object->user_data)->facing_dir;
 const NativeFighterAttachmentSpawn* setting=scripted_article_logics[kind==It_Kind_CLink_Boomerang].attachment;
 if(!setting||!setting->origin_at_center||setting->origin_at_owner||!setting->previous_at_bone||setting->flatten_z||!setting->initial_collision||!setting->register_as_held)abort();
 spawns[active]=(SpawnItem){.kind=kind,.prev_pos={-2,8,5},.facing_dir=item->facing_dir,.x0_parent_gobj=object,.x4_parent_gobj2=object};
 spawns[active].x44_flag.b0=1;it_8026BB68(object,&spawns[active].pos);item->pos=spawns[active].pos;item->x40_vel=(Vec3){0};
 item->xDD4_itemVar.native_article.attachment_owner=object;
 const NativeFighterArticleLogic* logic=&scripted_article_logics[kind==It_Kind_CLink_Boomerang];
 logic->on_created(&objects[active]);Item_8026AB54(&objects[active],object,7);
 ((Fighter*)object->user_data)->x1984_heldItemSpec=&objects[active];return 77;
}
static float get_common(NativeFighterCommonAttribute attribute)
{ if(attribute==NativeFighterCommon_HorizontalSmashThreshold)return common.dash_smash_stick_threshold;if(attribute==NativeFighterCommon_HorizontalSmashWindow)return common.dash_smash_window;if(attribute==NativeFighterCommon_HorizontalSmashExtension)return common.x44;abort(); }
static float read_field(Item* item,const char* name)
{ for(unsigned n=0;n<33;++n)if(!strcmp(scripted_article_0_state_fields[n].name,name))return scripted_article_state_float(item,scripted_article_0_state_fields[n].offset);abort(); }
static void write_field(Item* item,const char* name,float value)
{ for(unsigned n=0;n<33;++n)if(!strcmp(scripted_article_0_state_fields[n].name,name)){scripted_article_set_state_float(item,scripted_article_0_state_fields[n].offset,value);return;}abort(); }
static void write_int(Item* item,const char* name,int value)
{ for(unsigned n=0;n<33;++n)if(!strcmp(scripted_article_0_state_fields[n].name,name)){scripted_article_set_state_int(item,scripted_article_0_state_fields[n].offset,value);return;}abort(); }
static const NativeFighterArticleLogic* authored;
static void reset(unsigned family,unsigned state,float angle,int mode)
{
 memset(items,0,sizeof(items));memset(fighters,0,sizeof(fighters));memset(roots,0,sizeof(roots));memset(children,0,sizeof(children));memset(trails,0,sizeof(trails));
 memset(fighter_changes,0,sizeof(fighter_changes));memset(fighter_starts,0,sizeof(fighter_starts));
 memset(fighter_finishes,0,sizeof(fighter_finishes));memset(fighter_physics,0,sizeof(fighter_physics));
 memset(interrupt_order,0,sizeof(interrupt_order));memset(fighter_flags,0,sizeof(fighter_flags));
 memset(model_rates,0,sizeof(model_rates));
 frames_remaining=true;ground_contact=true;creation_fails=false;interrupt_accept=99;
 memset(changes,0,sizeof(changes));memset(removed,0,sizeof(removed));memset(sounds,0,sizeof(sounds));memset(pickups,0,sizeof(pickups));memset(matrix_reads,0,sizeof(matrix_reads));
 memset(adds,0,sizeof(adds));memset(requests,0,sizeof(requests));memset(advances,0,sizeof(advances));memset(removes_anim,0,sizeof(removes_anim));memset(destroys,0,sizeof(destroys));memset(change_flags,0,sizeof(change_flags));
 authored=&scripted_article_logics[family];
 for(unsigned n=0;n<2;++n){
  objects[n]=(HSD_GObj){.user_data=&items[n],.hsd_obj=&roots[n]};creators[n]=(HSD_GObj){.user_data=&fighters[n]};reflectors[n]=(HSD_GObj){0};
  fighters[n].parts=fighter_parts[n];fighter_parts[n][7].joint=&attachment[n];fighters[n].facing_dir=1;
  fighters[n].kind=family?Ft_Kind_CLink:Ft_Kind_Link;fighters[n].ground_or_air=GA_Ground;fighters[n].cur_pos=(Vec3){13,-7,0};
  fighters[n].u.lk.used_boomerang=true;fighters[n].u.lk.boomerang_gobj=&objects[n];fighters[n].dat_attrs=&fighter_attributes[n];
  fighter_attributes[n]=(ftLk_DatAttrs){.specialhi_pos_y_offset=2,.x14=.25f,.x18=.6f,.x1C=1.5f,.x20=4,.x24=3,.x2C=family?It_Kind_CLink_Boomerang:It_Kind_Link_Boomerang};
  fighters[n].native_module_state[1]=true;script_state_set_int(&fighters[n],8,77);fighters[n].native_module_state[2]=true;script_state_set_int(&fighters[n],12,7);script_state_set_float(&fighters[n],16,2);
  items[n].xB8_itemLogicTable=n?scripted_article_logics[family].logic:&stock_logic;items[n].x1C=77;items[n].entity=&objects[n];items[n].kind=family?It_Kind_CLink_Boomerang:It_Kind_Link_Boomerang;items[n].msid=state;items[n].owner=&creators[n];items[n].xC64_reflectGObj=&reflectors[n];items[n].xC70=1.25f;
  items[n].xDC4=7;items[n].facing_dir=1;items[n].pos=(Vec3){3,-4,5};items[n].x40_vel=(Vec3){3*cosf(angle),3*sinf(angle),.25f};items[n].xD44_lifeTimer=5;items[n].xD48_halfLifeTimer=10;
  roots[n].child=&children[n];roots[n].scale=children[n].scale=(Vec3){1,1,1};roots[n].rotate.z=.5f;children[n].rotate=(Quaternion){.x=.25f,.y=.5f,.z=.75f,.w=1};children[n].mtx[0][0]=.25f;children[n].mtx[1][0]=-.5f;children[n].mtx[2][0]=.75f;
  attributes[n]=(itLinkBoomerangAttributes){.x0=40,.x4=60,.x8=4,.xC=.25f,.x14=5,.x18=1,.x1C=30,.x20=10,.x24=25,.x28=8,.x2C=2,.x30=1.5f,.x38=4,.x3C=2,.x40=3,.x44=&immutable[0],.x48=&immutable[1],.x4C_anim={.anim=&animations[0]},.x58_anim={.anim=&animations[1]}};
  articles[n]=(Article){.x4_specialAttributes=&attributes[n],.xC_itemStates=&animation_table,.native_model_root_count=2,.native_model_animation_count=2};items[n].xC4_article_data=&articles[n];
  for(unsigned t=0;t<2;++t){trails[n][t].scale=(Vec3){1,1,1};articles[n].native_model_roots[t].name_hash=authored->model_roots[t].name_hash;articles[n].native_model_roots[t].root=&immutable[t];articles[n].native_model_animations[t].name_hash=authored->model_animations[t].name_hash;articles[n].native_model_animations[t].anim=&animations[t];}
 }
 items[0].xDD4_itemVar.linkboomerang.xF98=&creators[0];items[0].xDD4_itemVar.linkboomerang.xDE8=mode;items[0].xDD4_itemVar.linkboomerang.xF74=angle;items[0].xDD4_itemVar.linkboomerang.xF78=.5f;items[0].xDD4_itemVar.linkboomerang.xF7C=-.5f;items[0].xDD4_itemVar.linkboomerang.xF80=8;items[0].xDD4_itemVar.linkboomerang.xF84=.25f;items[0].xDD4_itemVar.linkboomerang.xF70=2;items[0].xDD4_itemVar.linkboomerang.xDEC=family?20:6;
 items[1].xDD4_itemVar.native_article.attachment_owner=&creators[1];write_int(&items[1],"mode",mode);write_field(&items[1],"angle",angle);write_field(&items[1],"render_angle",.5f);write_field(&items[1],"bounce_cosine",-.5f);write_field(&items[1],"turn_timer",8);write_field(&items[1],"maximum_turn",.25f);write_field(&items[1],"trail_timer",2);
 camera_target=(Vec3){-6,4,0};
}
static int read_int(Item* item,const char* name)
{ for(unsigned n=0;n<33;++n)if(!strcmp(scripted_article_0_state_fields[n].name,name))return scripted_article_state_int(item,scripted_article_0_state_fields[n].offset);abort(); }
static void enable_trails(unsigned mask)
{
 for(unsigned t=0;t<2;++t){items[0].xDD4_itemVar.linkboomerang.xF90[t]=(mask&(1u<<t))?&trails[0][t]:NULL;items[1].native_models[t]=(mask&(1u<<t))?&trails[1][t]:NULL;}
}
static bool same_float(float a,float b){return (!memcmp(&a,&b,4))||(isnan(a)&&isnan(b));}
static void compare(int stock,int script,const char* phase)
{
 ++comparisons;bool equal=stock==script&&items[0].msid==items[1].msid&&same_float(items[0].xD44_lifeTimer,items[1].xD44_lifeTimer)&&!memcmp(&items[0].x40_vel,&items[1].x40_vel,sizeof(Vec3))&&same_float(items[0].xDD4_itemVar.linkboomerang.xF74,read_field(&items[1],"angle"))&&same_float(items[0].xDD4_itemVar.linkboomerang.xF78,read_field(&items[1],"render_angle"))&&changes[0]==changes[1]&&change_flags[0]==change_flags[1]&&removed[0]==removed[1]&&sounds[0]==sounds[1]&&pickups[0]==pickups[1]&&matrix_reads[0]==matrix_reads[1]&&adds[0]==adds[1]&&requests[0]==requests[1]&&advances[0]==advances[1]&&removes_anim[0]==removes_anim[1]&&destroys[0]==destroys[1]&&(fighters[0].u.lk.boomerang_gobj!=NULL)==(script_state_get_int(&fighters[1],8)>=0);
 const struct { const char* name; float value; } floats[] = {
  {"bounce_cosine",items[0].xDD4_itemVar.linkboomerang.xF7C},
  {"turn_timer",items[0].xDD4_itemVar.linkboomerang.xF80},
  {"maximum_turn",items[0].xDD4_itemVar.linkboomerang.xF84},
  {"spin_step",items[0].xDD4_itemVar.linkboomerang.xF88},
  {"sound_timer",items[0].xDD4_itemVar.linkboomerang.xF8C},
  {"trail_timer",items[0].xDD4_itemVar.linkboomerang.xF70}};
 for(unsigned n=0;n<6;++n)equal &= same_float(floats[n].value,read_field(&items[1],floats[n].name));
 equal &= items[0].xDD4_itemVar.linkboomerang.xDE8==read_int(&items[1],"mode") &&
  items[0].xDD4_itemVar.linkboomerang.xDE4==read_int(&items[1],"turn_frames") &&
  items[0].xDD4_itemVar.linkboomerang.xDD8==read_int(&items[1],"history_write");
 for(unsigned n=0;n<16;++n){
  Vec3 a=items[0].xDD4_itemVar.linkboomerang.xDF0[n],b=items[0].xDD4_itemVar.linkboomerang.xEB0[n];
  const float values[]={a.x,a.y,a.z,b.x,b.y,b.z};
  for(unsigned lane=0;lane<6;++lane)equal &= same_float(values[lane],script_state_array_float(items[1].xDD4_itemVar.native_article.state,lane*64,n,16));
 }
 for(unsigned t=0;t<2;++t){
  equal &= items[0].xDD4_itemVar.linkboomerang.xDDC[t]==script_state_array_int(items[1].xDD4_itemVar.native_article.state,388,t,2);
  HSD_JObj* a=items[0].xDD4_itemVar.linkboomerang.xF90[t],*b=items[1].native_models[t];
  equal &= (a!=NULL)==(b!=NULL);
  if(a&&b)equal &= !memcmp(&a->translate,&b->translate,sizeof(Vec3)) && !memcmp(&a->rotate,&b->rotate,3*sizeof(float)) && a->flags==b->flags;
 }
 equal &= !memcmp(&items[0].pos,&items[1].pos,sizeof(Vec3)) && same_float(items[0].facing_dir,items[1].facing_dir) && same_float(items[0].xD48_halfLifeTimer,items[1].xD48_halfLifeTimer);
 equal &= (items[0].xDD4_itemVar.linkboomerang.xF98!=NULL)==(items[1].xDD4_itemVar.native_article.attachment_owner!=NULL);
 equal &= (items[0].owner==NULL)==(items[1].owner==NULL) && items[0].xDC8_word.flags.x13==items[1].xDC8_word.flags.x13;
 equal &= items[0].xDAC_itcmd_var0==items[1].xDAC_itcmd_var0 && items[0].xDB0_itcmd_var1==items[1].xDB0_itcmd_var1 && items[0].xDB4_itcmd_var2==items[1].xDB4_itcmd_var2;
 equal &= !memcmp(model_rates[0],model_rates[1],sizeof(model_rates[0]));
 equal &= fighters[0].motion_id==fighters[1].motion_id && fighters[0].ground_or_air==fighters[1].ground_or_air && fighter_changes[0]==fighter_changes[1] && fighter_flags[0]==fighter_flags[1] && fighter_starts[0]==fighter_starts[1] && fighter_finishes[0]==fighter_finishes[1] && fighter_physics[0]==fighter_physics[1] && interrupt_order[0]==interrupt_order[1];
 equal &= !memcmp(&roots[0].rotate,&roots[1].rotate,3*sizeof(float)) && !memcmp(&children[0].rotate,&children[1].rotate,3*sizeof(float)) && roots[0].flags==roots[1].flags && children[0].flags==children[1].flags;
 if(!equal){++failures;if(failures<=12){
 fprintf(stderr,"Boomerang %s result%d/%d state%d/%d angle%.9g/%.9g render%.9g/%.9g life%.9g/%.9g changes%d/%d matrices%d/%d velocities %a/%a %a/%a %a/%a pickups%d/%d removed%d/%d destroys%d/%d flags %x/%x %x/%x\n",phase,stock,script,items[0].msid,items[1].msid,items[0].xDD4_itemVar.linkboomerang.xF74,read_field(&items[1],"angle"),items[0].xDD4_itemVar.linkboomerang.xF78,read_field(&items[1],"render_angle"),items[0].xD44_lifeTimer,items[1].xD44_lifeTimer,changes[0],changes[1],matrix_reads[0],matrix_reads[1],items[0].x40_vel.x,items[1].x40_vel.x,items[0].x40_vel.y,items[1].x40_vel.y,items[0].x40_vel.z,items[1].x40_vel.z,pickups[0],pickups[1],removed[0],removed[1],destroys[0],destroys[1],(unsigned)roots[0].flags,(unsigned)roots[1].flags,(unsigned)children[0].flags,(unsigned)children[1].flags);
 for(unsigned n=0;n<6;++n)if(!same_float(floats[n].value,read_field(&items[1],floats[n].name)))fprintf(stderr,"field %s %a/%a\n",floats[n].name,floats[n].value,read_field(&items[1],floats[n].name));
 fprintf(stderr,"motion %d/%d reference %d/%d mode %d/%d turn_frames %d/%d history %d/%d\n",fighters[0].motion_id,fighters[1].motion_id,fighters[0].u.lk.boomerang_gobj!=NULL,script_state_get_int(&fighters[1],8)>=0,(int)items[0].xDD4_itemVar.linkboomerang.xDE8,read_int(&items[1],"mode"),(int)items[0].xDD4_itemVar.linkboomerang.xDE4,read_int(&items[1],"turn_frames"),(int)items[0].xDD4_itemVar.linkboomerang.xDD8,read_int(&items[1],"history_write"));
 }}
}
static void predicate(HSD_GObjPredicate stock,HSD_GObjPredicate script,const char* phase)
{active=0;int a=stock(&objects[0]);active=1;int b=script(&objects[1]);compare(a,b,phase);}
static void callback(HSD_GObjEvent stock,HSD_GObjEvent script,const char* phase)
{active=0;stock(&objects[0]);active=1;script(&objects[1]);compare(0,0,phase);}
static void fighter_callback(HSD_GObjEvent stock,HSD_GObjEvent script,const char* phase)
{ active=0;stock(&creators[0]);active=1;script(&creators[1]);compare(0,0,phase); }
static void set_reference(bool exists)
{
 fighters[0].u.lk.boomerang_gobj=exists?&objects[0]:NULL;
 script_state_set_int(&fighters[1],8,exists?77:-1);
}
static void fighter_cases(void)
{
 const float xs[]={-1,-.700001f,-.7f,-0.0f,0,.7f,.700001f,1};
 const float ys[]={-1,-.250001f,-.25f,-0.0f,0,.25f,.250001f,1};
 for(unsigned family=0;family<2;++family)for(unsigned face=0;face<2;++face)
 for(unsigned x=0;x<8;++x)for(unsigned y=0;y<8;++y)for(unsigned speed=0;speed<2;++speed){
  reset(family,0,0,0);Vec3 position;
  for(unsigned n=0;n<2;++n){fighters[n].facing_dir=face?-1:1;fighters[n].input.lstick[0]=(Vec2){xs[x],ys[y]};}
  float distance=speed?4:3;active=0;float angle=calcAnglePos(&creators[0],&position,distance);
  active=1;script_fn_boomerang_angle(&creators[1],distance);++comparisons;
  if(!same_float(angle,script_state_get_float(&fighters[1],20))||!same_float(position.x,script_state_get_float(&fighters[1],28))||!same_float(position.y,script_state_get_float(&fighters[1],32))){
   ++failures;if(failures<=12)fprintf(stderr,"Boomerang fighter angle face%u x%a y%a speed%a: %a/%a\n",face,xs[x],ys[y],distance,angle,script_state_get_float(&fighters[1],20));
  }
 }
 for(unsigned family=0;family<2;++family)for(unsigned x=0;x<8;++x)for(unsigned age=0;age<8;++age){
  reset(family,0,0,0);common.dash_smash_stick_threshold=.7f;common.dash_smash_window=4;common.x44=2;
  for(unsigned n=0;n<2;++n){fighters[n].input.lstick[0].x=xs[x];fighters[n].x673=age;}
  active=0;on21EC(&creators[0]);active=1;script_fn_boomerang_smash_input(&creators[1]);++comparisons;
  if(fighters[0].u.lk.x4!=fighters[1].native_module_state[3]||fighters[0].x2070.count_thrown_items!=fighters[1].x2070.count_thrown_items)++failures;
 }
 for(unsigned family=0;family<2;++family)for(unsigned air=0;air<2;++air)for(unsigned used=0;used<2;++used){
  reset(family,0,0,0);fighters[0].u.lk.used_boomerang=used;fighters[1].native_module_state[2]=used;
  fighter_callback(air?ftLk_SpecialAirS_Enter:ftLk_SpecialS_Enter,air?authored_air_s:authored_ground_s,"fighter entry");
  ++comparisons;if(fighters[0].throw_flags!=fighters[1].throw_flags||fighters[0].cmd_vars[0]!=fighters[1].cmd_vars[0]||(fighters[0].accessory4_cb!=NULL)!=(fighters[1].accessory4_cb!=NULL)||fighters[0].u.lk.x4!=fighters[1].native_module_state[3])++failures;
 }
 const HSD_GObjEvent stock_anim[]={ftLk_SpecialS1_Anim,ftLk_SpecialS2_Anim,ftLk_SpecialS1Empty_Anim,ftLk_SpecialAirS1_Anim,ftLk_SpecialAirS2_Anim,ftLk_SpecialAirS1Empty_Anim};
 const HSD_GObjEvent script_anim[]={authored_ground_animation_s,authored_action_0,authored_action_4,authored_air_animation_s,authored_action_7,authored_action_11};
 const HSD_GObjEvent stock_coll[]={ftLk_SpecialS1_Coll,ftLk_SpecialS2_Coll,ftLk_SpecialS1Empty_Coll,ftLk_SpecialAirS1_Coll,ftLk_SpecialAirS2_Coll,ftLk_SpecialAirS1Empty_Coll};
 const HSD_GObjEvent script_coll[]={authored_ground_collision_s,authored_action_3,authored_action_6,authored_air_collision_s,authored_action_10,authored_action_13};
 const HSD_GObjEvent stock_phys[]={ftLk_SpecialS1_Phys,ftLk_SpecialS2_Phys,ftLk_SpecialS1Empty_Phys,ftLk_SpecialAirS1_Phys,ftLk_SpecialAirS2_Phys,ftLk_SpecialAirS1Empty_Phys};
 const HSD_GObjEvent script_phys[]={authored_ground_physics_s,authored_action_2,authored_action_5,authored_air_physics_s,authored_action_9,authored_action_12};
 for(unsigned family=0;family<2;++family)for(unsigned state=0;state<6;++state){
  for(unsigned command=0;command<3;++command)for(unsigned remaining=0;remaining<2;++remaining)for(unsigned exists=0;exists<2;++exists){
   reset(family,0,0,0);set_reference(exists);frames_remaining=remaining;fighters[0].cmd_vars[1]=fighters[1].cmd_vars[1]=command;
   fighter_callback(stock_anim[state],script_anim[state],"fighter animation");
   ++comparisons;if(fighters[0].cmd_vars[1]!=fighters[1].cmd_vars[1])++failures;
  }
  for(unsigned contact=0;contact<2;++contact)for(unsigned exists=0;exists<2;++exists){
   reset(family,0,0,0);set_reference(exists);ground_contact=contact;
   for(unsigned n=0;n<2;++n){fighters[n].ground_or_air=state<3?GA_Ground:GA_Air;fighters[n].cur_anim_frame=7.25f;}
   fighter_callback(stock_coll[state],script_coll[state],"fighter collision");
   ++comparisons;if((fighters[0].accessory4_cb!=NULL)!=(fighters[1].accessory4_cb!=NULL)||!same_float(fighters[0].cur_anim_frame,fighters[1].cur_anim_frame))++failures;
  }
  reset(family,0,0,0);fighter_callback(stock_phys[state],script_phys[state],"fighter physics");
 }
 for(unsigned family=0;family<2;++family)for(unsigned air=0;air<2;++air)for(unsigned check=0;check<10;++check)for(unsigned exists=0;exists<2;++exists){
  reset(family,0,0,0);set_reference(exists);interrupt_accept=check;
  fighter_callback(air?ftLk_SpecialAirS2_IASA:ftLk_SpecialS2_IASA,air?authored_action_8:authored_action_1,"fighter input priority");
 }
 for(unsigned family=0;family<2;++family)for(unsigned face=0;face<2;++face)for(unsigned fail=0;fail<2;++fail)
 for(unsigned cue=0;cue<2;++cue)for(unsigned launch=0;launch<2;++launch)for(unsigned smash=0;smash<2;++smash){
  reset(family,0,0,0);set_reference(!cue);creation_fails=fail;
  for(unsigned n=0;n<2;++n){fighters[n].throw_flags_b0=cue;fighters[n].cmd_vars[0]=launch;fighters[n].cmd_vars[1]=1;fighters[n].facing_dir=face?-1:1;fighters[n].input.lstick[0]=(Vec2){-.5f,.75f};fighters[n].coll_data.ecb.top.y=3;fighters[n].coll_data.ecb.bottom.y=-1;}
  fighters[0].u.lk.x4=smash;fighters[1].native_module_state[3]=smash;fighters[1].native_module_state[4]=true;
  active=0;onAccessory4(&creators[0]);active=1;script_fn_boomerang_accessory(&creators[1]);compare(0,0,"fighter create/launch");
  if(cue&&!fail){++comparisons;if(memcmp(&spawns[0].prev_pos,&spawns[1].prev_pos,sizeof(Vec3))||memcmp(&spawns[0].pos,&spawns[1].pos,sizeof(Vec3))||memcmp(&spawns[0].vel,&spawns[1].vel,sizeof(Vec3))||spawns[0].kind!=spawns[1].kind||spawns[0].facing_dir!=spawns[1].facing_dir||spawns[0].x44_flag.b0!=spawns[1].x44_flag.b0)++failures;}
  ++comparisons;if(fighters[0].throw_flags!=fighters[1].throw_flags||fighters[0].cmd_vars[0]!=fighters[1].cmd_vars[0]||fighters[0].cmd_vars[1]!=fighters[1].cmd_vars[1]||fighters[0].u.lk.used_boomerang!=fighters[1].native_module_state[2]||(fighters[0].death3_cb!=NULL)!=(fighters[1].death3_cb!=NULL)||(fighters[0].take_dmg_cb!=NULL)!=(fighters[1].take_dmg_cb!=NULL))++failures;
 }
}
int main(void)
{
 static const NativeFighterHost host={.item_creator=creator,.item_current_owner=current_owner,.item_reflector=reflector,.target_camera_target_position=target_position,.item_joint_value=nativeArticleModel_Read,.item_set_joint_value=nativeArticleModel_Write,.item_set_joint_transform=nativeArticleModel_SetTransform,.item_change_state_flags=Item_80268E5C,.item_set_model_animation_rate=nativeArticleModel_SetAnimationRate,.item_advance_model_animation=nativeArticleModel_AdvanceAnimation,.item_destroy_model=nativeArticleModel_Destroy,.item_load_model=nativeArticleModel_Load,.item_add_model_animation=nativeArticleModel_AddAnimation,.item_request_model_animation=nativeArticleModel_RequestAnimation,.item_remove_model_animation=nativeArticleModel_RemoveAnimation,.item_read_joint_matrix_rotation=nativeArticleModel_ReadMatrixRotation,.item_add_animation_descriptor=add_descriptor,.item_read_attachment_matrix_rotation=attached_matrix,.item_set_lifetime=it_80275158,.item_loop_sound=Item_8026AF0C,.sin_float=sinf,.cos_float=cosf,.sqrt_float=sqrtf,.angle_atan2_float=atan2f,.vector_mirror=lbVector_Mirror,.item_set_airborne=it_802762BC,.item_sweep_floor=it_8026E0F4,.item_remove=Item_8026A8EC,.item_interact=it_8026B894,.item_attach_to_owner=Item_8026AB54,.bone_count=bones,.change_motion_state=Fighter_ChangeMotionState,.start_animation=ftAnim_8006EBA4,.common_attribute=get_common,.item_release_attachment=it_8027429C,.item_use_manual_lifetime=it_8026B3A8,
 .resolve_bone=ftParts_GetBoneIndex,.owned_article_exists=owned_article,.signal_owned_article=signal_article,.spawn_bone_attached_article=spawn_article,
 .animation_frames_remaining=ftAnim_IsFramesRemaining,.finish_ground_action=ft_8008A2BC,.enter_fall=ftCo_Fall_Enter,.enter_landing_basic=ftCo_Landing_Enter_Basic,
 .ground_friction=ft_80084F3C,.air_gravity_and_friction=ft_80084EEC,.ground_collision=ft_80082708,.air_collision=ft_80081D0C,
 .ground_to_air=ftCommon_8007D5D4,.air_to_ground=ftCommon_8007D7FC,
 .check_side_special=ftCo_SpecialS_CheckInput,.check_up_special=ftCo_Attack100_CheckInput,.check_neutral_special=ftCo_800D6824,.check_down_special=ftCo_800D68C0,
 .check_guard=ftCo_80091A4C,.check_ground_jump=ftCo_Jump_CheckInput,.check_dash=ftCo_Dash_CheckInput,.check_air_special=ftCo_SpecialAir_CheckInput,.check_air_jump=ftCo_800CB870};
 engine=&host;script_state_base=0;
 const float angles[]={-7,-3.1415927f,-.5f,-0.0f,0,.5f,3.1415927f,7};
 for(unsigned family=0;family<2;++family)for(unsigned state=0;state<4;++state)for(int mode=0;mode<2;++mode)for(unsigned angle=0;angle<8;++angle){
  reset(family,state,angles[angle],mode);predicate(it_802A1F08,authored->logic->dmg_dealt,"damage");
  reset(family,state,angles[angle],mode);predicate(it_802A1FA8,authored->logic->clanked,"clank");
  reset(family,state,angles[angle],mode);predicate(itLinkBoomerang_Logic18_Absorbed,authored->logic->absorbed,"absorb");
  reset(family,state,angles[angle],mode);predicate(it_802A2288,authored->logic->hit_shield,"shield");
  reset(family,state,angles[angle],mode);items[0].xC58=items[1].xC58=(Vec3){.6f,.8f,0};predicate(it_802A2320,authored->logic->shield_bounced,"shield mirror");
  for(unsigned transfer=0;transfer<2;++transfer){reset(family,state,angles[angle],mode);items[0].xDCC_flag.b1=items[1].xDCC_flag.b1=transfer;predicate(it_802A20E8,authored->logic->reflected,"reflect");}
  if(state==1||state==2){reset(family,state,angles[angle],mode);callback(state==1?itLinkboomerang_UnkMotion1_Phys:itLinkboomerang_UnkMotion2_Phys,authored->logic->states[state].physics_updated,"outbound");}
 }
 for(unsigned family=0;family<2;++family)for(unsigned state=1;state<4;++state)
 for(unsigned command=0;command<3;++command)for(unsigned mask=0;mask<4;++mask)
 for(unsigned trail_command=0;trail_command<3;++trail_command)for(unsigned index=0;index<16;++index)
 for(unsigned expiry=0;expiry<3;++expiry){
  reset(family,state,.5f,0);enable_trails(mask);
  items[0].xDAC_itcmd_var0=items[1].xDAC_itcmd_var0=command;
  items[0].xDB0_itcmd_var1=items[1].xDB0_itcmd_var1=trail_command;
  items[0].xDB4_itcmd_var2=items[1].xDB4_itcmd_var2=trail_command;
  items[0].xD44_lifeTimer=items[1].xD44_lifeTimer=expiry==0?-1:expiry==1?0:.25f;
  items[0].xDD4_itemVar.linkboomerang.xDD8=index;write_int(&items[1],"history_write",index);
  items[0].xDD4_itemVar.linkboomerang.xDE4=2;write_int(&items[1],"turn_frames",2);
  items[0].xDD4_itemVar.linkboomerang.xF88=-.25f;write_field(&items[1],"spin_step",-.25f);
  predicate(it_803F6920[state].animated,authored->logic->states[state].animated,"animation/history/expiry");
 }
 for(unsigned family=0;family<2;++family)for(unsigned category=0;category<16;++category)
 for(unsigned hitlag=0;hitlag<2;++hitlag)for(unsigned creator_case=0;creator_case<2;++creator_case)
 for(unsigned timer=0;timer<3;++timer)for(unsigned catch_case=0;catch_case<2;++catch_case){
  reset(family,3,.5f,0);enable_trails(3);
  for(unsigned n=0;n<2;++n){fighters[n].x2070.x2071_b0_3=category;fighters[n].x2219_b5=hitlag;
   if(catch_case)fighters[n].cur_pos=(Vec3){3,-6,5};}
  if(creator_case){items[0].xDD4_itemVar.linkboomerang.xF98=NULL;items[1].xDD4_itemVar.native_article.attachment_owner=NULL;}
  float duration=timer==0?-1:timer==1?0:.25f;
  items[0].xDD4_itemVar.linkboomerang.xF80=duration;write_field(&items[1],"turn_timer",duration);
  callback(itLinkboomerang_UnkMotion3_Phys,authored->logic->states[3].physics_updated,"return/homing/catch");
 }
 for(unsigned family=0;family<2;++family)for(unsigned category=0;category<16;++category){
  reset(family,0,.5f,0);fighters[0].x2070.x2071_b0_3=fighters[1].x2070.x2071_b0_3=category;
  predicate(itLinkboomerang_UnkMotion0_Anim,authored->logic->states[0].animated,"held eligibility");
 }
 const Vec3 normals[]={{.6f,.8f,1},{0,1,-1},{1,0,2},{-.6f,.8f,0},{0,0,0}};
 for(unsigned family=0;family<2;++family)for(unsigned state=1;state<=2;++state)
 for(unsigned mode=0;mode<2;++mode)for(unsigned face=0;face<2;++face)
 for(unsigned contact=0;contact<16;++contact)for(unsigned normal=0;normal<5;++normal){
  reset(family,state,-.5f,mode);enable_trails(3);
  for(unsigned n=0;n<2;++n){items[n].facing_dir=face?-1:1;
   CollData* c=&items[n].x378_itemColl;c->env_flags=0;
   if(contact&1)c->env_flags|=Collide_LeftWallMask;
   if(contact&2)c->env_flags|=Collide_RightWallMask;
   if(contact&4)c->env_flags|=Collide_CeilingMask;
   if(contact&8)c->env_flags|=Collide_FloorMask;
   c->left_facing_wall.normal=normals[normal];c->right_facing_wall.normal=normals[(normal+1)%5];c->ceiling.normal=normals[(normal+2)%5];c->floor.normal=normals[(normal+3)%5];
  }
  predicate(state==1?itLinkboomerang_UnkMotion1_Coll:itLinkboomerang_UnkMotion2_Coll,authored->logic->states[state].collided,"stage contact priority");
 }
 const float speeds[]={0,.5f,1,1.25f,1.250001f,5,6};
 for(unsigned family=0;family<2;++family)for(unsigned state=1;state<=3;++state)
 for(unsigned speed=0;speed<7;++speed)for(unsigned angle=0;angle<8;++angle){
  reset(family,state,angles[angle],0);enable_trails(3);
  for(unsigned n=0;n<2;++n)items[n].x40_vel=(Vec3){speeds[speed]*cosf(angles[angle]),speeds[speed]*sinf(angles[angle]),.25f};
  callback(it_803F6920[state].physics_updated,authored->logic->states[state].physics_updated,"speed clamp transition");
 }
 for(unsigned family=0;family<2;++family)for(unsigned state=0;state<4;++state)
 for(unsigned mode=0;mode<2;++mode)for(unsigned creator_case=0;creator_case<2;++creator_case)
 for(unsigned owner_case=0;owner_case<3;++owner_case)for(unsigned trails_case=0;trails_case<4;++trails_case){
  reset(family,state,0,mode);enable_trails(trails_case);
  if(creator_case){items[0].xDD4_itemVar.linkboomerang.xF98=NULL;items[1].xDD4_itemVar.native_article.attachment_owner=NULL;}
  if(owner_case)for(unsigned n=0;n<2;++n)items[n].owner=owner_case==1?NULL:&reflectors[n];
  callback(itLinkBoomerang_Logic18_Destroyed,authored->logic->destroyed,"destroyed ownership");
 }
 for(unsigned family=0;family<2;++family)for(unsigned state=0;state<4;++state)for(unsigned other=0;other<3;++other){
  reset(family,state,0,0);active=0;it_802A23CC(&objects[0],other==0?&creators[0]:other==1?&reflectors[0]:NULL);
  active=1;authored->logic->evt_unk(&objects[1],other==0?&creators[1]:other==1?&reflectors[1]:NULL);compare(0,0,"interaction creator removal");
 }
 for(unsigned family=0;family<2;++family)for(unsigned state=0;state<4;++state){
  reset(family,state,0,0);callback(it_802A0F84,authored->logic->dropped,"dropped");
  reset(family,state,0,0);callback(it_802A0F88,authored->logic->thrown,"thrown");
  reset(family,state,0,0);callback(it_802A0E70,authored->logic->picked_up,"picked up");
 }
 fighter_cases();
 printf("Boomerang original-C comparisons: %u, failures: %u\n",comparisons,failures);return failures?1:0;
}
