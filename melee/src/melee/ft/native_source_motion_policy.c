#include "native_source_motion_policy.h"
#include "kinds/ftCommon/forward.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan where,const char* text)
{if(d){d->source=where;snprintf(d->message,sizeof(d->message),"%s",text);}return false;}
static bool number(const char** cursor,uint32_t* output)
{
    const char* p=*cursor;uint32_t value=0;
    if(*p<'0'||*p>'9')return false;
    do {unsigned digit=(unsigned)(*p-'0');if(value>(UINT32_MAX-digit)/10)return false;value=value*10+digit;++p;}while(*p>='0'&&*p<='9');
    *cursor=p;*output=value;return true;
}

bool nativeSourceMotionPolicy_Owns(const NativeScriptSourceTree* tree,uint32_t node)
{
    return tree && node<tree->count && tree->nodes[node].kind==NativeScriptSource_Metadata &&
        nativeScriptSource_SpanEquals(tree,tree->nodes[node].name,"motion_policy");
}

static bool parse(const NativeScriptSourceTree* tree,uint32_t node,
    const NativeSourceImportedAssets* assets,uint32_t* index,uint32_t* common,NativeScriptDiagnostic* d)
{
    if(!nativeSourceMotionPolicy_Owns(tree,node))return false;
    const NativeScriptSourceNode* n=&tree->nodes[node];
    if(n->parent || !assets || !assets->ready || !assets->motions ||
       assets->facts.moves.common_count!=ftCo_MS_Count)
        return error(d,n->source,"Common motion policy requires root scope and exact imported Fighter assets.");
    if(n->value.offset>tree->source_bytes || n->value.length>tree->source_bytes-n->value.offset || n->value.length>=64)
        return error(d,n->source,"Common motion policy declaration exceeds its bounded format.");
    char text[64];memcpy(text,tree->source+n->value.offset,n->value.length);text[n->value.length]=0;
    uint32_t motion=0,shared=0;const char* cursor=text;
    if(!number(&cursor,&motion) || strncmp(cursor," common ",8) ||
       (cursor+=8,!number(&cursor,&shared)) || *cursor ||
       motion<assets->facts.moves.first || motion-assets->facts.moves.first>=assets->facts.moves.count ||
       shared>=ftCo_MS_Count)
        return error(d,n->source,"Motion policy needs an owned numeric motion and canonical common motion index.");
    *index=motion-assets->facts.moves.first;*common=shared;return true;
}

bool nativeSourceMotionPolicy_Prepare(const NativeScriptSourceTree* tree,
    const MotionState* common,NativeSourceImportedAssets* assets,NativeScriptDiagnostic* d)
{
    if(!tree || !common || !assets || !assets->ready || !assets->motions || assets->facts.moves.count>NATIVE_SOURCE_MOTIONS)
        return error(d,(NativeScriptSourceSpan){0},"Common motion preparation requires stable imported owned rows.");
    uint16_t selected[NATIVE_SOURCE_MOTIONS];
    memset(selected,0xff,sizeof(selected));
    // Validate the entire candidate before publishing any common callback.
    for(uint32_t node=0;node<tree->count;++node)if(nativeSourceMotionPolicy_Owns(tree,node)) {
        uint32_t index,common;if(!parse(tree,node,assets,&index,&common,d))return false;
        if(selected[index]!=UINT16_MAX)return error(d,tree->nodes[node].source,"Duplicate common motion policy.");
        selected[index]=(uint16_t)common;
    }
    for(uint32_t index=0;index<assets->facts.moves.count;++index)if(selected[index]!=UINT16_MAX) {
        MotionState* owned=&assets->motions[index];const MotionState* shared=&common[selected[index]];
        owned->anim_cb=shared->anim_cb;owned->input_cb=shared->input_cb;
        owned->phys_cb=shared->phys_cb;owned->coll_cb=shared->coll_cb;
    }
    assets->common_motions=common;
    return true;
}

bool nativeSourceMotionPolicy_Validate(const NativeScriptSourceTree* tree,uint32_t node,
    const NativeSourceImportedAssets* assets,NativeScriptDiagnostic* d)
{
    uint32_t index,common;if(!parse(tree,node,assets,&index,&common,d))return false;
    if(!assets->common_motions)return error(d,tree->nodes[node].source,"Canonical common motion table is not pinned.");
    const MotionState* owned=&assets->motions[index];const MotionState* shared=&assets->common_motions[common];
    if(owned->anim_cb!=shared->anim_cb || owned->input_cb!=shared->input_cb ||
       owned->phys_cb!=shared->phys_cb || owned->coll_cb!=shared->coll_cb)
        return error(d,tree->nodes[node].source,"Common motion policy differs from its actual owned callback rows.");
    return true;
}
