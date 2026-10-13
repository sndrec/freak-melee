#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_binding_colors.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftmaterial.h>
#include <melee/ft/ftdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/article_model_layers.h>
#include <melee/it/itmaterial.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=1024,F=32,V=64};
static char text[16384];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch call_graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
NativeScriptCompileBank presentation_bank;unsigned presentation_checks;static NativeSourcePackage source;
static HSD_JObj node_storage[2][4];static HSD_DObj draw_storage[2][4];static HSD_MObj material_storage[2][4];static HSD_Material color_storage[2][4];
static HSD_JObj* nodes;static HSD_DObj* draws;static HSD_MObj* materials;static HSD_Material* colors;
static ItemAttr item_attributes;static HSD_Joint descriptor;static ItemModelDesc model={.x0_joint=&descriptor,.x4_bone_count=1};
static const u8 normal[]={1},outline[]={2};static ArticleModelLayers layers={1,normal,1,outline};
static Article article={.x0_common_attr=&item_attributes,.x10_modelDesc=&model,.model_layers=&layers};
static NativeScriptItemAsset asset={.declaration="Paper",.kind=It_Kind_GameWatch_Chef,.article=&article};
static NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
static MotionState motions[1];static Item item;
static DynamicBoneTable bones;
static NativeScriptDiagnostic diagnostic;
static unsigned comparisons,cases;
HSD_JObjInfo hsdJObj;
ftCommonData* p_ftCommonData;
void presentation_check(bool result,const char* expression,unsigned line){++presentation_checks;if(!result){fprintf(stderr,"colors %u: %s (%u:%u %s)\n",line,expression,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return source.ready&&source.target==kind?&source:NULL;}
static void prepare(void)
{
 source.address=&source;source.ready=source.assets.ready=true;source.target=Ft_Kind_CustomFirst;
 NativeScriptCompileBank* bank=&source.compiler.bank;
 *bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=call_graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* script="fighter:\n    behavior independent\nparameter int red = 300\nparameter int green = -2\nparameter int blue = 258\nparameter int alpha = 511\nstate int result\nstate bool installed\nstate float scale\n"
 "func PaperColor() -> int:\n    if costume.index == 1: return color.rgba(255, 1, 2, 3)\n    if costume.index == 2: return color.rgba(4, 5, 6, 7)\n    if costume.index == 3: return color.rgba(8, 9, 10, 11)\n    return color.rgba(attrs.red, attrs.green, attrs.blue, attrs.alpha)\n"
 "func Paint():\n    state.result = PaperColor()\n    visuals.outline_color = color.rgba(20, 21, 22, 23)\n    visuals.preserve_secondary_material = true\n    model.instance_scale_z = 0.8\n    visuals.set_diffuse_color(state.result)\n    state.installed = visuals.install_outline_visibility(10)\n    state.scale = model.instance_scale_y * model.base_scale\n"
 "article Paper states 1:\n    state int result\n    func PaintItem():\n        item.outline_color = color.rgba(30, 31, 32, 33)\n        item.controller_tint = color.rgba(40, 41, 42, 43)\n        item.controller_tint_enabled = true\n        item.material_opacity = 127\n        item.layered = true\n        item.set_diffuse_color(color.rgba(50, 51, 52, 53))\n    on created:\n        PaintItem()\n    state 0 anim -1:\n";
 source.assets.facts.moves=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=script,.source_bytes=(uint32_t)strlen(script),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata,.motion_layout=&source.assets.facts.moves};
 const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptFighter_PrepareProgram(source.target,bank,&source.assets.facts.moves,&metadata,&source.program,&diagnostic));
 CHECK(nativeScriptFighter_CanPublish(source.target));nativeScriptFighter_PublishProgram(source.target,&source.program);
 source.assets.motions=motions;source.assets.facts.joints=4;source.assets.parts.parts_num=4;
 source.assets.facts.layout.visibility_mask=(1u<<10);source.assets.character.model_data.x0.model_num=1;
 static u8 ordinal=0;static TempS selection={1,&ordinal};static FtPartsVisLookup lookup={1,&selection};
 source.assets.character.visibility_assets[10]=&lookup;
 gFtDataList[source.target]=&source.assets.character.data;ftData_CharacterStateTables[source.target]=motions;
 HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);HSD_CLASS_INFO(&hsdJObj)->head.flags=1;
 presentation_bank=*bank;
}
static void graph(HSD_GObj* object)
{
 unsigned slot=object->classifier==HSD_GOBJ_CLASS_ITEM;nodes=node_storage[slot];draws=draw_storage[slot];materials=material_storage[slot];colors=color_storage[slot];
 memset(nodes,0,4*sizeof(*nodes));memset(draws,0,4*sizeof(*draws));memset(materials,0,4*sizeof(*materials));memset(colors,0,4*sizeof(*colors));
 for(unsigned i=0;i<4;++i){nodes[i].object.parent.class_info=HSD_CLASS_INFO(&hsdJObj);nodes[i].u.dobj=&draws[i];draws[i].mobj=&materials[i];materials[i].mat=&colors[i];colors[i].diffuse=(GXColor){1,2,3,4};nodes[i].scale=(Vec3){1,1,1};}
 nodes[0].child=&nodes[1];nodes[1].parent=&nodes[0];nodes[1].next=&nodes[2];nodes[2].parent=&nodes[0];
 nodes[1].flags=JOBJ_INSTANCE;nodes[1].child=&nodes[3];nodes[3].parent=&nodes[1];
 object->hsd_obj=&nodes[0];object->obj_kind=HSD_GObj_JObjKind;
}
static uint32_t function(const char* name)
{for(unsigned i=0;i<source.compiler.bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&source.compiler.bank.tree,declarations[i].name,name))return i;CHECK(false);return 0;}
static bool helper(HSD_GObj* object,const char* name)
{NativeScriptValue result;return nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function(name),NULL,0,&result,&diagnostic)==NativeScriptVM_Ok;}
static NativeScriptRuntimeContext context(HSD_GObj* object,uint32_t kind,uint32_t generation)
{return (NativeScriptRuntimeContext){.source=object,.source_class=kind,.generation=generation,.actor_id=generation,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue read(NativeScriptRuntimeContext* c,uint32_t semantic)
{NativeScriptValue result;CHECK(nativeScriptBindings_Read(c,semantic,NULL,0,&result,&diagnostic));return result;}
static void item_destroy(void* data){(void)data;nativeScriptItem_Detach(item.entity);}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();
 HSD_GObj* object=presentation_create(source.target,true);graph(object);Fighter* fighter=object->user_data;
 fighter->ft_data=&source.assets.character.data;gFtDataList[source.target]=fighter->ft_data;fighter->co_attrs.model_scaling=1.25f;fighter->x34_scale.y=1.5f;
 NativeFighterPoolSlot* pool=nativeFighterPool_Slot(fighter);CHECK(pool);fighter->parts=pool->parts;fighter->parts[0].joint=&nodes[0];
 fighter->x5AC.model_num=1;HSD_DObj* list[]={&draws[0]};fighter->dobj_list.data=list;fighter->dobj_list.count=1;
 NativeScriptRuntimeContext caller=context(object,NativeCallbackSource_Fighter,fighter->x8_spawnNum);CHECK(nativeScriptBindings_SourceFighterLayout(&caller,&diagnostic));
 const u8 costumes[]={0,1,2,3,255};const uint32_t expected[]={0x2cfe02ff,0xff010203,0x04050607,0x08090a0b,0x2cfe02ff};
 for(unsigned i=0;i<5;++i){fighter->x619_costume_id=costumes[i];CHECK(helper(object,"Paint"));
  CHECK(read(&caller,NativeScriptBinding_CostumeIndex).as.i64==costumes[i]);CHECK((uint32_t)presentation_state(object,"result",NULL).as.i64==expected[i]);
  CHECK(presentation_state(object,"installed",NULL).as.u64==1);CHECK(presentation_state(object,"scale",NULL).as.f32==1.875f);
  CHECK((uint32_t)read(&caller,NativeScriptBinding_VisualsOutlineColor).as.i64==0x14151617);CHECK(fighter->x2224_b0);
  CHECK(fighter->x5AC.xC[4]==source.assets.character.visibility_assets[10]);CHECK(read(&caller,NativeScriptBinding_ModelBaseScale).as.f32==1.25f);
  CHECK(colors[3].diffuse.r==1);++cases;
 }
 GXColor diffuse={200,201,202,203},before[4],reference[4];for(unsigned i=0;i<4;++i)before[i]=colors[i].diffuse;
 ftMaterial_800BFB4C(object,&diffuse);for(unsigned i=0;i<4;++i){reference[i]=colors[i].diffuse;colors[i].diffuse=before[i];}
 NativeScriptValue packed={.type=NativeScriptType_I32,.as.i64=(int32_t)UINT32_C(0xc8c9cacb)},result={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(nativeScriptBindings_Invoke(&caller,NativeScriptBinding_VisualsSetDiffuseColor,NULL,0,&packed,1,&result,&diagnostic));
 for(unsigned i=0;i<4;++i)CHECK(!memcmp(&colors[i].diffuse,&reference[i],sizeof(GXColor)));++comparisons;
 // Wide signed and unsigned channels preserve low bytes without F64 rounding.
 NativeScriptValue channel[4]={{.type=NativeScriptType_U64,.as.u64=UINT64_MAX},{.type=NativeScriptType_I64,.as.i64=INT64_MIN+258},{.type=NativeScriptType_U32,.as.u64=0xabcdef01},{.type=NativeScriptType_I32,.as.i64=-1}};
 CHECK(nativeScriptBindings_Invoke(&caller,NativeScriptBinding_ColorRGBA,NULL,0,channel,4,&result,&diagnostic));CHECK(result.type==NativeScriptType_I32&&(uint32_t)result.as.i64==0xff0201ff);++comparisons;
 for(unsigned i=0;i<4;++i){float invalid[]={0,-0.0f,NAN,INFINITY};NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=invalid[i]};float saved=fighter->x34_scale.z;CHECK(nativeScriptBindings_Write(&caller,NativeScriptBinding_ModelInstanceScaleZ,NULL,0,&value,&diagnostic));CHECK(fighter->x34_scale.z==saved);}
 NativeScriptValue wide_slot={.type=NativeScriptType_U64,.as.u64=UINT64_C(0x10000000a)};const FtPartsVisLookup* installed=fighter->x5AC.xC[4];
 CHECK(nativeScriptBindings_Invoke(&caller,NativeScriptBinding_VisualsInstallOutlineVisibility,NULL,0,&wide_slot,1,&result,&diagnostic));CHECK(result.type==NativeScriptType_Bool&&!result.as.u64&&fighter->x5AC.xC[4]==installed);
 source.assets.character.visibility_assets[10]=NULL;NativeScriptValue slot={.type=NativeScriptType_I32,.as.i64=10};CHECK(nativeScriptBindings_Invoke(&caller,NativeScriptBinding_VisualsInstallOutlineVisibility,NULL,0,&slot,1,&result,&diagnostic));CHECK(result.type==NativeScriptType_Bool&&!result.as.u64);++cases;
 HSD_GObj* article_object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(article_object);
 item=(Item){.entity=article_object,.kind=asset.kind,.x1C=900,.native_script_owner=source.program.items.programs[0].owner,.native_article_author_kind=source.target,.xC4_article_data=&article,.xBBC_dynamicBoneTable=&bones};
 GObj_InitUserData(article_object,6,item_destroy,&item);graph(article_object);bones.bones[0]=&nodes[0];bones.bones[1]=&nodes[1];bones.bones[2]=&nodes[3];bones.bones[3]=&nodes[2];
 CHECK(nativeScriptItem_Attach(article_object,&nativeScriptFighter_Program(source.target)->items.programs[0]));NativeScriptValue output;
 CHECK(nativeScriptItem_RunHook(article_object,NativeScriptItemHook_Created,NULL,0,&output)==NativeScriptVM_Ok);
 NativeScriptRuntimeContext item_context=context(article_object,NativeCallbackSource_Item,900);
 CHECK((uint32_t)read(&item_context,NativeScriptBinding_ItemOutlineColor).as.i64==0x1e1f2021);CHECK((uint32_t)read(&item_context,NativeScriptBinding_ItemControllerTint).as.i64==0x28292a2b);
 CHECK(item.x5C8&&item.x5C9==127&&item.xDCF_flag.b3&&item.model_layers==&layers);CHECK(colors[3].diffuse.r==50);
 for(unsigned i=0;i<4;++i)before[i]=colors[i].diffuse;it_80278574(article_object,&diffuse);for(unsigned i=0;i<4;++i){reference[i]=colors[i].diffuse;colors[i].diffuse=before[i];}
 CHECK(nativeScriptBindings_Invoke(&item_context,NativeScriptBinding_ItemSetDiffuseColor,NULL,0,&packed,1,&result,&diagnostic));for(unsigned i=0;i<4;++i)CHECK(!memcmp(&colors[i].diffuse,&reference[i],sizeof(GXColor)));++comparisons;
 NativeScriptValue opacity={.type=NativeScriptType_I32,.as.i64=-1};CHECK(nativeScriptBindings_Write(&item_context,NativeScriptBinding_ItemMaterialOpacity,NULL,0,&opacity,&diagnostic)&&item.x5C9==127);
 opacity.as.i64=256;CHECK(nativeScriptBindings_Write(&item_context,NativeScriptBinding_ItemMaterialOpacity,NULL,0,&opacity,&diagnostic)&&item.x5C9==127);
 article.model_layers=NULL;NativeScriptValue enabled={.type=NativeScriptType_Bool,.as.u64=1};CHECK(nativeScriptBindings_Write(&item_context,NativeScriptBinding_ItemLayered,NULL,0,&enabled,&diagnostic));CHECK(!item.xDCF_flag.b3&&!item.model_layers);article.model_layers=&layers;++cases;
 // Selected identity/root/schema failure must not publish outputs or mutate.
 result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};NativeScriptValue saved=result;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,fighter->x8_spawnNum,&view));
 const NativeScriptStorageParameters* original_parameters=view.storage->parameters;NativeScriptStorageParameters foreign_parameters=*original_parameters;
 view.storage->parameters=&foreign_parameters;CHECK(!nativeScriptBindings_Invoke(&caller,NativeScriptBinding_VisualsSetDiffuseColor,NULL,0,&packed,1,&result,&diagnostic));CHECK(!memcmp(&result,&saved,sizeof(result)));view.storage->parameters=original_parameters;
 channel[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};result=saved;
 CHECK(!nativeScriptBindings_Invoke(&caller,NativeScriptBinding_ColorRGBA,NULL,0,channel,4,&result,&diagnostic));CHECK(!memcmp(&result,&saved,sizeof(result)));++cases;
 const char* invalid[]={"fighter:\n    behavior independent\nfunc Bad() -> int:\n    return color.rgba(1.5,2,3,4)\n",
 "fighter:\n    behavior independent\nfunc Bad():\n    visuals.set_diffuse_color(1.5)\n",
 "fighter:\n    behavior independent\nfunc Bad():\n    costume.index = 2\n"};
 for(unsigned i=0;i<3;++i){NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));const NativeScriptCompileBank* accepted=&source.compiler.bank;
  NativeScriptCompileRequest request={.source=invalid[i],.source_bytes=(uint32_t)strlen(invalid[i])};CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));CHECK(accepted==&source.compiler.bank);nativeSourceBank_Release(&scratch);++cases;}
 HSD_GObjFree(article_object);HSD_GObjFree(object);CHECK(nativeGObjPool.used==0);
 printf("{\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u}\n",presentation_checks,cases,comparisons);return 0;
}
