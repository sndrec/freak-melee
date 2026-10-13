#include <melee/ft/native_source_assets.h>
#include <melee/ft/native_source_entity_assets.h>
#include <melee/ft/native_source_bank.h>
#include <melee/it/native_item_model_admission.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/object.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, cases, joints, model_draws;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"render import %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void* read_file(const char* directory,const char* name,size_t* size)
{
 char path[1024];snprintf(path,sizeof(path),"%s/%s",directory,name);
 FILE* file=fopen(path,"rb");CHECK(file);CHECK(!fseek(file,0,SEEK_END));long bytes=ftell(file);CHECK(bytes>0&&!fseek(file,0,SEEK_SET));
 void* result=malloc((size_t)bytes);CHECK(result&&fread(result,1,(size_t)bytes,file)==(size_t)bytes);fclose(file);*size=(size_t)bytes;return result;
}
static u32 walk(HSD_JObj* root)
{
 u32 count=0;HSD_JObj* at=root;
 while(at){CHECK(++count<=100);
  if(at->child)at=at->child;else if(at->next)at=at->next;else{while(at->parent&&!at->parent->next)at=at->parent;at=at->parent?at->parent->next:NULL;}
 }
 return count;
}
static void no_data_admission(const NativeSourceImportedAssets* accepted,const NativeSourceAssetInput* input,const NativeScriptSourceTree* tree,const char* declaration)
{
 NativeSourceImportedAssets* fixture=calloc(1,sizeof(*fixture));NativeSourceEntityAssets* result=calloc(1,sizeof(*result));CHECK(fixture&&result);
 fixture->ready=true;fixture->character.import=accepted->character.import;
 for(unsigned i=0;i<accepted->facts.article_count;++i)if(!strcmp(accepted->facts.articles[i].declaration,declaration)) {
  fixture->facts.articles[0]=accepted->facts.articles[i];fixture->article_kinds[0]=accepted->article_kinds[i];fixture->facts.article_count=1;
 }
 CHECK(fixture->facts.article_count==1);HSD_Joint joints[101]={0};for(unsigned i=0;i<100;++i)joints[i].child=&joints[i+1];
 ItemModelDesc model={.x0_joint=joints,.x4_bone_count=1};Article article={.x10_modelDesc=&model};
 fixture->character.fighter_items[fixture->facts.articles[0].slot]=&article;NativeSourceResources resources={0};
 CHECK(!nativeSourceEntityAssets_Prepare(result,fixture,input,&resources,tree,&diagnostic));CHECK(!result->count&&!result->blueprint_count);++cases;
 joints[99].child=NULL;CHECK(nativeSourceEntityAssets_Prepare(result,fixture,input,&resources,tree,&diagnostic));CHECK(result->count==1);nativeSourceEntityAssets_Release(result);++cases;
 model.x0_joint=NULL;CHECK(nativeSourceEntityAssets_Prepare(result,fixture,input,&resources,tree,&diagnostic));CHECK(result->count==1);nativeSourceEntityAssets_Release(result);++cases;
 free(result);free(fixture);
}
static void probe(const char* directory,const char* declaration)
{
 NativeSourceImportedAssets* imported=calloc(1,sizeof(*imported));NativeSourceEntityAssets* entities=calloc(1,sizeof(*entities));
 NativeScriptItemPrograms* prepared=calloc(1,sizeof(*prepared));NativeSourceBank bank={0};NativeSourceAssetFacts facts;
 size_t fact_size;void* record=read_file(directory,"assets.fma",&fact_size);
 CHECK(nativeSourceAssets_Read(record,fact_size,&facts,&diagnostic));free(record);
 NativeSourceAssetInput input={0};input.fighter=read_file(directory,"fighter.dat",&input.fighter_bytes);
 input.animation=read_file(directory,"animation.dat",&input.animation_bytes);input.costume=read_file(directory,"costume.dat",&input.costume_bytes);
 CHECK(nativeSourceAssets_Import(&facts,&input,Ft_Kind_CustomFirst,imported,&diagnostic));
 char source[2048];snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle %s states 1:\n    render standard\n    state int observed\n    on spawned:\n        item.state.observed = 1\n    state 0 anim -1:\n",declaration);
 NativeScriptSourceNode nodes[128];NativeScriptSourceTree tree={.source=source,.source_bytes=(u32)strlen(source),.nodes=nodes,.capacity=128};
 CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeSourceResources resources={0};
 CHECK(nativeSourceEntityAssets_Prepare(entities,imported,&input,&resources,&tree,&diagnostic));
 no_data_admission(imported,&input,&tree,declaration);
 const NativeScriptItemAsset* binding=NULL;for(unsigned i=0;i<entities->count;++i)if(!strcmp(entities->rows[i].declaration,declaration))binding=&entities->rows[i];
 CHECK(binding&&binding->article&&binding->article->x10_modelDesc&&binding->article->x10_modelDesc->x0_joint);
 CHECK(nativeSourceBank_Allocate(&bank,&diagnostic));
 NativeScriptItemMetadataContext context={.assets=entities->rows,.asset_count=entities->count,.source_facts=&imported->facts};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(u32)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&context};
 const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&bank.bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptItemPrograms_Prepare(&bank.bank,Ft_Kind_CustomFirst,83,&context,prepared,&diagnostic));
 CHECK(prepared->count==1&&prepared->programs[0].drawing==NativeScriptItemDrawing_Standard&&prepared->programs[0].article==binding->article);
 HSD_DatImport admission=imported->character.import;u32 predicted=0;CHECK(nativeItemModel_Admit(&admission,binding->article->x10_modelDesc,0,&predicted));
 HSD_JObj* actual=HSD_JObjLoadJoint(binding->article->x10_modelDesc->x0_joint);CHECK(actual&&walk(actual)==predicted);
 CHECK(actual->child || HSD_JObjGetDObj(actual));joints+=predicted;
 HSD_JObj* at=actual;while(at){if(HSD_JObjGetDObj(at))++model_draws;if(at->child)at=at->child;else if(at->next)at=at->next;else{while(at->parent&&!at->parent->next)at=at->parent;at=at->parent?at->parent->next:NULL;}}
 HSD_JObjRemoveAll(actual);
 // Validate explicit and omitted policies, and reject unsupported/duplicate
 // policy and secondary model declarations without publishing candidate data.
 NativeScriptItemProgram saved=prepared->programs[0];
 const char* bad[]={"render custom","render standard\n    render standard","model_root second 36","render standard extra"};
 for(unsigned i=0;i<4;++i){snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle %s states 1:\n    %s\n    state 0 anim -1:\n",declaration,bad[i]);
  request.source_bytes=(u32)strlen(source);NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));accepted=&bank.bank;
  CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&accepted==&bank.bank);CHECK(!memcmp(&saved,&prepared->programs[0],sizeof(saved)));nativeSourceBank_Release(&scratch);++cases;
 }
 snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle %s states 1:\n    state 0 anim -1:\n",declaration);
 request.source_bytes=(u32)strlen(source);NativeSourceBank omitted={0};CHECK(nativeSourceBank_Allocate(&omitted,&diagnostic));accepted=NULL;
 CHECK(nativeScriptCompile_Build(&omitted.bank,&request,&accepted,&diagnostic));NativeScriptItemPrograms* other=calloc(1,sizeof(*other));
 CHECK(nativeScriptItemPrograms_Prepare(&omitted.bank,Ft_Kind_CustomFirst,84,&context,other,&diagnostic));CHECK(other->programs[0].drawing==saved.drawing&&other->programs[0].article==saved.article);
 free(other);nativeSourceBank_Release(&omitted);nativeSourceBank_Release(&bank);nativeSourceEntityAssets_Release(entities);nativeSourceAssets_Release(imported);
 free((void*)input.fighter);free((void*)input.animation);free((void*)input.costume);free(entities);free(imported);free(prepared);++cases;
}
int main(int argc,char** argv)
{
 CHECK(argc==3);void* heap=malloc(32*1024*1024);CHECK(heap);void* start=OSInitAlloc(heap,(u8*)heap+32*1024*1024,1);
 OSHeapHandle handle=OSCreateHeap(start,(u8*)heap+32*1024*1024);CHECK(handle>=0);HSD_SetHeap(handle);OSSetCurrentHeap(handle);HSD_ObjSetHeap(32*1024*1024,NULL);HSD_ObjInit();
 probe(argv[1],"It_Kind_Link_Arrow");probe(argv[2],"It_Kind_CLink_Arrow");
 printf("{\"checks\":%u,\"cases\":%u,\"native_joints\":%u,\"model_draws\":%u}\n",checks,cases,joints,model_draws);return 0;
}
