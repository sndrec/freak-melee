#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "native_source_resource_tool.h"
#include <melee/ft/native_source_resources.h>
#include <melee/ft/native_source_item_blueprint.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_package.h>
#include <melee/lb/native_disc_archive.h>
#include <aurora/dvd.h>
#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PathBytes=1024 };
static bool path(char output[PathBytes],const char* project,const char* name)
{int n=project?snprintf(output,PathBytes,"%s/%s",project,name):-1;return n>0&&n<PathBytes;}
static bool number(const char* text,uint32_t maximum,uint32_t* result)
{
    if(!text||text[0]<'0'||text[0]>'9')return false;
    char* end;errno=0;unsigned long long value=strtoull(text,&end,0);
    if(errno||*end||value>maximum)return false;*result=(uint32_t)value;return true;
}
static bool read(const char* path,size_t maximum,uint8_t** data,size_t* size)
{
    FILE* file=fopen(path,"rb");if(!file)return false;
    bool ok=!fseek(file,0,SEEK_END);long length=ok?ftell(file):-1;
    ok=ok&&length>0&&(size_t)length<=maximum&&!fseek(file,0,SEEK_SET);
    uint8_t* bytes=ok?malloc((size_t)length):NULL;
    ok=bytes&&fread(bytes,1,(size_t)length,file)==(size_t)length&&!ferror(file);
    if(fclose(file))ok=false;if(!ok){free(bytes);return false;}
    *data=bytes;*size=(size_t)length;return true;
}
static bool save(const char* destination,const void* bytes,size_t size)
{
    char temporary[PathBytes];
    if(!destination||!destination[0])return false;
    HANDLE file=INVALID_HANDLE_VALUE;
    for(unsigned attempt=0;attempt<16;++attempt) {
        int n=snprintf(temporary,sizeof(temporary),"%s.%lu.%llu.%u.tmp",destination,
            (unsigned long)GetCurrentProcessId(),(unsigned long long)GetTickCount64(),attempt);
        if(n<=0||(size_t)n>=sizeof(temporary))return false;
        file=CreateFileA(temporary,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE)break;
        DWORD reason=GetLastError();if(reason!=ERROR_FILE_EXISTS&&reason!=ERROR_ALREADY_EXISTS)return false;
    }
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;bool ok=size<=UINT32_MAX&&WriteFile(file,bytes,(DWORD)size,&written,NULL)&&written==size&&FlushFileBuffers(file);
    if(!CloseHandle(file))ok=false;
    if(ok)ok=MoveFileExA(temporary,destination,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileA(temporary); // Only this successfully-created temporary.
    return ok;
}
static int put(const char* project,const char* name,NativeSourceResourceKind kind,const void* bytes,size_t size)
{
    NativeFighterProjectInfo info;char filename[PathBytes],diagnostic[256]={0};
    if(!nativeSourceResources_NameValid(name)||!nativeFighterProject_ReadInfo(project,&info)||!path(filename,project,"resources.fmr")) {
        fputs("Resource import needs a source draft and a canonical logical resource name.\n",stderr);return 1;
    }
    uint8_t* previous=NULL;size_t previous_bytes=0;NativeSourceResources view={0};
    DWORD attributes=GetFileAttributesA(filename);
    bool exists=attributes!=INVALID_FILE_ATTRIBUTES;
    DWORD reason=exists?0:GetLastError();
    bool ok=exists?!(attributes&FILE_ATTRIBUTE_DIRECTORY)&&read(filename,NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES,&previous,&previous_bytes):
        reason==ERROR_FILE_NOT_FOUND;
    if(!ok||!nativeSourceResources_Read(previous,previous_bytes,&view,diagnostic,sizeof(diagnostic))) {
        fprintf(stderr,"Existing resource bundle could not be read: %s\n",diagnostic);free(previous);return 1;
    }
    NativeSourceResourceInput entries[NATIVE_SOURCE_RESOURCE_ENTRIES];unsigned count=0;bool replaced=false;
    for(unsigned i=0;i<view.count;++i) {
        const NativeSourceResourceEntry* old=&view.entries[i];
        bool selected=!strcmp(old->name,name);
        entries[count++]=selected?(NativeSourceResourceInput){name,bytes,size,kind}:
            (NativeSourceResourceInput){old->name,old->bytes,old->size,old->kind};
        replaced|=selected;
    }
    if(!replaced) {
        if(count==NATIVE_SOURCE_RESOURCE_ENTRIES){fputs("Resource directory is full.\n",stderr);free(previous);return 1;}
        entries[count++]=(NativeSourceResourceInput){name,bytes,size,kind};
    }
    size_t output_size=0,written=0;
    ok=nativeSourceResources_Measure(entries,count,&output_size,diagnostic,sizeof(diagnostic));
    uint8_t* output=ok?malloc(output_size):NULL;
    ok=output&&nativeSourceResources_Write(entries,count,output,output_size,&written,diagnostic,sizeof(diagnostic));
    if(ok&&!save(filename,output,written)){snprintf(diagnostic,sizeof(diagnostic),"Atomic resources.fmr replacement failed");ok=false;}
    if(ok)printf("Imported source resource %s: %zu bytes (%s).\n",name,size,kind==NativeSourceResource_PNG?"png":"blob");
    else fprintf(stderr,"Source resource rejected: %s\n",diagnostic);
    free(output);free(previous);return ok?0:1;
}
int nativeSourceResourceTool_Import(const char* project,const char* name,const char* type,const char* filename)
{
    NativeSourceResourceKind kind;
    if(!strcmp(type,"png"))kind=NativeSourceResource_PNG;
    else if(!strcmp(type,"blob"))kind=NativeSourceResource_Blob;
    else{fputs("Resource kind must be png or blob.\n",stderr);return 1;}
    uint8_t* data=NULL;size_t size=0;
    if(!read(filename,kind==NativeSourceResource_PNG?NATIVE_SOURCE_RESOURCE_PNG_BYTES:NATIVE_SOURCE_RESOURCE_BLOB_BYTES,&data,&size)) {
        fputs("Resource input is unreadable, empty or oversized.\n",stderr);return 1;
    }
    int result=put(project,name,kind,data,size);free(data);return result;
}
static int blueprint(const char* project,const char* name,const void* bytes,size_t size,uint32_t offset)
{
    if(size>NATIVE_SOURCE_RESOURCE_BLOB_BYTES-NATIVE_SOURCE_BLUEPRINT_HEADER) {
        fputs("Item DAT exceeds the owned blueprint byte limit.\n",stderr);return 1;
    }
    size_t capacity=size+NATIVE_SOURCE_BLUEPRINT_HEADER,written=0;uint8_t* envelope=malloc(capacity);NativeScriptDiagnostic diagnostic={0};
    bool ok=envelope&&nativeSourceItemBlueprint_Write(bytes,size,offset,envelope,capacity,&written,&diagnostic);
    int result=ok?put(project,name,NativeSourceResource_Blob,envelope,written):1;
    if(!ok)fprintf(stderr,"Item blueprint rejected: %s\n",diagnostic.message);
    free(envelope);return result;
}
int nativeSourceResourceTool_Item(const char* project,const char* name,const char* filename,const char* article)
{
    uint32_t offset;uint8_t* bytes=NULL;size_t size=0;
    if(!number(article,UINT32_MAX,&offset)||!read(filename,NATIVE_SOURCE_RESOURCE_BLOB_BYTES-NATIVE_SOURCE_BLUEPRINT_HEADER,&bytes,&size)) {
        fputs("Item blueprint needs a bounded DAT and an explicit unsigned Article offset.\n",stderr);return 1;
    }
    int result=blueprint(project,name,bytes,size,offset);free(bytes);return result;
}
int nativeSourceResourceTool_StockItem(const char* project,const char* name,const char* iso,const char* kind_text)
{
    uint32_t kind;u32 root,table,article;const char* external;void* bytes=NULL;size_t size=0;HSD_Dat dat;
    if(!number(kind_text,42,&kind)||!aurora_dvd_open(iso)) {
        fputs("Stock Item authoring needs an open ISO and explicit common kind 0..42.\n",stderr);return 1;
    }
    bool ok=nativeDiscArchive_Read("ItCo.dat",&bytes,&size)&&HSD_DatOpen(&dat,bytes,size)==HSD_DAT_OK&&
        HSD_DatFindRoot(&dat,"itPublicData",&root)&&
        HSD_DatResolve(&dat,root+4,&table,&external)==HSD_DAT_REF_LOCAL&&
        HSD_DatBytes(&dat,table,43*4)&&HSD_DatNextTarget(&dat,table)>=table+43*4&&
        HSD_DatResolve(&dat,table+kind*4,&article,&external)==HSD_DAT_REF_LOCAL;
    int result=ok?blueprint(project,name,bytes,size,article):1;
    if(!ok)fputs("ItCo.dat has no owned common Article at the selected kind.\n",stderr);
    _aligned_free(bytes);return result;
}
int nativeSourceResourceTool_ExportFacts(const char* template_name,const char* filename)
{
    const NativeFighterDefinition* donor=nativeFighterRuntime_FindDefinition(template_name);
    NativeSourceAssetFacts facts;NativeScriptDiagnostic diagnostic={0};size_t written=0;
    uint8_t* record=donor?malloc(NATIVE_SOURCE_FACT_BYTES):NULL;
    bool ok=record&&nativeSourceAssets_Copy(donor,&facts,&diagnostic)&&
        nativeSourceAssets_Write(&facts,record,NATIVE_SOURCE_FACT_BYTES,&written,&diagnostic);
    if(ok&&!save(filename,record,written)){snprintf(diagnostic.message,sizeof(diagnostic.message),"Atomic facts export failed");ok=false;}
    if(ok)printf("Exported source fighter facts: %zu bytes.\n",written);
    else fprintf(stderr,"Facts export rejected: %s\n",donor?diagnostic.message:"Unknown authoring template");
    free(record);return ok?0:1;
}
