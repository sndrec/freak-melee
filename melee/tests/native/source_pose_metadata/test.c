#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_source_asset_facts.h>
#include <melee/ft/native_script_bindings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {N=1024,I=4096,F=32,V=64};
typedef struct Buffers {
 char source[16384];NativeScriptSourceNode syntax[N];NativeScriptIRNode expressions[I];uint32_t arguments[I];
 NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
 NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode code[F];
 NativeScriptExecutionFunction execution[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];static const NativeScriptCompileBank* accepted;
static NativeScriptDiagnostic diagnostic;static unsigned checks,builds;static bool entity_context;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"pose metadata %u: %s (%u:%u %s)\n",__LINE__,#v,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}while(0)
static uint32_t hash(const char* text)
{uint32_t result=2166136261u;while(*text){result^=(unsigned char)*text++;result*=16777619u;}return result;}
static uint64_t fingerprint(const void* data,size_t bytes)
{const unsigned char* p=data;uint64_t value=UINT64_C(1469598103934665603);for(size_t i=0;i<bytes;++i){value^=p[i];value*=UINT64_C(1099511628211);}return value;}
static void initialize(unsigned which)
{
 Buffers* b=&buffers[which];memset(b,0,sizeof(*b));banks[which]=(NativeScriptCompileBank){
 .source_storage=b->source,.source_capacity=sizeof(b->source),.tree={.nodes=b->syntax,.capacity=N},
 .expressions={.nodes=b->expressions,.node_capacity=I,.arguments=b->arguments,.argument_capacity=I},
 .declarations={.functions=b->functions,.function_capacity=F,.parameters=b->parameters,.parameter_capacity=V,.values=b->values,.value_capacity=V},
 .locals=b->locals,.local_capacity=V,.instructions=b->instructions,.instruction_capacity=I,.function_code=b->code,.function_code_capacity=F,
 .execution_functions=b->execution,.execution_function_capacity=F,.graph=b->graph,.graph_capacity=F,
 .storage_layout={.fields=b->fields,.field_capacity=V,.owners=b->owners,.owner_capacity=V},.parameter_slots=b->slots,.parameter_slot_capacity=V};
}
static bool metadata(void* opaque,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
 (void)opaque;
 // Controlled entity header validation only to reach real forbidden-owner and
 // borrowed-receiver expression checks. No Item/Effect instance is prepared.
 for(uint32_t node=at,bound=0;node&&node<tree->count&&bound<tree->count;++bound){
  if(tree->nodes[node].kind==NativeScriptSource_Article||tree->nodes[node].kind==NativeScriptSource_Effect)return true;
  node=tree->nodes[node].parent;
 }
 return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);
}
static bool build(unsigned which,const char* source,const NativeSourcePoseGraph* facts,uint32_t count,uint16_t mask)
{
 ++builds;NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
 .pose_graphs=facts,.pose_graph_count=count,.pose_graph_mask=mask,
 .metadata=entity_context?metadata:NULL};
 return nativeScriptCompile_Build(&banks[which],&request,&accepted,&diagnostic);
}
static const char* full=
 "fighter:\n    behavior independent\npose_graph ChainAirRest slot 5 path \"0\"\npose_graph ChainGroundRest slot 4 path \"0\"\n"
 "func All():\n    pose.load_animation(Pose.blend, 305)\n    pose.request_frame(Pose.blend, -0.5)\n"
 "    pose.copy_costume(Pose.blend, 1)\n    pose.evaluate(Pose.blend)\n"
 "    pose.blend_graph(Pose.blend, 1, ChainAirRest, 0.25, 0.75)\n"
 "    pose.copy_graph(Pose.live, 1, ChainGroundRest)\n"
 "    pose.blend_to_live(1, 0.4, 0.6)\n    pose.copy_to_live(1)\n"
 "action Hooks:\n    on load:\n        All()\n";
static NativeSourcePoseGraph facts[16]={ {.name=305010971,.slot=4,.path="0"},{.name=1363947600,.slot=5,.path="0"} };
static void successful(void)
{
 initialize(0);CHECK(build(0,full,facts,2,0x30));CHECK(accepted==&banks[0]&&accepted->valid);
 CHECK(banks[0].pose_symbol_count==2&&banks[0].pose_graph_count==2&&banks[0].pose_graph_mask==0x30);
 CHECK(banks[0].pose_symbols[0].row==1&&banks[0].pose_symbols[1].row==0);
 unsigned pose_calls=0,selectors=0;
 for(uint32_t i=0;i<banks[0].code.instruction_count;++i){
  NativeScriptInstruction* instruction=&buffers[0].instructions[i];
  if(instruction->opcode==NativeScriptBC_Invoke)++pose_calls;
  if(instruction->opcode==NativeScriptBC_Constant&&instruction->type==NativeScriptType_PoseGraph){CHECK(instruction->immediate<2);++selectors;}
 }
 CHECK(pose_calls==8&&selectors==2);
 for(uint32_t node=1;node<banks[0].tree.count;++node)if(nativeScriptPoseMetadata_Owns(&banks[0].tree,node))
  CHECK(nativeScriptPoseMetadata_Validate(&banks[0],&banks[0].tree,node,&diagnostic));
 NativeScriptSourceTree unrelated=banks[0].tree;
 CHECK(!nativeScriptPoseMetadata_Validate(&banks[0],&unrelated,2,&diagnostic));
 NativeScriptIRNode node={.kind=NativeScriptIR_Name,.text_offset=banks[0].pose_symbols[0].name.offset,.text_length=banks[0].pose_symbols[0].name.length};
 NativeScriptPoseResolverContext context={.bank=&banks[0],.context_mask=NativeScriptBindings_Fighter};NativeScriptIRResolution result;
 CHECK(nativeScriptBindingPose_TryResolve(&context,&banks[0].expressions,&node,&result,&diagnostic)==1);
 CHECK(result.type==NativeScriptType_PoseGraph&&result.literal_bits==1&&result.flags==NativeScriptFlag_Literal&&result.semantic==NATIVE_SCRIPT_SEMANTIC_NONE);
 for(unsigned owner=0;owner<3;++owner){context.owner=owner;context.context_mask=owner==1?NativeScriptBindings_Article:owner==2?NativeScriptBindings_Effect:NativeScriptBindings_Article;
  CHECK(nativeScriptBindingPose_TryResolve(&context,&banks[0].expressions,&node,&result,&diagnostic)==-1);}
}
static void rejected(const char* source,const NativeSourcePoseGraph* rows,uint32_t count,uint16_t mask)
{
 uint64_t old_bank=fingerprint(&banks[0],sizeof(banks[0])),old_buffer=fingerprint(&buffers[0],sizeof(buffers[0]));
 initialize(1);CHECK(!build(1,source,rows,count,mask));CHECK(!banks[1].valid&&accepted==&banks[0]);
 CHECK(diagnostic.message[0]);CHECK(old_bank==fingerprint(&banks[0],sizeof(banks[0]))&&old_buffer==fingerprint(&buffers[0],sizeof(buffers[0])));
}
static void failures(void)
{
 const char* declarations[]={
 "pose_graph ChainGroundRest slot 16 path \"0\"","pose_graph ChainGroundRest slot -1 path \"0\"",
 "pose_graph ChainGroundRest slot 4 path \"256\"","pose_graph ChainGroundRest slot 4 path \"0/\"",
 "pose_graph ChainGroundRest slot 4 path \"0//1\"","pose_graph ChainGroundRest slot 4 path \"0000\"",
 "pose_graph ChainGroundRest slot 4 path \"0/0/0/0/0/0/0/0/0/0/0/0/0\"",
 "pose_graph ChainGroundRest slot 4 path \"0\" extra","pose_graph ChainGroundRest path \"0\" slot 4",
 "pose_graph 7bad slot 4 path \"0\"","pose_graph ChainGroundRest slot 5 path \"0\"",
 "pose_graph ChainGroundRest slot 4 path \"1\"","pose_graph Unknown slot 4 path \"0\"",
 "pose_graph ChainGroundRest slot 4 path \"0\"\npose_graph ChainGroundRest slot 4 path \"0\""};
 char source[4096];for(unsigned i=0;i<sizeof(declarations)/sizeof(*declarations);++i){
  snprintf(source,sizeof(source),"fighter:\n    behavior independent\n%s\nfunc Run():\n    pass\n",declarations[i]);rejected(source,facts,2,0x30);}
 rejected(full,NULL,0,0);rejected(full,NULL,2,0x30);rejected(full,facts,17,0x30);rejected(full,facts,2,0x10);
 rejected("fighter:\n    behavior independent\nfunc Prior():\n    pass\npose_graph ChainGroundRest slot 4 path \"0\"\n",facts,2,0x30);
 rejected("fighter:\n    behavior independent\n    pose_graph ChainGroundRest slot 4 path \"0\"\nfunc Run():\n    pass\n",facts,2,0x30);
 NativeSourcePoseGraph changed[2];memcpy(changed,facts,sizeof(changed));changed[1].name=changed[0].name;rejected(full,changed,2,0x30);
 memcpy(changed,facts,sizeof(changed));changed[0].name=0;rejected(full,changed,2,0x30);
 memcpy(changed,facts,sizeof(changed));memset(changed[0].path,'0',sizeof(changed[0].path));rejected(full,changed,2,0x30);
 const char* collisions[]={"ChainGroundRest","state","position","Pose","true","false","None","and","or","not","creator","owner","current_owner","holder"};
 for(unsigned i=0;i<sizeof(collisions)/sizeof(*collisions);++i){NativeSourcePoseGraph row={.name=hash(collisions[i]),.slot=4,.path="0"};
  snprintf(source,sizeof(source),"fighter:\n    behavior independent\npose_graph %s slot 4 path \"0\"\n%s\nfunc Run():\n    pass\n",collisions[i],i==0?"state int ChainGroundRest":"");
  rejected(source,&row,1,0x10);}
 rejected("fighter:\n    behavior independent\npose_graph ChainGroundRest slot 4 path \"0\"\nfunc ChainGroundRest():\n    pass\n",facts,2,0x30);
 CHECK(hash("costarring")==hash("liquid"));NativeSourcePoseGraph collision={.name=hash("costarring"),.slot=4,.path="0"};
 rejected("fighter:\n    behavior independent\npose_graph costarring slot 4 path \"0\"\npose_graph liquid slot 4 path \"0\"\nfunc Run():\n    pass\n",&collision,1,0x10);
 const char* operations[]={"pose.copy_graph(Pose.live, 1, 0)","pose.copy_graph(Pose.live, 1, ChainGroundRest + 1)",
  "pose.copy_graph(Pose.live, 1, Unknown)","pose.copy_graph(0, 1, ChainGroundRest)","pose.copy_graph(Pose.live, 1.0, ChainGroundRest)",
  "pose.load_animation(Pose.live, 305.0)","pose.request_frame(Pose.live, ChainGroundRest)","int(ChainGroundRest)"};
 for(unsigned i=0;i<sizeof(operations)/sizeof(*operations);++i){snprintf(source,sizeof(source),"fighter:\n    behavior independent\npose_graph ChainGroundRest slot 4 path \"0\"\nfunc Run():\n    %s\n",operations[i]);rejected(source,facts,2,0x30);}
}
static void ownership(void)
{
 const char* source[]={
 "fighter:\n    behavior independent\npose_graph ChainGroundRest slot 4 path \"0\"\nfunc Run():\n    creator.pose.copy_graph(Pose.live, 1, ChainGroundRest)\n",
 "fighter:\n    behavior independent\npose_graph ChainGroundRest slot 4 path \"0\"\naction Probe:\n    on grabbed(opponent):\n        opponent.pose.copy_graph(Pose.live, 1, ChainGroundRest)\n",
 "fighter:\n    behavior independent\npose_graph ChainGroundRest slot 4 path \"0\"\narticle Probe kind 100 states 1:\n    func Run():\n        creator.pose.request_frame(Pose.live, 0)\n",
 "fighter:\n    behavior independent\npose_graph ChainGroundRest slot 4 path \"0\"\narticle Probe kind 100 states 1:\n    func Run():\n        let int graph = ChainGroundRest\n",
 "fighter:\n    behavior independent\npose_graph ChainGroundRest slot 4 path \"0\"\nmodel_effect Probe gfx 1150:\n    func Run():\n        let int graph = ChainGroundRest\n"
 };
 entity_context=true;
 for(unsigned i=0;i<sizeof(source)/sizeof(*source);++i){rejected(source[i],facts,2,0x30);
  if(i>=3)CHECK(strstr(diagnostic.message,"root Fighter ownership"));}
 entity_context=false;
}
static void boundary(void)
{
 // Declared-source ordering and owned row order are deliberately independent.
 NativeSourcePoseGraph reversed[]={facts[1],facts[0]};initialize(1);const NativeScriptCompileBank* previous=accepted;
 CHECK(build(1,full,reversed,2,0x30));CHECK(banks[1].pose_symbols[0].row==0&&banks[1].pose_symbols[1].row==1);accepted=previous;
 NativeSourcePoseGraph rows[16];char source[8192]="fighter:\n    behavior independent\n";
 for(unsigned i=0;i<16;++i){char name[32],line[96];snprintf(name,sizeof(name),"Graph%u",i);rows[i]=(NativeSourcePoseGraph){.name=hash(name),.slot=(uint16_t)i};snprintf(line,sizeof(line),"pose_graph %s slot %u path \"\"\n",name,i);strcat(source,line);}
 strcat(source,"func Run():\n    pose.copy_graph(Pose.live, 1, Graph15)\n");initialize(1);CHECK(build(1,source,rows,16,UINT16_MAX));
 CHECK(banks[1].pose_symbol_count==16&&banks[1].pose_symbols[15].row==15);accepted=previous;
 char extra[8192];size_t prefix=(size_t)(strstr(source,"func Run():")-source);
 snprintf(extra,sizeof(extra),"%.*spose_graph Beyond slot 0 path \"\"\nfunc Run():\n    pass\n",(int)prefix,source);rejected(extra,rows,16,UINT16_MAX);
 CHECK(strstr(diagnostic.message,"capacity"));
 // Reuse clears all selectors when compiling an ordinary graph-free source.
 CHECK(build(1,"fighter:\n    behavior independent\nfunc Run():\n    pass\n",NULL,0,0));
 CHECK(banks[1].pose_symbol_count==0&&banks[1].pose_graph_count==0&&banks[1].pose_graph_mask==0);accepted=previous;
}
int main(void)
{successful();failures();ownership();boundary();printf("{\"status\":\"PASS owned pose metadata\",\"checks\":%u,\"builds\":%u}\n",checks,builds);return 0;}
