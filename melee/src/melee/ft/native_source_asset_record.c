#include "native_source_assets.h"
#include <stdio.h>
#include <string.h>

typedef enum AssetRecordMode {Reading,Writing,Measuring} AssetRecordMode;
typedef struct Cursor {const uint8_t* input;uint8_t* output;size_t size,at;AssetRecordMode mode;bool ok;} Cursor;
static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool overlaps(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y)return true;
    return x<y+bn && y<x+an;
}
static void bytes(Cursor* c,void* value,size_t count)
{
    if(!c->ok || count>SIZE_MAX-c->at){c->ok=false;return;}
    if(c->mode!=Measuring) {
        if(c->at>c->size || count>c->size-c->at){c->ok=false;return;}
        if(c->mode==Reading)memcpy(value,c->input+c->at,count);
        else memcpy(c->output+c->at,value,count);
    }
    c->at+=count;
}
static void word8(Cursor* c,uint8_t* value){bytes(c,value,1);}
static void word16(Cursor* c,uint16_t* value)
{
    uint8_t data[2]={(uint8_t)*value,(uint8_t)(*value>>8)};bytes(c,data,2);
    if(c->mode==Reading && c->ok)*value=(uint16_t)(data[0]|(uint16_t)data[1]<<8);
}
static void word32(Cursor* c,uint32_t* value)
{
    uint8_t data[4];for(unsigned i=0;i<4;++i)data[i]=(uint8_t)(*value>>(i*8));bytes(c,data,4);
    if(c->mode==Reading && c->ok){*value=0;for(unsigned i=0;i<4;++i)*value|=(uint32_t)data[i]<<(i*8);}
}
static void word64(Cursor* c,uint64_t* value)
{
    uint32_t lo=(uint32_t)*value,hi=(uint32_t)(*value>>32);word32(c,&lo);word32(c,&hi);
    if(c->mode==Reading && c->ok)*value=(uint64_t)lo|(uint64_t)hi<<32;
}
static void i32(Cursor* c,int32_t* value)
{uint32_t bits;memcpy(&bits,value,4);word32(c,&bits);if(c->mode==Reading&&c->ok)memcpy(value,&bits,4);}
static void string(Cursor* c,char value[NATIVE_SOURCE_NAME_BYTES])
{
    uint8_t count=(uint8_t)strnlen(value,NATIVE_SOURCE_NAME_BYTES);word8(c,&count);
    if(count>=NATIVE_SOURCE_NAME_BYTES){c->ok=false;return;}
    bytes(c,value,count);
    if(c->mode==Reading && c->ok) {
        value[count]=0;
        for(unsigned i=0;i<count;++i)if((unsigned char)value[i]<33 || (unsigned char)value[i]>126)c->ok=false;
    }
}
static void record(Cursor* c,NativeSourceAssetFacts* f)
{
    uint8_t magic[4]={'F','M','A','S'};bytes(c,magic,4);
    if(c->mode==Reading && memcmp(magic,"FMAS",4))c->ok=false;
    string(c,f->fighter_root);string(c,f->costume_joint);string(c,f->costume_matanim);
    for(unsigned i=0;i<4;++i)string(c,f->demos[i]);
    word16(c,&f->actions);word16(c,&f->demo_count);word16(c,&f->joints);word16(c,&f->bone_count);
    word16(c,&f->omission_count);word16(c,&f->article_count);word16(c,&f->pose_count);
    word16(c,&f->sound_rule_count);word16(c,&f->ko_cue_count);
    word8(c,&f->animation_tag);word8(c,&f->effect_bank);word8(c,&f->sound_mute_channels);word64(c,&f->sound_banks);
    uint8_t friction=f->ignore_surface_friction?1:0;word8(c,&friction);
    if(friction>1)c->ok=false;else if(c->mode==Reading)f->ignore_surface_friction=friction!=0;
    word16(c,&f->layout.article_mask);word16(c,&f->layout.joint_graph_mask);word16(c,&f->layout.visibility_mask);
    word8(c,&f->layout.item_format);word8(c,&f->layout.byte_range_count);
    word32(c,&f->moves.common_count);word32(c,&f->moves.first);word32(c,&f->moves.count);
    for(unsigned i=0;i<NativeScriptMove_Count;++i){word32(c,&f->moves.ground[i]);word32(c,&f->moves.air[i]);word32(c,&f->moves.transition_flags[i]);}
    if(f->bone_count>FT_COMMON_PART_COUNT || f->omission_count>NATIVE_SOURCE_OMISSIONS ||
       f->article_count>NATIVE_SOURCE_ARTICLES || f->pose_count>NATIVE_SOURCE_POSE_GRAPHS ||
       f->sound_rule_count>NATIVE_FIGHTER_SOUND_RULE_CAPACITY || f->ko_cue_count>NATIVE_FIGHTER_KO_CUE_CAPACITY ||
       f->layout.byte_range_count>FT_CHARACTER_BYTE_RANGE_LIMIT || f->moves.count>NATIVE_SOURCE_MOTIONS){c->ok=false;return;}
    for(unsigned i=0;i<f->layout.byte_range_count;++i){word16(c,&f->layout.byte_ranges[i].offset);word16(c,&f->layout.byte_ranges[i].size);}
    for(unsigned i=0;i<f->moves.count;++i){i32(c,&f->motions[i].animation);word32(c,&f->motions[i].flags);
        word32(c,&f->motions[i].attack);word8(c,&f->motions[i].camera);}
    for(unsigned i=0;i<f->bone_count;++i){uint32_t part=(uint32_t)f->bones[i].part;word32(c,&part);word8(c,&f->bones[i].joint);
        if(part>=FT_COMMON_PART_COUNT)c->ok=false;else if(c->mode==Reading)f->bones[i].part=(Fighter_Part)part;}
    for(unsigned i=0;i<f->omission_count;++i){word8(c,&f->omissions[i].joint);word8(c,&f->omissions[i].parent);
        word8(c,&f->omissions[i].insertion_mode);word8(c,&f->omissions[i].donor_joint);}
    for(unsigned i=0;i<f->article_count;++i){string(c,f->articles[i].declaration);int32_t kind=f->articles[i].kind;uint16_t offset;memcpy(&offset,&f->articles[i].kind_attribute_offset,2);
        i32(c,&kind);word16(c,&f->articles[i].slot);word16(c,&offset);
        if(c->mode==Reading){f->articles[i].kind=kind;memcpy(&f->articles[i].kind_attribute_offset,&offset,2);}}
    for(unsigned i=0;i<f->pose_count;++i){word32(c,&f->poses[i].name);word16(c,&f->poses[i].slot);string(c,f->poses[i].path);}
    for(unsigned i=0;i<f->sound_rule_count;++i){NativeFighterSoundIdRule* row=&f->sound_rules[i];
        word32(c,&row->costume_mask);i32(c,&row->bank);i32(c,&row->first);i32(c,&row->last);i32(c,&row->offset);}
    for(unsigned i=0;i<f->ko_cue_count;++i){word32(c,&f->ko_cues[i].costume_mask);i32(c,&f->ko_cues[i].sound_id);}
}
bool nativeSourceAssets_Write(const NativeSourceAssetFacts* facts,void* output,size_t capacity,size_t* written,NativeScriptDiagnostic* d)
{
    if(!output || !written || !nativeSourceAssets_Validate(facts,d) ||
       overlaps(output,capacity,facts,sizeof(*facts)) || overlaps(written,sizeof(*written),facts,sizeof(*facts)) ||
       overlaps(output,capacity,written,sizeof(*written)))return error(d,"Invalid or overlapping source asset record destination");
    NativeSourceAssetFacts candidate=*facts;Cursor measure={.mode=Measuring,.ok=true};record(&measure,&candidate);
    if(!measure.ok || measure.at>capacity)return error(d,"Source asset record destination is too small");
    Cursor writer={.output=output,.size=capacity,.mode=Writing,.ok=true};record(&writer,&candidate);
    *written=writer.at;return true;
}
bool nativeSourceAssets_Read(const void* bytes_,size_t size,NativeSourceAssetFacts* output,NativeScriptDiagnostic* d)
{
    if(!bytes_ || !output || overlaps(bytes_,size,output,sizeof(*output)))return error(d,"Invalid or overlapping source asset record input");
    NativeSourceAssetFacts candidate={0};Cursor reader={.input=bytes_,.size=size,.mode=Reading,.ok=true};record(&reader,&candidate);
    if(!reader.ok || reader.at!=size || !nativeSourceAssets_Validate(&candidate,d))return error(d,"Invalid, truncated or noncanonical source asset record");
    *output=candidate;return true;
}
