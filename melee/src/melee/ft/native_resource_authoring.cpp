#include "native_resource_authoring.h"
#include "native_authoring_file.h"
#include "native_gx_image.h"
#include "native_source_item_blueprint_io.h"
#include <sysdolphin/baselib/dat.h>
#include <melee/lb/native_disc_archive.h>
#include <melee/it/forward.h>
#include <aurora/dvd.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <malloc.h>
#include <memory>

namespace NativeResourceAuthoring {
using NativeAuthoring::Reply;
using NativeAuthoring::add_string;
namespace {
constexpr size_t BlueprintDatLimit=(size_t)NATIVE_SOURCE_RESOURCE_BLOB_BYTES-NATIVE_SOURCE_BLUEPRINT_HEADER;
struct CommonItem {ItemKind kind;const char* name;};
constexpr CommonItem common_items[]={
    {It_Kind_Capsule,"Capsule"},{It_Kind_Box,"Crate"},{It_Kind_Taru,"Barrel"},
    {It_Kind_Egg,"Egg"},{It_Kind_Kusudama,"Party Ball"},{It_Kind_TaruCann,"Barrel Cannon"},
    {It_Kind_BombHei,"Bob-omb"},{It_Kind_Dosei,"Mr. Saturn"},{It_Kind_Heart,"Heart Container"},
    {It_Kind_Tomato,"Maxim Tomato"},{It_Kind_Star,"Super Star"},{It_Kind_Bat,"Home-Run Bat"},
    {It_Kind_Sword,"Beam Sword"},{It_Kind_Parasol,"Parasol"},{It_Kind_G_Shell,"Green Shell"},
    {It_Kind_R_Shell,"Red Shell"},{It_Kind_L_Gun,"Ray Gun"},{It_Kind_Freeze,"Freezie"},
    {It_Kind_Foods,"Food"},{It_Kind_MSBomb,"Proximity Mine"},{It_Kind_Flipper,"Flipper"},
    {It_Kind_S_Scope,"Super Scope"},{It_Kind_StarRod,"Star Rod"},{It_Kind_LipStick,"Lip's Stick"},
    {It_Kind_Harisen,"Fan"},{It_Kind_F_Flower,"Fire Flower"},{It_Kind_Kinoko,"Super Mushroom"},
    {It_Kind_DKinoko,"Poison Mushroom"},{It_Kind_Hammer,"Hammer"},{It_Kind_WStar,"Warp Star"},
    {It_Kind_ScBall,"Screw Attack"},{It_Kind_RabbitC,"Bunny Hood"},{It_Kind_MetalB,"Metal Box"},
    {It_Kind_Spycloak,"Cloaking Device"},{It_Kind_M_Ball,"Poke Ball"},{It_Kind_L_Gun_Ray,"Ray Gun Recoil"},
    {It_Kind_StarRod_Star,"Star Rod Star"},{It_Kind_LipStick_Spore,"Lip's Stick Spore"},
    {It_Kind_S_Scope_Beam,"Super Scope Beam"},{It_Kind_L_Gun_Beam,"Ray Gun Beam"},
    {It_Kind_Hammer_Head,"Hammer Head"},{It_Kind_F_Flower_Flame,"Fire Flower Flame"},
    {It_Kind_EvYoshiEgg,"Event Yoshi Egg"}
};
static_assert(std::size(common_items)==43 && It_Kind_EvYoshiEgg==42);
const char* text(yyjson_val* value,const char* key)
{const char* result=yyjson_get_str(yyjson_obj_get(value,key));return result?result:"";}
bool matches(const char* value,const char* query)
{
    const auto lower=[](unsigned char c){return c>='A'&&c<='Z'?c+('a'-'A'):c;};
    const std::string a=value,b=query;
    return std::search(a.begin(),a.end(),b.begin(),b.end(),[&](char x,char y){return lower(x)==lower(y);})!=a.end();
}
bool bundle(const NativeFighterProject& p,NativeSourceResources& view,Reply& reply)
{
    char error[256]{};
    if(nativeSourceResources_Read(p.resources,p.resource_size,&view,error,sizeof(error)))return true;
    reply.fail("invalid_resource_bundle",error);return false;
}
bool image_valid(const void* bytes,size_t size,Reply& reply)
{
    uint16_t width=0,height=0;char error[256]{};
    if(!nativeSourceResources_PNG(bytes,size,&width,&height,error,sizeof(error))) {
        reply.fail("invalid_resource_png",error);return false;
    }
    unsigned short decoded_width=0,decoded_height=0;
    auto* pixels=nativeGxImage_Decode((const unsigned char*)bytes,(unsigned)size,&decoded_width,&decoded_height);
    const bool ok=pixels && width==decoded_width && height==decoded_height;
    nativeGxImage_Free(pixels);
    if(!ok)reply.fail("invalid_resource_png","The PNG's pixels could not be decoded at its declared dimensions.");
    return ok;
}
void hash(Reply& reply,yyjson_mut_val* row,const void* bytes,size_t size)
{
    NativeContentId id{};char hex[65]{};
    if(nativeContent_Hash(bytes,size,&id))nativeContentStore_IdHex(&id,hex);
    add_string(reply,row,"sha256",hex);
}
void describe(Reply& reply,yyjson_mut_val* row,const NativeSourceResourceEntry& resource)
{
    add_string(reply,row,"name",resource.name);
    add_string(reply,row,"kind",resource.kind==NativeSourceResource_PNG?"png":"blob");
    add_string(reply,row,"format",resource.kind==NativeSourceResource_PNG?"png":
        !std::strcmp(resource.name,"sounds.ssm")?"sound_bank":
        resource.size>=4&&!std::memcmp(resource.bytes,"FMIB",4)?"item_blueprint":"opaque");
    yyjson_mut_obj_add_uint(reply.document,row,"bytes",resource.size);
    yyjson_mut_obj_add_uint(reply.document,row,"width",resource.width);
    yyjson_mut_obj_add_uint(reply.document,row,"height",resource.height);
    hash(reply,row,resource.bytes,resource.size);
}
}
void read(const NativeFighterProject& project,yyjson_val* args,Reply& reply)
{
    NativeSourceResources view{};if(!bundle(project,view,reply))return;
    auto* rows=yyjson_mut_arr(reply.document);yyjson_mut_obj_add_val(reply.document,reply.result,"resources",rows);
    const unsigned offset=(unsigned)yyjson_get_uint(yyjson_obj_get(args,"offset"));
    const unsigned limit=yyjson_obj_get(args,"limit")?(unsigned)yyjson_get_uint(yyjson_obj_get(args,"limit")):50;
    unsigned total=0;
    for(unsigned i=0;i<view.count;++i) {
        const auto& entry=view.entries[i];
        if(!matches(entry.name,text(args,"query")))continue;
        if(total++<offset || yyjson_mut_arr_size(rows)>=limit)continue;
        auto* row=yyjson_mut_obj(reply.document);yyjson_mut_arr_append(rows,row);describe(reply,row,entry);
    }
    yyjson_mut_obj_add_uint(reply.document,reply.result,"total",total);
    yyjson_mut_obj_add_uint(reply.document,reply.result,"bundle_entries",view.count);
    yyjson_mut_obj_add_uint(reply.document,reply.result,"bundle_bytes",project.resource_size);
    if(offset+limit<total)yyjson_mut_obj_add_uint(reply.document,reply.result,"next_offset",offset+limit);
    else yyjson_mut_obj_add_null(reply.document,reply.result,"next_offset");
    auto* limits=yyjson_mut_obj(reply.document);yyjson_mut_obj_add_val(reply.document,reply.result,"limits",limits);
    yyjson_mut_obj_add_uint(reply.document,limits,"entries",NATIVE_SOURCE_RESOURCE_ENTRIES);
    yyjson_mut_obj_add_uint(reply.document,limits,"name_bytes",NATIVE_SOURCE_RESOURCE_NAME_BYTES-1);
    yyjson_mut_obj_add_uint(reply.document,limits,"bundle_bytes",NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES);
    yyjson_mut_obj_add_uint(reply.document,limits,"blob_bytes",NATIVE_SOURCE_RESOURCE_BLOB_BYTES);
    yyjson_mut_obj_add_uint(reply.document,limits,"png_bytes",NATIVE_SOURCE_RESOURCE_PNG_BYTES);
    yyjson_mut_obj_add_uint(reply.document,limits,"png_dimension",NATIVE_SOURCE_RESOURCE_PNG_DIMENSION);
    auto* articles=yyjson_mut_arr(reply.document);yyjson_mut_obj_add_val(reply.document,reply.result,"articles",articles);
    for(unsigned i=0;i<project.facts.article_count;++i) {
        const auto& asset=project.facts.articles[i];auto* row=yyjson_mut_obj(reply.document);yyjson_mut_arr_append(articles,row);
        add_string(reply,row,"declaration",asset.declaration);yyjson_mut_obj_add_uint(reply.document,row,"slot",asset.slot);
        yyjson_mut_obj_add_int(reply.document,row,"kind",asset.kind);
    }
    auto* banks=yyjson_mut_arr(reply.document);yyjson_mut_obj_add_val(reply.document,reply.result,"sound_banks",banks);
    for(unsigned i=0;i<64;++i)if(project.facts.sound_banks&(UINT64_C(1)<<i))yyjson_mut_arr_add_uint(reply.document,banks,i);
    yyjson_mut_obj_add_uint(reply.document,reply.result,"effect_bank",project.facts.effect_bank);
    auto* items=yyjson_mut_arr(reply.document);yyjson_mut_obj_add_val(reply.document,reply.result,"common_items",items);
    for(const auto& item:common_items) {
        auto* row=yyjson_mut_obj(reply.document);yyjson_mut_arr_append(items,row);
        yyjson_mut_obj_add_uint(reply.document,row,"stock_kind",item.kind);add_string(reply,row,"name",item.name);
    }
    auto* formats=yyjson_mut_arr(reply.document);yyjson_mut_obj_add_val(reply.document,reply.result,"asset_formats",formats);
    for(const auto& entry:{std::pair{"textures","Named PNG resources; costume images use models.edit."},
        std::pair{"items","Owned DAT/Article blueprints plus item declarations in editable scripts."},
        std::pair{"articles","Owned fighter-DAT descriptors plus article declarations in editable scripts."},
        std::pair{"sounds","Import a DSP-ADPCM SSM as the blob sounds.ssm. Play its clips with sound.clip(channel, index, volume, pan)."},
        std::pair{"effects","Use effect-bank IDs or import a model DAT as an effects/*.dat blob for a model_effect declaration."}}) {
        auto* row=yyjson_mut_obj(reply.document);yyjson_mut_arr_append(formats,row);add_string(reply,row,"category",entry.first);add_string(reply,row,"authoring",entry.second);
    }
}
bool edit(NativeFighterProject& project,yyjson_val* args,Reply& reply,History* history)
{
    NativeSourceResources view{};if(!bundle(project,view,reply))return false;
    std::vector<NativeSourceResourceInput> entries;entries.reserve(NATIVE_SOURCE_RESOURCE_ENTRIES);
    for(unsigned i=0;i<view.count;++i) {const auto& row=view.entries[i];entries.push_back({row.name,row.bytes,row.size,row.kind});}
    std::vector<std::vector<uint8_t>> payloads;payloads.reserve(64);
    size_t remaining_import_bytes=NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES;
    std::vector<std::string> names;
    size_t index,count;yyjson_val* change;
    yyjson_arr_foreach(yyjson_obj_get(args,"changes"),index,count,change) {
        const char* name=text(change,"name");
        if(!nativeSourceResources_NameValid(name) || std::find(names.begin(),names.end(),name)!=names.end()) {
            reply.fail("invalid_resource_name","Supply each canonical logical resource name once.");return false;
        }
        names.emplace_back(name);
        auto found=std::find_if(entries.begin(),entries.end(),[&](const auto& row){return !std::strcmp(row.name,name);});
        const bool remove=!std::strcmp(text(change,"operation"),"remove");
        if(remove && found==entries.end()) {reply.fail("unknown_resource",std::string("No exact resource named ")+name);return false;}
        if(found!=entries.end())entries.erase(found);
        if(remove) {
            if(yyjson_obj_get(change,"path") || yyjson_obj_get(change,"kind")) {reply.fail("invalid_resource_change","A removal takes only its operation and logical name.");return false;}
            continue;
        }
        const bool png=!std::strcmp(text(change,"kind"),"png");
        if(!*text(change,"path") || (!png && std::strcmp(text(change,"kind"),"blob"))) {
            reply.fail("invalid_resource_change","An import needs its file path and png or blob kind.");return false;
        }
        payloads.emplace_back();auto& bytes=payloads.back();
        size_t limit=std::min<size_t>(remaining_import_bytes,png?NATIVE_SOURCE_RESOURCE_PNG_BYTES:NATIVE_SOURCE_RESOURCE_BLOB_BYTES);
        if(!NativeAuthoring::read_file(text(change,"path"),limit,bytes)) {
            reply.fail("resource_input_unavailable","Choose a nonempty readable file within the file and total bundle size limits.");return false;
        }
        remaining_import_bytes-=bytes.size();
        if(png && !image_valid(bytes.data(),bytes.size(),reply))return false;
        if(!png && bytes.size()>=4 && !std::memcmp(bytes.data(),"FMIB",4)) {
            NativeScriptDiagnostic diagnostic{};
            if(!nativeSourceItemBlueprint_Validate(bytes.data(),bytes.size(),&diagnostic)) {reply.fail("invalid_item_blueprint",diagnostic.message);return false;}
        }
        entries.push_back({name,bytes.data(),bytes.size(),png?NativeSourceResource_PNG:NativeSourceResource_Blob});
    }
    char error[256]{};size_t size=0,written=0;
    if(!nativeSourceResources_Measure(entries.data(),(uint32_t)entries.size(),&size,error,sizeof(error))) {reply.fail("resource_edit_rejected",error);return false;}
    auto bytes=std::unique_ptr<uint8_t,decltype(&std::free)>((uint8_t*)std::malloc(size?size:1),std::free);
    if(!bytes || !nativeSourceResources_Write(entries.data(),(uint32_t)entries.size(),bytes.get(),size,&written,error,sizeof(error))) {
        reply.fail("resource_edit_rejected",*error?error:"Could not allocate the resource bundle.");return false;
    }
    // Keep the optional absent representation when removing the final entry.
    if(entries.empty())written=0;
    const bool changed=written!=project.resource_size || (written && std::memcmp(bytes.get(),project.resources,written));
    if(changed && history) {
        History next;
        if(project.resource_size)next.before.assign(project.resources,project.resources+project.resource_size);
        if(written)next.after.assign(bytes.get(),bytes.get()+written);
        *history=std::move(next);
    }
    if(changed) {std::free(project.resources);project.resources=written?bytes.release():nullptr;project.resource_size=written;}
    yyjson_mut_obj_add_bool(reply.document,reply.result,"changed",changed);
    yyjson_mut_obj_add_bool(reply.document,reply.result,"saved",false);
    yyjson_mut_obj_add_bool(reply.document,reply.result,"installed",false);
    yyjson_mut_obj_add_uint(reply.document,reply.result,"bundle_entries",entries.size());
    yyjson_mut_obj_add_uint(reply.document,reply.result,"bundle_bytes",written);return true;
}
bool restore(NativeFighterProject& project,const History& history,bool redo,std::string& error)
{
    const auto& expected=redo?history.before:history.after;
    const auto& replacement=redo?history.after:history.before;
    if(expected.size()!=project.resource_size || (expected.size() && std::memcmp(expected.data(),project.resources,expected.size()))) {
        error="Resources changed since this edit. Undo the intervening resource edits first.";return false;
    }
    NativeSourceResources view{};
    if(!nativeSourceResources_Read(replacement.empty()?nullptr:replacement.data(),replacement.size(),&view,nullptr,0)) {
        error="The saved resource history is invalid.";return false;
    }
    uint8_t* bytes=nullptr;
    if(!replacement.empty()) {
        bytes=(uint8_t*)std::malloc(replacement.size());
        if(!bytes) {error="Could not allocate the restored resources.";return false;}
        std::memcpy(bytes,replacement.data(),replacement.size());
    }
    std::free(project.resources);project.resources=bytes;project.resource_size=replacement.size();return true;
}
void export_resource(const NativeFighterProject& project,yyjson_val* args,Reply& reply)
{
    NativeSourceResources view{};if(!bundle(project,view,reply))return;
    const NativeSourceResourceEntry* entry=nullptr;
    for(unsigned i=0;i<view.count;++i)if(!std::strcmp(view.entries[i].name,text(args,"name")))entry=&view.entries[i];
    if(!entry) {reply.fail("unknown_resource","Choose the exact resource name returned by resources.read.");return;}
    if(NativeAuthoring::export_file(text(args,"output"),entry->bytes,entry->size,reply)) {
        add_string(reply,reply.result,"name",entry->name);
        hash(reply,reply.result,entry->bytes,entry->size);
    }
}
void blueprint(yyjson_val* args,Reply& reply)
{
    std::vector<uint8_t> dat;uint32_t offset=(uint32_t)yyjson_get_uint(yyjson_obj_get(args,"article_offset"));
    if(yyjson_obj_get(args,"stock_kind")) {
        if(yyjson_obj_get(args,"dat") || yyjson_obj_get(args,"article_offset") || !*text(args,"iso")) {reply.fail("invalid_blueprint_source","A stock item needs only iso and stock_kind as its source.");return;}
        if(!aurora_dvd_open(text(args,"iso"))) {reply.fail("iso_unavailable","Could not open the supplied Melee ISO.");return;}
        void* bytes=nullptr;size_t size=0;HSD_Dat archive;u32 root,table,article;const char* external;
        const unsigned kind=(unsigned)yyjson_get_uint(yyjson_obj_get(args,"stock_kind"));
        const bool ok=nativeDiscArchive_Read("ItCo.dat",&bytes,&size) && HSD_DatOpen(&archive,bytes,size)==HSD_DAT_OK &&
            HSD_DatFindRoot(&archive,"itPublicData",&root) && HSD_DatResolve(&archive,root+4,&table,&external)==HSD_DAT_REF_LOCAL &&
            HSD_DatBytes(&archive,table,43*4) && HSD_DatNextTarget(&archive,table)>=table+43*4 &&
            HSD_DatResolve(&archive,table+kind*4,&article,&external)==HSD_DAT_REF_LOCAL;
        if(ok && size<=BlueprintDatLimit) {dat.assign((uint8_t*)bytes,(uint8_t*)bytes+size);offset=(uint32_t)article;}
        _aligned_free(bytes);
        if(dat.empty()) {reply.fail("stock_item_unavailable","No bounded common Article was found at this stock item kind.");return;}
    } else {
        if(!*text(args,"dat") || !yyjson_obj_get(args,"article_offset") || yyjson_obj_get(args,"iso")) {reply.fail("invalid_blueprint_source","Supply a DAT file and its explicit Article descriptor offset.");return;}
        if(!NativeAuthoring::read_file(text(args,"dat"),BlueprintDatLimit,dat)) {
            reply.fail("resource_input_unavailable","Choose a nonempty readable DAT within the item blueprint size limit.");return;
        }
    }
    std::vector<uint8_t> envelope(dat.size()+NATIVE_SOURCE_BLUEPRINT_HEADER);size_t written=0;NativeScriptDiagnostic diagnostic{};
    if(!nativeSourceItemBlueprint_Write(dat.data(),dat.size(),offset,envelope.data(),envelope.size(),&written,&diagnostic)) {
        reply.fail("invalid_item_blueprint",diagnostic.message);return;
    }
    const bool valid=nativeSourceItemBlueprint_Validate(envelope.data(),written,&diagnostic);
    if(!valid) {reply.fail("invalid_item_blueprint",diagnostic.message);return;}
    if(NativeAuthoring::export_file(text(args,"output"),envelope.data(),written,reply)) {
        hash(reply,reply.result,envelope.data(),written);
        yyjson_mut_obj_add_uint(reply.document,reply.result,"article_offset",offset);
        yyjson_mut_obj_add_uint(reply.document,reply.result,"dat_bytes",dat.size());
        yyjson_mut_obj_add_bool(reply.document,reply.result,"graph_validated",true);
    }
}
}
