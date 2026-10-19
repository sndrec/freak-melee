#include "native_fighter_mesh_source.h"
#include "native_fighter_authoring.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool text_valid(const char* text,size_t capacity)
{
    if(!text || !memchr(text,0,capacity) || !*text) return false;
    for(const unsigned char* at=(const unsigned char*)text;*at;++at)
        if(*at<32 || *at==127) return false;
    return true;
}
static bool valid(const NativeFighterMeshSource* source)
{
    if(!source || !source->used || !source->mesh || !source->vertices ||
       source->vertices>65535 || !source->corners || source->corners>1000000 ||
       source->corners%3 || source->binding>3 || !isfinite(source->scale) ||
       source->scale<=0 || source->scale>1000 ||
       !text_valid(source->path,sizeof(source->path)) ||
       !text_valid(source->identity,sizeof(source->identity))) return false;
    for(unsigned i=0;i<16;++i) if(!isfinite(source->transform[i])) return false;
    return true;
}
const NativeFighterMeshSource* nativeFighterMeshSource_Get(const NativeFighterProject* project,uint32_t mesh)
{
    if(!project || !project->authoring) return NULL;
    for(unsigned i=0;i<NATIVE_MESH_SOURCE_CAPACITY;++i) {
        const NativeFighterMeshSource* source=&project->authoring->sources[i];
        if(source->used && source->mesh==mesh) return source;
    }
    return NULL;
}
bool nativeFighterMeshSource_Set(NativeFighterProject* project,const NativeFighterMeshSource* source)
{
    if(!project || !valid(source)) return false;
    if(!project->authoring && !(project->authoring=calloc(1,sizeof(*project->authoring)))) return false;
    unsigned free_slot=NATIVE_MESH_SOURCE_CAPACITY;
    for(unsigned i=0;i<NATIVE_MESH_SOURCE_CAPACITY;++i) {
        if(project->authoring->sources[i].used && project->authoring->sources[i].mesh==source->mesh) {
            project->authoring->sources[i]=*source;return true;
        }
        if(!project->authoring->sources[i].used && free_slot==NATIVE_MESH_SOURCE_CAPACITY) free_slot=i;
    }
    if(free_slot==NATIVE_MESH_SOURCE_CAPACITY) return false;
    project->authoring->sources[free_slot]=*source;return true;
}
void nativeFighterMeshSource_Clear(NativeFighterProject* project,uint32_t mesh)
{
    if(!project || !project->authoring) return;
    for(unsigned i=0;i<NATIVE_MESH_SOURCE_CAPACITY;++i)
        if(project->authoring->sources[i].used && project->authoring->sources[i].mesh==mesh)
            memset(&project->authoring->sources[i],0,sizeof(project->authoring->sources[i]));
}
static bool integer(const char* text,uint64_t maximum,int base,uint64_t* value)
{
    if(!*text || *text=='-' || *text=='+') return false;
    char* end;errno=0;
    unsigned long long parsed=strtoull(text,&end,base);
    if(*end || errno || parsed>maximum) return false;
    *value=parsed;return true;
}
static int hex(char c)
{
    return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
}
static bool decode(char* destination,size_t capacity,const char* source)
{
    size_t length=0;
    while(*source) {
        unsigned char byte=(unsigned char)*source++;
        if(byte=='%') {
            if(!source[0]||!source[1]) return false;
            const int high=hex(source[0]),low=hex(source[1]);
            if(high<0||low<0) return false;
            byte=(unsigned char)(high*16+low);source+=2;
        }
        if(!byte || length+1>=capacity) return false;
        destination[length++]=(char)byte;
    }
    destination[length]=0;return true;
}
bool nativeFighterMeshSource_Parse(NativeFighterProject* project,char* row)
{
    char* fields[11];char* at=row;
    for(unsigned i=0;i<11;++i) {
        fields[i]=at;char* tab=strchr(at,'\t');
        if(i==10) {if(tab) return false;}
        else {if(!tab) return false;*tab=0;at=tab+1;}
    }
    NativeFighterMeshSource source={0};uint64_t value;
    if(!integer(fields[0],UINT32_MAX,10,&value)) return false;source.mesh=(uint32_t)value;
    if(!integer(fields[1],65535,10,&value)) return false;source.vertices=(uint32_t)value;
    if(!integer(fields[2],1000000,10,&value)) return false;source.corners=(uint32_t)value;
    if(!integer(fields[3],UINT64_MAX,16,&value)) return false;source.topology=value;
    if(!integer(fields[4],3,10,&value)) return false;source.binding=(unsigned)value;
    if(!integer(fields[5],1,10,&value)) return false;source.mirrored=value!=0;
    if(!integer(fields[6],1,10,&value)) return false;source.uses_bind_shape=value!=0;
    char* end;errno=0;source.scale=strtof(fields[7],&end);
    if(end==fields[7]||*end) return false;
    at=fields[8];
    for(unsigned i=0;i<16;++i) {
        errno=0;source.transform[i]=strtof(at,&end);
        if(end==at||*end!=(i==15?0:',')) return false;
        at=end+1;
    }
    if(!decode(source.path,sizeof(source.path),fields[9]) ||
       !decode(source.identity,sizeof(source.identity),fields[10])) return false;
    source.used=true;return nativeFighterMeshSource_Set(project,&source);
}
static bool encoded(FILE* file,const char* text)
{
    for(const unsigned char* at=(const unsigned char*)text;*at;++at) {
        if(*at=='%' || *at=='\t' || *at=='\r' || *at=='\n') {
            if(fprintf(file,"%%%02X",*at)<0) return false;
        } else if(fputc(*at,file)==EOF) return false;
    }
    return true;
}
bool nativeFighterMeshSource_Write(const NativeFighterProject* project,FILE* file)
{
    for(unsigned i=0;project->authoring && i<NATIVE_MESH_SOURCE_CAPACITY;++i) {
        const NativeFighterMeshSource* source=&project->authoring->sources[i];
        if(!source->used) continue;
        if(!valid(source) || fprintf(file,"mesh-source\t%u\t%u\t%u\t%llx\t%u\t%u\t%u\t%.9g\t",
            source->mesh,source->vertices,source->corners,(unsigned long long)source->topology,
            source->binding,source->mirrored,source->uses_bind_shape,source->scale)<0) return false;
        for(unsigned f=0;f<16;++f)
            if(fprintf(file,"%s%.9g",f?",":"",source->transform[f])<0) return false;
        if(fputc('\t',file)==EOF || !encoded(file,source->path) ||
           fputc('\t',file)==EOF || !encoded(file,source->identity) || fputc('\n',file)==EOF) return false;
    }
    return !ferror(file);
}
