#include "native_fighter_material_texture.h"
#include "native_asset_dat.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace {
u32 pointer(const HSD_Dat& dat, u32 slot)
{
    u32 target; const char* external;
    return HSD_DatResolve(&dat,slot,&target,&external)==HSD_DAT_REF_LOCAL ? target : 0;
}
void real(NativeAssetDat& dat, u32 offset, float value)
{u32 bits; std::memcpy(&bits,&value,4); dat.word(offset,bits);}
}
unsigned nativeFighterMaterial_TextureLayers(const NativeFighterModelEditor& model, unsigned material, u32 offsets[8])
{
    if(material>=model.material_count) return 0;
    unsigned count=0; u32 at=model.materials[material].texture_offset;
    while(at && count<8 && HSD_DatBytes(&model.dat,at,92)) {
        for(unsigned i=0;i<count;++i) if(offsets[i]==at) return count;
        offsets[count++]=at; at=pointer(model.dat,at+4);
    }
    return count;
}
bool nativeFighterMaterial_ReadTexture(const NativeFighterModelEditor& model, unsigned material, unsigned layer, NativeMaterialTextureState& state)
{
    u32 offsets[8]; const unsigned count=nativeFighterMaterial_TextureLayers(model,material,offsets);
    if(layer>=count) return false;
    const auto& dat=model.dat; const u32 at=offsets[layer];
    state={};
    std::fill_n(state.combine+8,4,15);std::fill_n(state.combine+12,4,7);
    if(!HSD_DatReadU32(&dat,at+8,&state.animation_id) || !HSD_DatReadU32(&dat,at+12,&state.coordinate_source) ||
       !HSD_DatReadU32(&dat,at+64,&state.flags) || !HSD_DatReadF32(&dat,at+68,&state.blend) ||
       !HSD_DatReadU32(&dat,at+52,&state.wrap[0]) || !HSD_DatReadU32(&dat,at+56,&state.wrap[1]) ||
       !HSD_DatReadU32(&dat,at+72,&state.magnification)) return false;
    for(unsigned i=0;i<9;++i) if(!HSD_DatReadF32(&dat,at+16+i*4,&state.transform[i])) return false;
    std::memcpy(state.repeat,HSD_DatBytes(&dat,at+60,2),2);
    const u32 lod=pointer(dat,at+84),tev=pointer(dat,at+88);
    if(lod) {
        if(!HSD_DatBytes(&dat,lod,16)) return false;
        state.lod_enabled=true;
        if(!HSD_DatReadU32(&dat,lod,&state.minification) || !HSD_DatReadF32(&dat,lod+4,&state.lod_bias) ||
           !HSD_DatReadU32(&dat,lod+12,&state.anisotropy)) return false;
        const auto* flags=HSD_DatBytes(&dat,lod+8,2);state.clamp_bias=flags[0]!=0;state.edge_lod=flags[1]!=0;
    }
    if(tev) {
        if(!HSD_DatBytes(&dat,tev,32) || !HSD_DatReadU32(&dat,tev+28,&state.active)) return false;
        state.combiner_enabled=true;std::memcpy(state.combine,HSD_DatBytes(&dat,tev,28),28);
    }
    return true;
}
bool nativeFighterMaterial_StageTexture(const NativeFighterModelEditor& model, unsigned material, unsigned layer,
    const NativeMaterialTextureState& state, NativeAssetDat& dat, std::string& error)
{
    u32 offsets[8];const unsigned count=nativeFighterMaterial_TextureLayers(model,material,offsets);
    if(layer>=count) {error="Choose an existing texture layer.";return false;}
    const auto invalid=[&](){error="Enter finite texture transforms and valid mapping, filtering and color-combiner choices.";return false;};
    if(state.animation_id>7 || state.coordinate_source>20 || (state.flags&15)>6 ||
       ((state.flags>>16)&15)>8 || ((state.flags>>20)&15)>7 || state.wrap[0]>2 || state.wrap[1]>2 ||
       state.magnification>1 || !state.repeat[0] || !state.repeat[1] || !std::isfinite(state.blend)) return invalid();
    for(float value:state.transform) if(!std::isfinite(value)) return invalid();
    if(state.lod_enabled && (state.minification>5 || state.anisotropy>2 || !std::isfinite(state.lod_bias))) return invalid();
    if(state.combiner_enabled) for(unsigned i=0;i<16;++i) {
        const unsigned value=state.combine[i];
        if(i>=8 && i<12 && !(state.active&(1u<<30))) continue;
        if(i>=12 && !(state.active&(1u<<31))) continue;
        const bool valid=i<2 ? (value<2 || (value>=8 && value<=15)) : i<4 ? value<=2 :
            i<6 ? value<=3 : i<8 ? value<=1 : i<12 ? (value==8 || value==9 || value==12 || value==13 || value==15 || (value>=128 && value<=136)) :
            (value==4 || value==7 || (value>=64 && value<=69));
        if(!valid) return invalid();
    }
    const u32 at=offsets[layer];
    u32 previous_id=0;HSD_DatReadU32(&model.dat,at+8,&previous_id);
    if(previous_id!=state.animation_id && model.project->facts.costume_matanim[0]) {
        error="Animated costumes use texture IDs tied to their render slots. Keep the existing ID when replacing images or changing material properties.";return false;
    }
    dat.word(at+8,state.animation_id);dat.word(at+12,state.coordinate_source);dat.word(at+64,state.flags);
    dat.word(at+52,state.wrap[0]);dat.word(at+56,state.wrap[1]);dat.word(at+72,state.magnification);
    real(dat,at+68,state.blend);std::memcpy(dat.data.data()+at+60,state.repeat,2);
    for(unsigned i=0;i<9;++i) real(dat,at+16+i*4,state.transform[i]);
    if(state.lod_enabled) {
        u32 lod=pointer(model.dat,at+84);if(!lod) lod=dat.allocate(16);
        if(lod==UINT32_MAX) {error="Could not allocate mipmap settings.";return false;}
        dat.word(lod,state.minification);dat.word(lod+12,state.anisotropy);real(dat,lod+4,state.lod_bias);
        dat.data[lod+8]=state.clamp_bias;dat.data[lod+9]=state.edge_lod;dat.link(at+84,lod);
    } else dat.link(at+84,0);
    if(state.combiner_enabled) {
        u32 tev=pointer(model.dat,at+88);if(!tev) tev=dat.allocate(32);
        if(tev==UINT32_MAX) {error="Could not allocate texture-combiner settings.";return false;}
        std::memcpy(dat.data.data()+tev,state.combine,28);dat.word(tev+28,state.active);dat.link(at+88,tev);
    } else dat.link(at+88,0);
    return true;
}
