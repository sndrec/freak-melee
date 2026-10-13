// Additional composed-module comparisons. Engine animation/presentation and
// Milk construction are controlled boundaries; full original decisions run.
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/itsword.h>
#include <melee/ft/native_fighter_trail.h>
static unsigned lifecycle_cases,taunt_cases,trail_cases;
static bool heavy_item;
static Item_HoldKinds hold_kind;
static ftLk_DatAttrs phase_attrs;
bool itIsHeavy(Item_GObj* g){(void)g;return heavy_item;}
Item_HoldKinds itGetHoldKind(Item_GObj* g){(void)g;return hold_kind;}
void ftParts_80074A4C(Fighter_GObj* g,int group,int index)
{CHECK(creator_side(g)==current);event(30,group,index);((Fighter*)g->user_data)->x5F4_arr[group].idx=index;}
void ftAnim_80070FB4(Fighter_GObj* g,s32 group,s32 index){CHECK(creator_side(g)==current);event(31,group,index);}
void ftAnim_80070C48(Fighter_GObj* g,s32 group){CHECK(creator_side(g)==current);event(32,group,0);}
void ftAnim_80070CC4(Fighter_GObj* g,int group){CHECK(creator_side(g)==current);event(33,group,0);}
void ftAnim_800704F0(Fighter_GObj* g,int group,float frame){CHECK(creator_side(g)==current);event(34,group,float_bits(frame));}
void ftCo_800DEAE8(Fighter_GObj* g,FtMotionId first,FtMotionId second)
{CHECK(creator_side(g)==current&&first==342&&second==343);event(35,first,second);((Fighter*)g->user_data)->motion_id=first;}
void ftCo_AppealS_IASA(Fighter_GObj* g){CHECK(creator_side(g)==current);event(36,0,0);}
void ft_80084F3C(Fighter_GObj* g){CHECK(creator_side(g)==current);event(37,0,0);}
void ft_80084280(Fighter_GObj* g){CHECK(creator_side(g)==current);event(38,0,0);}
static void phase_create_milk(unsigned side)
{
 CHECK(!article_live[side][Milk]);article_live[side][Milk]=true;
 article_objects[side][Milk].next=heads[HSD_GOBJ_PLINK_ITEM];heads[HSD_GOBJ_PLINK_ITEM]=&article_objects[side][Milk];
 if(!side)fighters[0].u.lk.x18=&article_objects[0][Milk];
}
HSD_GObj* it_802C8B28(Item_GObj* g,Vec3* position,u32 bone,float facing)
{
 CHECK(g==&creators[0]&&position==&fighters[0].cur_pos&&bone==0&&facing==fighters[0].facing_dir);
 event(39,bone,float_bits(facing));if(creation_failure)return NULL;phase_create_milk(0);return &article_objects[0][Milk];
}
static s32 phase_spawn(Fighter_GObj* g,ItemKind kind,Fighter_Part part)
{
 if(kind!=It_Kind_CLink_Milk)return spawn_attached(g,kind,part);
 CHECK(g==&creators[1]&&part==FtPart_LThumbNb);event(39,0,float_bits(fighters[1].facing_dir));
 if(creation_failure)return -1;phase_create_milk(1);return (s32)article_items[1][Milk].x1C;
}
ItemKind itGetKind(Item_GObj* g){(void)g;abort();return 0;}
itSword_UnkBytes* it_80285300(Item_GObj* g){(void)g;abort();return NULL;}
void it_802852B8(Item_GObj* g,float* a,float* b){(void)g;(void)a;(void)b;abort();}
HSD_JObj* it_80285314(Item_GObj* g){(void)g;abort();return NULL;}
#include "phase_originals.h"
static void phase_init(NativeFighterHost* host)
{
 host->item_is_heavy=itIsHeavy;host->set_visibility_group=ftParts_80074A4C;
 host->fighter_item_pickup=Fighter_OnItemPickup;host->fighter_item_drop=Fighter_OnItemDrop;
 host->fighter_item_visible=Fighter_OnItemVisible;host->fighter_item_invisible=Fighter_OnItemInvisible;
 host->fighter_knockback_enter=Fighter_OnKnockbackEnter;host->fighter_knockback_exit=Fighter_OnKnockbackExit;
 host->animation_frames_remaining=ftAnim_IsFramesRemaining;host->enter_taunt=ftCo_800DEAE8;host->taunt_actions=ftCo_AppealS_IASA;host->ground_friction=ft_80084F3C;
 host->ground_nudge_collision=ft_80084280;host->spawn_bone_attached_article=phase_spawn;
}
static void phase_compare(void)
{
 CHECK(trace_count[0]==trace_count[1]);CHECK(!memcmp(trace[0],trace[1],trace_count[0]*sizeof(CleanupEvent)));
 CHECK(fighters[0].motion_id==fighters[1].motion_id);
 for(unsigned i=0;i<3;i++)CHECK(fighters[0].x5F4_arr[i].idx==fighters[1].x5F4_arr[i].idx);
}
static void phase_matrix(void)
{
 for(unsigned path=0;path<8;path++)for(unsigned heavy=0;heavy<2;heavy++)for(unsigned held=0;held<6;held++)for(unsigned flag=0;flag<2;flag++){
  reset(0,0,0,0,0,0);for(unsigned side=0;side<2;side++){fighters[side].x5AC.model_num=3;fighters[side].item_gobj=&article_objects[side][Milk];}heavy_item=heavy;hold_kind=(Item_HoldKinds)held;
  current=0;
  switch(path){case 0:if(family==Ft_Kind_Link)ftLk_Init_OnItemPickupExt(&creators[0],flag);else ftCl_Init_OnItemPickupExt(&creators[0],flag);break;
   case 1:if(family==Ft_Kind_Link)ftLk_Init_OnItemDropExt(&creators[0],flag);else ftCl_Init_OnItemDropExt(&creators[0],flag);break;
   case 2:ftLk_Init_OnItemPickup(&creators[0],flag);break;case 3:ftLk_Init_OnItemDrop(&creators[0],flag);break;
   case 4:ftLk_Init_OnItemVisible(&creators[0]);break;case 5:ftLk_Init_OnItemInvisible(&creators[0]);break;
   case 6:ftLk_Init_OnKnockbackEnter(&creators[0]);break;case 7:ftLk_Init_OnKnockbackExit(&creators[0]);break;}
  current=1;
  switch(path){case 0:definition->on_item_pickup_ext(&creators[1],flag);break;case 1:definition->on_item_drop_ext(&creators[1],flag);break;
   case 2:definition->on_item_pickup(&creators[1],flag);break;case 3:definition->on_item_drop(&creators[1],flag);break;
   case 4:definition->on_item_visible(&creators[1]);break;case 5:definition->on_item_invisible(&creators[1]);break;
   case 6:definition->on_knockback_enter(&creators[1]);break;case 7:definition->on_knockback_exit(&creators[1]);break;}
  phase_compare();++lifecycle_cases;
 }
 reset(0,0,0,0,0,0);fighters[0].x5AC.model_num=fighters[1].x5AC.model_num=3;fighters[0].u.lk.used_boomerang=true;fighters[1].native_module_state[field_offset(definition,"boomerang_used")]=1;
 fighters[0].u.lk.boomerang_gobj=fighters[0].u.lk.xC=fighters[0].u.lk.arrow_gobj=fighters[0].u.lk.x14=fighters[0].u.lk.x18=(void*)(uintptr_t)1;
 for(unsigned i=0;i<(family==Ft_Kind_Link?4:5);i++)put(definition,&fighters[1],handles[i],16777219+i);
 current=0;if(family==Ft_Kind_Link)ftLk_Init_OnDeath(&creators[0]);else ftCl_Init_OnDeath(&creators[0]);current=1;definition->on_death(&creators[1]);phase_compare();
 CHECK(!fighters[0].u.lk.used_boomerang&&!fighters[0].u.lk.xC&&!fighters[0].u.lk.x18);
 CHECK(!fighters[1].native_module_state[field_offset(definition,"boomerang_used")]);for(unsigned i=0;i<(family==Ft_Kind_Link?4:5);i++)CHECK(get(definition,&fighters[1],handles[i])==-1);++lifecycle_cases;
 // Native trail reader copies the exact imported owned scalar/byte fields.
 memset(&phase_attrs,0,sizeof(phase_attrs));
 phase_attrs.x64.x0=parameter_float("sword_inner_fade");phase_attrs.x64.x4=parameter_float("sword_outer_fade");
 phase_attrs.x64.x8=parameter_value("sword_start_alpha");phase_attrs.x64.x9=parameter_value("sword_end_alpha");
 phase_attrs.x64.xA=parameter_value("sword_inner_red");phase_attrs.x64.xB=parameter_value("sword_inner_green");phase_attrs.x64.xC=parameter_value("sword_inner_blue");
 phase_attrs.x64.xE=parameter_value("sword_outer_red");phase_attrs.x64.xF=parameter_value("sword_outer_green");phase_attrs.x64.x10=parameter_value("sword_outer_blue");
 phase_attrs.x64.x14=parameter_value("sword_bone");phase_attrs.x64.x18=parameter_float("sword_inner_distance");phase_attrs.x64.x1C=parameter_float("sword_outer_distance");
 for(unsigned present=0;present<2;present++){
  reset(0,0,0,0,0,0);part_tables[family]=&part_table;
  for(unsigned s=0;s<2;s++){fighters[s].dat_attrs=&phase_attrs;parts[s][phase_attrs.x64.x14].joint=present?&common_joints[s]:NULL;}
  NativeFighterTrail trail[2];HSD_JObj* anchor[2];current=0;bool a=read_trail(&creators[0],&trail[0],&anchor[0]);current=1;bool b=read_trail(&creators[1],&trail[1],&anchor[1]);
  CHECK(a==b&&a==present);CHECK(!memcmp(&trail[0],&trail[1],sizeof(trail[0])));if(present)CHECK(anchor[0]==&common_joints[0]&&anchor[1]==&common_joints[1]);++trail_cases;
 }
 if(family==Ft_Kind_CLink){
  reset(0,0,0,0,0,0);fighters[0].cmd_vars[1]=fighters[1].cmd_vars[1]=7;current=0;ftCl_Init_80149318(&creators[0]);current=1;definition->on_taunt(&creators[1]);phase_compare();CHECK(!fighters[0].cmd_vars[1]&&!fighters[1].cmd_vars[1]);++taunt_cases;
  for(unsigned row=342;row<=343;row++)for(unsigned command=0;command<4;command++)for(unsigned present=0;present<2;present++)for(unsigned fail=0;fail<2;fail++)for(unsigned finished=0;finished<2;finished++){
   reset(present?16:0,0,0,0,0,0);creation_failure=fail;animation_remaining=!finished;
   for(unsigned s=0;s<2;s++){fighters[s].motion_id=row;fighters[s].cmd_vars[1]=command;fighters[s].facing_dir=-1;}
   current=0;ftCl_AppealS_Anim(&creators[0]);current=1;definition->motion_states[row].anim_cb(&creators[1]);phase_compare();
   CHECK(article_live[0][Milk]==article_live[1][Milk]);CHECK((fighters[0].u.lk.x18!=NULL)==(get(definition,&fighters[1],"milk_handle")!=-1));
   CHECK((fighters[0].take_dmg_cb!=NULL)==(fighters[1].take_dmg_cb!=NULL));CHECK((fighters[0].death2_cb!=NULL)==(fighters[1].death2_cb!=NULL));++taunt_cases;
   current=0;ftCl_Init_80149268(&creators[0]);current=1;script_fn_Cleanup_AllArticles(&creators[1]);no_leaks();
  }
  for(unsigned row=342;row<=343;row++)for(unsigned phase=0;phase<3;phase++){
   reset(0,0,0,0,0,0);current=0;if(phase==0)ftCl_AppealS_IASA(&creators[0]);if(phase==1)ftCl_AppealS_Phys(&creators[0]);if(phase==2)ftCl_AppealS_Coll(&creators[0]);
   current=1;HSD_GObjEvent callback=phase==0?definition->motion_states[row].input_cb:phase==1?definition->motion_states[row].phys_cb:definition->motion_states[row].coll_cb;callback(&creators[1]);phase_compare();++taunt_cases;
  }
 }
 no_leaks();
 fprintf(stderr,"phase cases: lifecycle=%u taunt=%u trail=%u\n",lifecycle_cases,taunt_cases,trail_cases);
}
