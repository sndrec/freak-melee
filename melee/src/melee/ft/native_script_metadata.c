#include "native_script_metadata.h"
#include <sysdolphin/baselib/dat.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan source,const char* text)
{
    if(diagnostic){diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",text);}return false;
}

static bool declaration(const NativeScriptProgramIndex* index,const NativeScriptValueDeclaration* field,
    NativeScriptDiagnostic* diagnostic)
{
    if(!field->parameter || !field->imported || field->count!=1 || field->array ||
        field->source_node>=index->tree->count || index->tree->nodes[field->source_node].kind!=NativeScriptSource_Attribute ||
        (field->type!=NativeScriptType_Bool && field->type!=NativeScriptType_I32 &&
         field->type!=NativeScriptType_U32 && field->type!=NativeScriptType_F32))
        return error(diagnostic,field->name,"Imported parameter shape/type differs from its indexed Attribute declaration");
    return true;
}

bool nativeScriptMetadata_ReadAttribute(const HSD_Dat* dat,uint32_t begin,uint32_t bytes,
    const NativeScriptValueDeclaration* field,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    uint32_t width=field->type==NativeScriptType_Bool || field->type==NativeScriptType_U32 ? 1 : 4;
    if((width==4 && (field->import_offset&3)) || field->import_offset>bytes || width>bytes-field->import_offset)
        return error(diagnostic,field->name,"Imported attribute exceeds its actual owned DAT block or alignment");
    NativeScriptValue result={.type=(uint8_t)field->type};
    if(width==1){
        const u8* data=HSD_DatBytes(dat,begin+field->import_offset,1);
        if(!data || (field->type==NativeScriptType_Bool && *data>1))
            return error(diagnostic,field->name,"Imported Boolean/byte attribute is invalid");
        result.as.u64=*data;
    }else{
        u32 bits;
        if(!HSD_DatReadU32(dat,begin+field->import_offset,&bits))
            return error(diagnostic,field->name,"Imported fighter attribute word is unavailable");
        if(field->type==NativeScriptType_F32){
            memcpy(&result.as.f32,&bits,sizeof(float));
            if(!isfinite(result.as.f32))return error(diagnostic,field->name,"Imported Float attribute is nonfinite");
        }else result.as.i64=bits<=INT32_MAX ? (int64_t)bits : -1-(int64_t)(UINT32_MAX-bits);
    }
    *output=result;return true;
}

bool nativeScriptMetadata_ImportFighterParameters(const NativeScriptProgramIndex* index,
    const NativeScriptMetadataDat* source,NativeScriptParameterSupply* supplies,uint32_t capacity,
    uint32_t* count,NativeScriptMetadataImport* report,NativeScriptDiagnostic* diagnostic)
{
    if(!index || !index->tree || !index->tree->nodes || !index->tree->source ||
        (index->value_count && !index->values) || !count || !report || !diagnostic)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Missing indexed fighter import inputs");
    NativeScriptMetadataImport imports={0};
    for(uint32_t i=0;i<index->value_count;++i){
        const NativeScriptValueDeclaration* field=&index->values[i];
        if(!field->imported)continue;
        if(!declaration(index,field,diagnostic))return false;
        if(field->owner){++imports.unresolved_entity_imports;continue;}
        ++imports.fighter_imports;
    }
    if(imports.fighter_imports>capacity || (imports.fighter_imports && !supplies))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Fighter parameter supply capacity exhausted");
    HSD_Dat dat={0};uint32_t attributes=0,bytes=0;
    if(imports.fighter_imports){
        u32 root,begin;const char* external;
        if(!source || !source->bytes || !source->fighter_root ||
            HSD_DatOpen(&dat,source->bytes,source->size)!=HSD_DAT_OK ||
            !HSD_DatFindRoot(&dat,source->fighter_root,&root) || root>UINT32_MAX-4 ||
            HSD_DatResolve(&dat,root+4,&begin,&external)!=HSD_DAT_REF_LOCAL || !begin)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Project DAT lacks the selected fighter's owned attribute block");
        uint32_t end=HSD_DatNextTarget(&dat,begin);
        if(end<begin || !HSD_DatBytes(&dat,begin,end-begin))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Project DAT fighter attribute block has invalid bounds");
        attributes=begin;bytes=end-begin;imports.attribute_bytes=bytes;
    }
    // Validation pass makes all typed values available without publishing an
    // incomplete supplies table. Source bytes remain read-only throughout.
    for(uint32_t i=0;i<index->value_count;++i){
        const NativeScriptValueDeclaration* field=&index->values[i];NativeScriptValue checked;
        if(field->imported && !field->owner && !nativeScriptMetadata_ReadAttribute(&dat,attributes,bytes,field,&checked,diagnostic))return false;
    }
    uint32_t written=0;
    for(uint32_t i=0;i<index->value_count;++i){
        const NativeScriptValueDeclaration* field=&index->values[i];
        if(!field->imported || field->owner)continue;
        NativeScriptValue checked;
        if(!nativeScriptMetadata_ReadAttribute(&dat,attributes,bytes,field,&checked,diagnostic))return false;
        supplies[written++]=(NativeScriptParameterSupply){.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,.owner=0,.value=checked};
    }
    *count=written;*report=imports;return true;
}
