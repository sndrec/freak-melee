#include <melee/it/article_model_layers.h>
#include <melee/it/itarticle_data.h>
#include <melee/it/itCharItems.h>
#include <melee/it/inlines.h>
#include <melee/it/itdraw.h>
#include <melee/ft/ftlib.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/jobj.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, failures, imports, draw_cases, invalid_cases;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; fprintf(stderr,"%d: %s\n",__LINE__,#c); } } while(0)
#include "fighter_import_bodies.h"
static int camera, body_color_calls, outline_color_calls, tint_calls, scale_calls, draw_count;
static Item* drawing;
static unsigned flag_trace[4], hidden_trace[4][100], node_count;
static HSD_JObj nodes[101];
static bool holder, displacement, hidden_by_owner;
void HSD_JObjSetMtxDirtySub(HSD_JObj* j) { (void)j; CHECK(false); }
void ftLib_8008770C(HSD_GObj* g,void* out) { (void)g; ++body_color_calls; *(GXColor*)out=(GXColor){1,2,3,4}; }
void ftLib_80087744(HSD_GObj* g,void* out) { (void)g; ++outline_color_calls; *(GXColor*)out=(GXColor){5,6,7,8}; }
bool ftLib_800870BC(HSD_GObj* g,GXColor* out) { (void)g; ++tint_calls; *out=(GXColor){9,10,11,12}; return true; }
void it_80278574(Item_GObj* g,GXColor* c) { (void)g; CHECK(c->r==1 && c->a==4); }
void it_80274594(Item_GObj* g) { (void)g; ++scale_calls; }
s32 Camera_80031060(void) { return camera; }
bool ftLib_80086960(HSD_GObj* g) { (void)g; return holder; }
bool ftLib_80087074(HSD_GObj* g,Vec3* p) { (void)g; *p=(Vec3){1,2,3}; return displacement; }
bool ftLib_800868D4(HSD_GObj* a,HSD_GObj* b) { (void)a;(void)b;return hidden_by_owner; }
void it_8026EB18(HSD_GObj* g,s32 arg,Vec3* pos) {
 (void)g;(void)arg;(void)pos; CHECK(draw_count<4);
 flag_trace[draw_count]=(drawing->xDCF_flag.b4?1:0)|(drawing->xDCF_flag.b5?2:0);
 for(unsigned i=0;i<node_count;++i) hidden_trace[draw_count][i]=!!(nodes[i].flags&JOBJ_HIDDEN);
 ++draw_count;
}
u32 it_8026ECE0(Item_GObj* g,u32 arg) { (void)g;(void)arg;return 0; }
void HSD_StateInvalidate(s32 a) { (void)a; CHECK(false); }
void HSD_StateInitTev(void) { CHECK(false); }
void HSD_ClearVtxDesc(void) { CHECK(false); }
u32 HSD_GObj_80390EB8(s32 a) { return a; }
void HSD_JObjDispAll(HSD_JObj* j,Mtx m,u32 a,u32 b) { (void)j;(void)m;(void)a;(void)b; CHECK(false); }
u8 HSD_GObj_JObjKind;
static HSD_JObj replacement_root;
static unsigned replacements;
void HSD_GObjObject_80390B0C(HSD_GObj* g) { Item* i=g->user_data;CHECK(!i->model_layers && !i->xDCF_flag.b3);g->hsd_obj=NULL;++replacements; }
void HSD_GObjObject_80390A70(HSD_GObj* g,u8 kind,void* model) { (void)kind;Item* i=g->user_data;CHECK(!i->model_layers && !i->xDCF_flag.b3);g->hsd_obj=model; }
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* descriptor) { CHECK(descriptor!=NULL);return &replacement_root; }
HSD_JObj* HSD_JObjAlloc(void) { return &replacement_root; }
void C_MTXIdentity(Mtx matrix) { memset(matrix,0,sizeof(Mtx));matrix[0][0]=matrix[1][1]=matrix[2][2]=1; }
void Item_8026849C(HSD_GObj* g) { Item* i=g->user_data;CHECK(!i->model_layers && !i->xDCF_flag.b3); }
void __assert(char* file,u32 line,char* condition) {fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,condition);abort();}
#include "render_bodies.h"
static u32 target(const HSD_Dat* d,u32 at) { u32 out=0;const char* x; CHECK(HSD_DatResolve(d,at,&out,&x)==HSD_DAT_REF_LOCAL);return out; }
static void make_model(const HSD_Joint* descriptor,HSD_JObj* parent,DynamicBoneTable* table) {
 while(descriptor) {
  CHECK(node_count<100);HSD_JObj* n=&nodes[node_count];table->bones[node_count++]=n;
  n->parent=parent;n->flags=descriptor->flags;
  if(parent && !parent->child) parent->child=n;
  if(descriptor->child) make_model(descriptor->child,n,table);
  const HSD_Joint* next=descriptor->next;
  unsigned next_index=node_count;
  if(next) n->next=&nodes[next_index];
  descriptor=next;
 }
}
static void runtime_matrix(Article* article) {
 DynamicBoneTable table={0};HSD_GObj g={0};Item item={0};
 memset(nodes,0,sizeof(nodes));node_count=0;make_model(article->x10_modelDesc->x0_joint,NULL,&table);
 g.user_data=&item;g.hsd_obj=&nodes[0];item.entity=&g;item.xC4_article_data=article;item.xBBC_dynamicBoneTable=&table;
 item.xDD4_itemVar.gamewatchchef.variant=3;item.xDD4_itemVar.native_article.state[511]=0xA5;
 unsigned char private_before[sizeof(item.xDD4_itemVar)];memcpy(private_before,&item.xDD4_itemVar,sizeof(private_before));
 CHECK(articleModelLayers_ValidateItem(&item));
 CHECK(articleModelLayers_Bind(&g,true));CHECK(articleModelLayers_Bind(&g,false));CHECK(!item.model_layers && !item.xDCF_flag.b3);
 HSD_GObj other={.user_data=&item};CHECK(!articleModelLayers_Bind(&other,true));CHECK(!articleModelLayers_Bind(NULL,true));
 body_color_calls=outline_color_calls=tint_calls=scale_calls=0;it_8027CE64(&g,NULL);
 CHECK(item.model_layers==article->model_layers && item.xDCF_flag.b3);
 CHECK(body_color_calls==1 && outline_color_calls==1 && tint_calls==1 && scale_calls==1);
 CHECK(memcmp(private_before,&item.xDD4_itemVar,sizeof(private_before))==0 && item.xDD4_itemVar.native_article.state[511]==0xA5);
 drawing=&item;item.xDAA_flag.b7=true;
 const ArticleModelLayers* l=article->model_layers;
 for(unsigned parent_hidden=0;parent_hidden<2;++parent_hidden)
 for(camera=0;camera<3;++camera)
 for(unsigned held=0;held<2;++held)
 for(unsigned disp=0;disp<2;++disp) {
  for(unsigned i=0;i<node_count;++i)nodes[i].flags=0;
  for(unsigned i=0;i<l->outline_count;++i) nodes[l->outline_indices[i]].flags=JOBJ_HIDDEN;
  if(parent_hidden)for(unsigned i=0;i<l->normal_count;++i)nodes[l->normal_indices[i]].parent->flags|=JOBJ_HIDDEN;
  unsigned initial[100];for(unsigned i=0;i<node_count;++i)initial[i]=nodes[i].flags;
  holder=held;displacement=disp;item.owner=held?&g:NULL;item.xDC8_word.flags.x13=held;hidden_by_owner=true;draw_count=0;
  it_8026EECC(&g,0);++draw_cases;
  CHECK(draw_count==(camera==1?3:0));
  if(camera==1) {
   CHECK(flag_trace[0]==1 && flag_trace[1]==0 && flag_trace[2]==3);
   for(unsigned i=0;i<l->normal_count;++i) {
    unsigned n=l->normal_indices[i];CHECK(hidden_trace[0][n]==(parent_hidden?!!(initial[n]&JOBJ_HIDDEN):1));
   }
   for(unsigned i=0;i<l->outline_count;++i) {
    unsigned n=l->outline_indices[i];CHECK(hidden_trace[0][n]==((nodes[n].parent->flags&JOBJ_HIDDEN)?1:0));
   }
  }
  for(unsigned i=0;i<node_count;++i)CHECK(nodes[i].flags==initial[i]);
 }
 // Actual binding must reject stale/mismatched dynamic ordinals and no parent.
 unsigned n=l->normal_indices[0];HSD_JObj* old=table.bones[n];table.bones[n]=&nodes[100];CHECK(!articleModelLayers_ValidateItem(&item));it_8027CE64(&g,NULL);CHECK(!item.xDCF_flag.b3 && item.model_layers==NULL);table.bones[n]=old;
 HSD_JObj* parent=nodes[n].parent;nodes[n].parent=NULL;CHECK(!articleModelLayers_ValidateItem(&item));nodes[n].parent=parent;
 item.xBBC_dynamicBoneTable=NULL;CHECK(!articleModelLayers_ValidateItem(&item));
 it_8027CE64(NULL,NULL);g.user_data=NULL;it_8027CE64(&g,NULL);g.user_data=&item;
 item.model_layers=l;item.xDCF_flag.b3=true;replacement_root.flags=JOBJ_MTX_INDEP_SRT;
 it_80273318(&g,article->x10_modelDesc->x0_joint);CHECK(!item.model_layers && !item.xDCF_flag.b3);
 item.model_layers=l;item.xDCF_flag.b3=true;item.xC8_joint=NULL;Item_802680CC(&g);CHECK(!item.model_layers && !item.xDCF_flag.b3);
}
static void retail(u8* bytes,size_t size) {
 HSD_Dat dat;CHECK(HSD_DatOpen(&dat,bytes,size)==HSD_DAT_OK);u32 root;CHECK(HSD_DatFindRoot(&dat,"ftDataGamewatch",&root));
 // Root symbol is the actual fighter data record, whose +0x48 is item slots.
 u32 table_source=target(&dat,root+0x48);
 for(unsigned i=0;i<10;++i) {
  void* arena=malloc(16*1024*1024);HSD_DatImport import;CHECK(HSD_DatImportInit(&import,&dat,arena,16*1024*1024,32768)==HSD_DAT_OK);
  u32 source=target(&dat,table_source+4*i);ItArticleImportStats stats={0};ItScriptEntry entries[512];ItScriptCollect scripts={entries,0,512};
  Article* a=itArticle_Import(&import,source,&stats,&scripts,false,i==8?It_Kind_Kirby_GameWatchChef:It_Kind_Kirby_GameWatchChefPan,NULL,0);
  if(!a) {fprintf(stderr,"article %u status%d offset%u\n",i,import.status,(unsigned)import.error_offset);CHECK(a!=NULL);free(arena);continue;}
  ++imports;CHECK(a->model_layers!=NULL && stats.article_count==1);
  u32 attrs=target(&dat,source+4),layers=target(&dat,attrs);
  const ArticleModelLayers* l=a->model_layers;u16 count;
  CHECK(HSD_DatReadU16(&dat,layers,&count) && l->normal_count==count);
  CHECK(memcmp(l->normal_indices,HSD_DatBytes(&dat,target(&dat,layers+4),count),count)==0);
  CHECK(HSD_DatReadU16(&dat,layers+8,&count) && l->outline_count==count);
  CHECK(memcmp(l->outline_indices,HSD_DatBytes(&dat,target(&dat,layers+12),count),count)==0);
  Article fighter_a={.x10_modelDesc=a->x10_modelDesc};
  void* special=i==8?(void*)import_gamewatch_chef_attributes(&import,source,&fighter_a):(void*)import_gamewatch_article_attributes(&import,source,&fighter_a);
  CHECK(special!=NULL && fighter_a.model_layers!=NULL);
  CHECK(fighter_a.model_layers!=l && fighter_a.model_layers->normal_indices!=l->normal_indices);
  CHECK(memcmp(fighter_a.model_layers->normal_indices,l->normal_indices,l->normal_count)==0);
  CHECK(memcmp(fighter_a.model_layers->outline_indices,l->outline_indices,l->outline_count)==0);
  if(i==8) {itGamewatchchefAttributes* x=a->x4_specialAttributes,*y=special;CHECK(x->wall_bounce_velocity_multiplier==0.5f && x->active_lifetime==80 && x->inactive_lifetime==30);CHECK(memcmp(&x->wall_bounce_velocity_multiplier,&y->wall_bounce_velocity_multiplier,0x70)==0);}
  runtime_matrix(a);free(arena);
 }
}
static void word(u8* p,u32 at,u32 v) {for(unsigned i=0;i<4;++i)p[at+i]=(u8)(v>>(24-8*i));}
static void half(u8* p,u32 at,u16 v) {p[at]=(u8)(v>>8);p[at+1]=(u8)v;}
static void synthetic(void) {
 u8 bytes[1024]={0},rels[16]={0},arena[65536];HSD_Joint graph[101]={0};
 word(rels,0,0);word(rels,4,4);word(rels,8,36);word(rels,12,44);
 HSD_Dat dat={.data=bytes,.data_size=sizeof(bytes),.relocations=rels,.relocation_count=4,.relocations_sorted=true};
 word(bytes,0,32);word(bytes,4,48);word(bytes,36,48);word(bytes,44,304);half(bytes,32,256);half(bytes,40,256);
 for(unsigned i=0;i<101;++i) {if(i<100)graph[i].child=&graph[i+1];}
 // Exact capacity succeeds, with repeat and unsorted index preservation.
 graph[99].child=NULL;for(unsigned i=0;i<256;++i) bytes[48+i]=bytes[304+i]=(u8)(99-i%99);
 HSD_DatImport imp;CHECK(HSD_DatImportInit(&imp,&dat,arena,sizeof(arena),64)==HSD_DAT_OK);
 ArticleModelLayers* l=articleModelLayers_Import(&imp,0,graph);CHECK(l && l->normal_count==256 && memcmp(l->normal_indices,bytes+48,256)==0 && l->normal_indices!=bytes+48);
 for(unsigned mode=0;mode<9;++mode) {
  half(bytes,32,256);word(bytes,36,48);bytes[48]=99;graph[99].child=NULL;
  if(mode==0)half(bytes,32,257);
  if(mode==1)bytes[48]=100;
  if(mode==2)bytes[48]=0;
  if(mode==3)word(bytes,36,0);
  if(mode==4)graph[99].child=&graph[100];
  if(mode==5)graph[99].child=&graph[0];
  if(mode==6)half(bytes,32,0); // Non-NULL list with zero count rejected.
  if(mode==7)word(bytes,4,47); // Descriptor extent not sixteen.
  CHECK(HSD_DatImportInit(&imp,&dat,arena,sizeof(arena),64)==HSD_DAT_OK);
  CHECK(articleModelLayers_Import(&imp,0,mode==8?NULL:graph)==NULL && imp.status!=HSD_DAT_OK);++invalid_cases;
  word(bytes,4,48);
 }
 dat.relocation_count=2;graph[99].child=NULL;half(bytes,32,0);half(bytes,40,0);word(bytes,36,0);word(bytes,44,0);
 CHECK(HSD_DatImportInit(&imp,&dat,arena,sizeof(arena),64)==HSD_DAT_OK);l=articleModelLayers_Import(&imp,0,graph);CHECK(l && !l->normal_indices && !l->outline_indices);
 HSD_GObj g={.hsd_obj=&nodes[0]};Article a={.model_layers=l};Item item={.entity=&g,.xC4_article_data=&a};g.user_data=&item;memset(nodes,0,sizeof(nodes));CHECK(articleModelLayers_ValidateItem(&item));
 CHECK(articleModelLayers_Bind(&g,true));CHECK(articleModelLayers_Bind(&g,false));CHECK(!item.model_layers && !item.xDCF_flag.b3);
 CHECK(!articleModelLayers_ValidateItem(NULL));item.entity=NULL;CHECK(!articleModelLayers_ValidateItem(&item));
}
int main(int argc,char** argv) {
 CHECK(argc==2);FILE* f=fopen(argv[1],"rb");CHECK(f!=NULL);fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);u8* bytes=malloc(size);CHECK(fread(bytes,1,size,f)==(size_t)size);fclose(f);retail(bytes,size);synthetic();free(bytes);
 printf("{\"checks\":%u,\"failures\":%u,\"full_article_imports\":%u,\"draw_cases\":%u,\"invalid_import_cases\":%u}\n",checks,failures,imports,draw_cases,invalid_cases);return failures?1:0;
}
