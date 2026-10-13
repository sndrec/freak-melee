#include "native_source_parameters.h"
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan span,const char* text)
{if(d){d->source=span;snprintf(d->message,sizeof(d->message),"%s",text);}return false;}
bool nativeSourceParameters_Parse(NativeSourceParameters* output,NativeScriptDiagnostic* d)
{
    if(!output||output->text_bytes>NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY||(!output->text&&output->text_bytes))return false;
    unsigned line=0;
    if(!nativeFighterParameters_Parse(output->text?output->text:"",output->text_bytes,&output->overrides,&line))
        return error(d,(NativeScriptSourceSpan){.line=line},"Invalid owned source parameter sidecar");
    return true;
}
static bool named(const NativeScriptProgramIndex* index,const NativeScriptValueDeclaration* field,const char* name)
{
    const NativeScriptSourceTree* tree=index->tree;size_t length=strlen(name),prefix=0;
    if(field->owner) {
        if(field->owner>=tree->count)return false;NativeScriptSourceSpan owner=tree->nodes[field->owner].name;
        if(owner.length>=length||memcmp(name,tree->source+owner.offset,owner.length)||name[owner.length]!='.')return false;
        prefix=owner.length+1;
    }
    return length-prefix==field->name.length&&!memcmp(name+prefix,tree->source+field->name.offset,field->name.length);
}
static bool value(const NativeFighterParameter* override,const NativeScriptValueDeclaration* field,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    NativeScriptValue converted={.type=field->type};
    if(override->type==NativeFighterParameter_Float&&field->type==NativeScriptType_F32)memcpy(&converted.as.f32,&override->bits,4);
    else if(override->type==NativeFighterParameter_Bool&&field->type==NativeScriptType_Bool)converted.as.u64=override->bits;
    else if(override->type==NativeFighterParameter_Int&&field->type==NativeScriptType_I32)
        converted.as.i64=override->bits<=INT32_MAX?(int64_t)override->bits:-1-(int64_t)(UINT32_MAX-override->bits);
    else if(override->type==NativeFighterParameter_Int&&field->type==NativeScriptType_U32) {
        if(override->bits>INT32_MAX||(field->imported&&override->bits>UINT8_MAX))return error(d,field->name,"Unsigned/byte sidecar override is outside its exact declared range");
        converted.as.u64=override->bits;
    } else return error(d,field->name,"Sidecar override type differs from its exact indexed parameter");
    *output=converted;return true;
}
bool nativeSourceParameters_Provide(void* opaque,const NativeScriptProgramIndex* index,
    const NativeScriptParameterSupply** output,uint32_t* count,NativeScriptDiagnostic* d)
{
    NativeSourceParameters* p=opaque;
    if(!p||!index||!index->tree||!output||!count||index->value_count>NATIVE_SOURCE_VALUES)return error(d,(NativeScriptSourceSpan){0},"Missing/bounded indexed source parameter inputs");
    uint32_t roots=0,articles=0;NativeScriptMetadataImport report;
    if(!nativeScriptMetadata_ImportFighterParameters(index,&p->fighter,p->supplies,NATIVE_SOURCE_VALUES,&roots,&report,d))return false;
    if(!nativeScriptItemMetadata_ImportParameters(index,p->items,p->supplies+roots,NATIVE_SOURCE_VALUES-roots,&articles,d))return false;
    if(report.unresolved_entity_imports!=articles)return error(d,(NativeScriptSourceSpan){0},"Imported source entity attributes have no complete owned resource provider");
    uint32_t written=roots+articles;
    for(unsigned o=0;o<p->overrides.count;++o) {
        const NativeFighterParameter* override=&p->overrides.fields[o];uint32_t selected=UINT32_MAX;
        for(uint32_t f=0;f<index->value_count;++f)if(index->values[f].parameter&&named(index,&index->values[f],override->name)) {
            if(selected!=UINT32_MAX)return error(d,index->values[f].name,"Sidecar parameter name is ambiguous in the source index");selected=f;
        }
        if(selected==UINT32_MAX)return error(d,(NativeScriptSourceSpan){0},"Sidecar override does not name an exact declared source parameter");
        const NativeScriptValueDeclaration* field=&index->values[selected];NativeScriptValue converted;
        if(field->array||field->count!=1||!value(override,field,&converted,d))return false;
        uint32_t semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC|selected,slot=0;
        while(slot<written&&p->supplies[slot].semantic!=semantic)++slot;
        if(slot==written){if(written==NATIVE_SOURCE_VALUES)return error(d,field->name,"Source parameter supply capacity exhausted");++written;}
        p->supplies[slot]=(NativeScriptParameterSupply){semantic,field->owner,converted};
    }
    p->count=written;p->root_imports=roots;p->article_imports=articles;p->override_count=p->overrides.count;
    *output=p->supplies;*count=written;return true;
}
