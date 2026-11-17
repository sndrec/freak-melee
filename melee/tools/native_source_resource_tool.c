#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "native_source_resource_tool.h"
#include <melee/ft/native_source_package.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PathBytes=1024 };
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
