#include "native_fighter_rig_animation.h"
#include "native_fighter_animation_nodes.h"
#include "native_asset_dat.h"
#include "ftparts.h"
#include <melee/lb/lbanim_dat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

namespace {
struct Root {
    uint32_t offset;
    const char* name;
    unsigned nodes;
    bool bound = false, owned = false;
    std::vector<uint16_t> order;
};
struct Blob {
    size_t before, after;
    uint32_t size, rewritten_size;
    HSD_Dat dat;
    std::vector<Root> roots;
};
struct Descriptor { uint32_t record; unsigned blob; };

bool fail(char* text,size_t capacity,const char* message)
{ if(text&&capacity)std::snprintf(text,capacity,"%s",message);return false; }
bool include(const NativeSourceAssetFacts& facts,unsigned part,uint32_t mask)
{
    for(unsigned i=0;i<facts.omission_count;++i)
        if(facts.omissions[i].joint==part)return (mask&(UINT32_C(1)<<i))!=0;
    return true;
}
bool rig_map(const NativeSourceAssetFacts& old,const NativeSourceAssetFacts& next,
    const uint16_t* order,unsigned count)
{
    const unsigned before=old.joints+old.omission_count;
    if(!order||!old.joints||!next.joints||before>MAX_FT_PARTS||
       count!=next.joints+next.omission_count||count>MAX_FT_PARTS||
       old.omission_count>NATIVE_SOURCE_OMISSIONS||next.omission_count!=old.omission_count||
       next.animation_tag!=old.animation_tag)return false;
    bool used[MAX_FT_PARTS]={};
    for(unsigned n=0;n<count;++n)if(order[n]!=UINT16_MAX){
        if(order[n]>=before||used[order[n]])return false;used[order[n]]=true;
    }
    for(unsigned i=0;i<old.omission_count;++i){
        unsigned a=old.omissions[i].joint,b=next.omissions[i].joint;
        if(a>=before||b>=count||order[b]!=a)return false;
        for(unsigned j=0;j<i;++j)if(old.omissions[j].joint==a||next.omissions[j].joint==b)return false;
    }
    return true;
}
bool node_map(const NativeSourceAssetFacts& old,const NativeSourceAssetFacts& next,
    const uint16_t* order,unsigned count,uint32_t mask,unsigned nodes,
    std::vector<uint16_t>& result)
{
    uint16_t indices[MAX_FT_PARTS];unsigned n=0;
    for(unsigned part=0;part<old.joints+old.omission_count;++part)
        indices[part]=include(old,part,mask)?(uint16_t)n++:UINT16_MAX;
    if(n!=nodes)return false;
    result.clear();
    for(unsigned part=0;part<count;++part)if(include(next,part,mask))
        result.push_back(order[part]==UINT16_MAX?UINT16_MAX:indices[order[part]]);
    return !result.empty();
}
bool index(const NativeFighterProject& project,std::vector<Blob>& blobs)
{
    size_t at=0;unsigned symbols=0;
    std::vector<FigaTrack> tracks(4096);
    while(at<project.animation_size){
        if(project.animation_dat[at]==255){++at;continue;}
        if(project.animation_size-at<32||blobs.size()==1024)return false;
        uint32_t size=NativeAssetDat::read32(project.animation_dat+at);
        if(size<32||size>project.animation_size-at)return false;
        Blob blob={};blob.before=at;blob.size=size;
        if(HSD_DatOpen(&blob.dat,project.animation_dat+at,size)!=HSD_DAT_OK||
           !blob.dat.root_count||blob.dat.extern_count||blob.dat.root_count>1024-symbols)return false;
        symbols+=(unsigned)blob.dat.root_count;
        for(u32 i=0;i<blob.dat.root_count;++i){
            u32 root,nodes;const char* name;FigaTree tree;
            if(!HSD_DatSymbol(&blob.dat,false,i,&root,&name)||!*name||
               lbAnim_DecodeFigaTree(&blob.dat,root,&tree,tracks.data(),4096,&nodes)!=HSD_DAT_OK||
               !nodes||nodes>512)return false;
            for(const auto& other:blob.roots)if(!std::strcmp(name,other.name))return false;
            blob.roots.push_back({(uint32_t)root,name,(unsigned)nodes});
        }
        blobs.push_back(std::move(blob));at+=size;
    }
    return !blobs.empty();
}
bool find(const std::vector<Blob>& blobs,const char* name,int32_t offset,int32_t size,
    unsigned* blob,unsigned* root,bool* found)
{
    *found=false;
    for(unsigned i=0;i<blobs.size();++i)if(offset>=0&&(size_t)offset==blobs[i].before&&size==(int32_t)blobs[i].size)
        for(unsigned j=0;j<blobs[i].roots.size();++j)if(!std::strcmp(blobs[i].roots[j].name,name)){
            *blob=i;*root=j;*found=true;return true;
        }
    for(unsigned i=0;i<blobs.size();++i)for(unsigned j=0;j<blobs[i].roots.size();++j)
        if(!std::strcmp(blobs[i].roots[j].name,name)){
            if(*found)return false;*blob=i;*root=j;*found=true;
        }
    return true;
}
}

extern "C" bool nativeFighterRig_RemapAnimationBank(const NativeFighterProject* project,
    const NativeSourceAssetFacts* destination,const uint16_t* order,unsigned count,
    NativeFighterRigAnimation* output,char* error,size_t capacity)
{
    if(!project||!destination||!output||output->fighter||output->fighter_size||
       output->animation||output->animation_size||!project->fighter_dat||!project->animation_dat||
       project->fighter_size>NATIVE_CONTENT_MAX_PACKAGE_BYTES||
       project->animation_size>NATIVE_CONTENT_MAX_PACKAGE_BYTES||
       !rig_map(project->facts,*destination,order,count))
        return fail(error,capacity,"Animation rig edit needs a valid part map and empty output.");
    std::vector<Blob> blobs;
    if(!index(*project,blobs))return fail(error,capacity,"Animation bank contains a malformed or unsupported clip archive.");
    HSD_Dat fighter;u32 root;
    if(HSD_DatOpen(&fighter,project->fighter_dat,project->fighter_size)!=HSD_DAT_OK||
       !HSD_DatFindRoot(&fighter,project->fighter_root,&root))
        return fail(error,capacity,"Animation rig edit cannot open the fighter descriptor root.");
    std::vector<Descriptor> descriptors;
    const unsigned counts[]={project->action_count,project->demo_count};
    for(unsigned bank=0;bank<2;++bank){
        if(!counts[bank])continue;
        u32 table;const char* external;
        if(counts[bank]>1024||HSD_DatResolve(&fighter,root+(bank?20:12),&table,&external)!=HSD_DAT_REF_LOCAL||
           !HSD_DatBytes(&fighter,table,counts[bank]*24))
            return fail(error,capacity,"Animation descriptor table is malformed.");
        for(unsigned i=0;i<counts[bank];++i){
            u32 record=table+i*24,name_at,offset,size,flags;
            if(!HSD_DatReadU32(&fighter,record+4,&offset)||!HSD_DatReadU32(&fighter,record+8,&size)||
               !HSD_DatReadU32(&fighter,record+16,&flags))return fail(error,capacity,"Animation descriptor is truncated.");
            if(!size)continue;
            if(HSD_DatResolve(&fighter,record,&name_at,&external)!=HSD_DAT_REF_LOCAL)
                return fail(error,capacity,"Animation descriptor has no owned name.");
            const char* name=(const char*)HSD_DatBytes(&fighter,name_at,1);
            if(!name||!*name||!std::memchr(name,0,fighter.data_size-name_at))
                return fail(error,capacity,"Animation descriptor name is invalid.");
            unsigned b=0,r=0;bool found;
            if(!find(blobs,name,(int32_t)offset,(int32_t)size,&b,&r,&found))
                return fail(error,capacity,"Animation descriptor has an ambiguous embedded name.");
            if(!found){if(bank)continue;return fail(error,capacity,"Action animation is absent from the owned bank.");}
            auto& selected=blobs[b].roots[r];
            const bool owned=(flags&63)==project->facts.animation_tag;
            std::vector<uint16_t> mapped;
            if(owned){
                if(!node_map(project->facts,*destination,order,count,(flags>>9)&0x1fff,selected.nodes,mapped)){
                    if(error&&capacity)std::snprintf(error,capacity,"Clip %s has %u nodes; its owned rig/mask requires a different count.",name,selected.nodes);
                    return false;
                }
            }else for(unsigned n=0;n<selected.nodes;++n)mapped.push_back((uint16_t)n);
            if(selected.bound&&mapped!=selected.order)
                return fail(error,capacity,"Shared animation root needs incompatible rig or mask maps.");
            selected.bound=true;selected.owned|=owned;selected.order=std::move(mapped);
            descriptors.push_back({(uint32_t)record,b});
        }
    }
    NativeFighterRigAnimation result={};
    std::vector<uint8_t> animation;
    size_t before=0;
    for(auto& blob:blobs){
        if(blob.before-before>NATIVE_CONTENT_MAX_PACKAGE_BYTES-animation.size())
            return fail(error,capacity,"Remapped animation padding exceeds the asset size limit.");
        animation.insert(animation.end(),project->animation_dat+before,project->animation_dat+blob.before);
        blob.after=animation.size();bool changed=false;
        std::vector<NativeFighterAnimationNodeMap> maps;
        // An unreferenced symbol can alias a referenced tree. Its shared tree
        // follows that tree's edit, preserving the alias instead of demanding
        // an incompatible identity map for the same source object.
        for(auto& row:blob.roots)if(!row.bound)
            for(const auto& other:blob.roots)if(other.bound&&other.offset==row.offset){row.order=other.order;break;}
        for(auto& row:blob.roots){
            if(!row.bound){++result.unreferenced_roots;if(row.order.empty())for(unsigned n=0;n<row.nodes;++n)row.order.push_back((uint16_t)n);}
            else if(row.owned)++result.owned_roots;else ++result.foreign_roots;
            if(row.order.size()!=row.nodes)changed=true;
            else for(unsigned n=0;n<row.nodes;++n)if(row.order[n]!=n)changed=true;
            maps.push_back({row.nodes,(unsigned)row.order.size(),row.order.data()});
        }
        void* rewritten=nullptr;size_t bytes=blob.size;
        const uint8_t* data=project->animation_dat+blob.before;
        if(changed){bytes=0;if(!nativeFighterAnimation_RemapNodes(data,blob.size,maps.data(),(unsigned)maps.size(),&rewritten,&bytes,error,capacity))return false;data=(const uint8_t*)rewritten;}
        if(bytes>NATIVE_CONTENT_MAX_PACKAGE_BYTES-animation.size()){
            std::free(rewritten);return fail(error,capacity,"Remapped animation bank exceeds the asset size limit.");
        }
        animation.insert(animation.end(),data,data+bytes);std::free(rewritten);
        blob.rewritten_size=(uint32_t)bytes;before=blob.before+blob.size;
    }
    if(project->animation_size-before>NATIVE_CONTENT_MAX_PACKAGE_BYTES-animation.size())
        return fail(error,capacity,"Remapped animation padding exceeds the asset size limit.");
    animation.insert(animation.end(),project->animation_dat+before,project->animation_dat+project->animation_size);
    std::vector<uint8_t> patched(project->fighter_dat,project->fighter_dat+project->fighter_size);
    for(const auto& desc:descriptors){
        const auto& blob=blobs[desc.blob];
        if(blob.after>INT32_MAX||blob.rewritten_size>INT32_MAX)return fail(error,capacity,"Animation descriptor exceeds its signed offset range.");
        NativeAssetDat::be32(patched.data()+32+desc.record+4,(uint32_t)blob.after);
        NativeAssetDat::be32(patched.data()+32+desc.record+8,blob.rewritten_size);
    }
    result.fighter=(uint8_t*)std::malloc(patched.size());result.animation=(uint8_t*)std::malloc(animation.size());
    if(!result.fighter||!result.animation){std::free(result.fighter);std::free(result.animation);return fail(error,capacity,"Could not allocate the remapped animation pair.");}
    result.fighter_size=patched.size();result.animation_size=animation.size();result.descriptors=(unsigned)descriptors.size();
    std::memcpy(result.fighter,patched.data(),patched.size());std::memcpy(result.animation,animation.data(),animation.size());
    *output=result;if(error&&capacity)error[0]=0;return true;
}
