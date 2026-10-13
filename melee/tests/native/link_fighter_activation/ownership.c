// Actual independent DLL and exact copied-base metadata. No modified fixture
// base. Stock pointers are poisoned sentinels used only for ownership rejection.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftLink/forward.h>
#include <melee/ft/kinds/ftCLink/forward.h>
#include <melee/it/kinds/types.h>
#include <melee/lb/types.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned checks,callbacks,phases,article_callbacks;
static HMODULE library;
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"line%d: %s\n",__LINE__,#c);exit(1);}}while(0)
static void stock_slot(HSD_GObj* g){(void)g;CHECK(false);}
static void unexpected(void){CHECK(false);}
#include "base.h"
static void own_pointer(const void* p)
{if(!p)return;MEMORY_BASIC_INFORMATION info;CHECK(VirtualQuery(p,&info,sizeof(info))==sizeof(info));CHECK(info.AllocationBase==library);}
// Controlled guard construction boundary for this DLL ownership-only fixture.
// The composition fixture compares the actual native volume setup/reaction.
static unsigned guard_constructions;
static unsigned guard_bone_count(Fighter* f)
{CHECK(f);return PHYSICAL_JOINTS;}
static int guard_previous_group(Fighter* f,int group)
{CHECK(f&&group==2);return 0;}
static void guard_construct(Fighter_GObj* g,ShieldDesc* shape,HSD_GObjEvent cb)
{
 CHECK(g&&g->user_data&&shape&&shape->bone>=0&&shape->bone<PHYSICAL_JOINTS&&cb);
 own_pointer((const void*)cb);
 Fighter* f=g->user_data;f->shield_hit_cb=cb;f->x221B_b0=true;
 ++guard_constructions;
}
#define ARTICLE(p) do{if(p){own_pointer((const void*)p);++article_callbacks;}}while(0)
int main(int argc,char** argv)
{
 CHECK(argc==2);library=LoadLibraryA(argv[1]);CHECK(library);NativeFighterHost host={0};
#include "required_host.h"
 typedef const NativeFighterDefinition*(*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
 Entry open=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");CHECK(open);
 const NativeFighterDefinition* original=&BASE_DEFINITION;
 const NativeFighterDefinition* d=open(original,&host);CHECK(d);
 CHECK(d->source_kind==Ft_Kind_None&&original->source_kind==BASE_KIND);
 CHECK(d->animation_source_kind==BASE_KIND);CHECK(d->joint_count==PHYSICAL_JOINTS);
 CHECK(d->action_count==314&&d->demo_count==14&&d->effect_bank==6);
 CHECK(d->motion_state_first==341&&d->motion_state_count==21);
 CHECK(d->demo_motion_names==original->demo_motion_names&&d->asset_layout.joint_graph_mask==0x40);
 CHECK(d->sound_banks==(UINT64_C(1)<<SOUND_BANK));
 CHECK(!d->items&&!d->item_count&&!d->neutral_article&&!d->owns_model_attachments);
 CHECK(d->on_load&&d->on_load_special_attrs&&d->on_death&&d->on_trail);
 CHECK(d->on_wait_enter&&d->on_crouch_wait_enter&&d->on_aerial_attack_enter);
 CHECK(d->on_forward_smash_start);
 Fighter selected={0};HSD_GObj object={0};object.user_data=&selected;
 host.create_guard=guard_construct;host.bone_count=guard_bone_count;
 host.previous_model_group=guard_previous_group;
 d->on_wait_enter(&object);
 CHECK(guard_constructions==1&&selected.x221B_b0&&selected.shield_hit_cb);
 own_pointer((const void*)selected.shield_hit_cb);
 // Invoke the actual generated post-common policy: the common entry selected
 // each row already. This verifies direct input binding, not common angle or
 // lifetime continuation engine behavior (tracked as a separate root boundary).
 for(unsigned row=58;row<=62;row++){
  selected.motion_id=row;selected.input_cb=stock_slot;d->on_forward_smash_start(&object);
  CHECK(selected.motion_id==row);CHECK(selected.input_cb&&selected.input_cb!=stock_slot);own_pointer((const void*)selected.input_cb);
 }

 CHECK(d->on_air_tether_input&&d->on_catch_allowed&&d->on_catch_start&&d->on_catch_frame&&d->on_catch_exit);
 CHECK(d->on_catch_wait_release&&d->on_catch_pull_anchor&&d->on_catch_pull_ready&&d->on_catch_pull_follow);
 CHECK(d->on_capture_wait_enter);if(BASE_KIND==Ft_Kind_CLink)CHECK(d->on_taunt);
#define VERIFY_CALLBACK(field) do{if(d->field){own_pointer((const void*)d->field);++callbacks;}}while(0)
#include "definition_callbacks.h"
#undef VERIFY_CALLBACK
 for(unsigned i=0;i<21;i++){
  const MotionState* a=&original->motion_states[i];const MotionState* b=&d->motion_states[i];
  HSD_GObjEvent before[]={a->anim_cb,a->input_cb,a->phys_cb,a->coll_cb};
  HSD_GObjEvent after[]={b->anim_cb,b->input_cb,b->phys_cb,b->coll_cb};
  for(unsigned c=0;c<4;c++){if(before[c]){CHECK(after[c]);++phases;}own_pointer((const void*)after[c]);}
  CHECK(a->cam_cb==b->cam_cb);
 }
 for(unsigned i=0;i<d->callback_export_count;i++)own_pointer((const void*)d->callback_exports[i].callback);
 CHECK(d->article_logic_count==ARTICLE_COUNT);
 for(unsigned i=0;i<d->article_logic_count;i++){
  const NativeFighterArticleLogic* a=&d->article_logics[i];ItemLogicTable* l=a->logic;CHECK(l&&l->states&&a->state_count);
  own_pointer(l);own_pointer(l->states);ARTICLE(a->on_created);ARTICLE(a->on_out_of_bounds);
  ARTICLE(l->spawned);ARTICLE(l->destroyed);ARTICLE(l->picked_up);ARTICLE(l->dropped);ARTICLE(l->thrown);
  ARTICLE(l->dmg_dealt);ARTICLE(l->dmg_received);ARTICLE(l->entered_air);ARTICLE(l->reflected);ARTICLE(l->clanked);
  ARTICLE(l->absorbed);ARTICLE(l->shield_bounced);ARTICLE(l->hit_shield);ARTICLE(l->evt_unk);
  for(unsigned s=0;s<a->state_count;s++){ARTICLE(l->states[s].animated);ARTICLE(l->states[s].physics_updated);ARTICLE(l->states[s].collided);}
  for(unsigned s=0;s<a->signal_count;s++)ARTICLE(a->signals[s].callback);
  if(a->projectile){ARTICLE(a->projectile->read_values);ARTICLE(a->projectile->on_launched);}
 }
 // Immutable metadata must be actual production copied-base data, not fixture
 // replacement values. The pre51 metadata gaps intentionally reject here.
 CHECK(d->bones==original->bones&&d->bone_count==53);CHECK(d->omissions==original->omissions&&d->omission_count==1);
 CHECK(d->omissions[0].joint==OMITTED_PART&&d->omissions[0].parent==OMITTED_PART-1);
 CHECK(d->omissions[0].insertion_mode==0&&d->omissions[0].donor_joint==255);
 CHECK(d->article_assets==original->article_assets&&d->article_asset_count==ARTICLE_COUNT);
 for(unsigned i=0;i<d->article_logic_count;i++){
  bool mapped=false;for(unsigned j=0;j<d->article_asset_count;j++)if(d->article_assets[j].kind==d->article_logics[i].kind)mapped=true;CHECK(mapped);
 }
 FreeLibrary(library);printf("{\"checks\":%u,\"definition_callbacks\":%u,\"stock_phases\":%u,\"article_callbacks\":%u,\"failures\":0}\n",checks,callbacks,phases,article_callbacks);return 0;
}
