#include "native_script_pose_metadata.h"
#include "native_script_compile.h"
#include "native_script_bindings.h"
#include "native_script_constants.h"
#include "native_source_asset_facts.h"
#include "native_fighter_pose.h"
#include <stdio.h>
#include <string.h>

typedef struct PoseDeclaration {NativeScriptSourceSpan name;uint32_t hash,slot;char path[64];} PoseDeclaration;
typedef struct Cursor {const NativeScriptSourceTree* tree;NativeScriptSourceSpan span;uint32_t at;} Cursor;
static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan source,const char* message)
{if(d){d->source=source;snprintf(d->message,sizeof(d->message),"%s",message);}return false;}
static void space(Cursor* c)
{while(c->at<c->span.length&&(c->tree->source[c->span.offset+c->at]==' '||c->tree->source[c->span.offset+c->at]=='\t'))++c->at;}
static NativeScriptSourceSpan word(Cursor* c)
{
 space(c);uint32_t first=c->at;
 while(c->at<c->span.length&&c->tree->source[c->span.offset+c->at]!=' '&&c->tree->source[c->span.offset+c->at]!='\t')++c->at;
 NativeScriptSourceSpan span=c->span;span.offset+=first;span.column+=first;span.length=c->at-first;return span;
}
static bool equal(const NativeScriptSourceTree* tree,NativeScriptSourceSpan a,NativeScriptSourceSpan b)
{return a.length==b.length&&a.offset<=tree->source_bytes&&a.length<=tree->source_bytes-a.offset&&
 b.offset<=tree->source_bytes&&b.length<=tree->source_bytes-b.offset&&!memcmp(tree->source+a.offset,tree->source+b.offset,a.length);}
static uint32_t hash(const char* bytes,uint32_t length)
{uint32_t value=UINT32_C(2166136261);for(uint32_t i=0;i<length;++i){value^=(uint8_t)bytes[i];value*=UINT32_C(16777619);}return value;}
static bool identifier(const char* bytes,uint32_t length)
{
 if(!length||length>31)return false;
 for(uint32_t i=0;i<length;++i){unsigned char c=bytes[i];if(!(c=='_'||(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(i&&c>='0'&&c<='9')))return false;}
 return true;
}
bool nativeScriptPoseMetadata_Owns(const NativeScriptSourceTree* tree,uint32_t at)
{return tree&&tree->nodes&&at<tree->count&&tree->nodes[at].kind==NativeScriptSource_Metadata&&
 nativeScriptSource_SpanEquals(tree,tree->nodes[at].name,"pose_graph");}
static bool declaration(const NativeScriptSourceTree* tree,uint32_t at,PoseDeclaration* out,NativeScriptDiagnostic* d)
{
 const NativeScriptSourceNode* node=&tree->nodes[at];Cursor c={tree,node->value,0};
 if(node->parent||node->opens||node->value.offset>tree->source_bytes||node->value.length>tree->source_bytes-node->value.offset)
  return error(d,node->source,"Pose graph must be a root declaration without a body");
 PoseDeclaration value={0};value.name=word(&c);
 if(!identifier(tree->source+value.name.offset,value.name.length))return error(d,value.name,"Pose graph needs a bounded ordinary identifier");
 if(!nativeScriptSource_SpanEquals(tree,word(&c),"slot"))return error(d,node->value,"Expected pose_graph Name slot N path \"child/path\"");
 NativeScriptSourceSpan slot=word(&c);uint32_t parsed=0;
 if(!slot.length)return error(d,slot,"Pose graph slot must be decimal 0..15");
 for(uint32_t i=0;i<slot.length;++i){char digit=tree->source[slot.offset+i];if(digit<'0'||digit>'9')return error(d,slot,"Pose graph slot must be decimal 0..15");parsed=parsed*10+(unsigned)(digit-'0');if(parsed>=16)return error(d,slot,"Pose graph slot must be decimal 0..15");}
 if(parsed>=16)return error(d,slot,"Pose graph slot must be decimal 0..15");value.slot=parsed;
 if(!nativeScriptSource_SpanEquals(tree,word(&c),"path"))return error(d,node->value,"Expected pose graph path keyword");
 space(&c);if(c.at>=c.span.length||tree->source[c.span.offset+c.at++]!='"')return error(d,node->value,"Pose graph path must be quoted");
 uint32_t first=c.at;while(c.at<c.span.length&&tree->source[c.span.offset+c.at]!='"')++c.at;
 uint32_t length=c.at-first;
 if(c.at==c.span.length||length>=sizeof(value.path))return error(d,node->value,"Pose graph path is missing its quote or exceeds 63 bytes");
 memcpy(value.path,tree->source+c.span.offset+first,length);value.path[length]=0;++c.at;space(&c);
 if(c.at!=c.span.length||!nativeFighterPose_PathValid(value.path))return error(d,node->value,"Pose graph has trailing syntax or an invalid bounded child path");
 value.hash=hash(tree->source+value.name.offset,value.name.length);
 if(!value.hash)return error(d,value.name,"Pose graph name has invalid zero identity");*out=value;return true;
}
bool nativeScriptPoseMetadata_Prepare(NativeScriptCompileBank* bank,const NativeSourcePoseGraph* graphs,
 uint32_t count,uint16_t mask,NativeScriptDiagnostic* d)
{
 if(!bank||!bank->tree.source||!bank->tree.nodes||count>NATIVE_SCRIPT_POSE_SYMBOLS||(count&&!graphs))
  return error(d,(NativeScriptSourceSpan){0},"Missing or oversized owned pose graph inputs");
 NativeScriptPoseSymbol symbols[NATIVE_SCRIPT_POSE_SYMBOLS]={0};uint32_t used=0;bool code_seen=false;
 for(uint32_t i=0;i<count;++i){
  if(!graphs[i].name||graphs[i].slot>=16||!(mask&(1u<<graphs[i].slot))||
     !memchr(graphs[i].path,0,sizeof(graphs[i].path))||!nativeFighterPose_PathValid(graphs[i].path))
   return error(d,(NativeScriptSourceSpan){0},"Owned pose graph facts have invalid identity, slot or path");
  for(uint32_t j=0;j<i;++j)if(graphs[i].name==graphs[j].name)return error(d,(NativeScriptSourceSpan){0},"Owned pose graph facts have duplicate identity");
 }
 for(uint32_t at=1;at<bank->tree.count;++at){
  const NativeScriptSourceNode* node=&bank->tree.nodes[at];
  if(nativeScriptPoseMetadata_Owns(&bank->tree,at)){
   if(code_seen)return error(d,node->source,"Pose graph declarations must precede authored code blocks");
   if(used==NATIVE_SCRIPT_POSE_SYMBOLS)return error(d,node->source,"Pose graph declaration capacity exceeds 16");
   PoseDeclaration parsed;if(!declaration(&bank->tree,at,&parsed,d))return false;
   for(uint32_t i=0;i<used;++i)if(hash(bank->tree.source+symbols[i].name.offset,symbols[i].name.length)==parsed.hash)
    return error(d,parsed.name,"Duplicate or colliding pose graph declaration");
   uint32_t row=count;for(uint32_t i=0;i<count;++i)if(graphs[i].name==parsed.hash){row=i;break;}
   if(row==count||graphs[row].slot!=parsed.slot||strcmp(graphs[row].path,parsed.path))
    return error(d,parsed.name,"Pose graph declaration does not match its exact owned facts tuple");
   symbols[used++]=(NativeScriptPoseSymbol){parsed.name,node->source,row};
  }
  if(node->kind==NativeScriptSource_Function||node->kind==NativeScriptSource_Action||node->kind==NativeScriptSource_Move||
     node->kind==NativeScriptSource_Article||node->kind==NativeScriptSource_Item||node->kind==NativeScriptSource_Effect)code_seen=true;
 }
 memcpy(bank->pose_symbols,symbols,sizeof(symbols));bank->pose_symbol_count=used;bank->pose_graph_count=count;bank->pose_graph_mask=mask;return true;
}
bool nativeScriptPoseMetadata_Validate(const NativeScriptCompileBank* bank,const NativeScriptSourceTree* tree,
 uint32_t at,NativeScriptDiagnostic* d)
{
 if(!bank||tree!=&bank->tree||!nativeScriptPoseMetadata_Owns(tree,at)||
    bank->pose_symbol_count>NATIVE_SCRIPT_POSE_SYMBOLS||bank->pose_graph_count>NATIVE_SCRIPT_POSE_SYMBOLS)
  return error(d,tree&&at<tree->count?tree->nodes[at].source:(NativeScriptSourceSpan){0},"Pose graph is not part of this prepared source bank");
 PoseDeclaration parsed;if(!declaration(tree,at,&parsed,d))return false;
 for(uint32_t i=0;i<bank->pose_symbol_count;++i){const NativeScriptPoseSymbol* symbol=&bank->pose_symbols[i];
  if(symbol->source.offset==tree->nodes[at].source.offset&&symbol->source.length==tree->nodes[at].source.length&&
     symbol->name.offset==parsed.name.offset&&symbol->name.length==parsed.name.length&&
     symbol->row<bank->pose_graph_count&&(bank->pose_graph_mask&(1u<<parsed.slot)))return true;
 }
 return error(d,parsed.name,"Pose graph declaration lacks its validated owned row");
}
bool nativeScriptPoseMetadata_CheckSymbols(const NativeScriptCompileBank* bank,NativeScriptDiagnostic* d)
{
 if(!bank||bank->pose_symbol_count>NATIVE_SCRIPT_POSE_SYMBOLS)return error(d,(NativeScriptSourceSpan){0},"Invalid prepared pose symbol count");
 const NativeScriptSourceTree* tree=&bank->tree;
 for(uint32_t i=0;i<bank->pose_symbol_count;++i){NativeScriptSourceSpan name=bank->pose_symbols[i].name;
  for(uint32_t j=0;j<bank->declarations.function_count;++j)if(!bank->declarations.functions[j].owner&&equal(tree,name,bank->declarations.functions[j].name))
   return error(d,name,"Pose graph collides with a root function symbol");
  for(uint32_t j=0;j<bank->declarations.value_count;++j)if(!bank->declarations.values[j].owner&&equal(tree,name,bank->declarations.values[j].name))
   return error(d,name,"Pose graph collides with a root value symbol");
  for(uint32_t j=1;j<tree->count;++j)if(!tree->nodes[j].parent&&tree->nodes[j].kind!=NativeScriptSource_Metadata&&equal(tree,name,tree->nodes[j].name))
   return error(d,name,"Pose graph collides with a root declaration symbol");
  static const char* const reserved[]={"attrs","state","types","event","true","false","None","and","or","not",
   "creator","owner","current_owner","holder"};
  for(unsigned j=0;j<sizeof(reserved)/sizeof(*reserved);++j)if(nativeScriptSource_SpanEquals(tree,name,reserved[j]))return error(d,name,"Pose graph collides with a reserved source namespace");
  for(uint32_t j=1;j<NativeScriptBinding_Count;++j){const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(j);
   if(descriptor&&!descriptor->parent&&descriptor->name&&nativeScriptSource_SpanEquals(tree,name,descriptor->name))return error(d,name,"Pose graph collides with a native root symbol");}
  NativeScriptIRNode node={.kind=NativeScriptIR_Name,.text_offset=name.offset,.text_length=name.length,.source=name};
  NativeScriptIRArena arena={.source=tree->source,.source_bytes=tree->source_bytes,.nodes=&node,.node_count=1};
  NativeScriptIRResolution result;NativeScriptDiagnostic ignored={0};
  if(nativeScriptConstants_TryResolve(NULL,&arena,&node,&result,&ignored))return error(d,name,"Pose graph collides with a constant symbol");
 }
 return true;
}
int nativeScriptBindingPose_TryResolve(void* opaque,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
 NativeScriptIRResolution* result,NativeScriptDiagnostic* d)
{
 const NativeScriptPoseResolverContext* context=opaque;
 if(!context||!context->bank||!arena||!node||!result||node->kind!=NativeScriptIR_Name)return 0;
 const NativeScriptCompileBank* bank=context->bank;
 if(bank->pose_symbol_count>NATIVE_SCRIPT_POSE_SYMBOLS){error(d,node->source,"Invalid prepared pose symbol table");return -1;}
 for(uint32_t i=0;i<bank->pose_symbol_count;++i){const NativeScriptPoseSymbol* symbol=&bank->pose_symbols[i];
  if(symbol->name.length!=node->text_length||symbol->name.offset>bank->tree.source_bytes||symbol->name.length>bank->tree.source_bytes-symbol->name.offset||
     node->text_offset>arena->source_bytes||node->text_length>arena->source_bytes-node->text_offset||
     memcmp(bank->tree.source+symbol->name.offset,arena->source+node->text_offset,node->text_length))continue;
  if(context->owner||context->context_mask!=NativeScriptBindings_Fighter||symbol->row>=bank->pose_graph_count){error(d,node->source,"Pose graph selector requires its declared root Fighter ownership");return -1;}
  *result=(NativeScriptIRResolution){.type=NativeScriptType_PoseGraph,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,.flags=NativeScriptFlag_Literal,.literal_bits=symbol->row};return 1;
 }
 return 0;
}
