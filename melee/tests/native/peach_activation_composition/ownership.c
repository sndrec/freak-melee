// Actual generated DLL, copied production base and item behavior selector.
// Runtime lookup, shared common callbacks and GX drawing are explicit sinks.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftPeach/forward.h>
#include <melee/ft/kinds/ftPeach/types.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/itdraw.h>
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/types.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, callbacks, phases, article_callbacks, selection_cases, reset_pairs, load_pairs;
static HMODULE library;
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"line%d: %s\n",__LINE__,#c); exit(1); } } while(0)
static void stock_slot(HSD_GObj* g) { (void)g; CHECK(false); }
static void unexpected(void) { CHECK(false); }
#include "base.h"
#include "parameters.h"
static const NativeFighterDefinition* definition;
static MotionState common[147];
static void common_anim(HSD_GObj* g) { (void)g; unexpected(); }
static void common_input(HSD_GObj* g) { (void)g; unexpected(); }
static void common_phys(HSD_GObj* g) { (void)g; unexpected(); }
static void common_coll(HSD_GObj* g) { (void)g; unexpected(); }
static Article assets[5];
static Fighter fighter;
static HSD_GObj owner;
static unsigned group_count;
static struct { int group, value; } groups[8];
void ftParts_80074A4C(Fighter_GObj* g, int group, int value)
{ CHECK(g==&owner && group_count<8); groups[group_count].group=group; groups[group_count++].value=value; }
static ftPe_DatAttrs attributes, backup;
static struct ftData fighter_data;
static Article* load_assets[5];
static float clip_frames[2];
static unsigned clip_reads, registrations;
FigaTree* ftData_80085E50(Fighter* f, enum_t index)
{ CHECK(f==&fighter&&index>=18&&index<=19);return (FigaTree*)(uintptr_t)(index+1); }
float lbAnim_8001E8F8(FigaTree* clip)
{ unsigned index=(unsigned)(uintptr_t)clip-19;CHECK(index<2&&index==clip_reads);++clip_reads;return clip_frames[index]; }
void it_8026B3F8(Article* asset,s32 kind)
{
 CHECK(registrations<5);const NativeFighterArticleAsset* row=&definition->article_assets[registrations];
 CHECK(row->slot==registrations&&asset==load_assets[row->slot]&&row->kind==kind);++registrations;
}
static void copy_attributes(Fighter_GObj* g)
{ CHECK(g==&owner);backup=attributes;fighter.dat_attrs=&backup; }
static void reload_attributes(Fighter_GObj* g)
{ CHECK(g==&owner&&fighter.dat_attrs==&backup);backup=attributes; }
#include "lifecycle.h"

static int32_t state_int(const char* name)
{
 for(unsigned i=0;i<definition->state_field_count;i++) {
  const NativeFighterStateField* f=&definition->state_fields[i];
  if(strcmp(f->name,name)==0) {
   int32_t value=0; CHECK(f->offset+f->stride<=sizeof(fighter.native_module_state));
   memcpy(&value,fighter.native_module_state+f->offset,f->stride); return value;
  }
 }
 unexpected(); return 0;
}
static float state_float(const char* name)
{
 for(unsigned i=0;i<definition->state_field_count;i++)if(strcmp(definition->state_fields[i].name,name)==0) {
  const NativeFighterStateField* f=&definition->state_fields[i];float value;
  CHECK(f->stride==4);memcpy(&value,fighter.native_module_state+f->offset,4);return value;
 }
 unexpected();return 0;
}
static void compare_load(void)
{
 for(unsigned test=0;test<8;test++) {
  memset(&fighter,0,sizeof fighter);memset(&attributes,0x5A,sizeof attributes);
  ftPe_DatAttrs original_attributes=attributes;
  fighter.ft_data=&fighter_data;fighter_data.ext_attr=&attributes;fighter_data.x48_items=(UNK_T*)load_assets;
  fighter.dat_attrs_backup=&backup;owner.user_data=&fighter;
  clip_frames[0]=13.5f+test;clip_frames[1]=31.75f-test;
  for(unsigned i=0;i<5;i++)load_assets[i]=&assets[i];
  clip_reads=registrations=0;ftPe_Init_OnLoad(&owner);CHECK(clip_reads==2&&registrations==5);
  CHECK(fighter.dat_attrs==&backup);
  float forward=backup.floatfallf_anim_start,backward=backup.floatfallb_anim_start;
  ftPe_DatAttrs native_backup=backup;
  attributes=original_attributes;clip_reads=registrations=0;definition->on_load(&owner);
  CHECK(clip_reads==2&&registrations==0&&fighter.dat_attrs==&backup);
  CHECK(state_float("float_fall_forward_end")==forward&&state_float("float_fall_backward_end")==backward);
  // Derived animation lengths moved to ordinary authored state. Other copied
  // DAT bytes retain the full source structure, with no donor registration.
  native_backup.floatfallf_anim_start=backup.floatfallf_anim_start;
  native_backup.floatfallb_anim_start=backup.floatfallb_anim_start;
  CHECK(memcmp(&native_backup,&backup,sizeof backup)==0);
  memset(&backup,0xA5,sizeof backup);ftPe_Init_LoadSpecialAttrs(&owner);native_backup=backup;
  memset(&backup,0xA5,sizeof backup);definition->on_load_special_attrs(&owner);
  CHECK(memcmp(&native_backup,&backup,sizeof backup)==0);++load_pairs;
 }
}
static void compare_reset(void)
{
 for(unsigned costume=0;costume<5;costume++) for(unsigned old=0;old<4;old++) {
  memset(&fighter,0,sizeof fighter); memset(fighter.native_module_state,0xA5,sizeof fighter.native_module_state);
  owner.user_data=&fighter; fighter.x619_costume_id=costume;fighter.x5AC.model_num=7;
  fighter.u.pe.has_float=old&1; fighter.u.pe.specialairn_used=(old&2)!=0;
  fighter.u.pe.attacks4_motion_id=363;
  fighter.u.pe.parasol_gobj_0=fighter.u.pe.parasol_gobj_1=fighter.u.pe.toad_gobj=fighter.u.pe.veg_gobj=&owner;
  group_count=0; ftPe_Init_OnDeath(&owner); CHECK(group_count==7);
  unsigned char original[sizeof groups]; memcpy(original,groups,sizeof groups);
  group_count=0; definition->on_death(&owner); CHECK(group_count==7);
  CHECK(memcmp(original,groups,sizeof groups)==0);
  CHECK(state_int("has_float")==fighter.u.pe.has_float);
  CHECK(state_int("toad_air_boost_used")==fighter.u.pe.specialairn_used);
  CHECK(state_int("last_smash_motion")==fighter.u.pe.attacks4_motion_id);
  CHECK(state_int("parasol_handle")==-1 && state_int("saved_handle")==-1);
  CHECK(state_int("toad_handle")==-1 && state_int("vegetable_handle")==-1);
  ++reset_pairs;
 }
}
// These identity lookups control asset lifetime; no renderer/model allocation
// is implemented by this fixture. The production selector itself is unchanged.
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_Peach?definition:NULL; }
Article* nativeFighterRuntime_ArticleAssets(FighterKind author,ItemKind kind)
{
 if(author!=Ft_Kind_Peach)return NULL;
 for(unsigned i=0;i<5;i++)if(definition->article_logics[i].kind==kind)return &assets[i];
 return NULL;
}
const NativeFighterArticleLogic* nativeFighterRuntime_ArticleLogic(const Fighter* f,ItemKind kind)
{
 if(f!=&fighter)return NULL;
 for(unsigned i=0;i<5;i++)if(definition->article_logics[i].kind==kind)return &definition->article_logics[i];
 return NULL;
}
bool nativeFighterRuntime_ResolveArticleForItem(const Fighter* f,ItemKind kind,Article** out)
{ if(f!=&fighter)return false; *out=nativeFighterRuntime_ArticleAssets(f->kind,kind); return true; }
bool ftLib_80086960(HSD_GObj* g) { return g==&owner; }
void it_8026EECC(HSD_GObj* g,int pass) { (void)g;(void)pass;unexpected(); }
void OSReport(char* format,...) { (void)format;unexpected(); }
void __assert(char* path,u32 line,char* text) { fprintf(stderr,"%s:%u %s\n",path,(unsigned)line,text);exit(1); }
#include "selection.h"
static void select_articles(void)
{
 Item item={0}; HSD_GObj object={0}; SpawnItem spawn={0};
 object.user_data=&item;owner.user_data=&fighter;fighter.kind=Ft_Kind_Peach;
 for(unsigned i=0;i<5;i++)for(unsigned mode=0;mode<3;mode++)for(unsigned transferred=0;transferred<2;transferred++) {
  const NativeFighterArticleLogic* logic=&definition->article_logics[i];
  memset(&item,0xA5,sizeof item); item.kind=logic->kind;
  // Damage owner can be NULL/foreign; explicit author selection never reads it.
  item.owner=transferred?NULL:&owner;
  memset(&spawn,0,sizeof spawn); spawn.x0_parent_gobj=&owner;
  ItemLogicTable legacy={0};ItemStateTable legacy_states[1]={0};Article legacy_asset={0};
  item.xB8_itemLogicTable=&legacy;item.xBC_itemStateContainer=legacy_states;item.xC4_article_data=&legacy_asset;
  item.native_article_author_kind=Ft_Kind_None;object.render_cb=NULL;
  nativeItemBehavior_Select(&object,&spawn,(NativeItemBehaviorSelection)mode,Ft_Kind_Peach);
  if(mode==NativeItemBehavior_Engine) {
   CHECK(item.xB8_itemLogicTable==&legacy&&item.xBC_itemStateContainer==legacy_states&&item.xC4_article_data==&legacy_asset);
   CHECK(object.render_cb==NULL && item.native_article_author_kind==Ft_Kind_None);
  } else {
   CHECK(item.xB8_itemLogicTable==logic->logic && item.xBC_itemStateContainer==logic->logic->states);
   CHECK(item.xC4_article_data==&assets[i] && item.native_article_author_kind==Ft_Kind_Peach);
   CHECK(object.render_cb==it_8026EECC);
   CHECK(item.xDD4_itemVar.native_article.attachment_owner==NULL);
   CHECK(item.xDD4_itemVar.native_article.creator_spawn_id==0);
   for(unsigned byte=0;byte<sizeof item.xDD4_itemVar.native_article.state;byte++)CHECK(item.xDD4_itemVar.native_article.state[byte]==0);
  }
  CHECK(item.owner==(transferred?NULL:&owner));++selection_cases;
 }
}
static void own_pointer(const void* p)
{ if(!p)return;MEMORY_BASIC_INFORMATION info;CHECK(VirtualQuery(p,&info,sizeof info)==sizeof info);CHECK(info.AllocationBase==library); }
#define ARTICLE(p) do { if(p) { own_pointer((const void*)p);++article_callbacks; } } while(0)
int main(int argc,char** argv)
{
 CHECK(argc==2);library=LoadLibraryA(argv[1]);CHECK(library);
 NativeFighterHost host={0};
#include "required_host.h"
 common[144]=common[146]=(MotionState){.anim_cb=common_anim,.input_cb=common_input,.phys_cb=common_phys,.coll_cb=common_coll};
 host.common_motion_states=common;host.set_visibility_group=ftParts_80074A4C;
 host.animation_clip=ftData_80085E50;host.clip_length=lbAnim_8001E8F8;
 host.copy_ext_attributes=copy_attributes;host.reload_ext_attributes=reload_attributes;
 typedef int(*SetParameters)(const uint32_t*,unsigned,uint64_t);
 SetParameters set=(SetParameters)GetProcAddress(library,"FreakMelee_SetFighterParameters");CHECK(set);
 CHECK(set(parameter_bits,sizeof parameter_bits/sizeof parameter_bits[0],PARAMETER_SCHEMA));
 typedef const NativeFighterDefinition*(*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
 Entry open=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");CHECK(open);
 definition=open(&ftPeachClone_Definition,&host);CHECK(definition);const NativeFighterDefinition* d=definition;
 CHECK(d->source_kind==Ft_Kind_None&&d->animation_source_kind==Ft_Kind_Peach);
 CHECK(d->bones==ftPeachClone_Definition.bones&&d->bone_count==53&&d->joint_count==114);
 CHECK(!d->omission_count&&!d->omissions&&d->action_count==318&&d->demo_count==14);
 CHECK(d->motion_state_first==341&&d->motion_state_count==30);
 CHECK(d->article_asset_count==5&&d->article_assets==ftPeachClone_Definition.article_assets);
 CHECK(d->asset_layout.article_mask==0x1F&&!d->asset_layout.joint_graph_mask);
 CHECK(d->effect_bank==15&&d->sound_banks==(UINT64_C(1)<<22));
 CHECK(d->demo_motion_names==ftPeachClone_Definition.demo_motion_names);
 CHECK(d->on_load&&d->on_load_special_attrs&&d->on_death);
 CHECK(d->on_neutral_enter&&d->on_air_resume_motion&&d->on_held_timer_expired&&d->on_held_motion);
 CHECK(d->on_grounded_motion_setup&&d->on_aerial_action_manual&&d->on_aerial_action_automatic);
 CHECK(d->on_cpu_attack_motion&&d->on_cpu_ranged_candidates&&d->on_dynamics_interaction_start&&d->on_body_rebound);
 CHECK(d->on_forward_smash_enter&&d->on_aerial_jump);
 // Release-versus has ordinary common taunt; no silent debug-only mutation.
 CHECK(!d->on_taunt&&!d->items&&!d->item_count&&!d->neutral_article&&!d->owns_model_attachments);
#define VERIFY_CALLBACK(field) do { if(d->field) { own_pointer((const void*)d->field);++callbacks; } } while(0)
#include "definition_callbacks.h"
#undef VERIFY_CALLBACK
 for(unsigned row=0;row<30;row++) {
  const MotionState* a=&ftPeachClone_Definition.motion_states[row];const MotionState* b=&d->motion_states[row];
  HSD_GObjEvent before[]={a->anim_cb,a->input_cb,a->phys_cb,a->coll_cb};
  HSD_GObjEvent after[]={b->anim_cb,b->input_cb,b->phys_cb,b->coll_cb};
  for(unsigned phase=0;phase<4;phase++) {
   if(before[phase]) { CHECK(after[phase]);++phases; }
   if(row>=28) { HSD_GObjEvent shared[]={common_anim,common_input,common_phys,common_coll};CHECK(after[phase]==shared[phase]); }
   else own_pointer((const void*)after[phase]);
  }
  CHECK(b->cam_cb==a->cam_cb);
 }
 CHECK(phases==114&&d->article_logic_count==5);
 for(unsigned i=0;i<d->callback_export_count;i++)own_pointer((const void*)d->callback_exports[i].callback);
 for(unsigned i=0;i<5;i++) {
  const NativeFighterArticleLogic* a=&d->article_logics[i];ItemLogicTable* l=a->logic;
  CHECK(l&&l->states&&a->state_count&&a->standard_render);own_pointer(l);own_pointer(l->states);
  bool mapped=false;for(unsigned j=0;j<5;j++)if(d->article_assets[j].kind==a->kind) { mapped=true;CHECK(d->article_assets[j].slot==j&&d->article_assets[j].kind_attribute_offset==-1); } CHECK(mapped);
  ARTICLE(a->on_created);ARTICLE(a->on_out_of_bounds);ARTICLE(a->on_holder_neutral);
  ARTICLE(l->spawned);ARTICLE(l->destroyed);ARTICLE(l->picked_up);ARTICLE(l->dropped);ARTICLE(l->thrown);
  ARTICLE(l->dmg_dealt);ARTICLE(l->dmg_received);ARTICLE(l->entered_air);ARTICLE(l->reflected);ARTICLE(l->clanked);
  ARTICLE(l->absorbed);ARTICLE(l->shield_bounced);ARTICLE(l->hit_shield);ARTICLE(l->evt_unk);
  for(unsigned s=0;s<a->state_count;s++) { ARTICLE(l->states[s].animated);ARTICLE(l->states[s].physics_updated);ARTICLE(l->states[s].collided); }
  for(unsigned s=0;s<a->signal_count;s++)ARTICLE(a->signals[s].callback);
 }
 compare_reset();compare_load();select_articles();
 FreeLibrary(library);
 printf("{\"checks\":%u,\"definition_callbacks\":%u,\"stock_phases\":%u,\"article_callbacks\":%u,\"selection_cases\":%u,\"reset_pairs\":%u,\"load_pairs\":%u,\"failures\":0}\n",checks,callbacks,phases,article_callbacks,selection_cases,reset_pairs,load_pairs);return 0;
}
