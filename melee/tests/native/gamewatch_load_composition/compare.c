// Full original OnLoad versus actual generated policy. No full fighter claim.
#include "behavior.generated.h"
#include "parameters.h"
#include <melee/ft/inlines.h>
#include <melee/ft/ftmaterial.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/ft/kinds/ftGameWatch/forward.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned checks,failures,pairs,edited_pairs,registration_count,family;
#define CHECK(c) do {++checks;if(!(c)){if(failures++<16)fprintf(stderr,"%d: %s family%u pair%u\n",__LINE__,#c,family,pairs);}}while(0)
static Fighter fighters[2];static HSD_GObj objects[2];static ftData data[2];
static ftGameWatchAttributes tuning[2],backup[2];static void* articles[11];
static Article stock_articles[10];static FtPartsVisLookup outline_lookup[11];
static HSD_JObj joints[2][6];static HSD_DObj dobjs[2][6];
static HSD_MObj materials[2][6];static HSD_Material colors[2][6];
static const NativeFighterDefinition* definition;
static NativeFighterDefinition base;static MotionState stub_motion;
static bool install_result=true;
static int diffuse_calls[2],install_calls;
u32 HSD_JObjGetFlags(HSD_JObj* j) {return j?j->flags:0;}
void it_8026B3F8(Article* a,s32 kind) {
 static const ItemKind expected[]={It_Kind_GameWatch_Greenhouse,It_Kind_GameWatch_Manhole,It_Kind_GameWatch_Fire,It_Kind_GameWatch_Parachute,It_Kind_GameWatch_Turtle,It_Kind_GameWatch_Breath,It_Kind_GameWatch_Judge,It_Kind_GameWatch_Panic,It_Kind_GameWatch_Chef,It_Kind_GameWatch_Rescue};
 CHECK(registration_count<10);CHECK(a==(Article*)articles[registration_count]);CHECK(kind==expected[registration_count]);++registration_count;
}
#include "material_body.h"
static void diffuse(Fighter_GObj* g,GXColor* color) {++diffuse_calls[family];ftMaterial_800BFB4C(g,color);}
static bool install(Fighter_GObj* g,int32_t slot) {
 CHECK(g==&objects[1] && slot==10);CHECK(diffuse_calls[1]==1);++install_calls;
 if(install_result)((Fighter*)g->user_data)->x5AC.xC[4]=articles[10];return install_result;
}
// The original complete body calls the real material traversal above. Its ten
// registrations are verified separately; owned package binding replaces these.
#include "original_load.h"
static void set_value(const char* name,uint32_t bits) {
 for(unsigned i=0;i<FIXTURE_PARAMETER_COUNT;++i)if(!strcmp(fixture_parameter_names[i],name)){fixture_parameter_bits[i]=bits;return;}
 CHECK(false);
}
static void set_float(const char* name,float value) {uint32_t bits;memcpy(&bits,&value,4);set_value(name,bits);}
static int state_int(const char* name) {
 for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(definition->state_fields[i].name,name)) {
  int value;memcpy(&value,fighters[1].native_module_state+definition->state_fields[i].offset,4);return value;
 }CHECK(false);return -1;
}
static void reset(unsigned costume,unsigned graph,unsigned initial) {
 memset(fighters,0,sizeof(fighters));memset(objects,0,sizeof(objects));memset(data,0,sizeof(data));memset(joints,0,sizeof(joints));memset(dobjs,0,sizeof(dobjs));memset(materials,0,sizeof(materials));
 for(unsigned f=0;f<2;++f) {
  objects[f].user_data=&fighters[f];objects[f].hsd_obj=joints[f];fighters[f].gobj=&objects[f];fighters[f].ft_data=&data[f];data[f].ext_attr=&tuning[f];data[f].x48_items=articles;fighters[f].dat_attrs_backup=&backup[f];
  fighters[f].x619_costume_id=costume;fighters[f].x34_scale.z=2;fighters[f].x2224_b0=initial&1;fighters[f].u.gw.x2238_panicCharge=initial;fighters[f].x5AC.cleared[4]=initial&1;
  memset(fighters[f].native_module_state,0x7E,sizeof(fighters[f].native_module_state));
  for(unsigned n=0;n<6;++n) {
   colors[f][n].diffuse=(GXColor){71,72,73,74};joints[f][n].u.dobj=&dobjs[f][n];dobjs[f][n].mobj=&materials[f][n];materials[f][n].mat=&colors[f][n];
  }
  joints[f][0].child=&joints[f][1];joints[f][1].parent=&joints[f][0];joints[f][1].next=&joints[f][2];joints[f][2].parent=&joints[f][0];joints[f][2].child=&joints[f][3];joints[f][3].parent=&joints[f][2];joints[f][0].next=&joints[f][4];joints[f][4].child=&joints[f][5];joints[f][5].parent=&joints[f][4];
  if(graph&1)joints[f][2].flags|=JOBJ_INSTANCE;
  if(graph&2)dobjs[f][1].mobj=NULL;
  if(graph&4)materials[f][4].mat=NULL;
  if(graph&8)objects[f].hsd_obj=NULL;
 }
 diffuse_calls[0]=diffuse_calls[1]=install_calls=registration_count=0;
}
static void compare(unsigned costume,unsigned graph,unsigned initial) {
 reset(costume,graph,initial);family=0;ftGw_Init_OnLoad(&objects[0]);family=1;definition->on_load(&objects[1]);++pairs;
 CHECK(registration_count==10);CHECK(install_calls==1);CHECK(state_int("panic_charge")==fighters[0].u.gw.x2238_panicCharge);
 CHECK(fighters[0].x34_scale.z==fighters[1].x34_scale.z);CHECK(fighters[0].x2224_b0==fighters[1].x2224_b0);
 CHECK(!memcmp(&fighters[0].x610_color_rgba[1],&fighters[1].x610_color_rgba[1],sizeof(GXColor)));
 CHECK(fighters[0].x5AC.xC[4]==fighters[1].x5AC.xC[4]);CHECK(fighters[0].x5AC.cleared[4]==fighters[1].x5AC.cleared[4]);
 for(unsigned n=0;n<6;++n)CHECK(!memcmp(&colors[0][n].diffuse,&colors[1][n].diffuse,sizeof(GXColor)));
 // Dead x2223_b1 assignment is intentionally excluded; common reset x2222_b6
 // is the required real load precondition, not a fabricated script flag setter.
 CHECK(fighters[0].x2222_b6==fighters[1].x2222_b6);
}
int main(void) {
 for(unsigned i=0;i<10;++i)articles[i]=&stock_articles[i];articles[10]=outline_lookup;
 NativeFighterHost host={.set_fighter_diffuse_color=diffuse,.install_outline_visibility=install};base.source_kind=Ft_Kind_GameWatch;
 base.joint_count=53;base.motion_state_first=341;base.motion_state_count=1;base.motion_states=&stub_motion;definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition!=NULL);if(!definition)return 2;
 memcpy(&tuning[0].x0_GAMEWATCH_WIDTH,fixture_parameter_bits,4);
 for(unsigned p=0;p<4;++p) {u8* c=(u8*)&tuning[0].x4_GAMEWATCH_COLOR[p];for(unsigned k=0;k<4;++k)c[k]=(u8)fixture_parameter_bits[1+p*4+k];}
 for(unsigned k=0;k<4;++k)((u8*)&tuning[0].x14_GAMEWATCH_OUTLINE)[k]=(u8)fixture_parameter_bits[17+k];
 tuning[1]=tuning[0];CHECK(FreakMelee_SetFighterParameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA));
 for(unsigned c=0;c<4;++c)for(unsigned g=0;g<16;++g)for(unsigned initial=0;initial<8;++initial)compare(c,g,initial);
 for(unsigned edit=0;edit<64;++edit) {
  tuning[0].x0_GAMEWATCH_WIDTH=edit&1?-.125f:.25f;set_float("model_width",tuning[0].x0_GAMEWATCH_WIDTH);
  for(unsigned p=0;p<4;++p)for(unsigned k=0;k<4;++k) {char name[32];snprintf(name,sizeof(name),"body_%u_%c",p,"rgba"[k]);u8 v=(u8)(edit*31+p*19+k*71);((u8*)&tuning[0].x4_GAMEWATCH_COLOR[p])[k]=v;set_value(name,v);}
  for(unsigned k=0;k<4;++k){char name[32];snprintf(name,sizeof(name),"outline_%c","rgba"[k]);u8 v=(u8)(edit*23+k*37);((u8*)&tuning[0].x14_GAMEWATCH_OUTLINE)[k]=v;set_value(name,v);}
  CHECK(FreakMelee_SetFighterParameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA));
  for(unsigned c=0;c<4;++c)for(unsigned g=0;g<16;++g){compare(c,g,3);++edited_pairs;}
 }
 // Missing immutable visibility does not fabricate a lookup or clear flags.
 install_result=false;reset(0,0,1);family=1;definition->on_load(&objects[1]);CHECK(fighters[1].x5AC.xC[4]==NULL && fighters[1].x5AC.cleared[4]==1);
 printf("{\"checks\":%u,\"failures\":%u,\"pairs\":%u,\"edited_pairs\":%u}\n",checks,failures,pairs,edited_pairs);return failures?1:0;
}
