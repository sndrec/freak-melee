#include "native_source_declarations.h"
#include "native_fighter_sound_policy.h"
#include <stdio.h>
#include <string.h>

enum { Mute=1,Surface,Visibility,Remap,Ko };
typedef struct Policies {
    NativeFighterSoundIdRule rules[NATIVE_FIGHTER_SOUND_RULE_CAPACITY];
    NativeFighterKoCue cues[NATIVE_FIGHTER_KO_CUE_CAPACITY];
    uint16_t visibility;
    uint8_t mute,rule_count,cue_count;
    bool mute_declared,surface_declared,sound_declared,ignore_surface;
} Policies;
static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan where,const char* text)
{if(d){d->source=where;snprintf(d->message,sizeof(d->message),"%s",text);}return false;}
static unsigned kind(const NativeScriptSourceTree* tree,const NativeScriptSourceNode* n)
{
    if(n->kind!=NativeScriptSource_Metadata)return 0;
    if(nativeScriptSource_SpanEquals(tree,n->name,"sound_mute_channels"))return Mute;
    if(nativeScriptSource_SpanEquals(tree,n->name,"surface_friction"))return Surface;
    if(nativeScriptSource_SpanEquals(tree,n->name,"visibility_asset"))return Visibility;
    if(nativeScriptSource_SpanEquals(tree,n->name,"sound_remap"))return Remap;
    if(nativeScriptSource_SpanEquals(tree,n->name,"sound_ko_cue"))return Ko;
    return 0;
}
bool nativeSourceDeclarations_Owns(const NativeScriptSourceTree* tree,uint32_t at)
{return tree&&tree->nodes&&at<tree->count&&kind(tree,&tree->nodes[at])!=0;}

static bool integers(const NativeScriptSourceTree* tree,NativeScriptSourceSpan span,
                     int64_t* values,unsigned count)
{
    if(span.offset>tree->source_bytes || span.length>tree->source_bytes-span.offset)return false;
    const char* bytes=tree->source+span.offset;uint32_t at=0;
    for(unsigned i=0;i<count;++i) {
        while(at<span.length&&(bytes[at]==' '||bytes[at]=='\t'))++at;
        bool negative=at<span.length&&bytes[at]=='-';if(negative)++at;
        unsigned base=10;
        if(at+1<span.length&&bytes[at]=='0'&&(bytes[at+1]=='x'||bytes[at+1]=='X')){base=16;at+=2;}
        uint64_t magnitude=0;unsigned digits=0;
        while(at<span.length) {
            unsigned char c=(unsigned char)bytes[at];unsigned digit=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:16;
            if(digit>=base)break;
            if(magnitude>(UINT32_MAX-digit)/base)return false;
            magnitude=magnitude*base+digit;++at;++digits;
        }
        if(!digits || (at<span.length&&bytes[at]!=' '&&bytes[at]!='\t'&&bytes[at]!='#'))return false;
        values[i]=negative?-(int64_t)magnitude:(int64_t)magnitude;
    }
    while(at<span.length&&(bytes[at]==' '||bytes[at]=='\t'))++at;
    return at==span.length || bytes[at]=='#';
}
static bool parse(const NativeScriptSourceTree* tree,Policies* p,NativeScriptDiagnostic* d)
{
    if(!tree||!tree->source||!tree->nodes||!tree->count)return error(d,(NativeScriptSourceSpan){0},"Missing owned source declaration tree");
    *p=(Policies){0};bool code_seen=false;
    for(uint32_t i=1;i<tree->count;++i) {
        const NativeScriptSourceNode* n=&tree->nodes[i];unsigned type=kind(tree,n);
        if(!type) {
            if(n->parent==0&&(n->kind==NativeScriptSource_Function||n->kind==NativeScriptSource_Move||n->kind==NativeScriptSource_Action||n->kind==NativeScriptSource_Article||n->kind==NativeScriptSource_Item||n->kind==NativeScriptSource_Effect))code_seen=true;
            continue;
        }
        if(n->parent!=0||n->opens||code_seen)return error(d,n->source,"Owned asset recipes must be root declarations before authored code");
        int64_t v[5]={0};
        if(type==Surface) {
            if(p->surface_declared)return error(d,n->source,"Duplicate surface friction recipe");
            if(nativeScriptSource_SpanEquals(tree,n->value,"ignore"))p->ignore_surface=true;
            else if(!nativeScriptSource_SpanEquals(tree,n->value,"stage"))return error(d,n->source,"Expected surface_friction stage or ignore");
            p->surface_declared=true;continue;
        }
        if(!integers(tree,n->value,v,type==Remap?5:type==Ko?2:1))return error(d,n->source,"Owned asset recipe requires exact integer literals");
        if(type==Mute) {
            if(p->mute_declared||v[0]<0||v[0]>127)return error(d,n->source,"Duplicate or out-of-range sound channel mask");
            p->mute_declared=true;p->mute=(uint8_t)v[0];
        } else if(type==Visibility) {
            if(v[0]<0||v[0]>=16||(p->visibility&(1u<<(unsigned)v[0])))return error(d,n->source,"Duplicate or out-of-range owned visibility slot");
            p->visibility|=(uint16_t)(1u<<(unsigned)v[0]);
        } else if(type==Remap) {
            if(p->rule_count==NATIVE_FIGHTER_SOUND_RULE_CAPACITY||v[0]<=0||v[0]>UINT32_MAX||v[1]<0||v[1]>=64||v[2]<0||v[2]>INT32_MAX||v[3]<v[2]||v[3]>INT32_MAX||v[4]<INT32_MIN||v[4]>INT32_MAX||v[2]+v[4]<0||v[3]+v[4]>INT32_MAX)
                return error(d,n->source,"Sound remap exceeds native mask/bank/range or mapped Int32 bounds");
            p->rules[p->rule_count++]=(NativeFighterSoundIdRule){(uint32_t)v[0],(int32_t)v[1],(int32_t)v[2],(int32_t)v[3],(int32_t)v[4]};p->sound_declared=true;
        } else {
            if(p->cue_count==NATIVE_FIGHTER_KO_CUE_CAPACITY||v[0]<=0||v[0]>UINT32_MAX||v[1]<0||v[1]>INT32_MAX)return error(d,n->source,"KO cue exceeds native mask/Int32 bounds");
            for(unsigned j=0;j<p->cue_count;++j)if(p->cues[j].costume_mask&(uint32_t)v[0])return error(d,n->source,"Overlapping KO costume masks");
            p->cues[p->cue_count++]=(NativeFighterKoCue){(uint32_t)v[0],(int32_t)v[1]};p->sound_declared=true;
        }
    }
    NativeFighterSoundPolicy sound={p->rule_count?p->rules:NULL,p->cue_count?p->cues:NULL,p->rule_count,p->cue_count};
    return nativeFighterSoundPolicy_Validate(&sound)||error(d,(NativeScriptSourceSpan){0},"Invalid owned sound recipe table");
}
bool nativeSourceDeclarations_Prepare(const NativeScriptSourceTree* tree,
    const NativeSourceAssetFacts* input,NativeSourceAssetFacts* output,NativeScriptDiagnostic* d)
{
    if(!input||!output||!nativeSourceAssets_Validate(input,d))return false;
    Policies p;if(!parse(tree,&p,d))return false;
    NativeSourceAssetFacts candidate=*input;
    if(p.mute_declared)candidate.sound_mute_channels=p.mute;
    if(p.surface_declared)candidate.ignore_surface_friction=p.ignore_surface;
    candidate.layout.visibility_mask|=p.visibility;
    if(p.sound_declared) {
        memset(candidate.sound_rules,0,sizeof(candidate.sound_rules));memset(candidate.ko_cues,0,sizeof(candidate.ko_cues));
        candidate.sound_rule_count=p.rule_count;candidate.ko_cue_count=p.cue_count;
        memcpy(candidate.sound_rules,p.rules,(size_t)p.rule_count*sizeof(*p.rules));memcpy(candidate.ko_cues,p.cues,(size_t)p.cue_count*sizeof(*p.cues));
    }
    if(!nativeSourceAssets_Validate(&candidate,d))return false;
    *output=candidate;return true;
}
bool nativeSourceDeclarations_Validate(const NativeScriptSourceTree* tree,uint32_t at,
    const NativeSourceAssetFacts* facts,NativeScriptDiagnostic* d)
{
    if(!nativeSourceDeclarations_Owns(tree,at))return false;
    if(!facts)return error(d,tree->nodes[at].source,"Owned asset recipes require their prepared source facts");
    Policies p;if(!parse(tree,&p,d)||!nativeSourceAssets_Validate(facts,d))return false;
    if((p.mute_declared&&facts->sound_mute_channels!=p.mute)||
       (p.surface_declared&&facts->ignore_surface_friction!=p.ignore_surface)||
       (facts->layout.visibility_mask&p.visibility)!=p.visibility||
       (p.sound_declared&&(facts->sound_rule_count!=p.rule_count||facts->ko_cue_count!=p.cue_count||
        memcmp(facts->sound_rules,p.rules,(size_t)p.rule_count*sizeof(*p.rules))||
        memcmp(facts->ko_cues,p.cues,(size_t)p.cue_count*sizeof(*p.cues)))))
        return error(d,tree->nodes[at].source,"Source asset recipes differ from their exact prepared owned facts");
    return true;
}
