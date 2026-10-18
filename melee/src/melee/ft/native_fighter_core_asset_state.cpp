#include "native_fighter_core_asset_state.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>

namespace {
bool fail(char* error,size_t capacity,const char* message)
{if(error&&capacity)std::snprintf(error,capacity,"%s",message);return false;}
}

bool nativeFighterCoreAssets_Capture(const NativeFighterProject& p,
                                    NativeFighterCoreAssetState& output) try
{
    const uint8_t* data[]={p.fighter_dat,p.animation_dat,p.costume_dat,p.asset_record};
    const size_t sizes[]={p.fighter_size,p.animation_size,p.costume_size,p.asset_record_size};
    NativeFighterCoreAssetState candidate;
    for(unsigned n=0;n<4;++n){
        if(!data[n]||!sizes[n]||sizes[n]>NATIVE_CONTENT_MAX_PACKAGE_BYTES)return false;
        candidate.bytes[n].assign(data[n],data[n]+sizes[n]);
    }
    candidate.facts=p.facts;candidate.rig_revision=p.rig_edit_revision;
    output=std::move(candidate);return true;
}
catch(const std::bad_alloc&){return false;}

bool nativeFighterCoreAssets_Matches(const NativeFighterProject& p,
                                    const NativeFighterCoreAssetState& state)
{
    if(p.rig_edit_revision!=state.rig_revision||std::memcmp(&p.facts,&state.facts,sizeof(p.facts)))return false;
    const uint8_t* data[]={p.fighter_dat,p.animation_dat,p.costume_dat,p.asset_record};
    const size_t sizes[]={p.fighter_size,p.animation_size,p.costume_size,p.asset_record_size};
    for(unsigned n=0;n<4;++n)
        if(!data[n]||sizes[n]!=state.bytes[n].size()||
           std::memcmp(data[n],state.bytes[n].data(),sizes[n]))return false;
    return true;
}

bool nativeFighterCoreAssets_Restore(NativeFighterProject& p,
    const NativeFighterCoreAssetState& expected,
    const NativeFighterCoreAssetState& replacement,char* error,size_t capacity)
{
    if(!nativeFighterCoreAssets_Matches(p,expected))
        return fail(error,capacity,"Other fighter data has changed since this edit. Undo those changes first.");
    if(replacement.rig_revision>p.rig_edit_serial)
        return fail(error,capacity,"This bone edit belongs to a different draft history.");
    using Owned=std::unique_ptr<uint8_t,decltype(&std::free)>;
    Owned storage[4]={{nullptr,std::free},{nullptr,std::free},{nullptr,std::free},{nullptr,std::free}};
    NativeFighterProject candidate=p;
    uint8_t** data[]={&candidate.fighter_dat,&candidate.animation_dat,&candidate.costume_dat,&candidate.asset_record};
    size_t* sizes[]={&candidate.fighter_size,&candidate.animation_size,&candidate.costume_size,&candidate.asset_record_size};
    for(unsigned n=0;n<4;++n){
        const auto& bytes=replacement.bytes[n];
        if(bytes.empty()||bytes.size()>NATIVE_CONTENT_MAX_PACKAGE_BYTES)
            return fail(error,capacity,"The bone edit has an invalid asset size.");
        storage[n].reset((uint8_t*)std::malloc(bytes.size()));
        if(!storage[n])return fail(error,capacity,"Could not allocate storage to restore the bone edit.");
        std::memcpy(storage[n].get(),bytes.data(),bytes.size());*data[n]=storage[n].get();*sizes[n]=bytes.size();
    }
    candidate.facts=replacement.facts;candidate.rig_edit_revision=replacement.rig_revision;
    if(!nativeFighterProject_ValidateAssets(&candidate,error,capacity))return false;
    std::free(p.fighter_dat);std::free(p.animation_dat);std::free(p.costume_dat);std::free(p.asset_record);
    p.fighter_dat=storage[0].release();p.fighter_size=candidate.fighter_size;
    p.animation_dat=storage[1].release();p.animation_size=candidate.animation_size;
    p.costume_dat=storage[2].release();p.costume_size=candidate.costume_size;
    p.asset_record=storage[3].release();p.asset_record_size=candidate.asset_record_size;
    p.facts=candidate.facts;p.rig_edit_revision=candidate.rig_edit_revision;
    if(error&&capacity)error[0]=0;return true;
}
