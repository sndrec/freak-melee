#include "native_script_storage.h"
#include "native_script_expression.h"

#include <stdio.h>
#include <string.h>

#define DECLARATION_DOMAIN NATIVE_SCRIPT_STORAGE_DOMAIN_MASK
#define DECLARATION_INDEX NATIVE_SCRIPT_STORAGE_INDEX_MASK

static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan source,const char* message)
{
    if (diagnostic) { diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message); }
    return false;
}

static uint32_t width(unsigned type)
{
    return type==NativeScriptType_Bool?1:type==NativeScriptType_I32||type==NativeScriptType_U32||type==NativeScriptType_F32?4:0;
}

static bool scalar_valid(const NativeScriptValue* value,unsigned type)
{
    return value->type==type && (type!=NativeScriptType_I32 ||
        (value->as.i64>=INT32_MIN&&value->as.i64<=INT32_MAX)) &&
        (type!=NativeScriptType_U32 || value->as.u64<=UINT32_MAX);
}

static uint64_t hash_number(uint64_t hash,uint64_t number)
{
    for (unsigned i=0;i<8;++i) { hash^=(uint8_t)(number>>(i*8));hash*=UINT64_C(1099511628211); }
    return hash;
}

const NativeScriptStorageOwner* nativeScriptStorage_Owner(const NativeScriptStorageLayout* layout,uint32_t owner)
{
    if (!layout || !layout->owners || layout->owner_count>layout->owner_capacity)return NULL;
    for (uint32_t i=0;i<layout->owner_count;++i)if(layout->owners[i].owner==owner)return &layout->owners[i];
    return NULL;
}

static bool owner_valid(const NativeScriptProgramIndex* program,uint32_t owner)
{
    if (owner>=program->tree->count)return false;
    unsigned kind=program->tree->nodes[owner].kind;
    return owner==0 || kind==NativeScriptSource_Article || kind==NativeScriptSource_Item || kind==NativeScriptSource_Effect;
}

static bool add_owner(NativeScriptStorageLayout* layout,uint32_t owner,NativeScriptDiagnostic* diagnostic)
{
    if (nativeScriptStorage_Owner(layout,owner))return true;
    if (!owner_valid(layout->program,owner))return error(diagnostic,(NativeScriptSourceSpan){0},"Storage declaration owner is invalid.");
    if (layout->owner_count==layout->owner_capacity)return error(diagnostic,(NativeScriptSourceSpan){0},"Storage owner capacity exhausted.");
    layout->owners[layout->owner_count++]=(NativeScriptStorageOwner){.owner=owner};return true;
}

bool nativeScriptStorage_BuildLayout(NativeScriptStorageLayout* layout,
    const NativeScriptProgramIndex* program,NativeScriptDiagnostic* diagnostic)
{
    if (!layout || !program || !program->tree || !program->tree->source || !program->tree->nodes ||
        !program->tree->count || (program->value_count&&!program->values) ||
        (program->function_count&&!program->functions) || !layout->owners ||
        (program->value_count&&!layout->fields))return error(diagnostic,(NativeScriptSourceSpan){0},"Missing storage layout/index arrays.");
    layout->program=program;layout->field_count=layout->owner_count=0;layout->identity=0;
    if (program->value_count>layout->field_capacity || program->value_count>DECLARATION_INDEX)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Storage declaration capacity exhausted.");
    uint64_t identity=UINT64_C(1469598103934665603);
    for (uint32_t i=0;i<program->tree->source_bytes;++i) { identity^=(uint8_t)program->tree->source[i];identity*=UINT64_C(1099511628211); }
    if (!add_owner(layout,0,diagnostic))goto failure;
    // Entity ownership exists even when its declaration has no mutable state,
    // parameters or callbacks. Register the indexed source owners themselves.
    for(uint32_t i=1;i<program->tree->count;++i) {
        unsigned kind=program->tree->nodes[i].kind;
        if((kind==NativeScriptSource_Article || kind==NativeScriptSource_Item ||
            kind==NativeScriptSource_Effect) && !add_owner(layout,i,diagnostic))goto failure;
    }
    for (uint32_t i=0;i<program->function_count;++i)
        if (!add_owner(layout,program->functions[i].owner,diagnostic))goto failure;
    for (uint32_t i=0;i<program->value_count;++i) {
        const NativeScriptValueDeclaration* value=&program->values[i];
        uint32_t bytes=width(value->type);
        if (!bytes || (!value->parameter&&value->type==NativeScriptType_U32) || !value->count || value->source_node>=program->tree->count ||
            !owner_valid(program,value->owner) || (value->parameter&&(value->count!=1||value->array)) ||
            (!value->array&&value->count!=1) || (value->imported&&!value->parameter)) {
            error(diagnostic,value->name,"Storage declaration shape/type is invalid.");goto failure;
        }
        unsigned kind=program->tree->nodes[value->source_node].kind;
        if ((!value->parameter&&kind!=NativeScriptSource_State) ||
            (value->parameter&&kind!=NativeScriptSource_Parameter&&kind!=NativeScriptSource_Attribute) ||
            value->imported!=(kind==NativeScriptSource_Attribute) ||
            (!value->imported&&!scalar_valid(&value->default_value,value->type))) {
            error(diagnostic,value->name,"Storage declaration/default does not match its indexed source.");goto failure;
        }
        if (!add_owner(layout,value->owner,diagnostic))goto failure;
        NativeScriptStorageOwner* owner=NULL;
        for (uint32_t o=0;o<layout->owner_count;++o)if(layout->owners[o].owner==value->owner)owner=&layout->owners[o];
        NativeScriptStorageField field={.source=value->name,.owner=value->owner,
            .offset=value->parameter?0:value->byte_offset,.count=value->count,.type=(uint8_t)value->type,
            .parameter=value->parameter,.imported=value->imported,.array=value->array};
        if (field.parameter)++owner->parameter_count;
        else {
            if (field.count>(UINT32_MAX-field.offset)/bytes || field.offset%bytes) {
                error(diagnostic,value->name,"State declaration byte range/alignment is invalid.");goto failure;
            }
            uint32_t end=field.offset+field.count*bytes;
            for (uint32_t previous=0;previous<layout->field_count;++previous) {
                const NativeScriptStorageField* other=&layout->fields[previous];
                if (other->parameter||other->owner!=field.owner)continue;
                uint32_t other_end=other->offset+other->count*width(other->type);
                if (field.offset<other_end&&other->offset<end) {
                    error(diagnostic,value->name,"State declaration byte ranges overlap.");goto failure;
                }
            }
            if (end>owner->state_bytes)owner->state_bytes=end;
        }
        layout->fields[layout->field_count++]=field;
        identity=hash_number(identity,field.owner);identity=hash_number(identity,field.offset);
        identity=hash_number(identity,field.count);identity=hash_number(identity,field.type);
        identity=hash_number(identity,field.parameter|(field.imported<<1)|(field.array<<2));
    }
    layout->identity=identity?identity:1;return true;
failure:
    layout->field_count=layout->owner_count=0;return false;
}

static const NativeScriptStorageField* declaration(const NativeScriptStorageLayout* layout,
    uint32_t semantic,uint32_t* index,NativeScriptDiagnostic* diagnostic)
{
    uint32_t domain=semantic&DECLARATION_DOMAIN,id=semantic&DECLARATION_INDEX;
    if (!layout || !layout->identity || layout->field_count>layout->field_capacity ||
        (domain!=NATIVE_SCRIPT_STATE_SEMANTIC&&domain!=NATIVE_SCRIPT_PARAMETER_SEMANTIC&&
         domain!=NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC) || id>=layout->field_count) {
        error(diagnostic,(NativeScriptSourceSpan){0},"Unknown storage semantic declaration ID.");return NULL;
    }
    const NativeScriptStorageField* field=&layout->fields[id];
    if (field->parameter!=(domain!=NATIVE_SCRIPT_STATE_SEMANTIC)) {
        error(diagnostic,field->source,"Storage semantic domain does not match the declaration.");return NULL;
    }
    if(index)*index=id;return field;
}

static bool parameters_match(const NativeScriptStorageLayout* layout,
    const NativeScriptStorageParameters* parameters,NativeScriptDiagnostic* diagnostic)
{
    return layout&&parameters&&parameters->layout_identity==layout->identity&&parameters->identity&&
        parameters->count==layout->field_count&&(!parameters->count||parameters->slots) ? true :
        error(diagnostic,(NativeScriptSourceSpan){0},"Immutable parameter table does not match the active storage layout.");
}

bool nativeScriptStorage_PrepareParameters(const NativeScriptStorageLayout* layout,
    NativeScriptParameterSlot* slots,uint32_t capacity,
    const NativeScriptParameterSupply* supplies,uint32_t supply_count,
    NativeScriptStorageParameters* result,NativeScriptDiagnostic* diagnostic)
{
    if (!layout||!layout->identity||!result||capacity<layout->field_count||
        (layout->field_count&&!slots)||(supply_count&&!supplies))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Missing or exhausted parameter table storage.");
    // Complete validation precedes publication, including duplicate overrides.
    for (uint32_t i=0;i<supply_count;++i) {
        const NativeScriptParameterSupply* supply=&supplies[i];
        if ((supply->semantic&DECLARATION_DOMAIN)!=NATIVE_SCRIPT_PARAMETER_SEMANTIC)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Parameter supply needs its canonical declaration identity.");
        const NativeScriptStorageField* field=declaration(layout,supply->semantic,NULL,diagnostic);
        if (!field)return false;
        if (!field->parameter||field->owner!=supply->owner||!scalar_valid(&supply->value,field->type))
            return error(diagnostic,field->source,"Supplied parameter owner/type does not match its declaration.");
        for (uint32_t p=0;p<i;++p)if(supplies[p].semantic==supply->semantic)
            return error(diagnostic,field->source,"Duplicate supplied immutable parameter.");
    }
    for (uint32_t i=0;i<layout->field_count;++i) {
        slots[i]=(NativeScriptParameterSlot){0};
        const NativeScriptStorageField* field=&layout->fields[i];
        if (field->parameter&&!field->imported) {
            slots[i].value=layout->program->values[i].default_value;slots[i].resolved=true;
        }
    }
    for (uint32_t i=0;i<supply_count;++i) {
        uint32_t id=supplies[i].semantic&DECLARATION_INDEX;
        slots[id]=(NativeScriptParameterSlot){.value=supplies[i].value,.resolved=true};
    }
    uint64_t identity=hash_number(UINT64_C(1469598103934665603),layout->identity);
    for (uint32_t i=0;i<layout->field_count;++i) {
        if (!layout->fields[i].parameter)continue;
        identity=hash_number(identity,i);identity=hash_number(identity,slots[i].resolved);
        if (!slots[i].resolved)continue;
        if(slots[i].value.type==NativeScriptType_Bool)slots[i].value.as.u64=slots[i].value.as.u64!=0;
        uint32_t bits;
        if(slots[i].value.type==NativeScriptType_F32)memcpy(&bits,&slots[i].value.as.f32,sizeof(bits));
        else bits=(uint32_t)slots[i].value.as.u64;
        identity=hash_number(identity,slots[i].value.type);identity=hash_number(identity,bits);
    }
    *result=(NativeScriptStorageParameters){.slots=slots,.count=layout->field_count,
        .layout_identity=layout->identity,.identity=identity?identity:1};return true;
}

bool nativeScriptStorage_ParametersReady(const NativeScriptStorageLayout* layout,
    const NativeScriptStorageParameters* parameters,uint32_t owner,
    NativeScriptDiagnostic* diagnostic)
{
    if(!nativeScriptStorage_Owner(layout,owner))return error(diagnostic,(NativeScriptSourceSpan){0},"Unknown parameter owner.");
    if(!parameters_match(layout,parameters,diagnostic))return false;
    for(uint32_t i=0;i<layout->field_count;++i) {
        const NativeScriptStorageField* field=&layout->fields[i];
        if(field->parameter&&field->owner==owner&&(!parameters->slots[i].resolved||parameters->slots[i].value.type!=field->type))
            return error(diagnostic,field->source,"Required immutable attribute/parameter value has not been supplied.");
    }
    return true;
}

static void store(uint8_t* bytes,const NativeScriptStorageField* field,const NativeScriptValue* value)
{
    if(field->type==NativeScriptType_Bool)*bytes=(uint8_t)(value->as.u64!=0);
    else if(field->type==NativeScriptType_F32)memcpy(bytes,&value->as.f32,4);
    else {uint32_t bits=(uint32_t)value->as.i64;memcpy(bytes,&bits,4);}
}

bool nativeScriptStorage_Create(const NativeScriptStorageLayout* layout,
    const NativeScriptStorageParameters* parameters,uint32_t owner,
    uint32_t identity,uint32_t generation,uint8_t* bytes,uint32_t capacity,
    NativeScriptStorageInstance* result,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptStorageOwner* row=nativeScriptStorage_Owner(layout,owner);
    if(!layout||!layout->identity||!result||!row||capacity<row->state_bytes||(row->state_bytes&&!bytes))
        return error(diagnostic,(NativeScriptSourceSpan){0},"State instance owner or buffer capacity is invalid.");
    if(parameters&&!parameters_match(layout,parameters,diagnostic))return false;
    if(row->parameter_count&&!parameters)return error(diagnostic,(NativeScriptSourceSpan){0},"State instance needs its immutable parameter table.");
    if(row->state_bytes)memset(bytes,0,row->state_bytes);
    for(uint32_t i=0;i<layout->field_count;++i) {
        const NativeScriptStorageField* field=&layout->fields[i];
        if(field->parameter||field->owner!=owner)continue;
        const NativeScriptValue* value=&layout->program->values[i].default_value;
        for(uint32_t element=0;element<field->count;++element)store(bytes+field->offset+element*width(field->type),field,value);
    }
    *result=(NativeScriptStorageInstance){layout,parameters,bytes,capacity,owner,identity,generation,true};return true;
}

void nativeScriptStorage_Destroy(NativeScriptStorageInstance* instance)
{
    if(instance)instance->live=false;
}

static bool live(const NativeScriptStorageInstance* instance,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptStorageOwner* owner=instance?nativeScriptStorage_Owner(instance->layout,instance->owner):NULL;
    return instance&&instance->live&&instance->layout&&instance->layout->identity&&owner&&
        instance->capacity>=owner->state_bytes&&(!owner->state_bytes||instance->state) ? true :
        error(diagnostic,(NativeScriptSourceSpan){0},"Storage instance is retired or its owner/buffer is invalid.");
}

static bool access(const NativeScriptStorageInstance* instance,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptStorageField** selected,
    uint32_t* declaration_index,uint32_t* element,NativeScriptDiagnostic* diagnostic)
{
    if(!live(instance,diagnostic))return false;
    const NativeScriptStorageField* field=declaration(instance->layout,semantic,declaration_index,diagnostic);
    if(!field)return false;
    if((semantic&DECLARATION_DOMAIN)!=NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC&&field->owner!=instance->owner)
        return error(diagnostic,field->source,"Storage declaration belongs to a different owner.");
    if(count!=(field->array?1u:0u)||(count&&!selectors))
        return error(diagnostic,field->source,"Storage scalar/array selector shape does not match its declaration.");
    uint64_t index=0;
    if(field->array) {
        const NativeScriptValue* value=&selectors[0];
        if(!nativeScriptExpression_IsIntegral(value->type))return error(diagnostic,field->source,"State array selector must be integral.");
        if(value->type==NativeScriptType_I32||value->type==NativeScriptType_I64) {
            if(value->as.i64<0)return error(diagnostic,field->source,"State array selector is negative.");
            index=(uint64_t)value->as.i64;
        } else index=value->type==NativeScriptType_Bool?value->as.u64!=0:value->as.u64;
        if(index>=field->count)return error(diagnostic,field->source,"State array selector is outside its declared count.");
    }
    *selected=field;*element=(uint32_t)index;return true;
}

bool nativeScriptStorage_Read(const NativeScriptStorageInstance* instance,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic* diagnostic)
{
    if(!result)return error(diagnostic,(NativeScriptSourceSpan){0},"Missing storage read output.");
    const NativeScriptStorageField* field;uint32_t id,element;
    if(!access(instance,semantic,selectors,count,&field,&id,&element,diagnostic))return false;
    NativeScriptValue value={.type=field->type};
    if(field->parameter) {
        if(!parameters_match(instance->layout,instance->parameters,diagnostic))return false;
        const NativeScriptParameterSlot* slot=&instance->parameters->slots[id];
        if(!slot->resolved||slot->value.type!=field->type)return error(diagnostic,field->source,"Immutable attribute/parameter value is unresolved.");
        value=slot->value;
    } else {
        const uint8_t* bytes=instance->state+field->offset+element*width(field->type);
        if(field->type==NativeScriptType_Bool)value.as.u64=*bytes!=0;
        else if(field->type==NativeScriptType_F32)memcpy(&value.as.f32,bytes,4);
        else {uint32_t bits;memcpy(&bits,bytes,4);value.as.i64=bits<=INT32_MAX?(int64_t)bits:-1-(int64_t)(UINT32_MAX-bits);}
    }
    *result=value;return true;
}

bool nativeScriptStorage_Write(NativeScriptStorageInstance* instance,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* input,
    NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptStorageField* field;uint32_t id,element;
    if(!access(instance,semantic,selectors,count,&field,&id,&element,diagnostic))return false;
    if(field->parameter)return error(diagnostic,field->source,"Immutable parameter storage cannot be written at runtime.");
    if(!input||!nativeScriptExpression_Assignable(field->type,input->type))return error(diagnostic,field->source,"Storage write value is not assignable to its declaration.");
    NativeScriptDiagnostic scratch;
    NativeScriptValue value;
    if(!nativeScriptValue_Convert(input,field->type,&value,diagnostic?diagnostic:&scratch))return false;
    store(instance->state+field->offset+element*width(field->type),field,&value);return true;
}

bool nativeScriptStorage_Save(const NativeScriptStorageInstance* instance,
    NativeScriptStorageSnapshot* snapshot,NativeScriptDiagnostic* diagnostic)
{
    if(!live(instance,diagnostic))return false;
    const NativeScriptStorageOwner* owner=nativeScriptStorage_Owner(instance->layout,instance->owner);
    if(!snapshot||snapshot->capacity<owner->state_bytes||(owner->state_bytes&&!snapshot->bytes))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Storage snapshot buffer is missing or too small.");
    if(instance->parameters&&!parameters_match(instance->layout,instance->parameters,diagnostic))return false;
    if(owner->state_bytes)memmove(snapshot->bytes,instance->state,owner->state_bytes);
    snapshot->state_bytes=owner->state_bytes;snapshot->owner=instance->owner;
    snapshot->identity=instance->identity;snapshot->generation=instance->generation;
    snapshot->layout_identity=instance->layout->identity;
    snapshot->parameter_identity=instance->parameters?instance->parameters->identity:0;return true;
}

bool nativeScriptStorage_Restore(NativeScriptStorageInstance* instance,
    const NativeScriptStorageSnapshot* snapshot,NativeScriptDiagnostic* diagnostic)
{
    if(!live(instance,diagnostic))return false;
    const NativeScriptStorageOwner* owner=nativeScriptStorage_Owner(instance->layout,instance->owner);
    uint64_t parameters=instance->parameters?instance->parameters->identity:0;
    if(!snapshot||snapshot->layout_identity!=instance->layout->identity||
        snapshot->parameter_identity!=parameters||snapshot->owner!=instance->owner||
        snapshot->identity!=instance->identity||snapshot->generation!=instance->generation||
        snapshot->state_bytes!=owner->state_bytes||snapshot->capacity<snapshot->state_bytes||
        (snapshot->state_bytes&&!snapshot->bytes))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Storage snapshot layout, parameters, ownership or instance lifetime does not match.");
    if(instance->parameters&&!parameters_match(instance->layout,instance->parameters,diagnostic))return false;
    if(snapshot->state_bytes)memmove(instance->state,snapshot->bytes,snapshot->state_bytes);return true;
}
