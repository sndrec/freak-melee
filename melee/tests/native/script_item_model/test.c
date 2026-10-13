#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/it/native_article_model.h>
#include <melee/it/types.h>
#include <melee/it/inlines.h>
#include <melee/it/itanimlist.h>
#include <melee/it/ithitbox.h>
#include <melee/it/itcoll.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_26B1.h>
#include <melee/ft/fighter.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/lb/lbcommand.h>
#include <melee/lb/native_model_joint.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <placeholder.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum{Bytes=16384,Nodes=512,Expr=2048,Functions=32,Values=64,Instructions=4096};
typedef struct Buffers {
 char source[Bytes];NativeScriptSourceNode nodes[Nodes];NativeScriptIRNode expressions[Expr];uint32_t arguments[Expr];
 NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[Values];NativeScriptValueDeclaration values[Values];NativeScriptLocal locals[32];
 NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];NativeScriptExecutionFunction execution[Functions];NativeScriptCompileFunctionScratch graph[Functions];
 NativeScriptStorageField fields[Values];NativeScriptStorageOwner owners[16];NativeScriptParameterSlot slots[Values];
} Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];static NativeScriptDiagnostic diagnostic;static unsigned checks,stages,retire_stage;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"Item model %u: %s (%u:%u %s)\n",__LINE__,#x,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}while(0)
static NativeScriptItemProgram program;static NativeScriptStorageInstance storage;static NativeScriptItemModelSelector selectors[128];
static Article article;static ItemModelDesc model;static HSD_Joint descriptor[4];static HSD_JObj joints[4];
static HSD_ClassInfo subclass,wrong_class;HSD_JObjInfo hsdJObj;static Item item;static HSD_GObj* actor;
// Primary-only fixture: secondary scene operations are explicitly unavailable.
bool nativeArticleModelScene_CaptureSlot(Item_GObj* o,unsigned slot,NativeArticleModelSceneSlot* out){(void)o;(void)slot;(void)out;abort();}
bool nativeArticleModelScene_SlotCurrent(const NativeArticleModelSceneSlot* s){(void)s;abort();}
bool nativeArticleModelScene_Load(Item_GObj* o,unsigned slot,unsigned root){(void)o;(void)slot;(void)root;abort();}
bool nativeArticleModelScene_BeginMatrices(Item_GObj* o,unsigned slot){(void)o;(void)slot;abort();}
bool nativeArticleModelScene_EndMatrices(void){abort();}
void nativeArticleModel_Destroy(Item_GObj* o,unsigned slot){(void)o;(void)slot;abort();}
static struct ItemStateArray appearance_states;static ItemStateDesc appearance_descriptors[2];
static HSD_AnimJoint appearance_animation[2];static HSD_MatAnimJoint appearance_material[2];static HSD_ShapeAnimJoint appearance_shape[2];
static bool appearance_mode;static unsigned appearance_events[8],appearance_event_count,mutation;
static float appearance_frame=7;
static uint64_t state_revision=1;
NativeMatchPool nativeGObjPool;static HSD_GObj payload[2];static NativeMatchPoolSlot pool_slots[2];static uint32_t free_order[2];
u8 HSD_GObj_JObjKind=3;
static HSD_GObj* lists[32];HSD_GObj** HSD_GObjPLinkHead=lists;
static ItemAttr scale_attributes;
static unsigned scale_pairs,scale_negatives,projection_pairs;
static unsigned model_control_pairs,model_control_negatives,world_pairs,world_negatives;
static unsigned dynamic_pairs,matrix_pairs,joint_model_negatives;
static bool world_mode;static unsigned world_mutation;
static bool model_control_observation;static unsigned model_control_mutation;
static bool scaling_observation;static unsigned scale_mutation;
static Fighter damage_owner;static HSD_GObj damage_actor;struct Fighter_804D6524_t* Fighter_804D6524;
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object){return object==actor?&program:NULL;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){return author==program.author_kind&&owner==program.owner?&program:NULL;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* object){return object==actor&&storage.live&&!item.native_removal_in_progress?NativeScriptVM_Ok:NativeScriptVM_SourceRetired;}
uint64_t nativeScriptItem_Revision(Item_GObj* object){return object==actor?state_revision:0;}
bool nativeScriptItem_View(Item_GObj* object,uint32_t generation,NativeScriptReceiverView* view){if(object!=actor||generation!=17||!storage.live)return false;*view=(NativeScriptReceiverView){.program=&banks[0].declarations,.storage=&storage};return true;}
Fighter_GObj* nativeScriptItem_Creator(Item_GObj* object){(void)object;return NULL;}
Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj* object){(void)object;return NULL;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&nativeCallbackSource_Alive(c->source,c->generation,c->source_class);}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* v,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic* d){return nativeScriptBindingItemModel_Read(c,semantic,v,n,out,d);}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* out,NativeScriptDiagnostic* d){return nativeScriptBindingItemModel_Write(c,semantic,v,n,out,d);}
static void stage(void){++stages;if(stages!=retire_stage)return;
 if(!appearance_mode||mutation==0)++item.x1C;
 else if(mutation==1)storage.live=false;
 else if(mutation==2)actor->hsd_obj=&joints[1];
 else if(mutation==3)actor->user_data=&damage_owner;
 else if(mutation==4){++state_revision;item.msid=9;item.x524_cmd.u=(union CmdUnion*)&appearance_shape[1];item.xD0_itemStateDesc=&appearance_descriptors[1];}
 else if(mutation==5)appearance_states.x0_itemStateDesc=&appearance_descriptors[1];
 else if(mutation==6)program.identity++;
 else if(mutation==7){NativeMatchPoolReference r;CHECK(nativeMatchPool_Reference(&nativeGObjPool,actor,&r));CHECK(nativeMatchPool_Free(&nativeGObjPool,r));HSD_GObj* replacement=nativeMatchPool_Allocate(&nativeGObjPool,&r);CHECK(replacement==actor);replacement->classifier=HSD_GOBJ_CLASS_ITEM;replacement->obj_kind=HSD_GObj_JObjKind;replacement->hsd_obj=&joints[0];replacement->user_data=&item;}
}
static void appearance_event(unsigned event){CHECK(appearance_event_count<8);appearance_events[appearance_event_count++]=event;stage();}
void HSD_JObjRemoveAnimAll(HSD_JObj* root){CHECK(root==&joints[0]&&appearance_mode);root->aobj=NULL;appearance_event(1);}
void HSD_JObjAddAnimAll(HSD_JObj* root,HSD_AnimJoint* anim,HSD_MatAnimJoint* material,HSD_ShapeAnimJoint* shape){CHECK(root==&joints[0]&&appearance_mode);unsigned index=(unsigned)(anim-appearance_animation);CHECK(index<2&&material==&appearance_material[index]&&shape==&appearance_shape[index]);root->aobj=(HSD_AObj*)anim;appearance_event(2);}
static void model_control_observe(void);
static void model_rate_observe(HSD_JObj* root,float rate){CHECK(root==&joints[0]);if(model_control_observation)model_control_observe();else{CHECK(rate==2);if(appearance_mode)appearance_event(3);else stage();}}
void HSD_JObjReqAnimAll(HSD_JObj* root,f32 frame){CHECK(root==&joints[0]);if(appearance_mode){CHECK(!memcmp(&frame,&appearance_frame,sizeof(frame)));root->rotate.w=frame;appearance_event(4);}else{CHECK(frame==7);stage();}}
void HSD_JObjAnimAll(HSD_JObj* root){CHECK(root==&joints[0]);if(appearance_mode){root->child->rotate.z=root->rotate.w;appearance_event(5);}else stage();}
f32 lbGetJObjCurrFrame(HSD_JObj* root){CHECK(root==&joints[0]);stage();return 7.5f;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint){joint->flags|=JOBJ_MTX_DIRTY;}
void(HSD_JObjSetMtxDirty)(HSD_JObj* joint){HSD_JObjSetMtxDirtySub(joint);}
void __assert(char* file,u32 line,char* text){fprintf(stderr,"native assert %s:%lu %s\n",file,line,text);abort();}
void lbBgFlash_80021C48(u32 a,u32 b){(void)a;(void)b;abort();}
float ftCo_CalcYScaledKnockback(float a,float b,float c){(void)a;(void)b;(void)c;abort();}
float ft_80089228(Fighter* owner,int attack,int instance,f32 value){CHECK(owner==&damage_owner&&attack==3&&instance==4&&item.x5CC_currentAnimFrame==7.5f);stage();return value+1;}
void HSD_JObjSetupMatrixSub(HSD_JObj* node){
 CHECK(world_mode);for(unsigned i=0;i<3;++i)node->mtx[i][3]=(&node->translate.x)[i]+100*(float)(i+1);
 node->flags&=~JOBJ_MTX_DIRTY;
 if(world_mutation==1)++item.x1C;
 else if(world_mutation==2)storage.live=false;
 else if(world_mutation==3)actor->hsd_obj=&joints[1];
 else if(world_mutation==4)joints[2].next=NULL;
 else if(world_mutation==5)HSD_CLASS_METHOD(node)=&wrong_class;
 else if(world_mutation==6)program.identity++;
 else if(world_mutation==7)++banks[0].storage_layout.identity;
 else if(world_mutation==8)actor->obj_kind=HSD_GOBJ_OBJ_NONE;
}
void C_MTXMultVec(Mtx matrix,Vec3* input,Vec3* output){(void)matrix;(void)input;(void)output;abort();}
static void unavailable_command(Item_GObj* object,CommandInfo* cmd){(void)object;(void)cmd;abort();}
ItCmd it_803F22A8[16]={unavailable_command,unavailable_command,it_80279544,it_802795EC,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command,unavailable_command};
#include "native_bodies.inc"
#include "animation_bodies.inc"
#include "appearance_bodies.inc"
#include "model_control_bodies.inc"
#include "joint_model_bodies.inc"
static void scale_observe(void);
#include "scale_bodies.inc"
static void setup(unsigned index){Buffers* b=&buffers[index];banks[index]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=Bytes,.tree={.nodes=b->nodes,.capacity=Nodes},
 .expressions={.nodes=b->expressions,.node_capacity=Expr,.arguments=b->arguments,.argument_capacity=Expr},.declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=Values,.values=b->values,.value_capacity=Values},
 .locals=b->locals,.local_capacity=32,.instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,.execution_functions=b->execution,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
 .storage_layout={.fields=b->fields,.field_capacity=Values,.owners=b->owners,.owner_capacity=16},.parameter_slots=b->slots,.parameter_slot_capacity=Values};}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d){(void)unused;const NativeScriptSourceNode* n=&tree->nodes[at];
 if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 1");
 if(n->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,n->value,"0 anim -1");
 if(n->kind==NativeScriptSource_Function&&n->parent<tree->count&&tree->nodes[n->parent].kind==NativeScriptSource_Article)return true;
 return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);}
static uint32_t named(const char* name){for(unsigned f=0;f<banks[0].declarations.function_count;++f)if(nativeScriptSource_SpanEquals(&banks[0].tree,banks[0].declarations.functions[f].name,name))return f;CHECK(false);return 0;}
static bool invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* a,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d){(void)v;CHECK(n==0);return nativeScriptBindings_Invoke(c,s,v,n,a,count,out,d);}
static NativeScriptRuntimeContext context(void){return(NativeScriptRuntimeContext){.source=actor,.source_class=NativeCallbackSource_Item,.generation=17,.scene_epoch=64};}
static NativeScriptVMStatus execute(const char* name,NativeScriptValue* out){NativeScriptValue registers[256];NativeScriptVMFrame frames[16];NativeScriptVM vm={.registers=registers,.register_capacity=256,.frames=frames,.frame_capacity=16};NativeScriptRuntimeContext c=context();NativeScriptVMHost host={.source_alive=nativeScriptBindings_SourceAlive,.read=nativeScriptBindings_Read,.write=nativeScriptBindings_Write,.invoke=invoke};
 NativeScriptVMStatus status=nativeScriptVM_Run(&vm,&banks[0].code,&host,&c,named(name),NULL,0,out);diagnostic=vm.diagnostic;return status;}
static void run(const char* name){NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(execute(name,&out)==NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_F32&&out.as.f32==3);}
static uint64_t hash(const void* p,size_t n){const unsigned char* bytes=p;uint64_t h=UINT64_C(1469598103934665603);while(n--){h^=*bytes++;h*=UINT64_C(1099511628211);}return h;}
static void negative(const char* body,const NativeScriptCompileBank** accepted){char text[Bytes];snprintf(text,sizeof(text),"fighter:\n    behavior independent\narticle Probe states 1:\n    func Bad():\n        %s\n    state 0 anim -1:\n",body);uint64_t before=hash(&buffers[0],sizeof(buffers[0]));NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=metadata};CHECK(!nativeScriptCompile_Build(&banks[1],&request,accepted,&diagnostic));CHECK(*accepted==&banks[0]&&hash(&buffers[0],sizeof(buffers[0]))==before);}
static NativeScriptValue recipe(const NativeScriptItemModelSelector* selector,NativeScriptValue args[2]){args[0]=(NativeScriptValue){.type=NativeScriptType_I32};args[selector->argument_count-1]=(NativeScriptValue){.type=NativeScriptType_Text,.as.text={selector->path_offset,selector->path_length}};return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};}
static void rejected_read(NativeScriptRuntimeContext* c){NativeScriptValue args[2],out=recipe(&selectors[0],args);CHECK(!nativeScriptBindingItemModel_Read(c,selectors[0].semantic,args,selectors[0].argument_count,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);}
static union CmdUnion commands[3];
static void command_seed(unsigned index,unsigned damage,unsigned size,bool enabled,bool owned){
 commands[0]=(union CmdUnion){.set_hitbox_scale={.opcode=13,.idx=index,.value=size}};
 commands[1]=(union CmdUnion){.set_hitbox_damage={.opcdoe=12,.idx=index,.value=damage}};commands[2]=(union CmdUnion){0};
 item.x524_cmd=(CommandInfo){.u=commands,.timer=0};item.x5CC_currentAnimFrame=1;item.xDBC_itcmd_var4_word=UINT32_MAX;
 item.xC3C=1.5f;item.xC40=2;item.scl=2;item.x5D4_hitboxes[index].hit.state=enabled?HitCapsule_Enabled:HitCapsule_Disabled;item.x5D4_hitboxes[index].hit.damage=-7;
 damage_owner.x34_scale.y=1;damage_actor.classifier=HSD_GOBJ_CLASS_FIGHTER;damage_actor.user_data=&damage_owner;item.owner=owned?&damage_actor:NULL;item.xD88_attackID=3;item.xD8C_attack_instance=4;
 stages=0;item.x1C=17;
}
static unsigned appearance_pairs,appearance_negatives;
static void appearance_seed(bool source_joint){
 stages=0;appearance_event_count=0;state_revision=1;item.x1C=17;storage.live=true;program.identity=123;
 actor->classifier=HSD_GOBJ_CLASS_ITEM;actor->obj_kind=HSD_GObj_JObjKind;actor->hsd_obj=&joints[0];actor->user_data=&item;
 article.xC_itemStates=&appearance_states;appearance_states=(struct ItemStateArray){.x0_itemStateDesc=appearance_descriptors,.count=2};
 for(unsigned i=0;i<2;++i)appearance_descriptors[i]=(ItemStateDesc){.x0_anim_joint=&appearance_animation[i],.x4_matanim_joint=&appearance_material[i],.x8_parameters=&appearance_shape[i],.xC_script=commands};
 item.xC8_joint=source_joint?&descriptor[0]:NULL;item.xD0_itemStateDesc=NULL;item.msid=3;item.x5CC_currentAnimFrame=91;
 item.x524_cmd=(CommandInfo){.u=commands,.timer=17};
 for(unsigned i=0;i<4;++i){descriptor[i].rotation=(Vec3){(float)i,2,3};descriptor[i].scale=(Vec3){4,5,6};descriptor[i].position=(Vec3){7,8,9};joints[i].rotate=(Quaternion){-7,-8,-9,-10};joints[i].scale=(Vec3){-1,-2,-3};joints[i].translate=(Vec3){-4,-5,-6};joints[i].flags=JOBJ_USE_QUATERNION;joints[i].aobj=NULL;}
}
static void appearance_tests(NativeScriptRuntimeContext* c){
 appearance_mode=true;retire_stage=0;
 float frames[]={7,-0.0f,-3.5f,INFINITY,NAN};
 for(unsigned f=0;f<5;++f)for(unsigned has_joint=0;has_joint<2;++has_joint)for(unsigned index=0;index<2;++index){
  appearance_frame=frames[f];
  appearance_seed(has_joint!=0);Item before=item;HSD_JObj before_joints[4];memcpy(before_joints,joints,sizeof(joints));
  it_80273670(actor,(int)index,appearance_frame);Item expected=item;HSD_JObj expected_joints[4];memcpy(expected_joints,joints,sizeof(joints));
  unsigned expected_events[8];memcpy(expected_events,appearance_events,sizeof(expected_events));unsigned count=appearance_event_count;
  item=before;memcpy(joints,before_joints,sizeof(joints));stages=0;appearance_event_count=0;
  NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=index},{.type=NativeScriptType_F32,.as.f32=appearance_frame}},out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemSetAppearance,NULL,0,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);
  CHECK(!memcmp(&expected,&item,sizeof(item))&&!memcmp(expected_joints,joints,sizeof(joints)));
  CHECK(count==6&&appearance_event_count==count&&!memcmp(expected_events,appearance_events,count*sizeof(*expected_events)));
  CHECK(item.xD0_itemStateDesc==&appearance_descriptors[index]&&item.x524_cmd.u==NULL&&item.x524_cmd.timer==17&&item.x5CC_currentAnimFrame==91&&item.msid==3&&state_revision==1);++appearance_pairs;
 }
 appearance_frame=7;
 int invalid[]={-1,2,INT32_MAX};
 for(unsigned i=0;i<3;++i){appearance_seed(true);Item before=item;HSD_JObj saved[4];memcpy(saved,joints,sizeof(joints));NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=invalid[i]},{.type=NativeScriptType_F32,.as.f32=7}},out={.type=NativeScriptType_U64,.as.u64=99};CHECK(nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemSetAppearance,NULL,0,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void&&stages==0&&!memcmp(&before,&item,sizeof(item))&&!memcmp(saved,joints,sizeof(joints)));++appearance_negatives;}
 for(mutation=0;mutation<8;++mutation)for(retire_stage=1;retire_stage<=6;++retire_stage){
  appearance_seed(true);NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_F32,.as.f32=7}},out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemSetAppearance,NULL,0,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&stages==retire_stage);
  CHECK(item.x524_cmd.u==(mutation==4?(union CmdUnion*)&appearance_shape[1]:commands));++appearance_negatives;
 }
 retire_stage=0;mutation=0;appearance_seed(true);
 NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_F32,.as.f32=7}},out={.type=NativeScriptType_U64,.as.u64=99};
 article.xC_itemStates=NULL;CHECK(!nativeScriptBindingItemModel_Invoke(c,NativeScriptBinding_ItemSetAppearance,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&stages==0);article.xC_itemStates=&appearance_states;
 appearance_states.x0_itemStateDesc=NULL;CHECK(!nativeScriptBindingItemModel_Invoke(c,NativeScriptBinding_ItemSetAppearance,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&stages==0);appearance_states.x0_itemStateDesc=appearance_descriptors;
 args[0].as.i64=INT64_MAX;CHECK(!nativeScriptBindingItemModel_Invoke(c,NativeScriptBinding_ItemSetAppearance,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&stages==0);args[0].as.i64=0;
 args[0].type=NativeScriptType_F32;CHECK(!nativeScriptBindingItemModel_Invoke(c,NativeScriptBinding_ItemSetAppearance,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&stages==0);appearance_negatives+=4;
 appearance_seed(true);run("Bake");CHECK(stages==6&&item.xD0_itemStateDesc==&appearance_descriptors[1]&&item.x524_cmd.u==NULL&&item.x5CC_currentAnimFrame==91&&item.msid==3);
 appearance_seed(true);run("InvalidAppearance");CHECK(stages==0&&item.x524_cmd.u==commands&&item.xD0_itemStateDesc==NULL);
 for(unsigned logical=0;logical<2;++logical){appearance_seed(true);mutation=logical;retire_stage=5;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};NativeScriptVMStatus status=execute("Bake",&out);CHECK(status==(logical?NativeScriptVM_Error:NativeScriptVM_SourceRetired));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&stages==5&&item.x524_cmd.u==commands);++appearance_negatives;}
 retire_stage=0;mutation=0;
 appearance_mode=false;appearance_seed(true);
}
static HSD_GObj* scale_owner_object;
static void scale_observe(void){
 switch(scale_mutation){
 case 1:++item.x1C;break;
 case 2:storage.live=false;break;
 case 3:actor->hsd_obj=&joints[1];break;
 case 4:actor->user_data=&damage_owner;break;
 case 5:++program.identity;break;
 case 6:++damage_owner.x8_spawnNum;break;
 case 7:item.owner=NULL;break;
 case 8:scale_owner_object->classifier=HSD_GOBJ_CLASS_ITEM;break;
 case 9:model.xC_bit_field^=64;break;
 case 10:item.xCC_item_attr=NULL;break;
 case 11:{NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,scale_owner_object,&ref));CHECK(nativeMatchPool_Free(&nativeGObjPool,ref));HSD_GObj* replacement=nativeMatchPool_Allocate(&nativeGObjPool,&ref);CHECK(replacement==scale_owner_object);replacement->classifier=HSD_GOBJ_CLASS_FIGHTER;replacement->user_data=&damage_owner;break;}
 }
}
static void scale_seed(void){
 item.x1C=17;storage.live=true;program.identity=123;
 actor->classifier=HSD_GOBJ_CLASS_ITEM;actor->obj_kind=HSD_GObj_JObjKind;actor->hsd_obj=&joints[0];actor->user_data=&item;
 article.x10_modelDesc=&model;article.x0_common_attr=&scale_attributes;item.xCC_item_attr=&scale_attributes;scale_attributes.x60_scale=1.375f;
 item.owner=scale_owner_object;damage_owner=(Fighter){.gobj=scale_owner_object,.kind=Ft_Kind_Zelda,.x8_spawnNum=UINT32_C(0x80000001),.x34_scale={1,0.75f,1}};
 scale_owner_object->classifier=HSD_GOBJ_CLASS_FIGHTER;scale_owner_object->user_data=&damage_owner;lists[HSD_GOBJ_PLINK_FIGHTER]=scale_owner_object;
 item.scl=3;item.x3C=2.25f;item.xD3C_spinSpeed=9;
 item.xBCC_unk=(Vec2){2,-3};item.xBD4_grabRange=(Vec2){4,-5};
 item.xBEC.top=1;item.xBEC.bottom=2;item.xBEC.left=3;item.xBEC.right=4;
 item.xBDC.top=5;item.xBDC.bottom=6;item.xBDC.left=7;item.xBDC.right=8;
 for(unsigned i=0;i<4;++i){joints[i].flags=0;joints[i].rotate=(Quaternion){1,2,3,4};joints[i].scale=(Vec3){4,5,6};joints[i].child=joints[i].next=NULL;HSD_CLASS_METHOD(&joints[i])=&subclass;item.x5D4_hitboxes[i].hit.state=i&1?HitCapsule_Disabled:HitCapsule_Enabled;item.x5D4_hitboxes[i].hit.scale=-3;}
 joints[0].child=&joints[1];joints[1].child=&joints[2];joints[2].next=&joints[3];joints[1].parent=&joints[0];joints[2].parent=joints[3].parent=&joints[1];model.xC_bit_field=0;
 scaling_observation=false;scale_mutation=0;
}
static void scale_tests(NativeScriptRuntimeContext* c){
 NativeMatchPoolReference ref;scale_owner_object=nativeMatchPool_Allocate(&nativeGObjPool,&ref);CHECK(scale_owner_object&&scale_owner_object!=actor);
 unsigned ids[]={NativeScriptBinding_ItemScaleByAttribute,NativeScriptBinding_ItemSetInstanceScale,NativeScriptBinding_ItemInheritOwnerScale};
 float inputs[]={-2,-0.0f,0,1.75f,INFINITY,NAN};
 for(unsigned op=0;op<3;++op)for(unsigned input=0;input<ARRAY_SIZE(inputs);++input){
  scale_seed();if(op==2)damage_owner.x34_scale.y=inputs[input];
  Item before=item;HSD_JObj before_joints[4];memcpy(before_joints,joints,sizeof(joints));
  if(op==0)it_8026BAE8(actor,inputs[input]);else if(op==1)it_80274484(actor,&joints[0],inputs[input]);else it_80274594(actor);
  Item expected=item;HSD_JObj expected_joints[4];memcpy(expected_joints,joints,sizeof(joints));
  item=before;memcpy(joints,before_joints,sizeof(joints));NativeScriptValue arg={.type=NativeScriptType_F32,.as.f32=inputs[input]},out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Invoke(c,ids[op],NULL,0,op==2?NULL:&arg,op==2?0:1,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);
  CHECK(!memcmp(&expected,&item,sizeof(item))&&!memcmp(expected_joints,joints,sizeof(joints)));++scale_pairs;
 }
 for(unsigned depth=0;depth<4;++depth)for(unsigned axis=0;axis<4;++axis){
  scale_seed();joints[2].child=&joints[3];joints[2].next=NULL;model.xC_bit_field=(uint8_t)(depth<<6);item.xDC8_word.flags.x17=axis;
  Item before=item;HSD_JObj saved[4];memcpy(saved,joints,sizeof(joints));it_80274740(actor);Item expected=item;HSD_JObj expected_joints[4];memcpy(expected_joints,joints,sizeof(joints));item=before;memcpy(joints,saved,sizeof(joints));NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemResetSpin,NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void&&!memcmp(&expected,&item,sizeof(item))&&!memcmp(expected_joints,joints,sizeof(joints)));++scale_pairs;
 }
 // Native functions contain no authored callbacks. Injection below occurs
 // only after the complete extracted native body, before returning to binding.
 for(unsigned op=0;op<4;++op)for(unsigned mode=1;mode<=5;++mode){
  scale_seed();scaling_observation=true;scale_mutation=mode;NativeScriptValue arg={.type=NativeScriptType_F32,.as.f32=2},out={.type=NativeScriptType_U64,.as.u64=99};unsigned id=op==3?NativeScriptBinding_ItemResetSpin:ids[op];
  CHECK(!nativeScriptBindings_Invoke(c,id,NULL,0,op>=2?NULL:&arg,op>=2?0:1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++scale_negatives;
 }
 for(unsigned slot=0;slot<4;++slot){unsigned mode=slot==3?11:6+slot;scale_seed();scaling_observation=true;scale_mutation=mode;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemInheritOwnerScale,NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++scale_negatives;}
 for(unsigned mode=9;mode<=10;++mode){scale_seed();scaling_observation=true;scale_mutation=mode;NativeScriptValue arg={.type=NativeScriptType_F32,.as.f32=2},out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindings_Invoke(c,mode==9?NativeScriptBinding_ItemResetSpin:NativeScriptBinding_ItemScaleByAttribute,NULL,0,mode==9?NULL:&arg,mode==9?0:1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++scale_negatives;}
 for(unsigned bad=0;bad<9;++bad){scale_seed();unsigned id=NativeScriptBinding_ItemInheritOwnerScale;
  switch(bad){case 0:item.owner=NULL;break;case 1:scale_owner_object->classifier=HSD_GOBJ_CLASS_ITEM;break;case 2:lists[HSD_GOBJ_PLINK_FIGHTER]=NULL;break;case 3:item.owner=&damage_actor;break;case 4:id=NativeScriptBinding_ItemScaleByAttribute;item.xCC_item_attr=NULL;break;case 5:id=NativeScriptBinding_ItemResetSpin;model.xC_bit_field=192;break;case 6:id=NativeScriptBinding_ItemResetSpin;joints[0].flags|=JOBJ_USE_QUATERNION;break;case 7:id=NativeScriptBinding_ItemResetSpin;article.x10_modelDesc=NULL;break;case 8:{static ItemAttr foreign;id=NativeScriptBinding_ItemScaleByAttribute;item.xCC_item_attr=&foreign;break;}}
  Item before=item;HSD_JObj saved[4];memcpy(saved,joints,sizeof(joints));NativeScriptValue arg={.type=NativeScriptType_F32,.as.f32=2},out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindings_Invoke(c,id,NULL,0,id==NativeScriptBinding_ItemScaleByAttribute?&arg:NULL,id==NativeScriptBinding_ItemScaleByAttribute?1:0,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(&before,&item,sizeof(item))&&!memcmp(saved,joints,sizeof(joints)));++scale_negatives;
 }
 scale_seed();run("ScaleOperations");CHECK(item.scl==0.375f&&joints[0].scale.y==0.375f);
 CHECK(nativeMatchPool_Reference(&nativeGObjPool,scale_owner_object,&ref));CHECK(nativeMatchPool_Free(&nativeGObjPool,ref));lists[HSD_GOBJ_PLINK_FIGHTER]=NULL;item.owner=NULL;scale_owner_object=NULL;
}
static void projection_tests(NativeScriptRuntimeContext* c){
 run("Projections");CHECK(joints[3].translate.x==1&&joints[3].translate.y==-2&&joints[3].translate.z==3&&(joints[3].flags&JOBJ_HIDDEN));
 HSD_JObj child={0},sibling={0};joints[3].child=&child;joints[3].next=&sibling;
 unsigned local=UINT32_MAX,recursive=UINT32_MAX;
 for(unsigned i=0;i<program.model_selector_count;++i){if((selectors[i].semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)!=NATIVE_SCRIPT_ITEM_MODEL_PROPERTY)continue;if(selectors[i].channel==15)local=i;if(selectors[i].channel==10)recursive=i;}
 CHECK(local!=UINT32_MAX&&recursive!=UINT32_MAX);
 NativeScriptValue args[2],value={.type=NativeScriptType_Bool,.as.u64=1};recipe(&selectors[local],args);
 CHECK(nativeScriptBindingItemModel_Write(c,selectors[local].semantic,args,selectors[local].argument_count,&value,&diagnostic));CHECK(!(joints[3].flags&JOBJ_HIDDEN)&&child.flags==0&&sibling.flags==0);
 value.as.u64=0;CHECK(nativeScriptBindingItemModel_Write(c,selectors[local].semantic,args,selectors[local].argument_count,&value,&diagnostic));CHECK((joints[3].flags&JOBJ_HIDDEN)&&child.flags==0&&sibling.flags==0);++projection_pairs;
 recipe(&selectors[recursive],args);CHECK(nativeScriptBindingItemModel_Write(c,selectors[recursive].semantic,args,selectors[recursive].argument_count,&value,&diagnostic));CHECK((child.flags&JOBJ_HIDDEN)&&sibling.flags==0);++projection_pairs;
 joints[3].child=joints[3].next=NULL;
}
static HSD_AObj model_aobj[6],material_aobj,texture_aobj,shape_aobj;
static HSD_JObj model_deep,model_sibling;
static HSD_DObj model_dobj;static HSD_MObj model_mobj;static HSD_TObj model_tobj;static HSD_PObj model_pobj;static HSD_ShapeSet model_shape;
static unsigned model_control_events;
static void model_control_observe(void){
 ++model_control_events;
 switch(model_control_mutation){
 case 1:++item.x1C;break;
 case 2:storage.live=false;break;
 case 3:actor->hsd_obj=&joints[1];break;
 case 4:actor->user_data=&damage_owner;break;
 case 5:++program.identity;break;
 case 6:{NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,actor,&ref));CHECK(nativeMatchPool_Free(&nativeGObjPool,ref));HSD_GObj* replacement=nativeMatchPool_Allocate(&nativeGObjPool,&ref);CHECK(replacement==actor);replacement->classifier=HSD_GOBJ_CLASS_ITEM;replacement->obj_kind=HSD_GObj_JObjKind;replacement->user_data=&item;replacement->hsd_obj=&joints[0];break;}
 }
}
static void model_control_seed(unsigned boundary){
 appearance_seed(true);appearance_mode=false;retire_stage=0;model_control_observation=true;model_control_mutation=0;model_control_events=0;
 joints[0].parent=NULL;joints[0].child=&joints[1];joints[1].parent=&joints[0];joints[1].child=&joints[2];joints[2].parent=joints[3].parent=&joints[1];joints[2].next=&joints[3];
 for(unsigned i=0;i<4;++i){joints[i].flags=JOBJ_LIGHTING|(i&1?JOBJ_HIDDEN:0);model_aobj[i]=(HSD_AObj){.framerate=10+(float)i};joints[i].aobj=&model_aobj[i];}
 model_aobj[4]=(HSD_AObj){.framerate=14};model_aobj[5]=(HSD_AObj){.framerate=15};
 model_deep=(HSD_JObj){.parent=&joints[2],.aobj=&model_aobj[4],.flags=JOBJ_LIGHTING};HSD_CLASS_METHOD(&model_deep)=&subclass;joints[2].child=&model_deep;
 model_sibling=(HSD_JObj){.aobj=&model_aobj[5],.flags=JOBJ_LIGHTING};HSD_CLASS_METHOD(&model_sibling)=&subclass;joints[0].next=&model_sibling;
 if(boundary<3)joints[boundary].flags|=JOBJ_INSTANCE;
 material_aobj=(HSD_AObj){.framerate=21};texture_aobj=(HSD_AObj){.framerate=22};shape_aobj=(HSD_AObj){.framerate=23};
 model_tobj=(HSD_TObj){.aobj=&texture_aobj};model_mobj=(HSD_MObj){.aobj=&material_aobj,.tobj=&model_tobj};model_shape=(HSD_ShapeSet){.aobj=&shape_aobj};model_pobj=(HSD_PObj){.flags=POBJ_SHAPEANIM,.u.shape_set=&model_shape};model_dobj=(HSD_DObj){.mobj=&model_mobj,.pobj=&model_pobj};joints[1].u.dobj=&model_dobj;
}
static void model_control_tests(NativeScriptRuntimeContext* c){
 for(unsigned boundary=0;boundary<4;++boundary)for(unsigned visible=0;visible<2;++visible){
  model_control_seed(boundary);if(visible)it_8026BB20(actor);else it_8026BB44(actor);
  unsigned expected[6];for(unsigned i=0;i<4;++i)expected[i]=joints[i].flags;expected[4]=model_deep.flags;expected[5]=model_sibling.flags;
  model_control_seed(boundary);Item before=item;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Invoke(c,visible?NativeScriptBinding_ItemShowModel:NativeScriptBinding_ItemHideModel,NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void&&model_control_events==1&&!memcmp(&before,&item,sizeof(item)));
  for(unsigned i=0;i<4;++i)CHECK(joints[i].flags==expected[i]);CHECK(model_deep.flags==expected[4]&&model_sibling.flags==expected[5]);
  CHECK((joints[0].flags&JOBJ_HIDDEN)==(visible?0:JOBJ_HIDDEN));CHECK(!(model_sibling.flags&JOBJ_HIDDEN));
  if(boundary==0)CHECK(!(joints[2].flags&JOBJ_HIDDEN));if(boundary==2)CHECK(!(model_deep.flags&JOBJ_HIDDEN));++model_control_pairs;
 }
 float rates[]={-2,-0.0f,0,1.75f,INFINITY,NAN};
 for(unsigned boundary=0;boundary<4;++boundary)for(unsigned missing=0;missing<2;++missing)for(unsigned input=0;input<ARRAY_SIZE(rates);++input){
  model_control_seed(boundary);if(missing)joints[3].aobj=NULL;lb_8000BA0C(&joints[0],rates[input]);HSD_AObj expected[6];memcpy(expected,model_aobj,sizeof(expected));
  model_control_seed(boundary);if(missing)joints[3].aobj=NULL;Item before=item;NativeScriptValue rate={.type=NativeScriptType_F32,.as.f32=rates[input]},out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemSetModelAnimationRate,NULL,0,&rate,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void&&model_control_events==1&&!memcmp(&before,&item,sizeof(item)));
  CHECK(!memcmp(expected,model_aobj,sizeof(expected)));CHECK(model_aobj[0].framerate==10&&model_aobj[5].framerate==15);
  CHECK(material_aobj.framerate==21&&texture_aobj.framerate==22&&shape_aobj.framerate==23);++model_control_pairs;
 }
 unsigned ids[]={NativeScriptBinding_ItemHideModel,NativeScriptBinding_ItemShowModel,NativeScriptBinding_ItemSetModelAnimationRate};
 for(unsigned op=0;op<3;++op)for(unsigned mode=1;mode<=6;++mode){model_control_seed(3);model_control_mutation=mode;NativeScriptValue rate={.type=NativeScriptType_F32,.as.f32=1},out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindings_Invoke(c,ids[op],NULL,0,op==2?&rate:NULL,op==2?1:0,&out,&diagnostic));CHECK(model_control_events==1&&out.type==NativeScriptType_U64&&out.as.u64==99);++model_control_negatives;}
 model_control_seed(3);run("WholeModel");CHECK(model_control_events==3&&item.x5D0_animFrameSpeed==2&&model_aobj[0].framerate==10&&model_aobj[1].framerate==-0.5f);for(unsigned i=0;i<4;++i)CHECK(!(joints[i].flags&JOBJ_HIDDEN));
 for(unsigned mode=1;mode<=2;++mode){model_control_seed(3);model_control_mutation=mode;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(execute("WholeModel",&out)==(mode==1?NativeScriptVM_SourceRetired:NativeScriptVM_Error));CHECK(model_control_events==1&&out.type==NativeScriptType_U64&&out.as.u64==99);++model_control_negatives;}
 model_control_observation=false;model_control_mutation=0;appearance_seed(true);for(unsigned i=0;i<4;++i)joints[i].aobj=NULL;joints[0].next=NULL;joints[2].child=NULL;
}
static void advancement(NativeScriptRuntimeContext* c){
 unsigned pairs=0;for(unsigned index=0;index<4;++index)for(unsigned enabled=0;enabled<2;++enabled)for(unsigned owned=0;owned<2;++owned){
  command_seed(index,12+index,256+index,enabled!=0,owned!=0);Item saved=item;Item_802694CC(actor);Item expected=item;
  item=saved;stages=0;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemAdvanceAnimation,NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);
  CHECK(item.x5CC_currentAnimFrame==expected.x5CC_currentAnimFrame&&item.x524_cmd.u==expected.x524_cmd.u&&item.x524_cmd.timer==expected.x524_cmd.timer&&item.x524_cmd.frame_count==expected.x524_cmd.frame_count);
  CHECK(item.xDBC_itcmd_var4_word==0&&item.x3C==expected.x3C);CHECK(!memcmp(item.x5D4_hitboxes,expected.x5D4_hitboxes,sizeof(item.x5D4_hitboxes)));CHECK(stages==2+owned);++pairs;
 }
 command_seed(1,22,512,true,true);Item saved=item;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemAdvanceModelAnimation,NULL,0,NULL,0,&out,&diagnostic));CHECK(stages==1&&out.type==NativeScriptType_Void);CHECK(!memcmp(&saved.x524_cmd,&item.x524_cmd,sizeof(item.x524_cmd))&&saved.x5CC_currentAnimFrame==item.x5CC_currentAnimFrame&&saved.xDBC_itcmd_var4_word==item.xDBC_itcmd_var4_word);CHECK(!memcmp(saved.x5D4_hitboxes,item.x5D4_hitboxes,sizeof(item.x5D4_hitboxes)));
 for(retire_stage=1;retire_stage<=3;++retire_stage){command_seed(2,20,512,true,true);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindings_Invoke(c,NativeScriptBinding_ItemAdvanceAnimation,NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&stages==retire_stage);if(retire_stage<3)CHECK(item.x524_cmd.u==commands&&item.x5CC_currentAnimFrame==1&&item.x5D4_hitboxes[2].hit.damage==-7);else CHECK(item.x5CC_currentAnimFrame==7.5f&&item.x524_cmd.u==NULL);}
 retire_stage=0;command_seed(0,10,256,true,false);run("Advance");CHECK(stages==3&&item.x524_cmd.u==NULL&&item.x5CC_currentAnimFrame==7.5f&&item.x5D4_hitboxes[0].hit.damage==30);item.owner=NULL;
 printf("{\"status\":\"PASS primary Item model, appearance and DATA advancement\",\"checks\":%u,\"native_pairs\":%u,\"appearance_native_pairs\":%u,\"appearance_negative_cases\":%u,\"prepared_selectors\":%u,\"scale_native_pairs\":%u,\"scale_negative_cases\":%u,\"projection_pairs\":%u,\"model_control_pairs\":%u,\"model_control_negative_cases\":%u,\"world_position_pairs\":%u,\"world_position_negative_cases\":%u,\"dynamic_joint_pairs\":%u,\"matrix_rotation_pairs\":%u,\"joint_model_negative_cases\":%u,\"animation_retirement_stages\":4,\"advancement_retirement_stages\":3}\n",checks,pairs+23+appearance_pairs+scale_pairs+projection_pairs+model_control_pairs,appearance_pairs,appearance_negatives,program.model_selector_count,scale_pairs,scale_negatives,projection_pairs,model_control_pairs,model_control_negatives,world_pairs,world_negatives,dynamic_pairs,matrix_pairs,joint_model_negatives);
}
static void world_seed(void){
 world_mode=true;world_mutation=0;item.x1C=17;storage.live=true;program.identity=123;
 actor->classifier=HSD_GOBJ_CLASS_ITEM;actor->obj_kind=HSD_GObj_JObjKind;actor->hsd_obj=&joints[0];actor->user_data=&item;
 for(unsigned i=0;i<4;++i){HSD_CLASS_METHOD(&joints[i])=&subclass;joints[i].translate=(Vec3){1,2,3};joints[i].flags=JOBJ_MTX_DIRTY;}
 joints[0].parent=NULL;joints[0].child=&joints[1];joints[1].child=&joints[2];joints[2].next=&joints[3];
 joints[1].parent=&joints[0];joints[2].parent=joints[3].parent=&joints[1];
}
static void world_tests(NativeScriptRuntimeContext* c){
 const NativeScriptItemModelSelector* selected=NULL;
 for(unsigned i=0;i<program.model_selector_count;++i)if((selectors[i].semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION&&selectors[i].depth==2)selected=&selectors[i];CHECK(selected);
 float values[]={0,-0.0f,3.75f,-18,INFINITY,NAN};
 for(unsigned v=0;v<ARRAY_SIZE(values);++v){world_seed();joints[3].translate=(Vec3){values[v],2,3};Vec3 expected={0};lb_8000B1CC(&joints[3],NULL,&expected);joints[3].flags|=JOBJ_MTX_DIRTY;
  NativeScriptValue args[5]={{.type=NativeScriptType_I32},{.type=NativeScriptType_Text,.as.text={selected->path_offset,selected->path_length}},{.type=NativeScriptType_F32,.as.f32=91},{.type=NativeScriptType_F32,.as.f32=92},{.type=NativeScriptType_F32,.as.f32=93}},out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindings_Invoke(c,selected->semantic,NULL,0,args,5,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);
  CHECK(!memcmp(&args[2].as.f32,&expected.x,4)&&!memcmp(&args[3].as.f32,&expected.y,4)&&!memcmp(&args[4].as.f32,&expected.z,4));++world_pairs;
 }
 world_seed();NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};CHECK(execute("World",&result)==NativeScriptVM_Ok);CHECK(result.type==NativeScriptType_F32&&result.as.f32==101+202*10+303*100);++world_pairs;
 world_seed();CHECK(execute("RootWorld",&result)==NativeScriptVM_Ok);CHECK(result.as.f32==321);++world_pairs;
 world_seed();CHECK(execute("MissingWorld",&result)==NativeScriptVM_Ok);CHECK(result.as.f32==0);++world_pairs;
 world_seed();actor->hsd_obj=NULL;actor->obj_kind=HSD_GOBJ_OBJ_NONE;CHECK(execute("RootWorld",&result)==NativeScriptVM_Ok);CHECK(result.as.f32==0);++world_pairs;
 for(unsigned mutation=1;mutation<=8;++mutation){world_seed();uint64_t layout=banks[0].storage_layout.identity;world_mutation=mutation;
  NativeScriptValue args[5]={{.type=NativeScriptType_I32},{.type=NativeScriptType_Text,.as.text={selected->path_offset,selected->path_length}},{.type=NativeScriptType_F32,.as.f32=91},{.type=NativeScriptType_F32,.as.f32=92},{.type=NativeScriptType_F32,.as.f32=93}},saved[5],out={.type=NativeScriptType_U64,.as.u64=99};memcpy(saved,args,sizeof(args));
  CHECK(!nativeScriptBindings_Invoke(c,selected->semantic,NULL,0,args,5,&out,&diagnostic));CHECK(!memcmp(args,saved,sizeof(args))&&out.type==NativeScriptType_U64&&out.as.u64==99);banks[0].storage_layout.identity=layout;++world_negatives;
 }
 world_seed();world_mutation=1;result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(execute("World",&result)!=NativeScriptVM_Ok);CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++world_negatives;
 for(unsigned invalid=0;invalid<6;++invalid){world_seed();NativeScriptRuntimeContext source=*c;
  NativeScriptValue args[5]={{.type=NativeScriptType_I32},{.type=NativeScriptType_Text,.as.text={selected->path_offset,selected->path_length}},{.type=NativeScriptType_F32,.as.f32=91},{.type=NativeScriptType_F32,.as.f32=92},{.type=NativeScriptType_F32,.as.f32=93}},saved[5],out={.type=NativeScriptType_U64,.as.u64=99};
  if(invalid==0)++args[1].as.text.offset;else if(invalid==1)++args[1].as.text.length;
  else if(invalid==2)args[0].as.i64=1;else if(invalid==3)args[3].type=NativeScriptType_I32;
  else if(invalid==4)source.source_class=NativeCallbackSource_Fighter;
  memcpy(saved,args,sizeof(args));CHECK(!nativeScriptBindings_Invoke(&source,selected->semantic,NULL,0,args,invalid==5?4:5,&out,&diagnostic));
  CHECK(!memcmp(saved,args,sizeof(args))&&out.type==NativeScriptType_U64&&out.as.u64==99);++world_negatives;
 }
 world_seed();world_mode=false;
}
static DynamicBoneTable dynamic_table;
static void joint_model_seed(void){
 world_seed();world_mode=false;memset(joints,0,sizeof(joints));memset(&dynamic_table,0,sizeof(dynamic_table));
 for(unsigned i=0;i<4;++i){HSD_CLASS_METHOD(&joints[i])=&subclass;joints[i].id=(uintptr_t)&descriptor[i];joints[i].scale=(Vec3){2,3,4};joints[i].translate=(Vec3){1,-2,3};joints[i].rotate=(Quaternion){0.2f,-0.3f,0.4f,0.5f};dynamic_table.bones[i]=&joints[i];}
 joints[0].child=&joints[1];joints[1].parent=&joints[0];joints[1].child=&joints[2];joints[2].next=&joints[3];joints[2].parent=joints[3].parent=&joints[1];
 descriptor[0].child=&descriptor[1];descriptor[1].child=&descriptor[2];descriptor[2].next=&descriptor[3];model.x0_joint=&descriptor[0];article.x10_modelDesc=&model;item.xBBC_dynamicBoneTable=&dynamic_table;
}
static void joint_model_tests(NativeScriptRuntimeContext* c){
 float values[]={-2,-0.0f,1.25f,INFINITY,NAN};
 for(unsigned i=0;i<program.model_selector_count;++i){const NativeScriptItemModelSelector* s=&selectors[i];
  if((s->semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)!=NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY)continue;
  for(unsigned missing=0;missing<3;++missing)for(unsigned quaternion=0;quaternion<2;++quaternion)for(unsigned value=0;value<ARRAY_SIZE(values);++value){
   joint_model_seed();if(missing==1)item.xBBC_dynamicBoneTable=NULL;if(missing==2)dynamic_table.bones[3]=NULL;if(quaternion)joints[3].flags|=JOBJ_USE_QUATERNION;
   NativeScriptValue args[1]={{.type=NativeScriptType_I32,.as.i64=s->dynamic_bone}},out={.type=NativeScriptType_U64,.as.u64=99};
   float direct=nativeArticleModel_ReadDynamic(actor,s->dynamic_bone,s->channel);
   CHECK(nativeScriptBindingItemModel_Read(c,s->semantic,args,1,&out,&diagnostic));
   bool boolean=s->channel==9||s->channel==10||s->channel==15;
   CHECK(out.type==(boolean?NativeScriptType_Bool:NativeScriptType_F32));CHECK(boolean?out.as.u64==(uint64_t)(direct!=0):!memcmp(&out.as.f32,&direct,4));
   if(s->channel!=9){NativeScriptValue input=boolean?(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value&1}:(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=values[value]};
    HSD_JObj original[4],expected[4];memcpy(original,joints,sizeof(joints));nativeArticleModel_WriteDynamic(actor,s->dynamic_bone,s->channel,boolean?(float)input.as.u64:input.as.f32);memcpy(expected,joints,sizeof(joints));memcpy(joints,original,sizeof(joints));
    CHECK(nativeScriptBindingItemModel_Write(c,s->semantic,args,1,&input,&diagnostic));CHECK(!memcmp(expected,joints,sizeof(joints)));
   }++dynamic_pairs;
  }
 }
 joint_model_seed();run("DynamicModify");CHECK(joints[3].translate.y==-7&&joints[3].rotate.x==0.5f&&joints[3].scale.z==2&&(joints[3].flags&JOBJ_HIDDEN));
 NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};CHECK(execute("DynamicMissing",&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_F32&&result.as.f32==1);
 const NativeScriptItemModelSelector* selected=NULL;
 for(unsigned i=0;i<program.model_selector_count;++i)if((selectors[i].semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY&&selectors[i].dynamic_bone==3&&selectors[i].channel==1)selected=&selectors[i];CHECK(selected);
 for(unsigned bad=0;bad<7;++bad){joint_model_seed();NativeScriptValue args[1]={{.type=NativeScriptType_I32,.as.i64=3}},out={.type=NativeScriptType_U64,.as.u64=99};
  if(bad==0)args[0].as.i64=2;else if(bad==1)args[0].type=NativeScriptType_Bool;else if(bad==2)dynamic_table.bones[3]=(HSD_JObj*)(uintptr_t)16;
  else if(bad==3)dynamic_table.bones[3]=&joints[2];else if(bad==4)joints[3].id=(uintptr_t)&descriptor[2];else if(bad==5)HSD_CLASS_METHOD(&joints[3])=&wrong_class;else item.x1C++;
  CHECK(!nativeScriptBindingItemModel_Read(c,selected->semantic,args,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++joint_model_negatives;
 }
 for(unsigned i=0;i<program.model_selector_count;++i){const NativeScriptItemModelSelector* s=&selectors[i];
  if((s->semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)!=NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION)continue;
  for(unsigned variant=0;variant<8;++variant){joint_model_seed();
   for(unsigned n=0;n<4;++n){float angle=(float)(variant+1)*0.17f;float a=cosf(angle),b=sinf(angle);Mtx m={{a,-b,0,7},{b,a,0,-8},{0,0,1,9}};memcpy(joints[n].mtx,m,sizeof(Mtx));joints[n].flags|=JOBJ_MTX_DIRTY;}
   if(variant==1)memset(joints[3].mtx,0,sizeof(Mtx));else if(variant==2)joints[3].mtx[2][0]=-2;else if(variant==3)joints[3].mtx[0][0]=-0.0f;else if(variant==4)joints[3].mtx[0][0]=NAN;else if(variant==5)joints[3].mtx[1][1]=INFINITY;
   else if(variant==6){actor->hsd_obj=NULL;actor->obj_kind=HSD_GOBJ_OBJ_NONE;}else if(variant==7)joints[2].next=NULL;
   const char* path=s->depth==2?"0/1":s->depth?"255":"";Vec3 expected={0};(void)nativeArticleModel_ReadMatrixRotation(actor,0,path,&expected);
   HSD_JObj before[4];memcpy(before,joints,sizeof(joints));unsigned stages_before=stages;
   NativeScriptValue args[5]={{.type=NativeScriptType_I32},{.type=NativeScriptType_Text,.as.text={s->path_offset,s->path_length}},{.type=NativeScriptType_F32,.as.f32=91},{.type=NativeScriptType_F32,.as.f32=92},{.type=NativeScriptType_F32,.as.f32=93}},out={.type=NativeScriptType_U64,.as.u64=99};
   CHECK(nativeScriptBindings_Invoke(c,s->semantic,NULL,0,args,5,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void&&!memcmp(before,joints,sizeof(joints))&&stages==stages_before);
   CHECK(!memcmp(&args[2].as.f32,&expected.x,4)&&!memcmp(&args[3].as.f32,&expected.y,4)&&!memcmp(&args[4].as.f32,&expected.z,4));++matrix_pairs;
   if(variant==0){CHECK(execute(s->depth==2?"Matrix":s->depth?"MissingMatrix":"RootMatrix",&result)==NativeScriptVM_Ok);float total=expected.x+expected.y*10+expected.z*100;CHECK(result.type==NativeScriptType_F32&&!memcmp(&result.as.f32,&total,4));}
  }
 }
 joint_model_seed();item.xBBC_dynamicBoneTable=NULL;
}
int main(void){
 const char* source="fighter:\n    behavior independent\narticle Probe states 1:\n    func Modify() -> float:\n        item.joint(0,\"0/1\").visible = false\n        item.joint(0,\"0/1\").rotation.x = 1\n        item.joint(0,\"0/1\").rotation.y = 2\n        item.joint(0,\"0/1\").rotation.z = 3\n        item.joint(0,\"0/1\").scale.x = 4\n        item.joint(0,\"0/1\").scale.y = 5\n        item.joint(0,\"0/1\").scale.z = 6\n        item.set_scale(2)\n        item.set_animation_frame(7)\n        return item.joint(0,\"0/1\").rotation.z\n    func Advance() -> float:\n        item.advance_animation()\n        item.advance_model_animation()\n        return 3\n    func Bake() -> float:\n        item.set_appearance(1,7)\n        return 3\n    func InvalidAppearance() -> float:\n        item.set_appearance(-1,7)\n        item.set_appearance(2,7)\n        return 3\n    func ScaleOperations() -> float:\n        item.reset_spin()\n        item.scale_by_attribute(2)\n        item.set_instance_scale(0.5)\n        item.inherit_owner_scale()\n        return 3\n    func Projections() -> float:\n        item.joint(0,\"0/1\").translation.x = 1\n        item.joint(0,\"0/1\").translation.y = -2\n        item.joint(0,\"0/1\").translation.z = 3\n        item.joint(0,\"0/1\").local_visible = false\n        return 3\n    func WholeModel() -> float:\n        item.hide_model()\n        item.set_model_animation_rate(-0.5)\n        item.show_model()\n        return 3\n    func Empty() -> bool:\n        return item.joint(\"\").exists\n    func World() -> float:\n        let float x = 91\n        let float y = 92\n        let float z = 93\n        item.joint_world_position(0,\"0/1\",x,y,z)\n        return x + y * 10 + z * 100\n    func RootWorld() -> float:\n        let float x = 91\n        let float y = 92\n        let float z = 93\n        item.joint_world_position(0,\"\",x,y,z)\n        return x + y * 10 + z * 100\n    func MissingWorld() -> float:\n        let float x = 91\n        let float y = 92\n        let float z = 93\n        item.joint_world_position(0,\"255\",x,y,z)\n        return x + y * 10 + z * 100\n    func DynamicModify() -> float:\n        item.dynamic_joint(3).translation.y = -7\n        item.dynamic_joint(3).rotation.x = 0.5\n        item.dynamic_joint(3).scale.z = 2\n        item.dynamic_joint(3).local_visible = false\n        return 3\n    func DynamicProbe() -> float:\n        let float total = item.dynamic_joint(3).translation.x + item.dynamic_joint(3).translation.y + item.dynamic_joint(3).translation.z\n        total = total + item.dynamic_joint(3).rotation.x + item.dynamic_joint(3).rotation.y + item.dynamic_joint(3).rotation.z\n        total = total + item.dynamic_joint(3).scale.x + item.dynamic_joint(3).scale.y + item.dynamic_joint(3).scale.z\n        total = total + item.dynamic_joint(3).quaternion.x + item.dynamic_joint(3).quaternion.y + item.dynamic_joint(3).quaternion.z + item.dynamic_joint(3).quaternion.w\n        if item.dynamic_joint(3).exists and item.dynamic_joint(3).visible and item.dynamic_joint(3).local_visible: total = total + 1\n        return total\n    func DynamicMissing() -> float:\n        return item.dynamic_joint(99).scale.z\n    func Matrix() -> float:\n        let float x = 91\n        let float y = 92\n        let float z = 93\n        item.read_joint_matrix_rotation(0,\"0/1\",x,y,z)\n        return x + y * 10 + z * 100\n    func RootMatrix() -> float:\n        let float x = 91\n        let float y = 92\n        let float z = 93\n        item.read_joint_matrix_rotation(0,\"\",x,y,z)\n        return x + y * 10 + z * 100\n    func MissingMatrix() -> float:\n        let float x = 91\n        let float y = 92\n        let float z = 93\n        item.read_joint_matrix_rotation(0,\"255\",x,y,z)\n        return x + y * 10 + z * 100\n    state 0 anim -1:\n";
 setup(0);setup(1);NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));CHECK(accepted==&banks[0]);
 program=(NativeScriptItemProgram){.bank=accepted,.owner=accepted->declarations.functions[0].owner,.identity=123,.author_kind=Ft_Kind_CustomFirst,.kind=It_Kind_Mario_Fire,.article=&article};
 descriptor[0].child=&descriptor[1];descriptor[1].child=&descriptor[2];descriptor[2].next=&descriptor[3];model.x0_joint=&descriptor[0];article.x10_modelDesc=&model;
 CHECK(nativeScriptBindingItemModel_PrepareSelectors(accepted,&program,selectors,128,&diagnostic));CHECK(program.model_selector_count==40&&program.model_selectors==selectors);
 for(unsigned i=0;i<program.model_selector_count;++i){const NativeScriptItemModelSelector* s=&selectors[i];CHECK((s->semantic&NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)<accepted->expressions.node_count);CHECK(accepted->expressions.nodes[s->semantic&NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK].semantic==s->semantic);CHECK(s->depth==0||(s->depth==2&&s->path[0]==0&&s->path[1]==1)||(s->depth==1&&s->path[0]==255));if(i)CHECK(selectors[i-1].semantic<s->semantic);}
 uint64_t before=hash(selectors,sizeof(selectors));const NativeScriptItemModelSelector* old=program.model_selectors;uint32_t old_count=program.model_selector_count;
 CHECK(!nativeScriptBindingItemModel_PrepareSelectors(accepted,&program,selectors,1,&diagnostic));CHECK(program.model_selectors==old&&program.model_selector_count==old_count&&hash(selectors,sizeof(selectors))==before);
 descriptor[2].next=NULL;CHECK(nativeScriptBindingItemModel_PrepareSelectors(accepted,&program,selectors,128,&diagnostic));CHECK(hash(selectors,sizeof(selectors))==before);descriptor[2].next=&descriptor[3];
 const char* bad[]={"item.joint(4,\"0\").visible = true","let int index = 0\n        item.joint(index,\"0\").visible = true","item.joint(0,\"0/\").visible = true","item.joint(0,\"256\").visible = true","item.joint(0,\"0/0/0/0/0/0/0/0/0/0/0/0/0\").visible = true","item.joint(0,\"0\").exists = true","item.joint(0,\"0\").visible = 1","item.joint(0,\"0\").translation.w = 2",
 "let float x = 0\n        let float z = 0\n        item.joint_world_position(0,\"\",x,x,z)",
 "let float x = 0\n        let float y = 0\n        let float z = 0\n        item.joint_world_position(4,\"\",x,y,z)",
 "let int x = 0\n        let float y = 0\n        let float z = 0\n        item.joint_world_position(0,\"\",x,y,z)",
 "let float x = 0\n        let float y = 0\n        let float z = 0\n        item.joint_world_position(0,\"0/\",x,y,z)",
 "let float x = 0\n        let float y = 0\n        let float z = 0\n        item.joint_world_position(0,\"\",x + 1,y,z)","item.dynamic_joint(100).translation.y = 1","item.dynamic_joint(-1).translation.y = 1","item.dynamic_joint(true).translation.y = 1","item.dynamic_joint(3.0).translation.y = 1","let int index = 3\n        item.dynamic_joint(index).translation.y = 1","item.dynamic_joint(3).exists = true","item.dynamic_joint(3).translation.w = 1","item.dynamic_joint(3).visible = 1","let float x = 0\n        let float z = 0\n        item.read_joint_matrix_rotation(0,\"\",x,x,z)","let float x = 0\n        let float y = 0\n        let float z = 0\n        item.read_joint_matrix_rotation(4,\"\",x,y,z)","let int x = 0\n        let float y = 0\n        let float z = 0\n        item.read_joint_matrix_rotation(0,\"\",x,y,z)","let float x = 0\n        let float y = 0\n        let float z = 0\n        item.read_joint_matrix_rotation(0,\"0/\",x,y,z)"};for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)negative(bad[i],&accepted);
 CHECK(nativeMatchPool_Bind(&nativeGObjPool,payload,sizeof(payload),pool_slots,free_order,2,sizeof(HSD_GObj),_Alignof(HSD_GObj),64));NativeMatchPoolReference reference;actor=nativeMatchPool_Allocate(&nativeGObjPool,&reference);CHECK(actor);actor->classifier=HSD_GOBJ_CLASS_ITEM;actor->obj_kind=HSD_GObj_JObjKind;actor->hsd_obj=&joints[0];actor->user_data=&item;lists[HSD_GOBJ_PLINK_ITEM]=actor;
 item=(Item){.entity=actor,.x1C=17,.kind=program.kind,.native_script_owner=program.owner,.native_article_author_kind=program.author_kind,.xC4_article_data=&article,.x5D0_animFrameSpeed=2,.scl=9};
 storage=(NativeScriptStorageInstance){.layout=&banks[0].storage_layout,.parameters=&banks[0].storage_parameters,.live=true,.owner=program.owner,.generation=17};
 HSD_ClassInfo* base=HSD_CLASS_INFO(&hsdJObj);*base=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj)}};subclass=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj),.parent=base}};wrong_class=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_Obj)}};
 for(unsigned i=0;i<4;++i){HSD_CLASS_METHOD(&joints[i])=&subclass;joints[i].scale=(Vec3){1,1,1};}joints[0].child=&joints[1];joints[1].child=&joints[2];joints[2].next=&joints[3];joints[1].parent=&joints[0];joints[2].parent=joints[3].parent=&joints[1];
 run("Modify");CHECK(stages==4&&item.x5CC_currentAnimFrame==7.5f&&item.scl==9);CHECK(joints[0].scale.x==2&&joints[0].scale.y==2&&joints[0].scale.z==2);CHECK((joints[3].flags&JOBJ_HIDDEN)&&!(joints[2].flags&JOBJ_HIDDEN));CHECK(joints[3].rotate.x==1&&joints[3].rotate.y==2&&joints[3].rotate.z==3);CHECK(joints[3].scale.x==4&&joints[3].scale.y==5&&joints[3].scale.z==6);
 NativeScriptRuntimeContext c=context();unsigned pairs=0;
 for(unsigned i=0;i<program.model_selector_count;++i){if(selectors[i].channel==11||(selectors[i].semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)!=NATIVE_SCRIPT_ITEM_MODEL_PROPERTY)continue;NativeScriptValue args[2],out=recipe(&selectors[i],args);const NativeScriptItemModelSelector* s=&selectors[i];HSD_JObj* node=s->depth?&joints[3]:&joints[0];CHECK(nativeScriptBindingItemModel_Read(&c,s->semantic,args,s->argument_count,&out,&diagnostic));float direct=nativeModelJoint_Read(node,s->channel);CHECK(out.type==(s->channel>=9?NativeScriptType_Bool:NativeScriptType_F32));CHECK(s->channel>=9?out.as.u64==(uint64_t)(direct!=0):out.as.f32==direct);++pairs;
  if(s->channel<9){NativeScriptValue input={.type=NativeScriptType_F32,.as.f32=-0.5f};HSD_JObj expected=*node;nativeModelJoint_Write(&expected,s->channel,input.as.f32);CHECK(nativeScriptBindingItemModel_Write(&c,s->semantic,args,s->argument_count,&input,&diagnostic));CHECK(!memcmp(&expected.rotate,&node->rotate,sizeof(node->rotate))&&!memcmp(&expected.scale,&node->scale,sizeof(node->scale)));input.as.f32=NAN;expected=*node;CHECK(nativeScriptBindingItemModel_Write(&c,s->semantic,args,s->argument_count,&input,&diagnostic));CHECK(!memcmp(&expected,node,sizeof(expected)));++pairs;}
  HSD_CLASS_METHOD(&joints[0])=&wrong_class;out.type=NativeScriptType_U64;out.as.u64=99;CHECK(!nativeScriptBindingItemModel_Read(&c,s->semantic,args,s->argument_count,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);HSD_CLASS_METHOD(&joints[0])=&subclass;
 }
 NativeScriptValue scalar={.type=NativeScriptType_F32,.as.f32=INFINITY},output={.type=NativeScriptType_U64,.as.u64=99};CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemSetScale,NULL,0,&scalar,1,&output,&diagnostic));CHECK(isinf(joints[0].scale.x)&&item.scl==9);
 NativeScriptStorageLayout copied_layout=banks[0].storage_layout;storage.layout=&copied_layout;rejected_read(&c);storage.layout=&banks[0].storage_layout;
 NativeScriptStorageParameters copied_parameters=banks[0].storage_parameters;storage.parameters=&copied_parameters;rejected_read(&c);storage.parameters=&banks[0].storage_parameters;
 uint32_t owner=storage.owner;storage.owner++;rejected_read(&c);storage.owner=owner;storage.live=false;rejected_read(&c);storage.live=true;
 program.model_selectors=NULL;rejected_read(&c);program.model_selectors=selectors;
 const NativeScriptCompileBank* pinned=program.bank;program.bank=&banks[1];rejected_read(&c);program.bank=pinned;
 // Missing paths retain native defaults at preparation and runtime.
 joints[2].next=NULL;for(unsigned i=0;i<program.model_selector_count;++i)if(selectors[i].depth&&selectors[i].channel!=11&&(selectors[i].semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_PROPERTY){NativeScriptValue args[2],out=recipe(&selectors[i],args);CHECK(nativeScriptBindingItemModel_Read(&c,selectors[i].semantic,args,selectors[i].argument_count,&out,&diagnostic));float direct=nativeModelJoint_Read(NULL,selectors[i].channel);CHECK(selectors[i].channel>=9?out.as.u64==(uint64_t)(direct!=0):out.as.f32==direct);}joints[2].next=&joints[3];
 scalar.as.f32=7;for(retire_stage=1;retire_stage<=4;++retire_stage){stages=0;item.x1C=17;item.x5CC_currentAnimFrame=9;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemSetAnimationFrame,NULL,0,&scalar,1,&output,&diagnostic));CHECK(stages==retire_stage&&output.type==NativeScriptType_U64&&output.as.u64==99&&item.x5CC_currentAnimFrame==9);}retire_stage=0;item.x1C=17;
 CHECK(pairs==23);projection_tests(&c);scale_tests(&c);model_control_tests(&c);appearance_tests(&c);world_tests(&c);joint_model_tests(&c);advancement(&c);return 0;
}
