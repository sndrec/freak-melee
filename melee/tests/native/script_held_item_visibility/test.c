#include "fixture.h"
#include <melee/ft/native_script_binding_visuals.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum {N=1024,I=4096,F=32,V=64};
static char source_bytes[4096];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
NativeSourcePackage observation_packages[3];
#define script_bank observation_packages[0].compiler.bank
static Fighter fighters[64],substitute;static unsigned allocated,checks,cases,pairs,vm_calls;
static MotionState motions[342];static FighterBone parts[MAX_FT_PARTS];static HSD_JObj joints[MAX_FT_PARTS];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static NativeScriptDiagnostic diagnostic;
static ftCommonData common;ftCommonData* p_ftCommonData=&common;
__typeof__(Fighter_SmashChargeShakeTable) Fighter_SmashChargeShakeTable;
#include "native_common.inc"
static const uint32_t control_ids[]={NativeScriptBinding_ControlsHorizontalSmashThreshold,NativeScriptBinding_ControlsHorizontalSmashWindow,NativeScriptBinding_ControlsHorizontalSmashExtension};
static const NativeFighterCommonAttribute oracle_ids[]={NativeFighterCommon_HorizontalSmashThreshold,NativeFighterCommon_HorizontalSmashWindow,NativeFighterCommon_HorizontalSmashExtension};
static const char* control_fields[]={"threshold","window","extension"};
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"observations %u case %u: %s (%s)\n",__LINE__,cases,#x,diagnostic.message);exit(1);}}while(0)
static void configure(void)
{
 script_bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},.declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static const char* source="fighter:\n    behavior independent\nstate bool visible\nstate float threshold\nstate float window\nstate float extension\nstate int zero\nfunc Observe():\n    state.visible = visuals.held_item_visible\nfunc ObserveControls():\n    state.threshold = controls.horizontal_smash_threshold\n    state.window = controls.horizontal_smash_window\n    state.extension = controls.horizontal_smash_extension\nfunc Fault():\n    state.zero = 1 / state.zero\naction Placeholder:\n    on tick:\n        pass\n";
static void prepare(void)
{
 configure();NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&script_bank,&request,&accepted,&diagnostic));CHECK(accepted==&script_bank);
 NativeSourcePackage* p=&observation_packages[0];CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&script_bank,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst;p->assets.facts.moves=layout;p->assets.facts.joints=MAX_FT_PARTS;p->assets.parts.parts_num=MAX_FT_PARTS;p->assets.parts.part_to_joint=p->assets.part_to_joint;p->assets.parts.joint_to_part=p->assets.joint_to_part;p->assets.motions=motions;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 for(unsigned n=0;n<MAX_FT_PARTS;++n)parts[n].joint=&joints[n];
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void remove_data(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static HSD_GObj* actor(bool owned)
{
 CHECK(allocated<64);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);Fighter* f=&fighters[allocated++];
 *f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Mario,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Mario,.x8_spawnNum=100+allocated,.ft_data=&observation_packages[0].assets.character.data,.parts=parts};
 GObj_InitUserData(o,4,remove_data,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t function(const char* name)
{for(unsigned n=0;n<script_bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&script_bank.tree,script_bank.declarations.functions[n].name,name))return n;CHECK(false);return 0;}
static NativeScriptValue field(HSD_GObj* o,const char* name)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));
 for(unsigned n=0;n<script_bank.declarations.value_count;++n)if(nativeScriptSource_SpanEquals(&script_bank.tree,script_bank.declarations.values[n].name,name)){
  NativeScriptValue r;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&r,&diagnostic));return r;}
 CHECK(false);return(NativeScriptValue){0};
}
static bool helper(HSD_GObj* o,const char* name)
{
 NativeScriptValue r={.type=NativeScriptType_U64,.as.u64=99};++vm_calls;NativeScriptVMStatus s=nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,function(name),NULL,0,&r,&diagnostic);
 CHECK(s==NativeScriptVM_Ok?r.type==NativeScriptType_Void:r.type==NativeScriptType_U64&&r.as.u64==99);return s==NativeScriptVM_Ok;
}

void ftCommon_8007F5CC(HSD_GObj*,bool);
static void positives(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);
 for(unsigned held=0;held<2;++held)for(unsigned visible=0;visible<2;++visible)for(unsigned rig=0;rig<2;++rig){
  // Real common visibility setter computes the native requested bit. With no
  // item or an unchanged requested bit its callback branch is not entered.
  f->item_gobj=held?o:NULL;f->x221E_b3=visible;f->parts=rig?parts:NULL;o->hsd_obj=rig?&joints[0]:NULL;
  static Fighter native,before;native=*f;HSD_GObj native_object=*o;native_object.user_data=&native;
  ftCommon_8007F5CC(&native_object,visible!=0);before=*f;
  NativeScriptValue got={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_VisualsHeldItemVisible,NULL,0,&got,&diagnostic));
  CHECK(got.type==NativeScriptType_Bool&&got.as.u64==native.x221E_b3);CHECK(!memcmp(f,&before,sizeof(*f)));++pairs;
  CHECK(helper(o,"Observe"));got=field(o,"visible");CHECK(got.type==NativeScriptType_Bool&&got.as.u64==native.x221E_b3);CHECK(!memcmp(f,&before,sizeof(*f)));++pairs;++cases;
 }
 for(unsigned visible=0;visible<2;++visible){f->item_gobj=NULL;f->x221E_b3=!visible;ftCommon_8007F5CC(o,visible!=0);static Fighter before;before=*f;CHECK(helper(o,"Observe"));CHECK(field(o,"visible").as.u64==visible&&!memcmp(f,&before,sizeof(*f)));++cases;++pairs;}
}

static void control_positives(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);
 const uint32_t bits[]={0,0x80000000,0x3f800000,0xc0200000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,1,0x80000001,0x7f7fffff};
 const int32_t windows[]={0,1,-1,INT32_MIN,INT32_MAX,16777215,16777216,16777217};
 for(unsigned n=0;n<3;++n){const NativeScriptBindingDescriptor*d=nativeScriptBindings_Descriptor(control_ids[n]);CHECK(d&&d->result_type==NativeScriptType_F32&&d->contexts==NativeScriptBindings_Fighter&&(d->flags&NativeScriptFlag_SourceOnly)&&!(d->flags&NativeScriptFlag_Writable));}
 for(unsigned b=0;b<sizeof(bits)/sizeof(bits[0]);++b)for(unsigned w=0;w<sizeof(windows)/sizeof(windows[0]);++w){
  memcpy(&common.dash_smash_stick_threshold,&bits[b],4);memcpy(&common.x44,&bits[(b+3)%12],4);common.dash_smash_window=windows[w];
  static Fighter before;static ftCommonData unchanged;before=*f;unchanged=common;
  float expected[3];for(unsigned n=0;n<3;++n){expected[n]=common_attribute(oracle_ids[n]);NativeScriptValue r;
   CHECK(nativeScriptBindings_Read(&c,control_ids[n],NULL,0,&r,&diagnostic));CHECK(r.type==NativeScriptType_F32&&!memcmp(&r.as.f32,&expected[n],4));++pairs;
  }
  CHECK(helper(o,"ObserveControls"));for(unsigned n=0;n<3;++n){NativeScriptValue r=field(o,control_fields[n]);CHECK(r.type==NativeScriptType_F32&&!memcmp(&r.as.f32,&expected[n],4));++pairs;}
  CHECK(!memcmp(f,&before,sizeof(*f))&&!memcmp(&common,&unchanged,sizeof(common)));++cases;
 }
 for(unsigned n=0;n<3;++n){NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=1};CHECK(!nativeScriptBindings_Write(&c,control_ids[n],NULL,0,&value,&diagnostic));++cases;}
 p_ftCommonData=NULL;
 for(unsigned n=0;n<3;++n){NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},r=sentinel;CHECK(!nativeScriptBindings_Read(&c,control_ids[n],NULL,0,&r,&diagnostic)&&!memcmp(&r,&sentinel,sizeof(r)));++cases;}
 p_ftCommonData=&common;
}
static void rejected(NativeScriptRuntimeContext* c)
{
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=0xabcdef},r=sentinel;
 CHECK(!nativeScriptBindingVisuals_Read(c,NativeScriptBinding_VisualsHeldItemVisible,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindings_Read(c,NativeScriptBinding_VisualsHeldItemVisible,NULL,0,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 for(unsigned n=0;n<3;++n){CHECK(!nativeScriptBindingCommonProperties_Read(c,control_ids[n],&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));CHECK(!nativeScriptBindings_Read(c,control_ids[n],NULL,0,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));}++cases;
}
static void negatives(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},r=sentinel,arg={.type=NativeScriptType_Bool,.as.u64=1};static Fighter before;before=*f;
 CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_VisualsHeldItemVisible,NULL,0,&arg,&diagnostic)&&!memcmp(f,&before,sizeof(*f)));++cases;
 CHECK(!nativeScriptBindingVisuals_Read(&c,NativeScriptBinding_Frame,&r,&diagnostic)&&r.as.u64==99);++cases;
 CHECK(!nativeScriptBindingVisuals_Read(&c,NativeScriptBinding_VisualsHeldItemVisible,NULL,&diagnostic));++cases;
 CHECK(!nativeScriptBindings_Read(&c,NativeScriptBinding_VisualsHeldItemVisible,&arg,1,&r,&diagnostic)&&r.as.u64==99);++cases;
 rejected(NULL);c.source_class=NativeCallbackSource_Item;rejected(&c);c.source_class=NativeCallbackSource_ModelEffect;rejected(&c);c=context(o);++c.generation;rejected(&c);c=context(o);++c.scene_epoch;rejected(&c);c=context(o);
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));NativeScriptStorageInstance saved=*view.storage;
 NativeScriptStorageLayout other_layout=script_bank.storage_layout;NativeScriptStorageParameters other_parameters=script_bank.storage_parameters;
 view.storage->layout=&other_layout;rejected(&c);*view.storage=saved;
 view.storage->parameters=&other_parameters;rejected(&c);*view.storage=saved;
 view.storage->live=false;rejected(&c);*view.storage=saved;
 ++view.storage->owner;rejected(&c);*view.storage=saved;
 ++view.storage->generation;rejected(&c);*view.storage=saved;
 for(unsigned mode=0;mode<9;++mode){uint64_t epoch=nativeGObjPool.lifetime;NativeSourcePackage* p=&observation_packages[0];bool ready=p->ready,assets=p->assets.ready;NativeScriptFighterProgram* runtime=(NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind);const NativeScriptCompileBank* selected=runtime->bank;
  switch(mode){case 0:p->ready=false;break;case 1:p->assets.ready=false;break;case 2:f->behavior_kind=Ft_Kind_Mario;break;case 3:f->ft_data=NULL;break;case 4:gFtDataList[f->kind]=NULL;break;case 5:ftData_CharacterStateTables[f->kind]=NULL;break;case 6:runtime->bank=NULL;break;case 7:++nativeGObjPool.lifetime;break;case 8:script_bank.valid=false;break;}
  rejected(&c);p->ready=ready;p->assets.ready=assets;runtime->bank=selected;script_bank.valid=true;nativeGObjPool.lifetime=epoch;f->behavior_kind=before.behavior_kind;f->ft_data=before.ft_data;gFtDataList[f->kind]=f->ft_data;ftData_CharacterStateTables[f->kind]=motions;
 }
 NativeFighterLifetime retained;CHECK(nativeFighterLifetime_Capture(f,&retained));substitute=*f;o->user_data=&substitute;CHECK(!nativeFighterLifetime_Current(&retained));substitute.gobj=NULL;rejected(&c);o->user_data=f;
 o->classifier=HSD_GOBJ_CLASS_ITEM;rejected(&c);o->classifier=HSD_GOBJ_CLASS_FIGHTER;f->gobj=NULL;rejected(&c);f->gobj=o;
 CHECK(!helper(o,"Fault"));rejected(&c);CHECK(!memcmp(f,&before,sizeof(*f)));
}
static void compile_negatives(void)
{
 const char* invalid[]={"func Invalid():\n    visuals.held_item_visible = true\n","func Invalid():\n    visuals.held_item_visible = 1\n","article Probe states 1:\n    state bool seen\n    on created:\n        item.state.seen = visuals.held_item_visible\n    state 0 anim -1:\n","func Invalid():\n    state.visible = visuals.held_item_visible(1)\n","func Invalid():\n    controls.horizontal_smash_threshold = 1.0\n","func Invalid():\n    controls.horizontal_smash_window = 1.0\n","func Invalid():\n    controls.horizontal_smash_extension = 1.0\n","article Probe states 1:\n    state float seen\n    on created:\n        item.state.seen = controls.horizontal_smash_threshold\n    state 0 anim -1:\n","article Probe states 1:\n    state float seen\n    on created:\n        item.state.seen = controls.horizontal_smash_window\n    state 0 anim -1:\n","article Probe states 1:\n    state float seen\n    on created:\n        item.state.seen = controls.horizontal_smash_extension\n    state 0 anim -1:\n"};
 for(unsigned n=0;n<sizeof(invalid)/sizeof(invalid[0]);++n){configure();NativeScriptCompileBank scratch=script_bank;NativeScriptCompileRequest request={.source=invalid[n],.source_bytes=(uint32_t)strlen(invalid[n])};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch,&request,&accepted,&diagnostic));CHECK(!scratch.valid);++cases;}
}
static unsigned native_callback_calls;
HSD_GObjEvent ftData_OnItemVisible[Ft_Kind_RuntimeCapacity];
void ftCommon_8007F578(HSD_GObj* o){(void)o;++native_callback_calls;abort();}
#include "native_visibility.inc"
int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(NativeScriptBinding_VisualsHeldItemVisible);CHECK(d&&d->result_type==NativeScriptType_Bool&&d->contexts==NativeScriptBindings_Fighter&&(d->flags&NativeScriptFlag_SourceOnly)&&!(d->flags&NativeScriptFlag_Writable));
 HSD_GObj* owned=actor(true);positives(owned);control_positives(owned);negatives(owned);HSD_GObjFree(owned);
 HSD_GObj* raw=actor(false);NativeScriptRuntimeContext c=context(raw);rejected(&c);HSD_GObjFree(raw);
 HSD_GObj* old=actor(true);c=context(old);HSD_GObjFree(old);HSD_GObj* replacement=actor(true);CHECK(replacement==old);rejected(&c);HSD_GObjFree(replacement);
 HSD_GObj* detached=actor(true);c=context(detached);nativeScriptFighter_Detach(detached);rejected(&c);HSD_GObjFree(detached);
 CHECK(native_callback_calls==0);CHECK(nativeGObjPool_RetireScene());compile_negatives();printf("{\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
