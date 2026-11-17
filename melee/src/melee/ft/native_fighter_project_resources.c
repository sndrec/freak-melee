#include "native_fighter_project.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool nativeFighterProject_EditResource(NativeFighterProject* project,const char* name,
    NativeSourceResourceKind kind,const void* bytes,size_t size)
{
    if (!project || !nativeSourceResources_NameValid(name) || (!bytes && size)) return false;
    NativeSourceResources view;
    if (!nativeSourceResources_Read(project->resources,project->resource_size,&view,NULL,0)) return false;
    NativeSourceResourceInput entries[NATIVE_SOURCE_RESOURCE_ENTRIES]; unsigned count=0;
    for (unsigned i=0;i<view.count;++i) {
        const NativeSourceResourceEntry* entry=&view.entries[i];
        if (!strcmp(entry->name,name)) continue;
        entries[count++]=(NativeSourceResourceInput){entry->name,entry->bytes,entry->size,entry->kind};
    }
    if (bytes) {
        if (count==NATIVE_SOURCE_RESOURCE_ENTRIES) return false;
        entries[count++]=(NativeSourceResourceInput){name,bytes,size,kind};
    }
    size_t capacity=0,written=0;
    bool ok=nativeSourceResources_Measure(entries,count,&capacity,NULL,0);
    void* built=ok?malloc(capacity?capacity:1):NULL;
    ok=ok && built && nativeSourceResources_Write(entries,count,built,capacity,&written,NULL,0);
    if (!ok) { free(built); return false; }
    free(project->resources);
    project->resources=built; project->resource_size=written;
    return true;
}

bool nativeFighterProject_SetResource(const char* path,const char* name,
    NativeSourceResourceKind kind,const void* bytes,size_t size)
{
    if (!path) return false;
    char file[560],temporary[568];
    if (snprintf(file,sizeof(file),"%s/resources.fmr",path)>=sizeof(file)) return false;
    snprintf(temporary,sizeof(temporary),"%s.new",file);
    unsigned char* old=NULL; size_t old_size=0;
    FILE* input=fopen(file,"rb");
    if (!input && GetFileAttributesA(file)!=INVALID_FILE_ATTRIBUTES) return false;
    if (input) {
        long length;
        bool ok=!fseek(input,0,SEEK_END) && (length=ftell(input))>=0 &&
            length<=NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES && !fseek(input,0,SEEK_SET);
        if (ok && length) { old=malloc(length); ok=old && fread(old,1,length,input)==length; old_size=length; }
        fclose(input);
        if (!ok) { free(old); return false; }
    }
    NativeFighterProject project={0};
    project.resources=old; project.resource_size=old_size;
    bool ok=nativeFighterProject_EditResource(&project,name,kind,bytes,size);
    FILE* output=ok?fopen(temporary,"wb"):NULL;
    ok=output && fwrite(project.resources,1,project.resource_size,output)==project.resource_size;
    if (output) ok=fclose(output)==0 && ok;
    free(project.resources);
    if (ok) ok=MoveFileExA(temporary,file,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if (!ok) DeleteFileA(temporary);
    return ok;
}
