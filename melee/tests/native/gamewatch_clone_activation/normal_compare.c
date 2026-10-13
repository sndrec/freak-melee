// Actual generated callbacks against unchanged frozen source. Shared item
// destruction, lookup, attachment and presentation are named controlled engine
// boundaries. No allocator, paper renderer, game or complete entry acceptance.
#include "behavior.generated.h"
#include "parameters.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatch.h>
#include <melee/ft/kinds/ftGameWatch/forward.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
static Fighter fighters[2];static HSD_GObj owners[2], objects[2][10],*heads[32];
static Item items[2][10];static HSD_JObj roots[2][10];
HSD_GObj** HSD_GObjPLinkHead=heads;
static unsigned side,checks,failures,pairs;
static const NativeFighterDefinition* definition;
static bool active,charged;
static ItemKind kinds[]={It_Kind_GameWatch_Greenhouse,It_Kind_GameWatch_Manhole,It_Kind_GameWatch_Fire,
 It_Kind_GameWatch_Parachute,It_Kind_GameWatch_Turtle,It_Kind_GameWatch_Breath,
 It_Kind_GameWatch_Judge,It_Kind_GameWatch_Panic,It_Kind_GameWatch_Rescue};
static const char* fields[]={"greenhouse_handle","manhole_handle","torch_handle","parachute_handle",
 "turtle_handle","sparky_handle","judge_handle","panic_handle","rescue_handle"};
typedef struct NormalEvent{char op;int a,b;float v;}NormalEvent;
static NormalEvent events[2][512];static unsigned count[2];static const char* label;
static void check(bool b,const char* why){++checks;if(!b&&failures++<24)fprintf(stderr,"%s %s pair%u\n",label?label:"setup",why,pairs);}
static void event(char op,int a,int b,float v){check(count[side]<512,"event capacity");events[side][count[side]++]=(NormalEvent){op,a,b,v};}
static unsigned index(Item_GObj*g){check(g>=objects[side]&&g<objects[side]+10,"item identity");return(unsigned)(g-objects[side]);}
static void set(const char*n,int v){for(unsigned i=0;i<definition->state_field_count;i++)if(!strcmp(n,definition->state_fields[i].name)){memcpy(fighters[1].native_module_state+script_state_base+definition->state_fields[i].offset,&v,definition->state_fields[i].type==NativeFighterState_Bool?1:4);return;}abort();}
static int get(const char*n){for(unsigned i=0;i<definition->state_field_count;i++)if(!strcmp(n,definition->state_fields[i].name)){int v=0;memcpy(&v,fighters[1].native_module_state+script_state_base+definition->state_fields[i].offset,definition->state_fields[i].type==NativeFighterState_Bool?1:4);return v;}abort();}
static const NativeFighterArticleLogic* logic(unsigned k){for(unsigned i=0;i<definition->article_logic_count;i++)if(definition->article_logics[i].kind==kinds[k])return definition->article_logics+i;abort();}
static HSD_GObj** original_handle(unsigned k){switch(k){case 0:return &fighters[0].u.gw.x224C_greenhouseGObj;case 1:return &fighters[0].u.gw.x2250_manholeGObj2;case 2:return &fighters[0].u.gw.x2254_fireGObj;case 3:return &fighters[0].u.gw.x2258_parachuteGObj;case 4:return &fighters[0].u.gw.x225C_turtleGObj;case 5:return &fighters[0].u.gw.x2260_sparkyGObj;case 6:return &fighters[0].u.gw.x2264_judgementGObj;case 7:return &fighters[0].u.gw.x2268_panicGObj;default:return &fighters[0].u.gw.x226C_rescueGObj;}}
static void select(unsigned s){side=s;heads[HSD_GOBJ_PLINK_FIGHTER]=owners+s;heads[HSD_GOBJ_PLINK_ITEM]=objects[s];}
static void dummy_callback(HSD_GObj*g){(void)g;}
void Item_80268E5C(Item_GObj*g,enum_t motion,Item_StateChangeFlags flags){items[side][index(g)].msid=motion;event('s',index(g),motion,(float)flags);}
void Item_802694CC(Item_GObj*g){event('a',index(g),0,0);}
bool it_8026B894(Item_GObj*g,Item_GObj* other){check(other==NULL,"interaction other");event('i',index(g),0,0);return false;}
bool it_80272C6C(Item_GObj*g){event('q',index(g),0,0);return active;}
static bool animation_active(HSD_JObj*j){check(j==&roots[side][0],"Greenhouse root animation");event('q',0,0,0);return active;}
void it_8026BB20(Item_GObj*g){roots[side][index(g)].flags&=~JOBJ_HIDDEN;event('v',index(g),1,0);}
void it_8026BB44(Item_GObj*g){roots[side][index(g)].flags|=JOBJ_HIDDEN;event('v',index(g),0,0);}
void lb_8000BA0C(HSD_JObj*j,float rate){check(j==&roots[side][2],"Torch root rate");event('r',2,0,rate);}
bool ftLib_800876D4(Fighter_GObj*g){check(g==owners+side,"charge owner");return charged;}
#ifdef BUCKET_ONLY
void ftParts_80074D7C(FtPartsVis*vis,int channel,DObjList*list){check(vis==&fighters[side].x5AC&&channel==0&&list==&fighters[side].dobj_list,"current group refresh arguments");event('D',0,0,0);}
#include "bucket_original.h"
#else
void ftParts_80074A4C(Fighter_GObj*g,int group,int value){check(g==owners+side,"group owner");event('g',group,value,0);}
#endif
void ftpickupitem_80094818(Fighter_GObj*g,bool v){check(g==owners+side&&v,"restore pickup args");event('p',9,v,0);}
bool itIsHeavy(Item_GObj*g){check(g==objects[side]+9,"presentation held item");return false;}
Item_HoldKinds itGetHoldKind(Item_GObj*g){check(g==objects[side]+9,"presentation hold kind");return (Item_HoldKinds)1;}
void ftAnim_80070FB4(Fighter_GObj*g,s32 flag,s32 variant){check(g==owners+side&&flag==1,"presentation group args");event('P',flag,variant,0);}
void ftAnim_80070C48(Fighter_GObj*g,s32 flag){check(g==owners+side&&flag==1,"presentation visible args");event('V',0,flag,0);}
void ftAnim_80070CC4(Fighter_GObj*g,int flag){check(g==owners+side&&flag==1,"presentation invisible args");event('V',1,flag,0);}
bool ftCo_800BFFD0(Fighter*f,enum_t id,bool a){check(f==fighters+side&&id==5&&!a,"full bucket color args");event('c',id,a,0);return true;}
// The real dispatcher is out of scope, but its destroyed callback is executed
// at this exact boundary, including repeated clear paths from original helpers.
void Item_8026A8EC(Item_GObj*g);
static uint32_t current_owner(Item_GObj*g){uint32_t id;if(!nativeCallbackSource_Snapshot(((Item*)g->user_data)->owner,NativeCallbackSource_Fighter,&id))return UINT32_MAX;return id;}
static HSD_GObj* creator(Item_GObj*g){Item*it=g->user_data;HSD_GObj* c=it->xDD4_itemVar.native_article.attachment_owner;return nativeCallbackSource_Alive(c,it->xDD4_itemVar.native_article.creator_spawn_id,NativeCallbackSource_Fighter)?c:NULL;}
static int32_t target_motion(uint32_t id){return nativeCallbackSource_Alive(owners+side,id,NativeCallbackSource_Fighter)?fighters[side].motion_id:-1;}
static bool target_is_fighter(uint32_t id){return nativeCallbackSource_Alive(owners+side,id,NativeCallbackSource_Fighter);}
static bool target_charge(uint32_t id){return target_is_fighter(id)&&charged;}
static void joint_set(Item_GObj*g,unsigned model,const char*path,unsigned channel,float v){check(model==0&&!strcmp(path,"0")&&channel==10,"root recursive visibility query");if(v)it_8026BB20(g);else it_8026BB44(g);}
static bool interaction(Item_GObj*g,HSD_GObj*other){return it_8026B894(g,other);}
static bool signal(Fighter_GObj*g,s32 id,uint32_t hash,float v){check(g==owners+side&&v==0,"owned signal args");for(unsigned k=0;k<9;k++)if((uint32_t)id==(uint32_t)items[side][k].x1C&&items[side][k].entity){const NativeFighterArticleLogic*a=logic(k);for(unsigned s=0;s<a->signal_count;s++)if(a->signals[s].name_hash==hash){a->signals[s].callback(objects[side]+k,v);return true;}abort();}return false;}
static bool remove_owned(Fighter_GObj*g,s32 id){check(g==owners+side,"remove owner");for(unsigned k=0;k<9;k++)if((uint32_t)id==(uint32_t)items[side][k].x1C&&items[side][k].entity){Item_8026A8EC(objects[side]+k);return true;}return false;}
static int32_t held_id(Fighter_GObj*g,bool auxiliary){check(g==owners+side&&!auxiliary,"held query");return fighters[side].item_gobj?(uint32_t)((Item*)fighters[side].item_gobj->user_data)->x1C:UINT32_MAX;}
static bool bind_held(Fighter_GObj*g,uint32_t id,bool auxiliary){check(!auxiliary,"held binding");check(g==owners+side&&id==(uint32_t)items[side][9].x1C,"restore held identity");fighters[side].item_gobj=objects[side]+9;return true;}
static bool visible(uint32_t id,bool v){check(id==(uint32_t)items[side][9].x1C,"retained visible identity");if(v)it_8026BB20(objects[side]+9);else it_8026BB44(objects[side]+9);return true;}
static bool paused(uint32_t id,bool pause){check(id==(uint32_t)items[side][9].x1C,"retained pause identity");if(pause)it_8026B724(objects[side]+9);else it_8026B73C(objects[side]+9);return true;}
static void unused(void){fprintf(stderr,"Unexecuted engine boundary unexpectedly used\n");abort();}
void lb_8000B1CC(HSD_JObj*j,Vec3*offset,Vec3*pos){(void)j;(void)offset;(void)pos;unused();}
void lb_800119DC(Vec3*pos,int frames,float strength,float decay,float frequency){(void)pos;(void)frames;(void)strength;(void)decay;(void)frequency;unused();}
static void ftGw_SpecialS_ItemJudgementExitHitlag(HSD_GObj*);
#include "normal_boundaries.h"
#include "original.h"
void Item_8026A8EC(Item_GObj*g){unsigned k=index(g);if(side==0){HSD_GObjEvent callbacks[]={itGamewatchGreenhouse_Destroyed,itGameWatchManhole_Logic72_Destroyed,itGamewatchFire_Destroyed,itGameWatchParachute_Logic74_Destroyed,itGameWatchTurtle_Logic75_Destroyed,itGameWatchBreath_Logic76_Destroyed,itGameWatchJudge_Logic77_Destroyed,itGameWatchPanic_Logic78_Destroyed,NULL};if(callbacks[k])callbacks[k](g);}else{const NativeFighterArticleLogic*a=logic(k);if(a->logic->destroyed)a->logic->destroyed(g);}event('d',k,0,0);items[side][k].entity=NULL;}
static void reset(int motion,unsigned mask,bool present,unsigned flags,float frame)
{
 memset(fighters,0,sizeof(fighters));memset(owners,0,sizeof(owners));memset(items,0,sizeof(items));memset(objects,0,sizeof(objects));memset(roots,0,sizeof(roots));memset(count,0,sizeof(count));
 for(unsigned s=0;s<2;s++){owners[s].user_data=fighters+s;fighters[s].gobj=owners+s;fighters[s].kind=Ft_Kind_CustomFirst;fighters[s].x8_spawnNum=(int32_t)0xFEDCBA98u;fighters[s].motion_id=motion;fighters[s].x5AC.model_num=11;fighters[s].ground_or_air=flags&1?GA_Air:GA_Ground;fighters[s].death2_cb=dummy_callback;fighters[s].take_dmg_cb=dummy_callback;fighters[s].accessory4_cb=dummy_callback;
  for(unsigned k=0;k<10;k++){objects[s][k].user_data=&items[s][k];objects[s][k].hsd_obj=&roots[s][k];objects[s][k].next=k==9?NULL:&objects[s][k+1];items[s][k].entity=&objects[s][k];items[s][k].x1C=0x80000010u+k;items[s][k].owner=present?owners+s:NULL;items[s][k].kind=k<9?kinds[k]:It_Kind_Capsule;items[s][k].x5CC_currentAnimFrame=frame;items[s][k].x5D0_animFrameSpeed=(flags>>1)%3;items[s][k].xDAC_itcmd_var0=0;items[s][k].xDB0_itcmd_var1=7;items[s][k].msid=3;items[s][k].xDC8_word.flags.x3=flags&1;items[s][k].xDC8_word.flags.x7=(flags>>1)&1;items[s][k].xDC8_word.flags.x5=(flags>>2)&1;items[s][k].xDD4_itemVar.native_article.attachment_owner=present?owners+s:NULL;items[s][k].xDD4_itemVar.native_article.creator_spawn_id=(uint32_t)fighters[s].x8_spawnNum;roots[s][k].flags=JOBJ_HIDDEN;}
 }
 for(unsigned k=0;k<9;k++){*original_handle(k)=mask&(1u<<k)?&objects[0][k]:NULL;set(fields[k],mask&(1u<<k)?(int)items[1][k].x1C:-1);}
 fighters[0].u.gw.x2248_manholeGObj=flags&8?objects[0]+9:NULL;set("saved_manhole_held",flags&8?(int)items[1][9].x1C:-1);active=flags&16;charged=flags&32;
}
static void equal(void)
{
 ++pairs;check(count[0]==count[1],"ordered count");for(unsigned i=0;i<count[0]&&i<count[1];i++){NormalEvent*a=events[0]+i,*b=events[1]+i;check(a->op==b->op&&a->a==b->a&&a->b==b->b&&a->v==b->v,"ordered shared boundary");}
 for(unsigned k=0;k<9;k++){check((*original_handle(k)==NULL)==(get(fields[k])==-1),"owned handle");check(items[0][k].msid==items[1][k].msid,"item state");check(items[0][k].xDAC_itcmd_var0==items[1][k].xDAC_itcmd_var0&&items[0][k].xDB0_itcmd_var1==items[1][k].xDB0_itcmd_var1,"item commands");check(items[0][k].x5D0_animFrameSpeed==items[1][k].x5D0_animFrameSpeed,"item rate");check(roots[0][k].flags==roots[1][k].flags,"root visibility");check(items[0][k].xDC8_word.flags.x3==items[1][k].xDC8_word.flags.x3&&items[0][k].xDC8_word.flags.x5==items[1][k].xDC8_word.flags.x5&&items[0][k].xDC8_word.flags.x7==items[1][k].xDC8_word.flags.x7,"pause flag semantics");check((items[0][k].entity==NULL)==(items[1][k].entity==NULL),"item removal");}
 check((fighters[0].death2_cb==NULL)==(fighters[1].death2_cb==NULL)&&(fighters[0].take_dmg_cb==NULL)==(fighters[1].take_dmg_cb==NULL),"cleanup callback flags");check((fighters[0].accessory4_cb==NULL)==(fighters[1].accessory4_cb==NULL),"accessory flag");check((fighters[0].item_gobj==NULL)==(fighters[1].item_gobj==NULL),"held restore");check((fighters[0].u.gw.x2248_manholeGObj==NULL)==(get("saved_manhole_held")==-1),"saved held clear");
}
#include "normal_matrix.h"
#ifdef BUCKET_ONLY
static void bucket_matrix(void){
 const int charges[]={-1,0,1,2,3,4,INT_MAX};
 for(unsigned c=0;c<sizeof(charges)/sizeof(charges[0]);++c)for(unsigned pattern=0;pattern<32;++pattern){
  reset(375,0,true,0,0);label="bucket current visibility";
  fighters[0].u.gw.x2238_panicCharge=charges[c];set("panic_charge",charges[c]);
  for(unsigned s=0;s<2;++s)for(unsigned g=0;g<11;++g){fighters[s].x5F4_arr[g].prev=(pattern+g)%4;fighters[s].x5F4_arr[g].idx=(pattern&(1u<<(g%5)))?2:-1;}
  select(0);ftGw_SpecialLw_UpdateBucketModel(owners);select(1);script_fn_BucketModels(owners+1);
  ++pairs;check(count[0]==count[1],"actual DObj refresh count");check(fighters[0].x221D_b2==fighters[1].x221D_b2,"visibility dirty flag");
  for(unsigned g=0;g<11;++g){check(fighters[0].x5F4_arr[g].prev==fighters[1].x5F4_arr[g].prev,"saved selection retained");check(fighters[0].x5F4_arr[g].idx==fighters[1].x5F4_arr[g].idx,"current selection matches source");}
 }
 printf("Bucket current visibility source: %u checks, %u failures, %u pairs\n",checks,failures,pairs);
}
#endif
static void stock_slot(HSD_GObj*g){(void)g;unused();}
#include "base.h"
int main(void)
{
 NativeFighterHost host={0};
#include "unused_hosts.h"
 host.callback_source_alive=nativeCallbackSource_Alive;host.item_creator=creator;host.item_current_owner=current_owner;host.target_motion=target_motion;host.target_is_fighter=target_is_fighter;host.target_smash_charge_active=target_charge;host.signal_owned_article=signal;host.remove_owned_article=remove_owned;host.item_change_state_flags=Item_80268E5C;host.item_advance_animation=Item_802694CC;host.item_pause=it_8026B724;host.item_resume=it_8026B73C;host.item_set_joint_value=joint_set;host.model_animation_active=animation_active;host.model_animation_rate=lb_8000BA0C;host.item_interact=interaction;host.item_remove=Item_8026A8EC;host.held_item_id=held_id;host.bind_existing_item=bind_held;host.bind_fighter_item=bind_held;host.set_item_visible=visible;host.set_item_paused=paused;host.notify_item_pickup=ftpickupitem_80094818;host.set_visibility_group=ftParts_80074A4C;
 host.fighter_item_pickup=Fighter_OnItemPickup;host.fighter_item_drop=Fighter_OnItemDrop;host.fighter_item_visible=Fighter_OnItemVisible;host.fighter_item_invisible=Fighter_OnItemInvisible;host.start_color=ftCo_800BFFD0;
 normal_hosts(&host);
#ifdef BUCKET_ONLY
 host.set_current_visibility_group=ftParts_80074B0C;
#endif
 definition=FreakMelee_GetFighterModule(&ftGameWatchClone_Definition,&host);check(definition!=NULL,"generated module accepted");if(!definition)return 2;
 check(FreakMelee_SetFighterParameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA),"parameters");
#ifdef BUCKET_ONLY
 bucket_matrix();
#else
 normal_phases(); printf("Full normals source: %u checks, %u failures, %u pairs\n",checks,failures,pairs);
#endif
 return failures?1:0;
}
