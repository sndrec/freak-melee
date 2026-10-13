#include "native_script_resource_metadata.h"
#include "native_source_asset_facts.h"
#include "ftparts.h"
#include <stdio.h>
#include <string.h>

typedef struct Cursor { const char* text; uint32_t length,at; } Cursor;
static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan at,const char* text)
{if(d){d->source=at;snprintf(d->message,sizeof(d->message),"%s",text);}return false;}
static bool space(char c){return c==' '||c=='\t';}
static bool token(Cursor* c,const char* word)
{
    while(c->at<c->length&&space(c->text[c->at]))++c->at;
    uint32_t begin=c->at;while(c->at<c->length&&!space(c->text[c->at]))++c->at;
    return c->at-begin==strlen(word)&&!memcmp(c->text+begin,word,c->at-begin);
}
static bool number(Cursor* c,uint32_t* output)
{
    while(c->at<c->length&&space(c->text[c->at]))++c->at;
    unsigned base=10;
    if(c->length-c->at>=2&&c->text[c->at]=='0'&&(c->text[c->at+1]=='x'||c->text[c->at+1]=='X')){base=16;c->at+=2;}
    uint32_t value=0,digits=0;
    while(c->at<c->length&&!space(c->text[c->at])){
        char x=c->text[c->at++];unsigned digit;
        if(x>='0'&&x<='9')digit=(unsigned)(x-'0');
        else if(x>='a'&&x<='f')digit=(unsigned)(x-'a'+10);
        else if(x>='A'&&x<='F')digit=(unsigned)(x-'A'+10);
        else return false;
        if(digit>=base||value>(UINT32_MAX-digit)/base)return false;
        value=value*base+digit;++digits;
    }
    if(!digits)return false;*output=value;return true;
}
static bool parse(const NativeScriptSourceTree* tree,const NativeScriptSourceNode* node,
    const NativeSourceAssetFacts* facts,NativeScriptResourceRecipe* output,NativeScriptDiagnostic* d)
{
    NativeScriptSourceSpan span=node->value;
    if(node->opens||node->first_child!=UINT32_MAX||span.offset>tree->source_bytes||span.length>tree->source_bytes-span.offset)
        return error(d,node->source,"Resources recipe cannot open a block or have invalid bytes");
    Cursor cursor={tree->source+span.offset,span.length,0};uint32_t capacity,part;
    if(!token(&cursor,"capacity")||!number(&cursor,&capacity)||capacity<NATIVE_SCRIPT_RESOURCE_CAPACITY_MIN||capacity>NATIVE_SCRIPT_RESOURCE_CAPACITY_MAX)
        return error(d,node->source,"Resources capacity must be an exact integer from 2 through 90");
    NativeScriptResourceRecipe recipe={(uint16_t)capacity,-1};
    while(cursor.at<cursor.length&&space(cursor.text[cursor.at]))++cursor.at;
    if(cursor.at<cursor.length){
        if(!token(&cursor,"reserve_part")||!number(&cursor,&part)||part>=MAX_FT_PARTS)
            return error(d,node->source,"Resources reserve_part must be one exact native part index");
        if(!facts||!facts->joints||facts->joints>MAX_FT_PARTS||
           facts->omission_count>NATIVE_SOURCE_OMISSIONS||
           facts->omission_count>MAX_FT_PARTS-facts->joints||
           part<(uint32_t)facts->joints+facts->omission_count)
            return error(d,node->source,"Resources reserve_part must be outside the final owned Fighter rig");
        recipe.reserved_part=(int16_t)part;
        while(cursor.at<cursor.length&&space(cursor.text[cursor.at]))++cursor.at;
        if(cursor.at!=cursor.length)return error(d,node->source,"Resources recipe has duplicate or trailing fields");
    }
    *output=recipe;return true;
}
bool nativeScriptResourceMetadata_Prepare(const NativeScriptSourceTree* tree,uint32_t owner,
    const NativeSourceAssetFacts* facts,NativeScriptResourceRecipe* output,NativeScriptDiagnostic* d)
{
    if(!tree||!tree->source||!tree->nodes||tree->count>tree->capacity||!output||!owner||owner>=tree->count||
       (tree->nodes[owner].kind!=NativeScriptSource_Article&&tree->nodes[owner].kind!=NativeScriptSource_Item))
        return error(d,(NativeScriptSourceSpan){0},"Resources recipe requires an exact Article/Item owner");
    NativeScriptResourceRecipe recipe={0,-1};bool seen=false;
    for(uint32_t node=1;node<tree->count;++node){
        const NativeScriptSourceNode* field=&tree->nodes[node];
        if(field->kind!=NativeScriptSource_Metadata||!nativeScriptSource_SpanEquals(tree,field->name,"resources"))continue;
        uint32_t at=field->parent;
        for(uint32_t bound=0;at!=owner&&at;++bound){
            if(at>=tree->count||bound==tree->count)return error(d,field->source,"Resources owner ancestry is invalid");
            if(tree->nodes[at].kind==NativeScriptSource_Article||tree->nodes[at].kind==NativeScriptSource_Item||tree->nodes[at].kind==NativeScriptSource_Effect)break;
            at=tree->nodes[at].parent;
        }
        if(at!=owner)continue;
        if(field->parent!=owner||seen)return error(d,field->source,"Resources recipe must be a singleton directly inside its entity");
        if(!parse(tree,field,facts,&recipe,d))return false;seen=true;
    }
    *output=recipe;return true;
}
