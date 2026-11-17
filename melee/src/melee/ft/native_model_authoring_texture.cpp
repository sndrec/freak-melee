#include "native_model_authoring_internal.h"
#include "native_fighter_material_texture.h"
#include "native_asset_dat.h"

namespace NativeModelAuthoring {
namespace {
const char* light_names[]={"diffuse_contribution","specular_contribution","ambient_contribution","extended_contribution","shadow_contribution","bump_mapping"};
const unsigned light_bits[]={16,32,64,128,256,1u<<24};
const char* combine_names[]={"color_operation","alpha_operation","color_bias","alpha_bias","color_scale","alpha_scale","color_clamp","alpha_clamp"};
const char* color_names[]={"constant","register0","register1"};
std::string integer(unsigned minimum,unsigned maximum) {return "{\"type\":\"integer\",\"minimum\":"+std::to_string(minimum)+",\"maximum\":"+std::to_string(maximum)+"}";}
std::string array(unsigned n,const std::string& item) {return "{\"type\":\"array\",\"minItems\":"+std::to_string(n)+",\"maxItems\":"+std::to_string(n)+",\"items\":"+item+"}";}
const char* scalar=R"({"type":"number","minimum":-10000,"maximum":10000})";
const char* flag=R"({"type":"boolean"})";
std::string schema()
{
    std::string fields="\"animation_id\":"+integer(0,7)+",\"coordinate_source\":"+integer(0,20)+",\"mapping\":"+integer(0,6)+
        ",\"color_operation\":"+integer(0,8)+",\"alpha_operation\":"+integer(0,7)+",\"blend\":"+scalar;
    for(const char* name:light_names) fields+=",\""+std::string(name)+"\":"+flag;
    for(const char* name:{"rotation","scale","offset"}) fields+=",\""+std::string(name)+"\":"+array(3,scalar);
    for(const char* name:{"wrap_u","wrap_v"}) fields+=",\""+std::string(name)+"\":"+integer(0,2);
    for(const char* name:{"repeat_u","repeat_v"}) fields+=",\""+std::string(name)+"\":"+integer(1,255);
    fields+=",\"magnification\":"+integer(0,1);
    fields+=R"(,"lod":{"type":"object","properties":{"enabled":{"type":"boolean"},"minification":)"+integer(0,5)+
        ",\"anisotropy\":"+integer(0,2)+",\"bias\":"+scalar+",\"clamp_bias\":"+flag+",\"edge_lod\":"+flag+R"(},"additionalProperties":false})";
    fields+=R"(,"combiner":{"type":"object","properties":{"enabled":{"type":"boolean"})";
    for(unsigned i=0;i<8;++i) fields+=",\""+std::string(combine_names[i])+"\":"+(i<2?std::string(R"({"type":"integer","enum":[0,1,8,9,10,11,12,13,14,15]})"):i<4?integer(0,2):i<6?integer(0,3):std::string(flag));
    fields+=",\"color_inputs\":"+array(4,R"({"type":"integer","enum":[8,9,12,13,15,128,129,130,131,132,133,134,135,136]})")+
        ",\"alpha_inputs\":"+array(4,R"({"type":"integer","enum":[4,7,64,65,66,67,68,69]})");
    for(const char* name:color_names) fields+=",\""+std::string(name)+"\":"+array(4,integer(0,255));
    fields+=",\"overrides\":"+array(12,flag)+",\"color_expression\":"+flag+",\"alpha_expression\":"+flag+R"(},"additionalProperties":false})";
    return "{\"type\":\"object\",\"properties\":{"+fields+"},\"additionalProperties\":false}";
}
void patch_vector(yyjson_val* values,float* out,unsigned n) {if(values) for(unsigned i=0;i<n;++i) out[i]=(float)yyjson_get_num(yyjson_arr_get(values,i));}
void patch_bit(yyjson_val* object,const char* name,unsigned bit,u32& flags) {if(has(object,name)) flags=boolean(object,name)?flags|bit:flags&~bit;}
}
const char* texture_schema() {static const auto result=schema();return result.c_str();}
bool texture_properties(const NativeFighterModelEditor& model,unsigned material,unsigned layer,Reply& r,yyjson_mut_val* out)
{
    NativeMaterialTextureState state;
    if(!nativeFighterMaterial_ReadTexture(model,material,layer,state)) {r.fail("unknown_texture_layer","Choose an inspected material texture layer.");return false;}
    yyjson_mut_obj_add_uint(r.document,out,"animation_id",state.animation_id);yyjson_mut_obj_add_uint(r.document,out,"coordinate_source",state.coordinate_source);
    yyjson_mut_obj_add_uint(r.document,out,"mapping",state.flags&15);yyjson_mut_obj_add_uint(r.document,out,"color_operation",(state.flags>>16)&15);yyjson_mut_obj_add_uint(r.document,out,"alpha_operation",(state.flags>>20)&15);
    yyjson_mut_obj_add_real(r.document,out,"blend",state.blend);
    for(unsigned i=0;i<6;++i) yyjson_mut_obj_add_bool(r.document,out,light_names[i],(state.flags&light_bits[i])!=0);
    vector(r,out,"rotation",state.transform);vector(r,out,"scale",state.transform+3);vector(r,out,"offset",state.transform+6);
    yyjson_mut_obj_add_uint(r.document,out,"wrap_u",state.wrap[0]);yyjson_mut_obj_add_uint(r.document,out,"wrap_v",state.wrap[1]);
    yyjson_mut_obj_add_uint(r.document,out,"repeat_u",state.repeat[0]);yyjson_mut_obj_add_uint(r.document,out,"repeat_v",state.repeat[1]);yyjson_mut_obj_add_uint(r.document,out,"magnification",state.magnification);
    auto* lod=yyjson_mut_obj(r.document);yyjson_mut_obj_add_val(r.document,out,"lod",lod);
    yyjson_mut_obj_add_bool(r.document,lod,"enabled",state.lod_enabled);yyjson_mut_obj_add_uint(r.document,lod,"minification",state.minification);yyjson_mut_obj_add_uint(r.document,lod,"anisotropy",state.anisotropy);
    yyjson_mut_obj_add_real(r.document,lod,"bias",state.lod_bias);yyjson_mut_obj_add_bool(r.document,lod,"clamp_bias",state.clamp_bias);yyjson_mut_obj_add_bool(r.document,lod,"edge_lod",state.edge_lod);
    auto* combine=yyjson_mut_obj(r.document);yyjson_mut_obj_add_val(r.document,out,"combiner",combine);
    yyjson_mut_obj_add_bool(r.document,combine,"enabled",state.combiner_enabled);
    for(unsigned i=0;i<8;++i) {if(i<6) yyjson_mut_obj_add_uint(r.document,combine,combine_names[i],state.combine[i]);else yyjson_mut_obj_add_bool(r.document,combine,combine_names[i],state.combine[i]!=0);}
    for(unsigned c=0;c<2;++c) {auto* inputs=yyjson_mut_arr(r.document);yyjson_mut_obj_add_val(r.document,combine,c?"alpha_inputs":"color_inputs",inputs);for(unsigned i=0;i<4;++i) yyjson_mut_arr_add_uint(r.document,inputs,state.combine[8+c*4+i]);}
    for(unsigned c=0;c<3;++c) {auto* rgba=yyjson_mut_arr(r.document);yyjson_mut_obj_add_val(r.document,combine,color_names[c],rgba);for(unsigned i=0;i<4;++i) yyjson_mut_arr_add_uint(r.document,rgba,state.combine[16+c*4+i]);}
    auto* overrides=yyjson_mut_arr(r.document);yyjson_mut_obj_add_val(r.document,combine,"overrides",overrides);for(unsigned i=0;i<12;++i) yyjson_mut_arr_add_bool(r.document,overrides,(state.active&(1u<<i))!=0);
    yyjson_mut_obj_add_bool(r.document,combine,"color_expression",(state.active&(1u<<30))!=0);yyjson_mut_obj_add_bool(r.document,combine,"alpha_expression",(state.active&(1u<<31))!=0);
    return true;
}
bool edit_texture(NativeFighterProject& p,unsigned material,unsigned layer,yyjson_val* fields,Reply& r)
{
    std::unique_ptr<NativeFighterModelEditor> model;if(!open(p,model,r)) return false;
    NativeMaterialTextureState state;
    if(!nativeFighterMaterial_ReadTexture(*model,material,layer,state)) {r.fail("unknown_texture_layer","Choose an inspected material texture layer.");return false;}
    const auto original=state;
    if(!fields || !yyjson_obj_size(fields)) {r.fail("empty_texture_edit","Supply changed texture properties.");return false;}
    if(has(fields,"animation_id")) state.animation_id=number(fields,"animation_id");if(has(fields,"coordinate_source")) state.coordinate_source=number(fields,"coordinate_source");
    const char* maps[]={"mapping","color_operation","alpha_operation"};const unsigned shifts[]={0,16,20};
    for(unsigned i=0;i<3;++i) if(has(fields,maps[i])) state.flags=(state.flags&~(15u<<shifts[i]))|(number(fields,maps[i])<<shifts[i]);
    for(unsigned i=0;i<6;++i) patch_bit(fields,light_names[i],light_bits[i],state.flags);
    for(unsigned i=0;i<2;++i) {const char* wrap=i?"wrap_v":"wrap_u";const char* repeat=i?"repeat_v":"repeat_u";if(has(fields,wrap)) state.wrap[i]=number(fields,wrap);if(has(fields,repeat)) state.repeat[i]=(uint8_t)number(fields,repeat);}
    if(has(fields,"blend")) state.blend=(float)real(fields,"blend");if(has(fields,"magnification")) state.magnification=number(fields,"magnification");
    patch_vector(yyjson_obj_get(fields,"rotation"),state.transform,3);patch_vector(yyjson_obj_get(fields,"scale"),state.transform+3,3);patch_vector(yyjson_obj_get(fields,"offset"),state.transform+6,3);
    if(auto* lod=yyjson_obj_get(fields,"lod")) {
        if(has(lod,"enabled")) state.lod_enabled=boolean(lod,"enabled");if(has(lod,"minification")) state.minification=number(lod,"minification");if(has(lod,"anisotropy")) state.anisotropy=number(lod,"anisotropy");
        if(has(lod,"bias")) state.lod_bias=(float)real(lod,"bias");if(has(lod,"clamp_bias")) state.clamp_bias=boolean(lod,"clamp_bias");if(has(lod,"edge_lod")) state.edge_lod=boolean(lod,"edge_lod");
    }
    if(auto* combine=yyjson_obj_get(fields,"combiner")) {
        if(has(combine,"enabled")) state.combiner_enabled=boolean(combine,"enabled");
        for(unsigned i=0;i<8;++i) if(has(combine,combine_names[i])) state.combine[i]=(uint8_t)(i<6?number(combine,combine_names[i]):boolean(combine,combine_names[i]));
        for(unsigned c=0;c<2;++c) if(auto* inputs=yyjson_obj_get(combine,c?"alpha_inputs":"color_inputs")) for(unsigned i=0;i<4;++i) state.combine[8+c*4+i]=(uint8_t)yyjson_get_uint(yyjson_arr_get(inputs,i));
        for(unsigned c=0;c<3;++c) if(auto* color=yyjson_obj_get(combine,color_names[c])) for(unsigned i=0;i<4;++i) state.combine[16+c*4+i]=(uint8_t)yyjson_get_uint(yyjson_arr_get(color,i));
        if(auto* overrides=yyjson_obj_get(combine,"overrides")) for(unsigned i=0;i<12;++i) state.active=yyjson_get_bool(yyjson_arr_get(overrides,i))?state.active|(1u<<i):state.active&~(1u<<i);
        patch_bit(combine,"color_expression",1u<<30,state.active);patch_bit(combine,"alpha_expression",1u<<31,state.active);
    }
    if(state==original) return true;
    NativeAssetDat dat;std::vector<uint8_t> bytes;std::string error;
    if(!dat.open(p.costume_dat,p.costume_size) || !nativeFighterMaterial_StageTexture(*model,material,layer,state,dat,error) || !dat.finish(&bytes)) {r.fail("texture_edit_rejected",error.empty()?"Could not encode this texture change.":error);return false;}
    auto* owned=(uint8_t*)std::malloc(bytes.size());if(!owned) {r.fail("allocation_failed","Could not allocate the edited texture.");return false;}
    std::memcpy(owned,bytes.data(),bytes.size());std::free(p.costume_dat);p.costume_dat=owned;p.costume_size=bytes.size();
    if(!open(p,model,r)) return false;
    for(unsigned mesh=0;mesh<model->mesh_count;++mesh) if(model->meshes[mesh].material==material && !nativeFighterModel_MaterialInputs(model.get(),mesh,material,&model->dat,model->materials[material].render_mode)) {
        r.fail("missing_material_inputs","The texture needs UV coordinates absent from a mesh using this material.");return false;
    }
    return true;
}
}
