#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "native_source_content_tool.h"
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_family_project.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool read(const char* path,uint8_t** output,size_t* bytes)
{
    FILE* file=fopen(path,"rb");if(!file)return false;
    bool ok=fseek(file,0,SEEK_END)==0;long size=ok?ftell(file):-1;ok=ok&&size>0&&size<=NATIVE_CONTENT_MAX_PACKAGE_BYTES&&fseek(file,0,SEEK_SET)==0;
    uint8_t* data=ok?malloc((size_t)size):NULL;ok=ok&&data&&fread(data,1,(size_t)size,file)==(size_t)size;ok=fclose(file)==0&&ok;
    if(!ok){free(data);return false;}*output=data;*bytes=(size_t)size;return true;
}
static bool save(const char* root,const void* bytes,size_t size)
{
    char destination[512],temporary[512];int a=snprintf(destination,sizeof(destination),"%s\\behavior.fms",root),b=snprintf(temporary,sizeof(temporary),"%s\\behavior.fms.next-%lu",root,(unsigned long)GetCurrentProcessId());
    if(a<=0||a>=sizeof(destination)||b<=0||b>=sizeof(temporary)||size>UINT32_MAX)return false;
    HANDLE file=CreateFileA(temporary,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)return false;DWORD written=0;
    bool ok=WriteFile(file,bytes,(DWORD)size,&written,NULL)&&written==size&&FlushFileBuffers(file);ok=CloseHandle(file)&&ok;
    if(ok)ok=MoveFileExA(temporary,destination,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileA(temporary);return ok;
}
int nativeSourceContentTool_Check(const char* path,const char* source_path,bool commit,bool audit)
{
    if(commit&&audit){fputs("Source audit cannot save a draft.\n",stderr);return 1;}
    static NativeScriptDiagnostic audit_entries[256];
    NativeScriptCompileDiagnostics report={.entries=audit_entries,.capacity=256};
    NativeFighterProject project;uint8_t* source=NULL;size_t size=0;NativeScriptDiagnostic diagnostic={0};
    if(!nativeFighterProject_Open(path,&project)){fputs("Source member draft needs valid assets.fma and copied DATs.\n",stderr);return 1;}
    NativeSourcePackage* candidate=calloc(1,sizeof(*candidate));NativeFighterProject* member=&project;
    bool ok=candidate&&read(source_path,&source,&size);
    NativeSourcePackageInput input={.family=NativeSourcePackage_Single,.facts=member->asset_record,.facts_bytes=member->asset_record_size,
        .assets={member->fighter_dat,member->fighter_size,member->animation_dat,member->animation_size,member->costume_dat,member->costume_size},
        .source=(const char*)source,.source_bytes=size,.parameters=(const char*)member->parameter_text,.parameter_bytes=member->parameter_size,
        .resources=member->resources,.resource_bytes=member->resource_size,.diagnostics=audit?&report:NULL};
    if(ok)ok=nativeSourcePackage_Prepare(candidate,&input,Ft_Kind_CustomFirst,NULL,&diagnostic);
    if(ok&&commit)ok=save(path,source,size);
    if(ok)printf("{\"status\":\"PASS source %s\",\"target\":%u,\"functions\":%u,\"imports\":%u,\"instructions\":%u}\n",commit?"saved":"checked",(unsigned)Ft_Kind_CustomFirst,candidate->stats.functions,candidate->parameters.root_imports+candidate->parameters.article_imports,candidate->stats.instructions);
    else {
        if(audit&&report.count)for(unsigned i=0;i<report.count;++i)
            fprintf(stderr,"%s:%u:%u: %s\n",source_path,report.entries[i].source.line,report.entries[i].source.column,report.entries[i].message);
        else fprintf(stderr,"%s:%u:%u: %s\n",source_path,diagnostic.source.line,diagnostic.source.column,diagnostic.message[0]?diagnostic.message:"Source candidate validation or atomic save failed");
    }
    if(audit) {
        unsigned package_diagnostic=!ok&&!report.count;
        printf("{\"audit\":true,\"diagnostics\":%u,\"stored\":%u,\"package_diagnostic\":%s,\"functions_checked\":%u,\"metadata_complete\":%s,\"bodies_complete\":%s,\"package_accepted\":%s}\n",
            report.total+package_diagnostic,report.count+package_diagnostic,package_diagnostic?"true":"false",report.functions_checked,report.metadata_complete?"true":"false",report.bodies_complete?"true":"false",ok?"true":"false");
    }
    if(candidate){nativeSourcePackage_Release(candidate);free(candidate);}free(source);nativeFighterProject_Close(&project);return ok?0:1;
}
int nativeSourceContentTool_Package(const char* cache,const char* path,const char* portrait_path)
{
    NativeFighterFamilyProject project;uint8_t* portrait=NULL,*packet=NULL;size_t portrait_bytes=0,packet_bytes=0;NativeContentId id;
    if(!nativeFighterFamilyProject_Open(path,&project)){fputs("Could not open a complete source draft.\n",stderr);return 1;}
    bool ok=read(portrait_path,&portrait,&portrait_bytes)&&
        nativeFighterFamilyProject_Build(&project,portrait,portrait_bytes,&packet,&packet_bytes,&id)&&nativeContentStore_Cache(cache,packet,packet_bytes,NULL,&id);
    if(ok){char hex[65];nativeContentStore_IdHex(&id,hex);printf("Cached source fighter %s (%zu bytes).\n",hex,packet_bytes);}
    else fputs("Source draft facts/assets/FMS, portrait or cache validation failed; no native-module fallback.\n",stderr);
    free(portrait);free(packet);nativeFighterFamilyProject_Close(&project);return ok?0:1;
}
int nativeSourceContentTool_Verify(const char* cache,const char* hex)
{
    NativeContentId id;NativeFighterPackage metadata;NativeFighterPackageData data;uint8_t* packet=NULL;size_t bytes=0;
    NativeSourcePackage* candidates=calloc(NATIVE_FIGHTER_FAMILY_MEMBERS,sizeof(*candidates));NativeScriptDiagnostic diagnostic={0};
    bool ok=candidates&&nativeContentStore_IdFromHex(hex,&id)&&nativeContentStore_Read(cache,&id,&packet,&bytes)&&
        nativeContentStore_ParseFighter(packet,bytes,&metadata,&data);
    unsigned bodies=0,imports=0,member=0,failed_member=0;
    for(;ok&&member<metadata.member_count;++member){const NativeFighterMemberData* m=&data.members[member];NativeSourcePackageInput input={.family=NativeSourcePackage_Single,.facts=m->asset_record,.facts_bytes=m->asset_record_size,
        .assets={m->fighter_dat,m->fighter_size,m->animation_dat,m->animation_size,m->costume_dat,m->costume_size},.source=(const char*)m->script_text,.source_bytes=m->script_size,
        .parameters=(const char*)m->parameter_text,.parameter_bytes=m->parameter_size,
        .resources=m->resources,.resource_bytes=m->resource_size};
        NativeFighterCpuProfile cpu;failed_member=member;
        ok=nativeFighterCpu_Parse((const char*)m->cpu_text,m->cpu_size,&cpu)&&nativeSourcePackage_Prepare(&candidates[member],&input,(FighterKind)(Ft_Kind_CustomFirst+member),NULL,&diagnostic);
        if(ok){bodies+=candidates[member].stats.functions;imports+=candidates[member].parameters.root_imports+candidates[member].parameters.article_imports;}}
    bool released=true;
    if(candidates){for(unsigned i=0;i<NATIVE_FIGHTER_FAMILY_MEMBERS;++i)if(!nativeSourcePackage_Release(&candidates[i]))released=false;if(released)free(candidates);}
    ok=ok&&released;
    if(ok)printf("Verified source package %s: %u members, %u bodies, %u imports.\n",hex,metadata.member_count,bodies,imports);
    else fprintf(stderr,"Source package member %u rejected at %u:%u: %s\n",failed_member,diagnostic.source.line,diagnostic.source.column,diagnostic.message);
    free(packet);return ok?0:1;
}
