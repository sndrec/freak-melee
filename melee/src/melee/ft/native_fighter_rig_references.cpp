#include "native_fighter_rig_references.h"
#include "native_fighter_action_editor.h"
#include "native_hitbox_lifetime.h"
#include "native_asset_dat.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
struct Editor {
    HSD_Dat dat;
    std::vector<uint8_t> bytes;
    const uint16_t* map;
    char* error;
    size_t capacity;
    unsigned metadata=0,commands=0;
    bool fail(const char* message,uint32_t at)
    {if(error&&capacity)std::snprintf(error,capacity,"%s (fighter data 0x%X).",message,at);return false;}
    bool local(uint32_t slot,uint32_t* result,bool optional=false)
    {
        u32 at;const char* name;
        auto kind=HSD_DatResolve(&dat,slot,&at,&name);
        if(optional&&kind==HSD_DAT_REF_NULL){*result=UINT32_MAX;return true;}
        if(kind!=HSD_DAT_REF_LOCAL)return fail("Bone reference record is missing or external",slot);
        *result=(uint32_t)at;return true;
    }
    bool value(uint32_t at,unsigned width,uint32_t* output)
    {
        if(!HSD_DatBytes(&dat,at,width))return fail("Bone reference record is truncated",at);
        if(width==1)*output=dat.data[at];
        else if(width==2){u16 v;HSD_DatReadU16(&dat,at,&v);*output=v;}
        else {u32 v;HSD_DatReadU32(&dat,at,&v);*output=(uint32_t)v;}
        return true;
    }
    bool mapped(uint32_t original,uint32_t at,uint32_t* output)
    {
        *output=original;
        if(original>=MAX_FT_PARTS)return true; // Native sentinel or absent slot.
        if(map[original]==UINT16_MAX)return fail("A removed bone still has a reference",at);
        *output=map[original];return true;
    }
    bool field(uint32_t at,unsigned width)
    {
        uint32_t before,after;
        if(!value(at,width,&before)||!mapped(before,at,&after))return false;
        if(width==1)bytes[32+at]=(uint8_t)after;
        else if(width==2)NativeAssetDat::be16(bytes.data()+32+at,(uint16_t)after);
        else NativeAssetDat::be32(bytes.data()+32+at,after);
        ++metadata;return true;
    }
    bool bits(uint32_t at,unsigned shift)
    {
        uint32_t raw,mapped_bone;
        if(!value(at,4,&raw)||!mapped((raw>>shift)&255,at,&mapped_bone))return false;
        NativeAssetDat::be32(bytes.data()+32+at,(raw&~(UINT32_C(255)<<shift))|(mapped_bone<<shift));
        ++commands;return true;
    }
    bool array(uint32_t base,unsigned count,unsigned stride,unsigned width)
    {
        if(count>256||!HSD_DatBytes(&dat,base,(size_t)count*stride))return fail("Bone reference array exceeds its record",base);
        for(unsigned n=0;n<count;++n)if(!field(base+n*stride,width))return false;
        return true;
    }
    bool dynamics(uint32_t at)
    {
        uint32_t active,colliders,bones,collision;
        if(!value(at,4,&active)||active>Ft_Dynamics_NumMax||!value(at+8,4,&colliders)||colliders>11)
            return fail("Dynamics record has invalid counts",at);
        if(colliders&&(!local(at+12,&collision)||!array(collision,colliders,20,4)))return false;
        if(!active)return true;
        if(!local(at+4,&bones))return false;
        const uint32_t count=(uint32_t)(HSD_DatNextTarget(&dat,bones)-bones)/24;
        if(count<active||count>Ft_Dynamics_NumMax||!HSD_DatBytes(&dat,bones,count*24))
            return fail("Dynamics descriptors exceed their array",bones);
        for(unsigned n=0;n<count;++n){
            uint32_t row=bones+n*24,start,nodes;
            if(!value(row,4,&start)||!value(row+8,4,&nodes)||nodes>256||!field(row,4))return false;
            // The common dynamics update follows child joints and increments
            // the physical part number for each node. Keep that same chain.
            if(start<MAX_FT_PARTS)for(unsigned i=0;i<nodes;++i)
                if(i>=MAX_FT_PARTS-start||map[start+i]==UINT16_MAX||map[start+i]!=map[start]+i)
                    return fail("This edit would split a native dynamics chain",row);
        }
        return true;
    }
    bool part_animations(uint32_t root)
    {
        uint32_t begin,end;
        if(!local(root+28,&begin)||!local(root+32,&end)||end<begin||(end-begin)%4||(end-begin)/4>5)
            return fail("Part animation groups exceed their table",root+28);
        for(uint32_t slot=begin;slot<end;slot+=4){
            uint32_t group;if(!local(slot,&group,true))return false;if(group==UINT32_MAX)continue;
            uint32_t count,parts;
            if(!field(group,2)||!value(group+2,2,&count)||count>140)return false;
            if(count&&(!local(group+4,&parts)||!array(parts,count,1,1)))return false;
        }
        return true;
    }
};
}

extern "C" bool nativeFighterRig_RemapReferences(const NativeFighterProject* project,
    const uint16_t map[MAX_FT_PARTS],NativeFighterRigReferences* output,char* error,size_t capacity)
{
    auto fail=[&](const char* message){if(error&&capacity)std::snprintf(error,capacity,"%s",message);return false;};
    if(!project||!map||!output||output->fighter||output->fighter_size||!project->fighter_dat||
       project->fighter_size>NATIVE_CONTENT_MAX_PACKAGE_BYTES)
        return fail("Bone reference editing needs a project, part map and empty output.");
    bool used[MAX_FT_PARTS]={};
    for(unsigned n=0;n<MAX_FT_PARTS;++n)if(map[n]!=UINT16_MAX){
        if(map[n]>=MAX_FT_PARTS||used[map[n]])return fail("Bone reference map has an invalid or repeated destination.");
        used[map[n]]=true;
    }
    Editor e={};e.map=map;e.error=error;e.capacity=capacity;
    if(HSD_DatOpen(&e.dat,project->fighter_dat,project->fighter_size)!=HSD_DAT_OK)return fail("Could not open the fighter's bone references.");
    u32 root;
    if(!HSD_DatFindRoot(&e.dat,project->fighter_root,&root)||!HSD_DatBytes(&e.dat,root,96))return fail("Fighter bone reference root is missing.");
    e.bytes.assign(project->fighter_dat,project->fighter_dat+project->fighter_size);
    uint32_t common,model,dynamics,hurt,scale,bubbles,collision,effects,ik;
    if(!e.local((uint32_t)root,&common)||!e.local(root+8,&model)||!e.local(root+44,&dynamics)||
       !e.local(root+48,&hurt)||!e.local(root+52,&scale)||!e.local(root+56,&bubbles)||
       !e.local(root+68,&collision)||!e.local(root+84,&effects)||!e.local(root+88,&ik,true))return false;
    if(!e.field(common+0x16c,4)||!e.array(model+16,5,1,1)||!e.dynamics(dynamics)||
       !e.field(scale,4)||!e.array(bubbles,2,20,4)||!e.array(collision,6,2,2)||
       !e.array(effects,5,4,4)||!e.part_animations((uint32_t)root))return false;
    uint32_t count,inits;
    if(!e.value(hurt,4,&count)||count>256||!e.local(hurt+4,&inits)||!e.array(inits,count,40,4))return false;
    if(ik!=UINT32_MAX)for(unsigned at:{0u,1u,8u,9u,16u,17u})if(!e.field(ik+at,1))return false;
    std::vector<NativeFighterActionCommand> commands,action;
    for(unsigned a=0;a<project->action_count;++a){
        size_t count;
        if(!nativeFighterAction_Commands(project,a,nullptr,0,&count))return fail("Could not trace the fighter's subaction graph.");
        action.resize(count);
        if(count&&!nativeFighterAction_Commands(project,a,action.data(),count,&count))return fail("Could not read the fighter's subaction graph.");
        commands.insert(commands.end(),action.begin(),action.end());
    }
    std::sort(commands.begin(),commands.end(),[](const auto& a,const auto& b){return a.source_offset<b.source_offset;});
    commands.erase(std::unique(commands.begin(),commands.end(),[](const auto& a,const auto& b){return a.source_offset==b.source_offset;}),commands.end());
    for(const auto& command:commands){
        uint32_t at=command.source_offset,raw;
        if(!e.value(at,4,&raw))return false;
        switch(command.opcode){
        case 10: // GFX: named roles and model-selected bones retain their selectors.
            if(!(raw&((1u<<17)|(1u<<15)))&&!e.bits(at,18))return false;break;
        case 11:case NATIVE_HITBOX_TIMED_OPCODE:
            if(!(raw&(1u<<10))&&!e.bits(at,11))return false;break;
        case 28:if(!e.bits(at,18))return false;break;
        case 58:if(!e.bits(at,0))return false;break;
        default:break;
        }
    }
    // Retail create-hitbox reads byte 1 bit 4 of the following command as
    // hit_grabbed_victim_only. GFX/hurt-state bone indices overlap that bit.
    // An owned timed hitbox carries the flag itself; a remaining retail
    // instruction must retain its original meaning until converted.
    for(const auto& command:commands)if(command.opcode==11){
        uint32_t following;
        const uint32_t at=command.source_offset+20;
        if(!e.value(at,4,&following))return false;
        if((following^NativeAssetDat::read32(e.bytes.data()+32+at))&(1u<<19))
            return e.fail("Convert the affected action's hitboxes to owned intervals before moving this bone",command.source_offset);
    }
    auto* bytes=(uint8_t*)std::malloc(e.bytes.size());
    if(!bytes)return fail("Could not allocate the rewritten fighter bone references.");
    std::memcpy(bytes,e.bytes.data(),e.bytes.size());
    *output={bytes,e.bytes.size(),e.metadata,e.commands};
    if(error&&capacity)error[0]=0;return true;
}
