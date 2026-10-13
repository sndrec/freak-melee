#include "fixture.h"
#include <melee/ft/native_script_binding_air_inputs.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftCo_SpecialAir.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/native_holdable.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/ithitbox.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum {N=2048,I=8192,F=64,V=128};
typedef struct Buffers {
 char source[16384];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
 NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
 NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];
 NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} Buffers;
NativeSourcePackage air_packages[3];static Buffers buffers;
#define bank air_packages[0].compiler.bank
static Fighter fighters[128],substitute;static Item items[128];static unsigned fighter_count,item_count;
static FighterBone parts[MAX_FT_PARTS];static HSD_JObj joints[MAX_FT_PARTS];static MotionState motions[345];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static ItemLogicTable logic;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
HSD_GObjEvent ftData_SpecialAirHi[Ft_Kind_RuntimeCapacity],ftData_SpecialAirLw[Ft_Kind_RuntimeCapacity],ftData_SpecialAirS[Ft_Kind_RuntimeCapacity],ftData_SpecialAirN[Ft_Kind_RuntimeCapacity];
void (*ftData_OnItemDrop[Ft_Kind_RuntimeCapacity])(Fighter_GObj*,bool);
static ftCommonData common;ftCommonData* p_ftCommonData=&common;
static unsigned checks,cases,pairs,vm_calls,mutation,trigger,motion_calls,animation_calls,drop_tails,timers,accessories;
static HSD_GObj* watched,*changed_held,*recycled;static bool mode_policy;static int parasol=-1;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"air %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static const char* source=
 "fighter:\n    behavior independent\nstate int seen\nstate int tail\nstate int zero\nstate bool fault\nstate int selected\nstate bool edge\nstate bool handled\n"
 "func Observe():\n    state.selected = controls.aerial_attack_motion()\n    state.edge = input.cstick_attack_edge()\n"
 "func Gun():\n    state.air_gun()\n    state.tail = state.tail + 1\n"
 "func Throw():\n    state.handled = controls.try_air_item_throw()\n    state.tail = state.tail + 1\n"
 "func Special():\n    state.handled = controls.try_air_special()\n    state.tail = state.tail + 1\n"
 "move Up:\n    on special_air_hi:\n        state.seen = 1\n        position.x = 1.0\n        if state.fault:\n            state.seen = 1 / state.zero\nmove Down:\n    on special_air_lw:\n        state.seen = 2\n        position.x = 2.0\nmove Side:\n    on special_air_s:\n        state.seen = 3\n        position.x = 3.0\nmove Neutral:\n    on special_air_n:\n        state.seen = 4\n        position.x = 4.0\naction Holder:\n    on item_drop:\n        state.seen = 5\n        position.x = 5.0\n        if state.fault:\n            state.seen = 1 / state.zero\n"
 "article Probe states 1:\n    state int seen\n    state int zero\n    state bool fault\n    on dropped:\n        state.seen = state.seen + 1\n        velocity.x = 9.0\n        if state.fault:\n            state.seen = 1 / state.zero\n    state 0 anim -1:\n";
static void prepare(void)
{
 Buffers* p=&buffers;
 bank=(NativeScriptCompileBank){.source_storage=p->source,.source_capacity=sizeof(p->source),.tree={.nodes=p->nodes,.capacity=N},.expressions={.nodes=p->ir,.node_capacity=I,.arguments=p->arguments,.argument_capacity=I},.declarations={.functions=p->functions,.function_capacity=F,.parameters=p->parameters,.parameter_capacity=V,.values=p->values,.value_capacity=V},.locals=p->locals,.local_capacity=V,.instructions=p->instructions,.instruction_capacity=I,.function_code=p->codes,.function_code_capacity=F,.execution_functions=p->entries,.execution_function_capacity=F,.graph=p->graph,.graph_capacity=F,.storage_layout={.fields=p->fields,.field_capacity=V,.owners=p->owners,.owner_capacity=V},.parameter_slots=p->slots,.parameter_slot_capacity=V};
 article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};attributes.x1C_damage_mul=2;attributes.x4_throw_speed_mul=1;
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Seak_Chain,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=4};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(accepted==&bank);
 NativeSourcePackage* package=&air_packages[0];CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&package->program,&diagnostic));
 package->address=package;package->ready=package->assets.ready=true;package->target=Ft_Kind_CustomFirst;package->assets.facts.moves=layout;package->assets.facts.joints=MAX_FT_PARTS;
 package->assets.parts.parts_num=MAX_FT_PARTS;package->assets.parts.part_to_joint=package->assets.part_to_joint;package->assets.parts.joint_to_part=package->assets.joint_to_part;package->assets.motions=motions;
 tables[package->target]=&package->assets.parts;gFtDataList[package->target]=&package->assets.character.data;ftData_CharacterStateTables[package->target]=motions;
 for(unsigned n=0;n<MAX_FT_PARTS;++n)parts[n].joint=&joints[n];
 CHECK(nativeScriptFighter_CanPublish(package->target));nativeScriptFighter_PublishProgram(package->target,&package->program);
 common.xDC=.7f;common.xE0=.7f;common.x20_radians=.6f;common.x21C=.5f;common.x218=.6f;common.x220=.3f;common.x224=4;common.x3FC=4;common.x400=2;
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* actor(void)
{
 CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);Fighter* f=&fighters[fighter_count++];
 *f=(Fighter){.gobj=o,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,.x8_spawnNum=100+fighter_count,.ft_data=&air_packages[0].assets.character.data,.parts=parts,.facing_dir=1};
 GObj_InitUserData(o,4,fighter_remove,f);CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* held(HSD_GObj* owner,bool owned,ItemKind kind)
{
 CHECK(item_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(o);Item* i=&items[item_count++];
 const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
 *i=(Item){.entity=o,.kind=owned?p->kind:kind,.x1C=200+item_count,.owner=owner,.msid=-1,.xC4_article_data=&article,.xCC_item_attr=&attributes,.xB8_itemLogicTable=&logic,.xD4C=1};
 GObj_InitUserData(o,6,item_remove,i);if(owned){i->native_script_owner=p->owner;i->native_article_author_kind=p->author_kind;CHECK(nativeScriptItem_Attach(o,p));}
 ((Fighter*)owner->user_data)->item_gobj=o;return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t function(const char* name)
{for(unsigned n=0;n<bank.declarations.function_count;++n)if(!bank.declarations.functions[n].owner&&nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[n].name,name))return n;CHECK(false);return 0;}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
 NativeScriptReceiverView view;bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
 CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
 for(unsigned n=0;n<bank.declarations.value_count;++n)if(bank.declarations.values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[n].name,name)){
  NativeScriptValue r;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&r,&diagnostic));return r;}
 CHECK(false);return(NativeScriptValue){0};
}
static bool helper(HSD_GObj* o,const char* name)
{
 NativeScriptValue r={.type=NativeScriptType_U64,.as.u64=99};++vm_calls;
 NativeScriptVMStatus status=nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,function(name),NULL,0,&r,&diagnostic);
 if(status==NativeScriptVM_Ok)CHECK(r.type==NativeScriptType_Void);else CHECK(r.type==NativeScriptType_U64&&r.as.u64==99);return status==NativeScriptVM_Ok;
}
static void inject(HSD_GObj* o,unsigned stage)
{
 if(o!=watched||!mutation||stage!=trigger)return;unsigned m=mutation;mutation=0;
 if(m==1)nativeScriptFighter_Detach(o);
 if(m==2)HSD_GObjFree(o);
 if(m==3){HSD_GObjFree(o);recycled=actor();}
 if(m==4)++((Fighter*)o->user_data)->x8_spawnNum;
 if(m==5){substitute=*(Fighter*)o->user_data;o->user_data=&substitute;}
 if(m==6)o->classifier=HSD_GOBJ_CLASS_ITEM;
 if(m==7)++bank.storage_layout.identity;
 if(m==8)++bank.storage_parameters.identity;
 if(m==9)((Fighter*)o->user_data)->item_gobj=changed_held;
}
void air_write_probe(NativeScriptRuntimeContext* c)
{
 if(c->source_class==NativeCallbackSource_Fighter){inject(c->source,3);return;}
 if(c->source!=watched||!mutation)return;unsigned m=mutation;mutation=0;
 if(m==10){NativeScriptValue r;CHECK(nativeScriptItem_RunHook(c->source,NativeScriptItemHook_Destroyed,NULL,0,&r)==NativeScriptVM_Ok);}
 if(m==11)HSD_GObjFree(c->source);
 if(m==12){HSD_GObj* owner=((Item*)((HSD_GObj*)c->source)->user_data)->owner;HSD_GObjFree(c->source);recycled=held(owner,false,It_Kind_L_Gun);}
 if(m==13)++((Item*)((HSD_GObj*)c->source)->user_data)->x1C;
}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId motion,MotionFlags flags,float start,float rate,float blend,Fighter_GObj* other)
{(void)flags;(void)start;(void)blend;(void)other;++motion_calls;Fighter* f=o->user_data;f->motion_id=motion;f->frame_speed_mul=rate;inject(o,1);}
void ftAnim_8006EBA4(Fighter_GObj* o){++animation_calls;inject(o,2);}
void ftCommon_ClampAirDrift(Fighter* f){f->self_vel.x*=.5f;}
void ftCommon_UpdateFacing(Fighter* f){f->facing_dir=-f->facing_dir;}
void ftCo_800CE1D4(HSD_GObj* o){(void)o;++accessories;}
void ftCo_800CE650(HSD_GObj* o){(void)o;++accessories;}
void ftCo_800CE620(HSD_GObj* o){(void)o;}
void fn_800D7938(HSD_GObj* o){(void)o;}
static void ftCo_80095EFC(HSD_GObj* o){(void)o;++accessories;}
void it_8028E938(HSD_GObj* o){(void)o;++timers;}
s32 ftGetParasolStatus(HSD_GObj* o){(void)o;return parasol;}
void ftCo_800968C8(HSD_GObj* o){inject(o,1);}
void ftCo_Fall_Enter(HSD_GObj* o){inject(o,1);}
bool gm_8016B0FC(void){return mode_policy;}
void Item_8026AE84(Item* i,enum_t sound,u8 volume,u8 pan){(void)i;(void)sound;(void)volume;(void)pan;}
void it_80273B50(Item_GObj* o,Vec3* v){((Item*)o->user_data)->x40_vel=*v;}
void it_80274198(Item_GObj* o,bool b){(void)o;CHECK(b);++drop_tails;}
void it_80275390(Item_GObj* o){(void)o;++drop_tails;}
NativeScriptVMStatus nativeHoldable_Dropped(Item_GObj* o,NativeScriptValue* r){(void)o;(void)r;abort();}
#include "native_air.inc"
#include "reference_air.inc"
static void raw_hi(HSD_GObj* o){((Fighter*)o->user_data)->cur_pos.x=1;}
static void raw_lw(HSD_GObj* o){((Fighter*)o->user_data)->cur_pos.x=2;}
static void raw_side(HSD_GObj* o){((Fighter*)o->user_data)->cur_pos.x=3;}
static void raw_neutral(HSD_GObj* o){((Fighter*)o->user_data)->cur_pos.x=4;}
static void raw_holder_drop(HSD_GObj* o,bool flag){CHECK(flag);((Fighter*)o->user_data)->cur_pos.x=5;}
static void raw_drop(HSD_GObj* o){((Item*)o->user_data)->x40_vel.x=9;}
static void pure(HSD_GObj* o)
{
 Fighter* f=o->user_data;float axes[]={-1,-.7f,-.6999f,-0.0f,0,.6999f,.7f,1,NAN};
 for(unsigned x=0;x<9;++x)for(unsigned y=0;y<9;++y)for(unsigned old=0;old<2;++old){f->input.cstick[0].x=axes[x];f->input.cstick[0].y=axes[y];f->input.cstick[1].x=old;f->input.cstick[1].y=old;f->input.lstick[0].x=-axes[y];f->input.lstick[0].y=axes[x];f->facing_dir=old?-1:1;static Fighter before;before=*f;
  int selected=reference_ftCo_AttackAir_GetMsidFromCStick(f);bool edge=reference_ftCo_800DF478(f);CHECK(helper(o,"Observe"));CHECK(field(o,"selected",NULL).as.i64==selected&&field(o,"edge",NULL).as.u64==edge);CHECK(!memcmp(&before,f,sizeof(*f)));++cases;pairs+=2;}
}
static void specials(HSD_GObj* o)
{
 Fighter* f=o->user_data;const Vec2 inputs[]={{0,.5f},{0,-.5f},{-.6f,0},{.6f,0},{0,0}};
 for(unsigned n=0;n<5;++n){f->input.lstick[0]=inputs[n];f->input.pressed_buttons=HSD_PAD_B;f->x2227_b5=false;f->facing_dir=1;f->x676_x=0;f->x2228_b7=0;static Fighter before,expected;before=*f;
  FighterKind kind=f->kind;nativeScriptFighter_Detach(o);CHECK(reference_ftCo_SpecialAir_CheckInput(o));expected=*f;*f=before;CHECK(nativeScriptFighter_Attach(o));CHECK(helper(o,"Special"));CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(field(o,"handled",NULL).as.u64&&field(o,"seen",NULL).as.i64==(n<2?n+1:n<4?3:4));CHECK(f->kind==kind);++cases;++pairs;}
 const NativeScriptFighterProgram* p=nativeScriptFighter_Program(f->kind);NativeScriptFighterProgram* mutable=(NativeScriptFighterProgram*)p;uint32_t code=p->hooks[NativeScriptFighter_SpecialAirS];mutable->hooks[NativeScriptFighter_SpecialAirS]=UINT32_MAX;
 f->input.lstick[0]=(Vec2){-1,0};f->input.pressed_buttons=HSD_PAD_B;f->facing_dir=1;f->x2227_b5=false;CHECK(helper(o,"Special"));CHECK(!field(o,"handled",NULL).as.u64&&f->facing_dir==1&&!f->x2227_b5);mutable->hooks[NativeScriptFighter_SpecialAirS]=code;++cases;
 f->input.pressed_buttons=0;CHECK(helper(o,"Special")&&!field(o,"handled",NULL).as.u64);++cases;
}
static void guns(HSD_GObj* o)
{
 Fighter* f=o->user_data;const ItemKind kinds[]={It_Kind_L_Gun,It_Kind_F_Flower,It_Kind_S_Scope,It_Kind_Seak_Chain};
 for(unsigned k=0;k<4;++k)for(unsigned ammo=0;ammo<2;++ammo){HSD_GObj* h=held(o,false,kinds[k]);Item* i=h->user_data;i->xD4C=ammo;f->self_vel.x=4;f->throw_flags=0xff;f->accessory4_cb=NULL;f->take_dmg_cb=NULL;static Fighter before,expected;before=*f;reference_ftCo_800CDDA0(o);expected=*f;*f=before;CHECK(helper(o,"Gun"));CHECK(!memcmp(f,&expected,sizeof(*f)));HSD_GObjFree(h);f->item_gobj=NULL;++cases;++pairs;}
 CHECK(helper(o,"Gun"));++cases;
}
static void throws(HSD_GObj* o)
{
 Fighter* f=o->user_data;HSD_GObj* h=held(o,false,It_Kind_Seak_Chain);Item* i=h->user_data;logic.dropped=raw_drop;
 const Vec2 dirs[]={{0,0},{1,0},{-1,0},{0,1},{0,-1}};
 for(unsigned use=0;use<2;++use)for(unsigned fast=0;fast<2;++fast)for(unsigned d=0;d<5;++d){attributes.use_kind=use;f->input.cstick[0]=(Vec2){0,0};f->input.cstick[1]=(Vec2){0,0};f->input.lstick[0]=dirs[d];f->input.pressed_buttons=HSD_PAD_A;f->input.held_buttons[0]=HSD_PAD_LR;f->x673=f->x674=fast?5:0;f->facing_dir=1;f->x2224_b1=false;static Fighter before,expected;static Item ibefore,iexpected;before=*f;ibefore=*i;bool handled_input=reference_ftCo_80095328(o,NULL);expected=*f;iexpected=*i;*f=before;*i=ibefore;CHECK(helper(o,"Throw"));CHECK(field(o,"handled",NULL).as.u64==handled_input);CHECK(!memcmp(f,&expected,sizeof(*f))&&!memcmp(i,&iexpected,sizeof(*i)));++cases;++pairs;}
 f->input.pressed_buttons=0;f->input.cstick[0].x=1;f->input.cstick[1].x=0;mode_policy=true;attributes.use_kind=1;CHECK(helper(o,"Throw")&&!field(o,"handled",NULL).as.u64);mode_policy=false;CHECK(helper(o,"Throw")&&field(o,"handled",NULL).as.u64);cases+=2;
 HSD_GObjFree(h);f->item_gobj=NULL;CHECK(helper(o,"Throw")&&!field(o,"handled",NULL).as.u64);++cases;
}
static void lifetime(void)
{
 for(unsigned stage=1;stage<=2;++stage)for(unsigned m=1;m<=8;++m){HSD_GObj* o=actor();Fighter* f=o->user_data;HSD_GObj* h=held(o,false,It_Kind_F_Flower);f->accessory4_cb=NULL;f->take_dmg_cb=NULL;watched=o;mutation=m;trigger=stage;uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;CHECK(!helper(o,"Gun"));CHECK(!mutation);CHECK(!f->accessory4_cb&&!f->take_dmg_cb);bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;watched=NULL;HSD_GObjFree(h);if(m!=2&&m!=3){o->user_data=f;o->classifier=HSD_GOBJ_CLASS_FIGHTER;nativeScriptFighter_Detach(o);HSD_GObjFree(o);}if(recycled){HSD_GObjFree(recycled);recycled=NULL;}++cases;}
 for(unsigned m=1;m<=8;++m){HSD_GObj* o=actor();Fighter* f=o->user_data;f->input.pressed_buttons=HSD_PAD_B;f->input.lstick[0].y=1;f->x2227_b5=false;watched=o;mutation=m;trigger=3;uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;CHECK(!helper(o,"Special"));CHECK(!f->x2227_b5);bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;watched=NULL;if(m!=2&&m!=3){o->user_data=f;o->classifier=HSD_GOBJ_CLASS_FIGHTER;nativeScriptFighter_Detach(o);HSD_GObjFree(o);}if(recycled){HSD_GObjFree(recycled);recycled=NULL;}++cases;}
 for(unsigned mode=0;mode<6;++mode){HSD_GObj* o=actor();Fighter* f=o->user_data;HSD_GObj* h=held(o,true,0);attributes.use_kind=1;f->input.pressed_buttons=HSD_PAD_A;f->input.held_buttons[0]=HSD_PAD_LR;unsigned tails=drop_tails;
  if(mode==1)field(h,"fault",&(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1});watched=h;mutation=mode>=2?mode+8:0;trigger=3;
  bool ok=helper(o,"Throw");CHECK(ok==(mode!=1&&mode!=5));CHECK(drop_tails==tails+(mode==0?1:0));if(ok)CHECK(field(o,"handled",NULL).as.u64);watched=NULL;if(mode!=3&&mode!=4){nativeScriptItem_Detach(h);HSD_GObjFree(h);}if(recycled){HSD_GObjFree(recycled);recycled=NULL;}f->item_gobj=NULL;HSD_GObjFree(o);++cases;}
 // Actual interpreted Fighter hooks can fault without physical deletion.
 for(unsigned kind=0;kind<2;++kind){HSD_GObj* source_actor=actor();Fighter* data=source_actor->user_data;HSD_GObj* item_actor=NULL;
  field(source_actor,"fault",&(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1});data->x2227_b5=false;data->input.pressed_buttons=HSD_PAD_B;data->input.lstick[0].y=1;
  if(kind){item_actor=held(source_actor,false,It_Kind_S_Scope);data->take_dmg_cb=NULL;}
  CHECK(!helper(source_actor,kind?"Gun":"Special"));CHECK(nativeScriptFighter_Status(source_actor)==NativeScriptVM_Error);CHECK(!data->x2227_b5);if(kind)CHECK(!data->take_dmg_cb);CHECK(source_actor->user_data==data&&data->gobj==source_actor);
  if(item_actor)HSD_GObjFree(item_actor);HSD_GObjFree(source_actor);++cases;
 }
 // A fall callback may legitimately replace the current held slot before Drop.
 HSD_GObj* o=actor();HSD_GObj* old=held(o,false,It_Kind_Seak_Chain);HSD_GObj* next=held(o,true,0);Fighter* f=o->user_data;f->item_gobj=old;attributes.use_kind=1;f->input.pressed_buttons=HSD_PAD_A;f->input.held_buttons[0]=HSD_PAD_LR;parasol=1;watched=o;changed_held=next;mutation=9;trigger=1;CHECK(helper(o,"Throw"));CHECK(f->item_gobj==next&&field(next,"seen",NULL).as.i64==1);watched=NULL;HSD_GObjFree(old);HSD_GObjFree(next);f->item_gobj=NULL;HSD_GObjFree(o);++cases;
 // Replacement B must remain the retained callback target, including failure.
 o=actor();old=held(o,false,It_Kind_Seak_Chain);next=held(o,true,0);f=o->user_data;f->item_gobj=old;attributes.use_kind=1;f->input.pressed_buttons=HSD_PAD_A;f->input.held_buttons[0]=HSD_PAD_LR;
 field(next,"fault",&(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1});watched=o;changed_held=next;mutation=9;trigger=1;CHECK(!helper(o,"Throw"));CHECK(nativeScriptItem_Status(next)==NativeScriptVM_Error);
 watched=NULL;HSD_GObjFree(old);HSD_GObjFree(next);f->item_gobj=NULL;HSD_GObjFree(o);++cases;
 // An already-faulted replacement is chosen, but never admitted or executed.
 o=actor();old=held(o,false,It_Kind_Seak_Chain);next=held(o,true,0);f=o->user_data;f->item_gobj=old;attributes.use_kind=1;f->input.pressed_buttons=HSD_PAD_A;f->input.held_buttons[0]=HSD_PAD_LR;
 field(next,"fault",&(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1});NativeScriptValue hook_result;CHECK(nativeScriptItem_RunHook(next,NativeScriptItemHook_Dropped,NULL,0,&hook_result)==NativeScriptVM_Error);
 watched=o;changed_held=next;mutation=9;trigger=1;unsigned tails_before=drop_tails;CHECK(!helper(o,"Throw"));CHECK(drop_tails==tails_before);
 parasol=-1;watched=NULL;HSD_GObjFree(old);HSD_GObjFree(next);f->item_gobj=NULL;HSD_GObjFree(o);++cases;
}
static void negatives(HSD_GObj* o)
{
 NativeScriptRuntimeContext c=context(o);NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel,argument={.type=NativeScriptType_I32};
 CHECK(!nativeScriptBindingAirInputs_Invoke(&c,NativeScriptBinding_StateAirGun,&argument,1,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));c.source_class=NativeCallbackSource_Item;CHECK(!nativeScriptBindingAirInputs_Invoke(&c,NativeScriptBinding_StateAirGun,NULL,0,&out,&diagnostic)&&out.as.u64==99);c=context(o);++c.generation;CHECK(!nativeScriptBindingAirInputs_Invoke(&c,NativeScriptBinding_ControlsAerialAttackMotion,NULL,0,&out,&diagnostic)&&out.as.u64==99);c=context(o);p_ftCommonData=NULL;CHECK(!nativeScriptBindingAirInputs_Invoke(&c,NativeScriptBinding_InputCStickAttackEdge,NULL,0,&out,&diagnostic)&&out.as.u64==99);p_ftCommonData=&common;cases+=4;
}
int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 ftData_SpecialAirHi[Ft_Kind_CustomFirst]=raw_hi;ftData_SpecialAirLw[Ft_Kind_CustomFirst]=raw_lw;ftData_SpecialAirS[Ft_Kind_CustomFirst]=raw_side;ftData_SpecialAirN[Ft_Kind_CustomFirst]=raw_neutral;
 ftData_OnItemDrop[Ft_Kind_CustomFirst]=raw_holder_drop;
 HSD_GObj* o=actor();pure(o);specials(o);guns(o);throws(o);negatives(o);HSD_GObjFree(o);lifetime();CHECK(nativeGObjPool_RetireScene());
 printf("{\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
