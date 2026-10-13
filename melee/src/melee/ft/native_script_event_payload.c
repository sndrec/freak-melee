#include "native_script_event_payload.h"
#include "native_script_receiver.h"
#include "native_callback_lifetime.h"

#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan span,const char* message)
{
    if (diagnostic) {
        diagnostic->source=span;
        snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    }
    return false;
}

static bool descends(const NativeScriptSourceTree* tree,uint32_t node,uint32_t parent)
{
    for (unsigned depth=0;node!=UINT32_MAX && depth<64;++depth) {
        if (node>=tree->count) return false;
        if (node==parent) return true;
        node=tree->nodes[node].parent;
    }
    return false;
}

bool nativeScriptEventPayload_ResolverContext(const NativeScriptSourceTree* tree,
    uint32_t callback,NativeScriptEventPayloadResolverContext* context,NativeScriptDiagnostic* diagnostic)
{
    if (!tree || !context || callback>=tree->count) return error(diagnostic,(NativeScriptSourceSpan){0},"Missing event payload source context");
    NativeScriptEventResolution event;
    if (!nativeScriptEvents_Resolve(tree,&tree->nodes[callback],&event,diagnostic)) return false;
    NativeScriptSourceSpan header=tree->nodes[callback].source;
    if (header.offset>tree->source_bytes || header.length>tree->source_bytes-header.offset)
        return error(diagnostic,header,"Event header span is outside its source");
    NativeScriptEventPayloadResolverContext value={.tree=tree,.descriptor=event.descriptor,
        .callback=callback,.event_id=event.event_id,.first_byte=header.offset+header.length,
        .last_byte=header.offset+header.length};
    for (uint32_t i=0;i<tree->count;++i) if (i!=callback && descends(tree,i,callback)) {
        NativeScriptSourceSpan span=tree->nodes[i].source;
        if (span.offset>tree->source_bytes || span.length>tree->source_bytes-span.offset)
            return error(diagnostic,span,"Event body span is outside its source");
        if (value.last_byte<span.offset+span.length) value.last_byte=span.offset+span.length;
    }
    *context=value;return true;
}

static bool name_is(const NativeScriptIRArena* arena,const NativeScriptIRNode* node,const char* name)
{
    return node->text_offset<=arena->source_bytes && node->text_length<=arena->source_bytes-node->text_offset &&
        strlen(name)==node->text_length && !memcmp(arena->source+node->text_offset,name,node->text_length);
}

static bool scoped(const NativeScriptEventPayloadResolverContext* context,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptDiagnostic* diagnostic)
{
    if (!context || !context->tree || !context->descriptor || arena->source!=context->tree->source ||
        arena->source_bytes!=context->tree->source_bytes ||
        node->source.offset<context->first_byte || node->source.offset>context->last_byte ||
        node->source.length>context->last_byte-node->source.offset)
        return error(diagnostic,node->source,"Event payload is outside its declared callback source scope");
    return true;
}

static uint32_t semantic(uint32_t event,uint32_t argument)
{
    return NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC|(event*NATIVE_SCRIPT_EVENT_PAYLOAD_ARGUMENTS+argument);
}

static bool record_name(const NativeScriptIRArena* arena,const NativeScriptIRNode* node)
{
    if(node->kind!=NativeScriptIR_Name)return false;
    for(uint32_t i=0;i<nativeScriptEvents_Count();++i) {
        const NativeScriptEventDescriptor* event=nativeScriptEvents_Descriptor(i);
        if(event->record&&name_is(arena,node,event->record->namespace_name))return true;
    }
    return false;
}

int nativeScriptEventPayload_TryResolve(void* opaque,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptEventPayloadResolverContext* context=opaque;
    if (!arena || !node || !result) return -1;
    bool event_name=node->kind==NativeScriptIR_Name && name_is(arena,node,"event");
    bool signal_name=node->kind==NativeScriptIR_Name && name_is(arena,node,"signal");
    bool record_namespace=record_name(arena,node);
    bool reference_name=node->kind==NativeScriptIR_Name && (name_is(arena,node,"other") || name_is(arena,node,"victim"));
    const NativeScriptIRNode* base=NULL;
    if (node->kind==NativeScriptIR_Member || node->kind==NativeScriptIR_Index || node->kind==NativeScriptIR_Call) {
        if (node->a>=arena->node_count) return 0;
        base=&arena->nodes[node->a];
    }
    bool event_base=base && (base->semantic==NATIVE_SCRIPT_EVENT_NAMESPACE ||
        base->semantic==NATIVE_SCRIPT_SIGNAL_NAMESPACE ||
        base->semantic==NATIVE_SCRIPT_EVENT_RECORD_NAMESPACE ||
        (base->semantic&NATIVE_SCRIPT_EVENT_SEMANTIC_MASK)==NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC ||
        ((base->semantic&NATIVE_SCRIPT_EVENT_SEMANTIC_MASK)==NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC &&
         (base->semantic&UINT32_C(0x00ffffff))<nativeScriptEvents_Count()*NATIVE_SCRIPT_EVENT_PAYLOAD_ARGUMENTS));
    if (!event_name && !signal_name && !reference_name && !record_namespace && !event_base) return 0;
    if (!scoped(context,arena,node,diagnostic)) return -1;
    const NativeScriptEventDescriptor* descriptor=context->descriptor;
    if(record_namespace) {
        if(!descriptor->record||!name_is(arena,node,descriptor->record->namespace_name)) {
            error(diagnostic,node->source,"Mutable event record is unavailable for this callback");return -1;
        }
        *result=(NativeScriptIRResolution){.type=NativeScriptType_Namespace,
            .semantic=NATIVE_SCRIPT_EVENT_RECORD_NAMESPACE};return 1;
    }
    if(base&&base->semantic==NATIVE_SCRIPT_EVENT_RECORD_NAMESPACE) {
        if(node->kind==NativeScriptIR_Member&&descriptor->record) {
            for(uint32_t i=0;i<descriptor->record->field_count;++i)
                if(name_is(arena,node,descriptor->record->fields[i].name)) {
                    *result=(NativeScriptIRResolution){.type=(NativeScriptIRType)descriptor->record->fields[i].type,
                        .semantic=NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC|(context->event_id*NATIVE_SCRIPT_EVENT_RECORD_FIELDS+i),
                        .flags=NativeScriptFlag_Readable|NativeScriptFlag_Addressable|
                            NativeScriptFlag_Writable|NativeScriptFlag_EventOutput};return 1;
                }
        }
        error(diagnostic,node->source,"Unknown member/index/call in this callback's mutable record");return -1;
    }
    if (event_name || signal_name) {
        const char* prefix=event_name ? "event." : "signal.";
        for (unsigned i=0;i<descriptor->argument_count;++i)
            if (!strncmp(descriptor->arguments[i].property,prefix,strlen(prefix))) {
                *result=(NativeScriptIRResolution){NativeScriptType_Namespace,
                    event_name ? NATIVE_SCRIPT_EVENT_NAMESPACE : NATIVE_SCRIPT_SIGNAL_NAMESPACE,0,0};return 1;
            }
        error(diagnostic,node->source,"Event payload namespace is unavailable for this callback");return -1;
    }
    if (reference_name) {
        for (unsigned i=0;i<descriptor->argument_count;++i)
            if (name_is(arena,node,descriptor->arguments[i].property)) {
                *result=(NativeScriptIRResolution){(NativeScriptIRType)descriptor->arguments[i].type,
                    semantic(context->event_id,i),NativeScriptFlag_Readable|NativeScriptFlag_ReceiverCapture,0};return 1;
            }
        error(diagnostic,node->source,"Object event payload is unavailable for this callback");return -1;
    }
    if(base && base->type==NativeScriptType_Object && (base->flags&NativeScriptFlag_ReceiverCapture))return 0;
    if (node->kind!=NativeScriptIR_Member || base->type!=NativeScriptType_Namespace ||
        (base->semantic!=NATIVE_SCRIPT_EVENT_NAMESPACE && base->semantic!=NATIVE_SCRIPT_SIGNAL_NAMESPACE)) {
        error(diagnostic,node->source,"Event payload is read-only; unknown member/index/call path");return -1;
    }
    const char* prefix=base->semantic==NATIVE_SCRIPT_EVENT_NAMESPACE ? "event." : "signal.";
    for (unsigned i=0;i<descriptor->argument_count;++i) {
        const char* property=descriptor->arguments[i].property;
        if (!strncmp(property,prefix,strlen(prefix)) && name_is(arena,node,property+strlen(prefix))) {
            *result=(NativeScriptIRResolution){(NativeScriptIRType)descriptor->arguments[i].type,
                semantic(context->event_id,i),NativeScriptFlag_Readable,0};return 1;
        }
    }
    error(diagnostic,node->source,"Unrecognized property in this callback's event payload");return -1;
}

bool nativeScriptEventPayload_Resolve(void* context,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    int status=nativeScriptEventPayload_TryResolve(context,arena,node,result,diagnostic);
    if (!status) return error(diagnostic,node->source,"Unknown event payload name or semantic path");
    return status>0;
}

static unsigned source_class(const NativeScriptEventDescriptor* descriptor)
{
    return descriptor->context==NativeScriptEvent_Fighter ? NativeCallbackSource_Fighter :
        descriptor->context==NativeScriptEvent_ModelEffect ? NativeCallbackSource_ModelEffect : NativeCallbackSource_Item;
}

static bool input(const NativeScriptEventArgument* argument,const NativeScriptValue* value,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    if (value->type==argument->type) {
        if (value->type==NativeScriptType_Bool && value->as.u64>1)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Event Boolean payload is not canonical");
        if (value->type==NativeScriptType_Object &&
            ((value->as.object.kind&NATIVE_SCRIPT_RECEIVER_KIND_MASK)!=NATIVE_SCRIPT_RECEIVER_RETAINED_KIND ||
             (value->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)>NativeCallbackSource_ModelEffect))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Event Object payload needs a retained supported actor identity");
        if (value->type==NativeScriptType_Object && !strcmp(argument->native_type,"Fighter_GObj*") &&
            (value->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)!=NativeCallbackSource_Fighter)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload actor reference differs from its native Fighter argument");
        if ((value->type==NativeScriptType_I32 && (value->as.i64<INT32_MIN || value->as.i64>INT32_MAX)) ||
            (value->type==NativeScriptType_U32 && value->as.u64>UINT32_MAX))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Event integer payload is outside its declared width");
        *output=*value;return true;
    }
    // History arrives as native UInt32, authored as signed Int bits. Keep the
    // exact identity: no Float intermediary and no broader numeric coercion.
    if (value->type==NativeScriptType_U32 && argument->type==NativeScriptType_I32 &&
        value->as.u64<=UINT32_MAX && !strcmp(argument->native_type,"uint32_t"))
        return nativeScriptValue_Convert(value,NativeScriptType_I32,output,diagnostic);
    return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload input differs from its audited native signature");
}

static uint8_t native_width(uint8_t type)
{
    switch(type) {
    case NativeScriptType_Bool:return 1;
    case NativeScriptType_I32:case NativeScriptType_U32:case NativeScriptType_F32:return 4;
    case NativeScriptType_I64:case NativeScriptType_U64:case NativeScriptType_F64:return 8;
    default:return 0;
    }
}
static bool record_valid(const NativeScriptEventRecordDescriptor* record,NativeScriptDiagnostic* d)
{
    if(!record||!record->namespace_name||!record->namespace_name[0]||
       !record->native_bytes||record->native_bytes>NATIVE_SCRIPT_EVENT_RECORD_BYTES||
       !record->field_count||record->field_count>NATIVE_SCRIPT_EVENT_RECORD_FIELDS)
        return error(d,(NativeScriptSourceSpan){0},"Event record schema exceeds its fixed native bounds");
    for(unsigned i=0;i<record->field_count;++i) {
        const NativeScriptEventRecordField* field=&record->fields[i];
        if(!field->name||!field->name[0]||!native_width(field->type)||
           field->width!=native_width(field->type)||field->offset>record->native_bytes||
           field->width>record->native_bytes-field->offset)
            return error(d,(NativeScriptSourceSpan){0},"Event record field has an unsupported scalar width or extent");
        for(unsigned j=0;j<i;++j) {
            const NativeScriptEventRecordField* prior=&record->fields[j];
            if(!strcmp(prior->name,field->name)||
               (field->offset<prior->offset+prior->width&&prior->offset<field->offset+field->width))
                return error(d,(NativeScriptSourceSpan){0},"Event record fields overlap or duplicate a name");
        }
    }
    return true;
}
static bool scalar_valid(uint8_t type,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!value||value->type!=type||!native_width(type)||
       (type==NativeScriptType_Bool&&value->as.u64>1)||
       (type==NativeScriptType_I32&&(value->as.i64<INT32_MIN||value->as.i64>INT32_MAX))||
       (type==NativeScriptType_U32&&value->as.u64>UINT32_MAX))
        return error(d,(NativeScriptSourceSpan){0},"Event record field differs from its exact canonical scalar type/width");
    return true;
}
static bool scalar_load(const NativeScriptEventRecordField* field,const uint8_t* bytes,
                         NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    NativeScriptValue value={.type=field->type};const uint8_t* at=bytes+field->offset;
    switch(field->type) {
    case NativeScriptType_Bool:{uint8_t native;memcpy(&native,at,1);value.as.u64=native;break;}
    case NativeScriptType_I32:{int32_t native;memcpy(&native,at,4);value.as.i64=native;break;}
    case NativeScriptType_U32:{uint32_t native;memcpy(&native,at,4);value.as.u64=native;break;}
    case NativeScriptType_I64:memcpy(&value.as.i64,at,8);break;
    case NativeScriptType_U64:memcpy(&value.as.u64,at,8);break;
    case NativeScriptType_F32:memcpy(&value.as.f32,at,4);break;
    case NativeScriptType_F64:memcpy(&value.as.f64,at,8);break;
    default:return error(d,(NativeScriptSourceSpan){0},"Unsupported native event record scalar");
    }
    if(!scalar_valid(field->type,&value,d))return false;
    *output=value;return true;
}
static void scalar_store(const NativeScriptEventRecordField* field,const NativeScriptValue* value,uint8_t* bytes)
{
    uint8_t* at=bytes+field->offset;
    switch(field->type) {
    case NativeScriptType_Bool:{uint8_t native=(uint8_t)value->as.u64;memcpy(at,&native,1);break;}
    case NativeScriptType_I32:{int32_t native=(int32_t)value->as.i64;memcpy(at,&native,4);break;}
    case NativeScriptType_U32:{uint32_t native=(uint32_t)value->as.u64;memcpy(at,&native,4);break;}
    case NativeScriptType_I64:memcpy(at,&value->as.i64,8);break;
    case NativeScriptType_U64:memcpy(at,&value->as.u64,8);break;
    case NativeScriptType_F32:memcpy(at,&value->as.f32,4);break;
    case NativeScriptType_F64:memcpy(at,&value->as.f64,8);break;
    }
}

bool nativeScriptEventPayload_Push(NativeScriptEventPayloadPool* pool,uint32_t event_id,
    NativeScriptRuntimeContext* context,NativeScriptEventPayloadSourceAlive alive,
    const NativeScriptValue* arguments,uint32_t count,const NativeScriptEventRecordIO* record,
    NativeScriptEventPayloadHandle* handle,
    NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptEventDescriptor* descriptor=nativeScriptEvents_Descriptor(event_id);
    if (!pool || !pool->frames || !context || !handle || !alive || !descriptor || !descriptor->signature_supported)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Missing/unsupported event payload execution contract");
    if (context->source_class!=source_class(descriptor) || !context->source || !alive(context))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload source is retired, mismatched or outside its actor context");
    if (pool->depth>=pool->capacity || pool->serial==UINT64_MAX)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload pool or generation capacity exhausted");
    if (count!=descriptor->argument_count || count>NATIVE_SCRIPT_EVENT_PAYLOAD_ARGUMENTS || (count && !arguments))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload argument count differs from its signature");
    if(descriptor->record ? (!record||!record->data||record->bytes!=descriptor->record->native_bytes) : record!=NULL)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event record input differs from this callback's native contract");
    uint32_t depth=pool->depth;uint64_t serial=pool->serial;
    NativeScriptEventPayloadFrame value={.source=context->source,.serial=pool->serial+1,
        .event_id=event_id,.source_class=context->source_class,.generation=context->generation,
        .frame=context->frame,.actor_id=context->actor_id,.scene_epoch=context->scene_epoch};
    for (unsigned i=0;i<count;++i) if (!input(&descriptor->arguments[i],&arguments[i],&value.arguments[i],diagnostic)) return false;
    if(descriptor->record) {
        if(!record_valid(descriptor->record,diagnostic))return false;
        memcpy(value.record_seed,record->data,record->bytes);
        for(unsigned i=0;i<descriptor->record->field_count;++i)
            if(!scalar_load(&descriptor->record->fields[i],value.record_seed,&value.fields[i],diagnostic))return false;
    }
    // Input validation has no native callbacks; nevertheless the published
    // frame must belong to the same live actor at this transition.
    if (!alive(context) || context->source!=value.source || context->source_class!=value.source_class ||
        context->generation!=value.generation || context->actor_id!=value.actor_id ||
        context->frame!=value.frame || context->scene_epoch!=value.scene_epoch ||
        pool->depth!=depth || pool->serial!=serial)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload source retired or changed before frame publication");
    pool->frames[pool->depth]=value;*handle=(NativeScriptEventPayloadHandle){value.serial,pool->depth};
    ++pool->depth;pool->serial=value.serial;return true;
}

static const NativeScriptEventPayloadFrame* active(const NativeScriptEventPayloadPool* pool,
    NativeScriptEventPayloadHandle handle,NativeScriptRuntimeContext* context,
    NativeScriptEventPayloadSourceAlive alive,NativeScriptDiagnostic* diagnostic)
{
    if (!pool || !pool->frames || !context || !alive || !pool->depth ||
        handle.slot!=pool->depth-1 || handle.slot>=pool->capacity || !handle.serial ||
        pool->frames[handle.slot].serial!=handle.serial) {
        error(diagnostic,(NativeScriptSourceSpan){0},"Event payload handle is suspended, stale or outside the active frame");return NULL;
    }
    const NativeScriptEventPayloadFrame* frame=&pool->frames[handle.slot];
    if (frame->source!=context->source || frame->source_class!=context->source_class ||
        frame->generation!=context->generation || frame->actor_id!=context->actor_id ||
        frame->frame!=context->frame || frame->scene_epoch!=context->scene_epoch || !alive(context) ||
        handle.slot!=pool->depth-1 || frame->serial!=handle.serial ||
        frame->source!=context->source || frame->source_class!=context->source_class ||
        frame->generation!=context->generation || frame->actor_id!=context->actor_id ||
        frame->frame!=context->frame || frame->scene_epoch!=context->scene_epoch) {
        error(diagnostic,(NativeScriptSourceSpan){0},"Event payload source retired, recycled or changed invocation identity");return NULL;
    }
    return frame;
}

bool nativeScriptEventPayload_Pop(NativeScriptEventPayloadPool* pool,NativeScriptEventPayloadHandle handle,
    NativeScriptDiagnostic* diagnostic)
{
    if (!pool || !pool->frames || !pool->depth || handle.slot!=pool->depth-1 ||
        handle.slot>=pool->capacity || !handle.serial || pool->frames[handle.slot].serial!=handle.serial)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload frames must be left in balanced stack order");
    --pool->depth;return true;
}

bool nativeScriptEventPayload_Read(const NativeScriptEventPayloadPool* pool,NativeScriptEventPayloadHandle handle,
    NativeScriptRuntimeContext* context,NativeScriptEventPayloadSourceAlive alive,uint32_t id,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    uint32_t domain=id&NATIVE_SCRIPT_EVENT_SEMANTIC_MASK;
    if (!result || (domain!=NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC&&domain!=NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Unknown event payload semantic domain");
    uint32_t stride=domain==NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC?NATIVE_SCRIPT_EVENT_RECORD_FIELDS:NATIVE_SCRIPT_EVENT_PAYLOAD_ARGUMENTS;
    uint32_t value=id&UINT32_C(0x00ffffff),event=value/stride,argument=value%stride;
    const NativeScriptEventPayloadFrame* frame=active(pool,handle,context,alive,diagnostic);
    if (!frame) return false;
    const NativeScriptEventDescriptor* descriptor=nativeScriptEvents_Descriptor(frame->event_id);
    if (!descriptor || event!=frame->event_id ||
        (domain==NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC?
            (!descriptor->record||argument>=descriptor->record->field_count):argument>=descriptor->argument_count))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event payload semantic does not belong to this callback frame");
    if(domain==NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC&&
       !scalar_valid(descriptor->record->fields[argument].type,&frame->fields[argument],diagnostic))return false;
    *result=domain==NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC?frame->fields[argument]:frame->arguments[argument];return true;
}

bool nativeScriptEventPayload_Write(NativeScriptEventPayloadPool* pool,NativeScriptEventPayloadHandle handle,
    NativeScriptRuntimeContext* context,NativeScriptEventPayloadSourceAlive alive,uint32_t id,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    if((id&NATIVE_SCRIPT_EVENT_SEMANTIC_MASK)!=NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Only mutable event record fields accept writes");
    const NativeScriptEventPayloadFrame* frame=active(pool,handle,context,alive,diagnostic);
    if(!frame)return false;
    const NativeScriptEventDescriptor* descriptor=nativeScriptEvents_Descriptor(frame->event_id);
    uint32_t offset=id&UINT32_C(0x00ffffff),field=offset%NATIVE_SCRIPT_EVENT_RECORD_FIELDS;
    if(!descriptor||offset/NATIVE_SCRIPT_EVENT_RECORD_FIELDS!=frame->event_id||!descriptor->record||
       field>=descriptor->record->field_count||
       !scalar_valid(descriptor->record->fields[field].type,value,diagnostic))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Record write differs from its active callback field");
    pool->frames[handle.slot].fields[field]=*value;return true;
}

bool nativeScriptEventPayload_ExportRecord(const NativeScriptEventPayloadPool* pool,NativeScriptEventPayloadHandle handle,
    NativeScriptRuntimeContext* context,NativeScriptEventPayloadSourceAlive alive,void* output,uint32_t bytes,
    NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptEventPayloadFrame* frame=active(pool,handle,context,alive,diagnostic);
    if(!frame)return false;
    const NativeScriptEventDescriptor* descriptor=nativeScriptEvents_Descriptor(frame->event_id);
    if(!output||!descriptor||!descriptor->record||bytes!=descriptor->record->native_bytes)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Event record output differs from its active native schema");
    uint8_t candidate[NATIVE_SCRIPT_EVENT_RECORD_BYTES];
    memcpy(candidate,frame->record_seed,bytes);
    for(unsigned i=0;i<descriptor->record->field_count;++i) {
        if(!scalar_valid(descriptor->record->fields[i].type,&frame->fields[i],diagnostic))return false;
        scalar_store(&descriptor->record->fields[i],&frame->fields[i],candidate);
    }
    if(!active(pool,handle,context,alive,diagnostic))return false;
    memcpy(output,candidate,bytes);return true;
}

const NativeScriptValue* nativeScriptEventPayload_Arguments(const NativeScriptEventPayloadPool* pool,
    NativeScriptEventPayloadHandle handle,NativeScriptRuntimeContext* context,NativeScriptEventPayloadSourceAlive alive,
    uint32_t* count,NativeScriptDiagnostic* diagnostic)
{
    if (!count) return NULL;
    *count=0;
    const NativeScriptEventPayloadFrame* frame=active(pool,handle,context,alive,diagnostic);
    if (!frame) return NULL;
    const NativeScriptEventDescriptor* descriptor=nativeScriptEvents_Descriptor(frame->event_id);
    if (!descriptor) { error(diagnostic,(NativeScriptSourceSpan){0},"Event descriptor disappeared from the active frame");return NULL; }
    *count=descriptor->argument_count;return frame->arguments;
}
