// Constructor decisions execute on real Item/Fighter structures and imported
// article descriptors. Allocation and HSD pose realization are controlled.
#include <melee/ft/native_fighter_module.h>
#include "parameters.h"
#include <melee/ft/inlines.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/ft/kinds/ftGameWatch/forward.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/itCharItems.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itzako.h>
#include <melee/it/itmaterial.h>
#include <melee/it/itarticle_data.h>
#include <melee/it/article_model_layers.h>
#include <melee/it/native_article.h>
#include <melee/it/native_article_model.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/native_model_joint.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static unsigned checks, failures, pairs, family, slot, variant, node_count[2];
static unsigned allocation_fail, graph_mode, debug_bits, transfer_mode, transfer_pairs, authored_safety_cases;
#define CHECK(c) do { ++checks; if (!(c) && failures++ < 24) fprintf(stderr,"line%d %s slot%u variant%u pair%u family%u\n",__LINE__,#c,slot,variant,pairs,family); } while(0)
static Fighter fighters[2]; static HSD_GObj owners[2], objects[2], *heads[32];
static Fighter holders[2]; static HSD_GObj holder_objects[2];
static ftData data[2]; static ftGameWatchAttributes tuning[2];
static Item items[2]; static Article* articles[10]; static ItemLogicTable stock_logic[10];
static HSD_JObj nodes[2][100]; static DynamicBoneTable bones[2];
static HSD_DObj dobjs[2][100]; static HSD_MObj mobjs[2][100]; static HSD_Material mats[2][100];
static float requested_frame[2]; static SpawnItem spawns[2];
static const NativeFighterDefinition* definition; static NativeFighterDefinition base;
typedef const NativeFighterDefinition* (*ModuleEntry)(const NativeFighterDefinition*,const NativeFighterHost*);
static ModuleEntry open_module;
static int (*set_parameters)(const uint32_t*, unsigned, uint64_t);
static MotionState motions[40]; static ItemCommonData common_item; static struct ftCommonData common_fighter;
HSD_GObj** HSD_GObjPLinkHead = heads;
ItemCommonData* it_804D6D28 = &common_item;
struct ftCommonData* p_ftCommonData = &common_fighter;
u8 db_ShowItemCollisionBubbles;
static const ItemKind kinds[10] = { It_Kind_GameWatch_Greenhouse, It_Kind_GameWatch_Manhole,
 It_Kind_GameWatch_Fire, It_Kind_GameWatch_Parachute, It_Kind_GameWatch_Turtle,
 It_Kind_GameWatch_Breath, It_Kind_GameWatch_Judge, It_Kind_GameWatch_Panic,
 It_Kind_GameWatch_Chef, It_Kind_GameWatch_Rescue };
typedef struct ConstructorEvent { int op, a; float value; } ConstructorEvent;
static ConstructorEvent events[2][64]; static unsigned event_count[2];
static void event(int op,int a,float v) { CHECK(event_count[family]<64); events[family][event_count[family]++]=(ConstructorEvent){op,a,v}; }
static void unused(void) { fprintf(stderr,"unexpected Host operation slot%u\n",slot); abort(); }
void __assert(char* f,u32 line,char* c) { fprintf(stderr,"%s:%u %s\n",f,(unsigned)line,c);abort(); }
void HSD_JObjSetMtxDirtySub(HSD_JObj* j) { j->flags |= JOBJ_MTX_DIRTY; }
static unsigned joint_index(HSD_JObj* j) { CHECK(j>=nodes[family] && j<nodes[family]+node_count[family]);return(unsigned)(j-nodes[family]); }
void HSD_JObjAnimAll(HSD_JObj* j) { CHECK(j==nodes[family]);event('A',0,0); }
void HSD_JObjRemoveAnimAll(HSD_JObj* j) { CHECK(j==nodes[family]);event('R',0,0); }
void HSD_JObjAddAnimAll(HSD_JObj* j,HSD_AnimJoint* a,HSD_MatAnimJoint* m,HSD_ShapeAnimJoint* s) {
 CHECK(j==nodes[family]);CHECK(a==items[family].xD0_itemStateDesc->x0_anim_joint && m==items[family].xD0_itemStateDesc->x4_matanim_joint && s==items[family].xD0_itemStateDesc->x8_parameters);event('D',0,0);
}
void HSD_JObjReqAnimAll(HSD_JObj* j,float frame) { CHECK(j==nodes[family]);requested_frame[family]=frame;event('F',0,frame); }
float lbGetJObjCurrFrame(HSD_JObj* j) { CHECK(j==nodes[family]);return requested_frame[family]; }
void lb_8000BA0C(HSD_JObj* j,float rate) { CHECK(j==nodes[family]);event('r',0,rate); }
void lb_8000B804(HSD_JObj* j,HSD_Joint* d) { CHECK(j!=NULL && d!=NULL);event('P',joint_index(j),0); }
void Item_8026849C(HSD_GObj* g) { CHECK(g==objects+family);event('m',0,0); }
void it_802756D0(Item_GObj* g) { CHECK(g==objects+family);event('v',0,0); }
void it_80274F48(Item_GObj* g,s32 attach,HSD_GObj* owner,Fighter_Part part) { CHECK(g==objects+family && owner==owners+family);event('b',part,(float)attach); }
void it_80274C88(Item_GObj* g) { CHECK(g==objects+family);event('c',0,0); }
void it_80279BBC(Item* i) { CHECK(i==items+family);event('d',0,0); }
void it_80275534(Item_GObj* g,float v) { CHECK(g==objects+family);event('s',0,v); }
void it_80274DFC(Item_GObj* g) { CHECK(g==objects+family);event('x',0,0); }
void it_80274E44(Item_GObj* g) { CHECK(g==objects+family);event('y',0,0); }
void it_80273168(Item_GObj* g) { CHECK(g==objects+family);event('p',0,0); }
int lbAudioAx_800236B8(int id) { event('S',id,0);return 0; }
void it_802799E4(Item_GObj* g) { CHECK(g==objects+family);event('C',0,0); }
void Item_80268E5C(Item_GObj* g,enum_t state,Item_StateChangeFlags flags) {
 CHECK(g==objects+family);items[family].msid=state;event('T',state,(float)flags);
}
u8 fn_8022697C(Fighter_GObj* g) { CHECK(g==owners+family||g==holder_objects+family);return debug_bits; }
float ftLib_80086A0C(HSD_GObj* g) { CHECK(g==owners+family||g==holder_objects+family);return ((Fighter*)g->user_data)->x34_scale.y; }
FighterKind ftLib_GetKind(HSD_GObj* g) { return ((Fighter*)g->user_data)->kind; }
void ftKb_Init_800EEB00(HSD_GObj* g,ItemModStruct* dst) { (void)g;(void)dst;unused(); }
void ftKb_Init_800EEB1C(HSD_GObj* g,ItemModStruct* dst) { (void)g;(void)dst;unused(); }
void it_8026BB68(HSD_GObj* g,Vec3* p) { CHECK(g==owners+family);*p=(Vec3){17,-23,5}; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k) { return k==Ft_Kind_CustomFirst||k==Ft_Kind_GameWatch?definition:NULL; }
const NativeFighterArticleLogic* nativeFighterRuntime_ArticleLogic(const Fighter* f,ItemKind kind) {
 CHECK(f==fighters+family);for(unsigned n=0;n<definition->article_logic_count;n++)if(definition->article_logics[n].kind==kind)return definition->article_logics+n;return NULL;
}
Article* nativeFighterRuntime_ArticleAssets(FighterKind author,ItemKind kind) {
 CHECK(author==Ft_Kind_CustomFirst);for(unsigned n=0;n<10;n++)if(kinds[n]==kind)return articles[n];return NULL;
}
Fighter_GObj* ftLib_8008741C(u32 id) { for(HSD_GObj* g=heads[HSD_GOBJ_PLINK_FIGHTER];g;g=g->next)if(g->user_data && (uint32_t)((Fighter*)g->user_data)->x8_spawnNum==id)return g;return NULL; }
uint32_t nativeFighterTarget_Id(Fighter_GObj* g) { uint32_t id;return nativeCallbackSource_Snapshot(g,NativeCallbackSource_Fighter,&id)?id:0; }
static void make_model(const HSD_Joint* d,HSD_JObj* parent) {
 while(d) { CHECK(node_count[family]<100);unsigned n=node_count[family]++;HSD_JObj* j=nodes[family]+n;bones[family].bones[n]=j;
 j->parent=parent;j->flags=d->flags & ~JOBJ_MTX_DIRTY;j->rotate=(Quaternion){d->rotation.x,d->rotation.y,d->rotation.z,0};j->scale=d->scale;j->translate=d->position;
 j->u.dobj=dobjs[family]+n;dobjs[family][n].mobj=mobjs[family]+n;mobjs[family][n].mat=mats[family]+n;mats[family][n].diffuse=(GXColor){71,72,73,74};
 if(parent&&!parent->child)parent->child=j;if(d->child)make_model(d->child,j);
 if(d->next)j->next=nodes[family]+node_count[family];d=d->next;
 }
}
Item_GObj* Item_80268B18(SpawnItem* spawn) {
 CHECK(spawn->kind==kinds[slot]);spawns[family]=*spawn;event('N',slot,0);if(allocation_fail)return NULL;
 Item* i=items+family;HSD_GObj* g=objects+family;i->entity=g;g->user_data=i;
 i->x1C=UINT32_C(0xF0000001);i->kind=spawn->kind;i->owner=transfer_mode?holder_objects+family:spawn->x0_parent_gobj;i->facing_dir=spawn->facing_dir;i->pos=spawn->pos;i->x40_vel=spawn->vel;
 i->xC4_article_data=articles[slot];i->xCC_item_attr=articles[slot]->x0_common_attr;i->xC8_joint=articles[slot]->x10_modelDesc->x0_joint;i->scl=.75f;
 i->xB8_itemLogicTable=family?nativeFighterRuntime_ArticleLogic(fighters+family,kinds[slot])->logic:stock_logic+slot;
 i->xDC8_word.flags.x15=1;i->xDCC_flag.b3=1;i->xDD0_flag.b6=1;i->xDC8_word.flags.xF=2;i->xD50_landNum=12;i->xD40=11;i->xDAA_byte=0xAC;i->xD5C=9;i->sfx_unk2=123;
 i->xDAC_itcmd_var0=10;i->xDB0_itcmd_var1=11;i->xDB4_itcmd_var2=12;i->xDB8_itcmd_var3=13;i->x5D0_animFrameSpeed=1.25f;i->xBC4=(GXColor){9,8,7,6};i->xBC8.x0_unk=(GXColor){8,7,6,5};
 make_model(i->xC8_joint,NULL);g->hsd_obj=nodes[family];i->xBBC_dynamicBoneTable=bones+family;if(graph_mode==1)i->xBBC_dynamicBoneTable=NULL;
 heads[HSD_GOBJ_PLINK_ITEM]=g;return g;
}
Item_GObj* Item_CreateAuthored(SpawnItem* s,FighterKind k) { CHECK(k==Ft_Kind_CustomFirst);Item_GObj* g=Item_80268B18(s);if(g)((Item*)g->user_data)->native_article_author_kind=k;return g; }
#include "original.h"

static uint32_t current_owner(Item_GObj* g) { return nativeFighterTarget_Id(((Item*)g->user_data)->owner); }
static bool target_is_fighter(uint32_t id) { return ftLib_8008741C(id)!=NULL; }
static int32_t target_motion(uint32_t id) { HSD_GObj* g=ftLib_8008741C(id);return g?((Fighter*)g->user_data)->motion_id:-1; }
static bool target_airborne(uint32_t id) { HSD_GObj* g=ftLib_8008741C(id);return g&&((Fighter*)g->user_data)->ground_or_air==GA_Air; }
static bool controller_color(uint32_t id,GXColor* c) { HSD_GObj* g=ftLib_8008741C(id);return g&&ftLib_800870BC(g,c); }
static unsigned bone_count(Fighter* g) { CHECK(g==fighters+family);return 53; }
static void param_set(const char* name,uint32_t value) { for(unsigned i=0;i<FIXTURE_PARAMETER_COUNT;i++)if(!strcmp(name,fixture_parameter_names[i])){fixture_parameter_bits[i]=value;return;}CHECK(false); }
static int article_int(const char* name) { const NativeFighterArticleLogic* l=nativeFighterRuntime_ArticleLogic(fighters+1,kinds[slot]);for(unsigned n=0;n<l->state_field_count;n++)if(!strcmp(name,l->state_fields[n].name)){int v;memcpy(&v,items[1].xDD4_itemVar.native_article.state+l->state_fields[n].offset,4);return v;}CHECK(false);return -1; }
static void reset(unsigned costume,unsigned controller,unsigned facing,unsigned air,unsigned scale,unsigned flags) {
 memset(fighters,0,sizeof(fighters));memset(objects,0,sizeof(objects));memset(owners,0,sizeof(owners));memset(items,0,sizeof(items));memset(nodes,0,sizeof(nodes));memset(bones,0,sizeof(bones));memset(dobjs,0,sizeof(dobjs));memset(mobjs,0,sizeof(mobjs));memset(mats,0,sizeof(mats));memset(event_count,0,sizeof(event_count));memset(node_count,0,sizeof(node_count));memset(requested_frame,0,sizeof(requested_frame));memset(heads,0,sizeof(heads));
 debug_bits=flags&3;
 for(unsigned f=0;f<2;f++){owners[f].user_data=fighters+f;owners[f].classifier=HSD_GOBJ_CLASS_FIGHTER;fighters[f].gobj=owners+f;fighters[f].kind=f?Ft_Kind_CustomFirst:Ft_Kind_GameWatch;fighters[f].x8_spawnNum=(int32_t)UINT32_C(0xE0000001);fighters[f].ft_data=data+f;data[f].ext_attr=tuning+f;fighters[f].dat_attrs=tuning+f;fighters[f].x619_costume_id=costume;fighters[f].x61A_controller_index=controller;fighters[f].facing_dir=facing?-1:1;fighters[f].ground_or_air=air?GA_Air:GA_Ground;fighters[f].x34_scale.y=scale==0?.5f:scale==1?1:2;fighters[f].player_id=flags%4;fighters[f].team=flags%3;fighters[f].motion_id=flags&4?ftCo_MS_Attack11:350;
 holders[f]=fighters[f];holders[f].kind=transfer_mode==2?Ft_Kind_Mario:fighters[f].kind;holders[f].x8_spawnNum=(int32_t)UINT32_C(0xD1234567);holders[f].gobj=holder_objects+f;holders[f].x619_costume_id=(costume+1)%4;holders[f].x61A_controller_index=controller==255?1:(controller+1)%6;holders[f].x34_scale.y=scale==0?3:.125f;
 holder_objects[f]=(HSD_GObj){.user_data=holders+f,.classifier=HSD_GOBJ_CLASS_FIGHTER};owners[f].next=transfer_mode?holder_objects+f:NULL;
 }
}
static void select_family(unsigned f) { family=f;heads[HSD_GOBJ_PLINK_FIGHTER]=owners+f;heads[HSD_GOBJ_PLINK_ITEM]=NULL; }
static Fighter_Part attachment(void) { static Fighter_Part p[]={FtPart_LHandNb,FtPart_LHandNb,FtPart_LHandNb,FtPart_TransN,FtPart_LShoulderN,FtPart_LHandN,FtPart_LHandNb,0,0,0};return p[slot]; }
static HSD_GObj* original_create(Vec3* p) { HSD_GObj* g=owners;float d=fighters[0].facing_dir;Fighter_Part b=attachment();switch(slot){case 0:return itGamewatchGreenhouse_Spawn(g,p,b,d);case 1:return it_802C65E4(g,p,b,d);case 2:return itGamewatchFire_Spawn(g,p,b,d);case 3:return it_802C6C38(g,p,b,d);case 4:return it_802C6F40(g,p,b,d);case 5:return it_802C720C(g,p,b,d);case 6:return it_802C7774(d,g,p,b,variant);case 7:return it_802C7D60(g,p,b,d);case 8:return it_802C837C(g,p,kinds[slot],variant,d);default:return it_802C8038(g,p,7,variant,d,19);}}
static void compare(unsigned costume,unsigned controller,unsigned facing,unsigned air,unsigned scale,unsigned flags) {
 reset(costume,controller,facing,air,scale,flags);Vec3 pos={3,-7,11}, velocity={0};select_family(0);HSD_GObj* old=original_create(&pos);
 select_family(1);if(slot>=8)pos.z=0;int32_t id=nativeArticle_Create(owners+1,kinds[slot],&pos,&velocity,fighters[1].facing_dir,slot==8,0,(float)variant,slot!=9);++pairs;
 CHECK((old==NULL)==(id==-1));CHECK(event_count[0]==event_count[1]);for(unsigned n=0;n<event_count[0]&&n<event_count[1];n++)CHECK(events[0][n].op==events[1][n].op&&events[0][n].a==events[1][n].a&&events[0][n].value==events[1][n].value);
 CHECK(spawns[0].kind==spawns[1].kind && spawns[0].facing_dir==spawns[1].facing_dir && spawns[0].x3C_damage==spawns[1].x3C_damage && spawns[0].x40==spawns[1].x40 && spawns[0].x44_flag.b0==spawns[1].x44_flag.b0);
 CHECK(!memcmp(&spawns[0].pos,&spawns[1].pos,sizeof(Vec3))&&!memcmp(&spawns[0].prev_pos,&spawns[1].prev_pos,sizeof(Vec3))&&!memcmp(&spawns[0].vel,&spawns[1].vel,sizeof(Vec3)));
 CHECK(spawns[0].x0_parent_gobj==owners && spawns[1].x0_parent_gobj==owners+1 && spawns[0].x4_parent_gobj2==owners && spawns[1].x4_parent_gobj2==owners+1);
 if(allocation_fail)return;
 CHECK(items[0].msid==items[1].msid && items[0].ground_or_air==items[1].ground_or_air);CHECK(!memcmp(&items[0].x40_vel,&items[1].x40_vel,sizeof(Vec3)));
 CHECK(items[0].xDAC_itcmd_var0==items[1].xDAC_itcmd_var0 && items[0].xDB0_itcmd_var1==items[1].xDB0_itcmd_var1 && items[0].xDB4_itcmd_var2==items[1].xDB4_itcmd_var2 && items[0].xDB8_itcmd_var3==items[1].xDB8_itcmd_var3);
 CHECK(items[0].scl==items[1].scl && items[0].x5CC_currentAnimFrame==items[1].x5CC_currentAnimFrame && items[0].xD44_lifeTimer==items[1].xD44_lifeTimer && items[0].xD48_halfLifeTimer==items[1].xD48_halfLifeTimer);
 CHECK(items[0].xD5C==items[1].xD5C && items[0].xDC8_word.flags.x15==items[1].xDC8_word.flags.x15 && items[0].xDCC_flag.b3==items[1].xDCC_flag.b3 && items[0].xDC8_word.flags.x13==items[1].xDC8_word.flags.x13 && items[0].xDC8_word.flags.xF==items[1].xDC8_word.flags.xF && items[0].xDAA_byte==items[1].xDAA_byte && items[0].sfx_unk2==items[1].sfx_unk2);
 CHECK(items[0].xDC4==items[1].xDC4 && items[0].xD50_landNum==items[1].xD50_landNum && items[0].xD40==items[1].xD40 && items[0].xDD0_flag.b6==items[1].xDD0_flag.b6);
 CHECK(items[0].x5C8==items[1].x5C8 && !memcmp(&items[0].xBC4,&items[1].xBC4,sizeof(GXColor)) && !memcmp(&items[0].xBC8,&items[1].xBC8,sizeof(GXColor)));
 CHECK((items[0].model_layers==NULL)==(items[1].model_layers==NULL));CHECK(items[0].xDCF_flag.b3==items[1].xDCF_flag.b3);
 CHECK(node_count[0]==node_count[1]);for(unsigned n=0;n<node_count[0];n++){CHECK(nodes[0][n].flags==nodes[1][n].flags);CHECK(!memcmp(&nodes[0][n].rotate,&nodes[1][n].rotate,sizeof(Quaternion))&&!memcmp(&nodes[0][n].scale,&nodes[1][n].scale,sizeof(Vec3)));CHECK(!memcmp(&mats[0][n].diffuse,&mats[1][n].diffuse,sizeof(GXColor)));}
 if(slot==8)CHECK(items[0].xDD4_itemVar.gamewatchchef.variant==article_int("variant"));
 if(slot==9)CHECK(items[0].xDD4_itemVar.gamewatchrescue.fighter_owner==owners && nativeArticle_Creator(objects+1)==owners+1);
}
static u32 target(const HSD_Dat* d,u32 at) { u32 out=0;const char* external;CHECK(HSD_DatResolve(d,at,&out,&external)==HSD_DAT_REF_LOCAL);return out; }
static void retail(const char* file) {
 FILE* f=fopen(file,"rb");CHECK(f!=NULL);fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);u8* bytes=malloc(size);CHECK(fread(bytes,1,size,f)==(size_t)size);fclose(f);HSD_Dat dat;CHECK(HSD_DatOpen(&dat,bytes,size)==HSD_DAT_OK);u32 root;CHECK(HSD_DatFindRoot(&dat,"ftDataGamewatch",&root));u32 table=target(&dat,root+0x48);
 for(unsigned n=0;n<10;n++){void* arena=malloc(16*1024*1024);HSD_DatImport import;CHECK(HSD_DatImportInit(&import,&dat,arena,16*1024*1024,32768)==HSD_DAT_OK);ItArticleImportStats stats={0};ItScriptEntry entries[512];ItScriptCollect scripts={entries,0,512};articles[n]=itArticle_Import(&import,target(&dat,table+4*n),&stats,&scripts,false,n==8?It_Kind_Kirby_GameWatchChef:It_Kind_Kirby_GameWatchChefPan,NULL,0);CHECK(articles[n]!=NULL && articles[n]->model_layers!=NULL);if(!articles[n])abort();}
 memcpy(&tuning[0],HSD_DatBytes(&dat,target(&dat,root+4),24),24); // Scalars repaired below; colors retain retail byte order.
 CHECK(HSD_DatReadF32(&dat,target(&dat,root+4),&tuning[0].x0_GAMEWATCH_WIDTH));tuning[1]=tuning[0];free(bytes);
}
static void authored_missing_creator(void) {
 // The original CE64 requires a healthy palette parent. Do not call it on an
 // undefined retired parent. Current damage owner remains a distinct live
 // fighter here, so its debug query is valid; only the retained creator fails.
 transfer_mode=2;allocation_fail=graph_mode=0;
 for(slot=8;slot<10;slot++)for(unsigned mode=0;mode<5;mode++){
  reset(0,1,0,0,0,0);select_family(1);SpawnItem spawn={.kind=kinds[slot]};Vec3 p={0};Item_InitSpawn(&spawn,owners+1,&p,1);
  HSD_GObj* g=Item_CreateAuthored(&spawn,Ft_Kind_CustomFirst);Item* item=g->user_data;
  item->xDD4_itemVar.native_article.attachment_owner=owners+1;item->xDD4_itemVar.native_article.creator_spawn_id=(uint32_t)fighters[1].x8_spawnNum;
  item->xDD4_itemVar.native_article.creation_value=0;
  if(mode==0)heads[HSD_GOBJ_PLINK_FIGHTER]=holder_objects+1;
  if(mode==1)++fighters[1].x8_spawnNum;
  if(mode==2)item->xDD4_itemVar.native_article.attachment_owner=holder_objects+1;
  if(mode==3)fighters[1].kind=Ft_Kind_Mario;
  if(mode==4)item->native_article_author_kind=Ft_Kind_None;
  CHECK(nativeArticle_Creator(g)==NULL);
  nativeFighterRuntime_ArticleLogic(fighters+1,kinds[slot])->on_created(g);
  CHECK(item->scl==.75f && item->x5C8==0 && item->xBC8.x0_unk.r==8);
  for(unsigned n=0;n<node_count[1];n++)CHECK(mats[1][n].diffuse.r==71 && mats[1][n].diffuse.a==74);
  CHECK(item->model_layers==articles[slot]->model_layers && item->xDCF_flag.b3);
  CHECK(item->msid==0); // Missing palette stops the helper, not Rescue setup.
  ++authored_safety_cases;
 }
 transfer_mode=0;slot=9;select_family(0);unsigned before=event_count[0];Vec3 pos={0};CHECK(it_802C8038(NULL,&pos,0,0,1,0)==NULL && event_count[0]==before);
 select_family(1);before=event_count[1];Vec3 v={0};CHECK(nativeArticle_Create(NULL,kinds[9],&pos,&v,1,false,0,0,false)==-1 && event_count[1]==before);
 ++authored_safety_cases;
}
int main(int argc,char** argv) {
 CHECK(argc==3);retail(argv[1]);common_item.x4C_float=.5f;common_item.x30_lifetime=500;
 HMODULE library=LoadLibraryA(argv[2]);CHECK(library!=NULL);if(!library)return 3;
 open_module=(ModuleEntry)GetProcAddress(library,"FreakMelee_GetFighterModule");
 set_parameters=(__typeof__(set_parameters))GetProcAddress(library,"FreakMelee_SetFighterParameters");
 CHECK(open_module!=NULL && set_parameters!=NULL);if(!open_module||!set_parameters)return 3;
 uint64_t (*layout)(void)=(__typeof__(layout))GetProcAddress(library,"FreakMelee_GetFighterModuleLayout");
 CHECK(layout!=NULL && layout()==nativeFighterModule_LayoutFingerprint());
 for(unsigned n=0;n<4;n++)common_fighter.x6DC_colorsByPlayer[n]=(GXColor){(u8)(19+31*n),(u8)(209-13*n),(u8)(47+19*n),(u8)(255-5*n)};
 NativeFighterHost host={0};
#include "unused_hosts.h"
 host.callback_source_alive=nativeCallbackSource_Alive;host.item_creator=nativeArticle_Creator;host.item_current_owner=current_owner;host.target_is_fighter=target_is_fighter;host.target_motion=target_motion;host.target_airborne=target_airborne;host.target_controller_color=controller_color;host.bone_count=bone_count;
 host.create_article_at=nativeArticle_Create;host.item_attach_to_owner=Item_8026AB54;host.item_set_debug_owner=db_80225DD8;host.item_change_state_flags=Item_80268E5C;host.item_advance_animation=Item_802694CC;host.item_set_appearance=it_80273670;host.item_set_joint_value=nativeArticleModel_Write;host.item_joint_value=nativeArticleModel_Read;host.item_set_lifetime=it_80275158;host.item_use_manual_lifetime=it_8026B3A8;host.item_inherit_owner_scale=it_80274594;host.set_item_layered=articleModelLayers_Bind;host.set_item_diffuse_color=it_80278574;
 base.source_kind=Ft_Kind_GameWatch;base.joint_count=53;base.motion_state_first=341;base.motion_state_count=40;base.motion_states=motions;base.special_ground_state[0]=353;base.special_air_state[0]=354;base.special_ground_state[1]=355;base.special_air_state[1]=364;base.special_ground_state[2]=373;base.special_air_state[2]=374;base.special_ground_state[3]=375;base.special_air_state[3]=378;
 definition=open_module(&base,&host);CHECK(definition!=NULL);if(!definition)return 2;
 CHECK(definition->article_logic_count==10);
 for(unsigned n=0;n<definition->article_logic_count;n++){MEMORY_BASIC_INFORMATION m;const NativeFighterArticleLogic* l=definition->article_logics+n;CHECK(l->on_created!=NULL);CHECK(VirtualQuery((void*)l->on_created,&m,sizeof(m))==sizeof(m)&&m.AllocationBase==library);if(l->logic->picked_up)CHECK(VirtualQuery((void*)l->logic->picked_up,&m,sizeof(m))==sizeof(m)&&m.AllocationBase==library);}
 HSD_GObjEvent pickups[]={itGamewatchGreenhouse_PickedUp,itGameWatchManhole_Logic72_PickedUp,itGamewatchFire_PickedUp,itGameWatchParachute_Logic74_PickedUp,itGameWatchTurtle_Logic75_PickedUp,itGameWatchBreath_Logic76_PickedUp,it_2725_Logic77_PickedUp,itGameWatchPanic_Logic78_PickedUp};for(unsigned n=0;n<8;n++)stock_logic[n].picked_up=pickups[n];
 CHECK(set_parameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA));
 for(unsigned edit=0;edit<2;edit++){
  if(edit){for(unsigned p=0;p<4;p++)for(unsigned c=0;c<4;c++){char name[32];snprintf(name,sizeof(name),"body_%u_%c",p,"rgba"[c]);u8 v=(u8)(197+p*37+c*71);((u8*)&tuning[0].x4_GAMEWATCH_COLOR[p])[c]=v;param_set(name,v);}for(unsigned c=0;c<4;c++){char name[32];snprintf(name,sizeof(name),"outline_%c","rgba"[c]);u8 v=(u8)(239+c*61);((u8*)&tuning[0].x14_GAMEWATCH_OUTLINE)[c]=v;param_set(name,v);}tuning[1]=tuning[0];CHECK(set_parameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA));}
  for(slot=0;slot<10;slot++)for(variant=0;variant<(slot==6?9:slot==8?5:slot==9?2:1);variant++)for(unsigned c=0;c<4;c++)for(unsigned player=0;player<7;player++)for(unsigned face=0;face<2;face++)for(unsigned air=0;air<2;air++)for(unsigned scale=0;scale<3;scale++)for(unsigned flags=0;flags<8;flags++){graph_mode=flags&1;allocation_fail=flags==7;compare(c,player==6?255:player,face,air,scale,flags);}
 }
 // These controlled allocator cases model a changed damage owner at paper
 // presentation for the two unattached constructors. They prove CE64 takes
 // palette from the retained creator and tint/scale from the current owner;
 // they are not evidence that a stock allocator reflects during construction.
 for(transfer_mode=1;transfer_mode<=2;transfer_mode++)for(slot=8;slot<10;slot++)for(variant=0;variant<(slot==8?5:2);variant++)for(unsigned c=0;c<4;c++)for(unsigned player=0;player<7;player++)for(unsigned face=0;face<2;face++)for(unsigned air=0;air<2;air++)for(unsigned scale=0;scale<3;scale++)for(unsigned flags=0;flags<4;flags++){graph_mode=flags&1;allocation_fail=0;compare(c,player==6?255:player,face,air,scale,flags);++transfer_pairs;}
 authored_missing_creator();
 printf("{\"checks\":%u,\"failures\":%u,\"constructor_pairs\":%u,\"presentation_transfer_pairs\":%u,\"authored_safety_cases\":%u,\"full_article_imports\":10,\"actual_dll\":true}\n",checks,failures,pairs-transfer_pairs,transfer_pairs,authored_safety_cases);return failures?1:0;
}
