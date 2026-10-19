#include "native_fighter_authoring.h"
#ifdef MELEE_NATIVE
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
constexpr size_t max_bytes=4*1024*1024;
const char* const categories[]={"bone","mesh","material","action","action-group"};

bool parse_row(char* row,NativeFighterProject* candidate)
{
    if(!*row)return true;
    char* tab=std::strchr(row,'\t');
    if(!tab)return false;
    *tab++=0;
    if(!std::strcmp(row,"mesh-source"))return nativeFighterMeshSource_Parse(candidate,tab);
    if(!std::strcmp(row,"mesh-rest"))return nativeFighterMeshSource_ParseRest(candidate,tab);
    char* end;
    if(*tab<'0'||*tab>'9')return false;
    errno=0;
    const unsigned long index=std::strtoul(tab,&end,10);
    if(errno || index>UINT32_MAX)return false;
    if(!std::strcmp(row,"hidden"))
        return !*end && nativeFighterAuthoring_SetHidden(candidate,index,true);
    for(unsigned c=0;c<NativeAuthoring_CategoryCount;++c)
        if(!std::strcmp(row,categories[c]))
            return *end=='\t' && nativeFighterAuthoring_SetName(candidate,c,index,end+1);
    return false;
}
}

bool nativeFighterAuthoring_Encode(const NativeFighterProject* p,uint8_t** bytes,size_t* size)
{
    if(!p || !bytes || !size)return false;
    std::string text;
    char row[4096];
    for(unsigned i=0;p->authoring && i<4096;++i) {
        const auto& label=p->authoring->labels[i];
        if(!label.used || !*label.name)continue;
        if(label.category>=NativeAuthoring_CategoryCount ||
           !std::memchr(label.name,0,sizeof(label.name)))return false;
        for(const unsigned char* c=(const unsigned char*)label.name;*c;++c)
            if(*c<32 || *c==127)return false;
        const int n=std::snprintf(row,sizeof(row),"%s\t%u\t%s\n",categories[label.category],label.key,label.name);
        if(n<=0 || (size_t)n>=sizeof(row))return false;
        text.append(row,n);
    }
    for(unsigned i=0;p->authoring && i<1024;++i) {
        if(!p->authoring->hidden_actions[i])continue;
        const int n=std::snprintf(row,sizeof(row),"hidden\t%u\n",i);
        text.append(row,n);
    }
    for(unsigned i=0;p->authoring && i<NATIVE_MESH_SOURCE_CAPACITY;++i) {
        const auto& source=p->authoring->sources[i];
        if(!source.used)continue;
        const size_t n=nativeFighterMeshSource_Format(&source,row,sizeof(row));
        if(!n)return false;
        text.append(row,n);
    }
    if(text.size()>max_bytes)return false;
    auto* result=(uint8_t*)std::malloc(text.size()+1);
    if(!result)return false;
    std::memcpy(result,text.c_str(),text.size()+1);
    *bytes=result;*size=text.size();return true;
}

bool nativeFighterAuthoring_Decode(const uint8_t* bytes,size_t size,NativeFighterProject* p)
{
    if(!p || size>max_bytes || (size && (!bytes || std::memchr(bytes,0,size))))return false;
    NativeFighterProject candidate={};
    char row[4096];
    bool ok=true;
    for(size_t start=0;ok && start<size;) {
        size_t end=start;
        while(end<size && bytes[end]!='\n' && bytes[end]!='\r')++end;
        if(end-start>=sizeof(row)){ok=false;break;}
        std::memcpy(row,bytes+start,end-start);row[end-start]=0;
        ok=parse_row(row,&candidate);
        start=end;
        while(start<size && (bytes[start]=='\n'||bytes[start]=='\r'))++start;
    }
    if(ok){std::free(p->authoring);p->authoring=candidate.authoring;}
    else std::free(candidate.authoring);
    return ok;
}

bool nativeFighterAuthoring_Load(const char* path,NativeFighterProject* p)
{
    char name[600];
    const int n=path?std::snprintf(name,sizeof(name),"%s/workshop_names.txt",path):-1;
    if(n<=0 || (size_t)n>=sizeof(name) || !p)return false;
    FILE* file=std::fopen(name,"rb");
    if(!file)return errno==ENOENT && nativeFighterAuthoring_Decode(nullptr,0,p);
    long length=0;
    bool ok=!std::fseek(file,0,SEEK_END) && (length=std::ftell(file))>=0 &&
        (size_t)length<=max_bytes && !std::fseek(file,0,SEEK_SET);
    auto* bytes=ok?(uint8_t*)std::malloc((size_t)length+1):nullptr;
    ok=ok && bytes && std::fread(bytes,1,(size_t)length,file)==(size_t)length;
    if(std::fclose(file))ok=false;
    if(ok)ok=nativeFighterAuthoring_Decode(bytes,(size_t)length,p);
    std::free(bytes);return ok;
}
#endif
