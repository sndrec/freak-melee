#ifdef ARTICLE_BOUNDARIES
#include <melee/it/kinds/itgamewatchjudge.h>
#include <melee/it/kinds/itgamewatchrescue.h>
#include <melee/it/kinds/itgamewatchpanic.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <sysdolphin/baselib/jobj.h>
static Item props[2];static HSD_GObj prop_objects[2];static HSD_JObj prop_root[2],child[2],numeral[2];
static bool removed[2];static unsigned article_pairs;
static bool itGamewatchpanic_UnkMotion1_Anim(Item_GObj*);
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){j->flags|=JOBJ_MTX_DIRTY;}
void __assert(char* f,u32 line,char* e){fprintf(stderr,"%s %u %s",f,line,e);abort();}
f32 ftLib_800865C0(Fighter_GObj* g){return ((Fighter*)g->user_data)->facing_dir;}
void Item_80268E5C(Item_GObj* g,enum_t state,Item_StateChangeFlags flags){(void)g;props[family].msid=state;event('s',state,flags,0,0);}
void Item_8026A8EC(Item_GObj* g){check(g==&prop_objects[family],"remove item identity");event('r',0,0,0,0);removed[family]=true;heads[HSD_GOBJ_PLINK_ITEM]=NULL;}
bool it_8026B894(Item_GObj* g,Item_GObj* other){(void)g;(void)other;event('i',0,0,0,0);return false;}
static bool interaction(Item_GObj* g,HSD_GObj* other){return it_8026B894(g,other);}
static HSD_GObj* item_creator(Item_GObj* g){Item* item=g->user_data;HSD_GObj* c=item->xDD4_itemVar.native_article.attachment_owner;return c&&nativeCallbackSource_Alive(c,item->xDD4_itemVar.native_article.creator_spawn_id,NativeCallbackSource_Fighter)?c:NULL;}
static uint32_t current_owner(Item_GObj* g){HSD_GObj* owner=((Item*)g->user_data)->owner;uint32_t id;if(!nativeCallbackSource_Snapshot(owner,NativeCallbackSource_Fighter,&id))return UINT32_MAX;return id;}
static int32_t target_motion(uint32_t id){HSD_GObj* g=heads[HSD_GOBJ_PLINK_FIGHTER];if(!g||!nativeCallbackSource_Alive(g,id,NativeCallbackSource_Fighter))return -1;return ((Fighter*)g->user_data)->motion_id;}
static float joint_value(Item_GObj* g,unsigned model,const char* path,unsigned channel){(void)g;(void)model;(void)path;(void)channel;abort();}
static void joint_set(Item_GObj* g,unsigned model,const char* path,unsigned channel,float v){check(g==&prop_objects[family]&&!model&&!strcmp(path,"0/0")&&channel==4,"numeral direct child path");HSD_JObjSetRotationY(&numeral[family],v);}
#else
static void prop_reset(unsigned motion,unsigned f,bool present,unsigned kind)
{
 reset(motion,f,9.5f,0);memset(props,0,sizeof(props));memset(prop_objects,0,sizeof(prop_objects));memset(prop_root,0,sizeof(prop_root));memset(child,0,sizeof(child));memset(numeral,0,sizeof(numeral));memset(removed,0,sizeof(removed));
 for(unsigned i=0;i<2;i++){prop_objects[i].user_data=&props[i];prop_objects[i].hsd_obj=&prop_root[i];props[i].entity=&prop_objects[i];props[i].x1C=0x80000001u;props[i].kind=kind;props[i].owner=present?&owners[i]:NULL;prop_root[i].child=&child[i];child[i].child=&numeral[i];numeral[i].rotate.y=.125f;props[i].xDAC_itcmd_var0=5;props[i].xDB0_itcmd_var1=7;props[i].msid=1;}
 props[0].xDD4_itemVar.gamewatchrescue.fighter_owner=present?&owners[0]:NULL;props[1].xDD4_itemVar.native_article.attachment_owner=present?&owners[1]:NULL;props[1].xDD4_itemVar.native_article.creator_spawn_id=(uint32_t)fighters[1].x8_spawnNum;
}
static void prop_select(unsigned f){select(f);heads[HSD_GOBJ_PLINK_ITEM]=&prop_objects[f];}
static const NativeFighterArticleLogic* article(ItemKind kind){for(unsigned i=0;i<definition->article_logic_count;i++)if(definition->article_logics[i].kind==kind)return &definition->article_logics[i];abort();}
static void article_matrix(void)
{
 ItemKind kinds[]={It_Kind_GameWatch_Judge,It_Kind_GameWatch_Panic,It_Kind_GameWatch_Rescue};
 for(unsigned k=0;k<3;k++)for(unsigned motion=340;motion<390;motion++)for(unsigned present=0;present<2;present++)for(unsigned sign=0;sign<2;sign++)for(unsigned phase=0;phase<3;phase++){
  // Judge pickup/animation/interaction; Panic animation/destroyed/interaction;
  // Rescue animation/no-op physics/collision. Constructors and Panic pickup open.
  prop_reset(motion,sign,present,kinds[k]);current="article";int old_result=0,new_result=0;prop_select(0);
  if(k==0){if(phase==0)old_result=itGamewatchjudge_UnkMotion0_Anim(&prop_objects[0]);else if(phase==1)it_2725_Logic77_PickedUp(&prop_objects[0]);else itGameWatchJudge_Logic77_EvtUnk(&prop_objects[0],NULL);}
  else if(k==1){if(phase==0)old_result=itGamewatchpanic_UnkMotion1_Anim(&prop_objects[0]);else if(phase==1)itGameWatchPanic_Logic78_Destroyed(&prop_objects[0]);else itGameWatchPanic_Logic78_EvtUnk(&prop_objects[0],NULL);}
  else {if(phase==0)old_result=itGamewatchrescue_UnkMotion1_Anim(&prop_objects[0]);else if(phase==1)itGamewatchrescue_UnkMotion1_Phys(&prop_objects[0]);else old_result=itGamewatchrescue_UnkMotion1_Coll(&prop_objects[0]);}
  prop_select(1);const NativeFighterArticleLogic* a=article(kinds[k]);if(phase==0)new_result=a->logic->states[0].animated(&prop_objects[1]);else if(k==0&&phase==1)a->logic->picked_up(&prop_objects[1]);else if(k==1&&phase==1)a->logic->destroyed(&prop_objects[1]);else if(k<2)a->logic->evt_unk(&prop_objects[1],NULL);else if(phase==1)a->logic->states[0].physics_updated(&prop_objects[1]);else new_result=a->logic->states[0].collided(&prop_objects[1]);
  check(old_result==new_result||(k==2&&phase==0&&removed[0]&&removed[1]),"terminal removal return boundary");check(removed[0]==removed[1],"immediate removal");check((props[0].owner==NULL)==(props[1].owner==NULL),"owner detach");check(props[0].msid==props[1].msid&&props[0].xDAC_itcmd_var0==props[1].xDAC_itcmd_var0&&props[0].xDB0_itcmd_var1==props[1].xDB0_itcmd_var1,"pickup fields");check(numeral[0].rotate.y==numeral[1].rotate.y&&numeral[0].flags==numeral[1].flags,"numeral rotation");equal();article_pairs++;
 }
 for(unsigned k=0;k<3;k++)for(unsigned operation=0;operation<2;operation++)for(unsigned flags=0;flags<8;flags++) {
  prop_reset(373,0,true,kinds[k]);current="pause/resume";
  for(unsigned i=0;i<2;i++){props[i].xDC8_word.flags.x3=flags&1;props[i].xDC8_word.flags.x7=flags&2?1:0;props[i].xDC8_word.flags.x5=flags&4?1:0;}
  prop_select(0);if(!k){if(operation)it_802C7AF0(&prop_objects[0]);else it_802C7AD0(&prop_objects[0]);}else if(k==1){if(operation)it_802C7F00(&prop_objects[0]);else it_802C7EE0(&prop_objects[0]);}else {if(operation)it_802C81E8(&prop_objects[0]);else it_802C81C8(&prop_objects[0]);}
  prop_select(1);const NativeFighterArticleLogic* a=article(kinds[k]);unsigned hash=operation?1162681738u:1157218093u;bool found=false;for(unsigned i=0;i<a->signal_count;i++)if(a->signals[i].name_hash==hash){a->signals[i].callback(&prop_objects[1],0);found=true;}check(found,"authored signal present");check(props[0].xDC8_word.flags.x3==props[1].xDC8_word.flags.x3&&props[0].xDC8_word.flags.x5==props[1].xDC8_word.flags.x5&&props[0].xDC8_word.flags.x7==props[1].xDC8_word.flags.x7,"actual pause/resume bits");equal();article_pairs++;
 }
}
#endif
