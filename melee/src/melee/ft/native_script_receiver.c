#include "native_script_receiver.h"
#include "native_script_bindings.h"
#include "native_fighter_runtime.h"
#include "types.h"
#include <melee/it/types.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_article.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false;}
static bool view_snapshot(const NativeScriptReceiverFrame*,void*,uint32_t,uint32_t,
    NativeScriptReceiverView*,NativeScriptDiagnostic*);

static bool caller_alive(const NativeScriptReceiverFrame* frame,const NativeScriptRuntimeContext* caller)
{
    return caller && frame->caller.source==caller->source && frame->caller.source_class==caller->source_class &&
        frame->caller.generation==caller->generation && frame->caller.actor_id==caller->actor_id &&
        frame->caller.frame==caller->frame && frame->caller.user==caller->user &&
        (!frame->scene_epoch || (frame->caller_reference.pool==&nativeGObjPool &&
            frame->caller_reference.lifetime==frame->scene_epoch && nativeMatchPool_Resolve(frame->caller_reference)==caller->source)) &&
        (!frame->caller_data || ((HSD_GObj*)caller->source)->user_data==frame->caller_data) &&
        nativeScriptBindings_SourceAlive((NativeScriptRuntimeContext*)caller);
}

static NativeScriptReceiverFrame* selected(const NativeScriptReceiverScope* scope,
    NativeScriptRuntimeContext* caller,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverPool* pool=scope?scope->pool:NULL;
    if(!pool || !pool->frames || !pool->records || !pool->depth || pool->depth>pool->frame_capacity || pool->top>pool->capacity ||
        scope->frame.slot!=pool->depth-1 || pool->frames[scope->frame.slot].serial!=scope->frame.serial) {
        error(diagnostic,"Receiver invocation is absent, suspended, popped or reused");return NULL;
    }
    NativeScriptReceiverFrame* frame=&pool->frames[scope->frame.slot];
    if(frame->first>pool->top || !caller_alive(frame,caller)) {
        error(diagnostic,"Receiver caller/source or scene retired");return NULL;
    }
    NativeScriptReceiverView current;
    if(!view_snapshot(frame,caller->source,caller->source_class,caller->generation,&current,diagnostic))return NULL;
    if(!caller_alive(frame,caller) || current.program!=frame->caller_view.program || current.storage!=frame->caller_view.storage ||
        (current.storage && (current.storage->owner!=frame->caller_storage_owner || current.storage->layout->identity!=frame->caller_layout_identity ||
            (current.storage->parameters?current.storage->parameters->identity:0)!=frame->caller_parameter_identity))) {
        error(diagnostic,"Receiver caller's pinned program/storage/schema changed");return NULL;
    }
    return frame;
}

bool nativeScriptReceiver_Push(NativeScriptReceiverPool* pool,const NativeScriptRuntimeContext* caller,uint64_t epoch,
    const NativeScriptProgramIndex* program,uint32_t expected_kind,NativeScriptReceiverLookup lookup,
    NativeScriptReceiverHelper helper,void* user,NativeScriptReceiverFrameHandle* output,NativeScriptDiagnostic* diagnostic)
{
    if(!pool || !caller || !output || !pool->frames || !pool->records || pool->depth>=pool->frame_capacity ||
        pool->top>pool->capacity || pool->serial==UINT32_MAX)
        return error(diagnostic,"Receiver frame pool or serial capacity exhausted");
    NativeScriptReceiverFrame frame={.caller=*caller,.scene_epoch=epoch,.expected_program=program,.expected_fighter_kind=expected_kind,
        .lookup=lookup,.helper=helper,.user=user,.first=pool->top,.serial=pool->serial+1};
    if((epoch && (!nativeMatchPool_Reference(&nativeGObjPool,caller->source,&frame.caller_reference) || frame.caller_reference.lifetime!=epoch)) ||
        !caller_alive(&frame,caller))
        return error(diagnostic,"Receiver entry caller is not a live source in the pinned GObj scene");
    if(epoch)frame.caller_data=((HSD_GObj*)caller->source)->user_data;
    if(!view_snapshot(&frame,caller->source,caller->source_class,caller->generation,&frame.caller_view,diagnostic) || !caller_alive(&frame,caller))return false;
    if(epoch && program && frame.caller_view.program!=program)return error(diagnostic,"Receiver caller view differs from its expected source program");
    frame.caller_layout_identity=frame.caller_view.storage?frame.caller_view.storage->layout->identity:0;
    frame.caller_storage_owner=frame.caller_view.storage?frame.caller_view.storage->owner:0;
    frame.caller_parameter_identity=frame.caller_view.storage&&frame.caller_view.storage->parameters?frame.caller_view.storage->parameters->identity:0;
    NativeScriptReceiverFrameHandle handle={pool->depth,frame.serial};
    pool->frames[pool->depth++]=frame;pool->serial=frame.serial;*output=handle;return true;
}

bool nativeScriptReceiver_Pop(NativeScriptReceiverPool* pool,NativeScriptReceiverFrameHandle handle,
    NativeScriptDiagnostic* diagnostic)
{
    if(!pool || !pool->frames || !pool->depth || pool->depth>pool->frame_capacity || pool->top>pool->capacity || handle.slot!=pool->depth-1 ||
        pool->frames[handle.slot].serial!=handle.serial || pool->frames[handle.slot].first>pool->top)
        return error(diagnostic,"Receiver frame pop is stale or out of order");
    pool->top=pool->frames[handle.slot].first;--pool->depth;return true;
}

static bool view_snapshot(const NativeScriptReceiverFrame* frame,void* actor,uint32_t source_class,
    uint32_t generation,NativeScriptReceiverView* output,NativeScriptDiagnostic* diagnostic)
{
    *output=(NativeScriptReceiverView){0};
    if(!frame->lookup || !frame->lookup(frame->user,actor,source_class,generation,output)) {
        *output=(NativeScriptReceiverView){0};return true;
    }
    // Lookup is pure. Check native membership again before reading its view.
    if(!nativeCallbackSource_Alive(actor,generation,source_class))return error(diagnostic,"Receiver retired during view lookup");
    NativeScriptStorageInstance* storage=output->storage;
    const NativeScriptStorageOwner* owner=storage && storage->layout?nativeScriptStorage_Owner(storage->layout,storage->owner):NULL;
    if(!output->program || !storage || !storage->live || !storage->layout || storage->layout->program!=output->program ||
        !storage->layout->identity || !owner || storage->identity!=generation || storage->generation!=generation ||
        storage->capacity<owner->state_bytes || (owner->state_bytes&&!storage->state) ||
        (owner->parameter_count && !nativeScriptStorage_ParametersReady(storage->layout,storage->parameters,storage->owner,diagnostic)))
        return error(diagnostic,"Receiver interpreted view has an invalid program, owner, schema or lifetime");
    return true;
}

static bool native_identity(void* actor,uint32_t source_class,uint32_t generation,
    uint32_t* kind,const void** definition)
{
    if(!nativeCallbackSource_Alive(actor,generation,source_class))return false;
    HSD_GObj* object=actor;
    if(source_class==NativeCallbackSource_Fighter) {
        Fighter* fighter=object->user_data;*kind=(uint32_t)fighter->kind;
        *definition=nativeFighterRuntime_Definition(fighter->kind);return fighter->gobj==object;
    }
    if(source_class==NativeCallbackSource_Item) {
        Item* item=object->user_data;*kind=(uint32_t)item->kind;
        FighterKind author=item->native_article_author_kind!=Ft_Kind_None?item->native_article_author_kind:item->native_creator_kind;
        *definition=nativeFighterRuntime_Definition(author);return item->entity==object;
    }
    EF_Effect* effect=nativeModelEffect_Resolve(actor,generation);
    if(!effect)return false;*kind=0;*definition=effect->native_author;return true;
}

static bool owned(const NativeScriptReceiverFrame* frame,const NativeScriptReceiverRecord* record,
    NativeScriptDiagnostic* diagnostic)
{
    if(record->source_class!=NativeCallbackSource_Fighter || frame->expected_fighter_kind==UINT32_MAX ||
        record->kind!=frame->expected_fighter_kind || !frame->expected_program ||
        record->view.program!=frame->expected_program || !record->view.storage || record->view.storage->owner!=0)
        return error(diagnostic,"Receiver owned data/helper needs the expected fighter kind and exact pinned root program/schema");
    return true;
}

static bool stable(const NativeScriptReceiverFrame* frame,const NativeScriptReceiverRecord* record,
    NativeScriptDiagnostic* diagnostic)
{
    uint32_t kind;const void* definition;
    if(record->reference.pool!=frame->caller_reference.pool || record->reference.lifetime!=frame->caller_reference.lifetime ||
        nativeMatchPool_Resolve(record->reference)!=record->actor ||
        ((HSD_GObj*)record->actor)->user_data!=record->native_data ||
        !native_identity(record->actor,record->source_class,record->generation,&kind,&definition) ||
        kind!=record->kind || definition!=record->definition)
        return error(diagnostic,"Retained receiver class, kind, generation, Definition or GObj slot retired");
    NativeScriptReceiverView view;
    if(!view_snapshot(frame,record->actor,record->source_class,record->generation,&view,diagnostic))return false;
    if(view.program!=record->view.program || view.storage!=record->view.storage ||
        (view.storage && (view.storage->owner!=record->storage_owner || view.storage->layout->identity!=record->layout_identity ||
            (view.storage->parameters?view.storage->parameters->identity:0)!=record->parameter_identity)))
        return error(diagnostic,"Retained receiver program, storage or parameter schema changed");
    return true;
}

static bool retain(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    void* actor,uint32_t source_class,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame)return false;
    if(!frame->scene_epoch)return error(diagnostic,"Receiver capture requires a pinned nonzero native GObj scene");
    uint32_t generation,kind;const void* definition;
    NativeMatchPoolReference reference;
    if(!nativeMatchPool_Reference(&nativeGObjPool,actor,&reference) || reference.lifetime!=frame->caller_reference.lifetime ||
        !nativeCallbackSource_Snapshot(actor,source_class,&generation) || !native_identity(actor,source_class,generation,&kind,&definition))
        return error(diagnostic,"Receiver is not a live registered actor in this GObj scene");
    NativeScriptReceiverView view;if(!view_snapshot(frame,actor,source_class,generation,&view,diagnostic))return false;
    if(!caller_alive(frame,caller))return error(diagnostic,"Receiver caller retired during capture");
    NativeScriptReceiverPool* pool=scope->pool;
    for(uint32_t i=frame->first;i<pool->top;++i) {
        const NativeScriptReceiverRecord* record=&pool->records[i];
        if(record->actor!=actor || record->generation!=generation || record->source_class!=source_class ||
            record->reference.generation!=reference.generation || record->reference.slot!=reference.slot)continue;
        if(!stable(frame,record,diagnostic))return false;
        *output=(NativeScriptValue){.type=NativeScriptType_Object,.as.object={i+1,record->serial,NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|source_class}};return true;
    }
    if(pool->top>=pool->capacity || pool->serial==UINT32_MAX)return error(diagnostic,"Receiver record pool or serial capacity exhausted");
    NativeScriptReceiverRecord record={.actor=actor,.native_data=((HSD_GObj*)actor)->user_data,.definition=definition,.view=view,.reference=reference,
        .generation=generation,.kind=kind,.source_class=source_class,.serial=pool->serial+1,.frame_serial=frame->serial,
        .storage_owner=view.storage?view.storage->owner:0,
        .layout_identity=view.storage?view.storage->layout->identity:0,
        .parameter_identity=view.storage&&view.storage->parameters?view.storage->parameters->identity:0};
    NativeScriptValue result={.type=NativeScriptType_Object,
        .as.object={pool->top+1,record.serial,NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|source_class}};
    pool->records[pool->top++]=record;pool->serial=record.serial;*output=result;return true;
}

bool nativeScriptReceiver_CaptureExternal(void* actor,uint32_t source_class,uint64_t epoch,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    if(!output || source_class>NativeCallbackSource_ModelEffect || !epoch || epoch!=nativeGObjPool.lifetime)
        return error(diagnostic,"External receiver class, scene epoch or destination is invalid");
    NativeScriptValue result={.type=NativeScriptType_Object,.as.object={0,0,NATIVE_SCRIPT_RECEIVER_ENGINE_KIND|source_class}};
    if(actor) {
        NativeMatchPoolReference reference;uint32_t generation;
        if(!nativeMatchPool_Reference(&nativeGObjPool,actor,&reference) || reference.lifetime!=epoch ||
            !nativeCallbackSource_Snapshot(actor,source_class,&generation))return error(diagnostic,"External receiver is not a live actor/pool reference");
        result.as.object.identity=reference.slot+1;result.as.object.generation=reference.generation;
    }
    *output=result;return true;
}

static bool article_owner(const NativeScriptReceiverFrame* frame,uint32_t owner)
{
    const NativeScriptProgramIndex* program=frame->expected_program;
    return frame->scene_epoch && frame->expected_fighter_kind!=UINT32_MAX &&
        (frame->caller.source_class==NativeCallbackSource_Fighter || frame->caller.source_class==NativeCallbackSource_Item) &&
        program && program==frame->caller_view.program && program->tree &&
        owner && owner<program->tree->count &&
        program->tree->nodes[owner].kind==NativeScriptSource_Article;
}

bool nativeScriptReceiver_ValidateArticle(const NativeScriptReceiverScope* scope,
    NativeScriptRuntimeContext* caller,const NativeScriptValue* token,uint32_t owner,
    const NativeScriptReceiverRecord** output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame)return false;
    if(!output || !article_owner(frame,owner))return error(diagnostic,"Article receiver declaration/root program is invalid");
    const NativeScriptReceiverRecord* record;
    if(!nativeScriptReceiver_Validate(scope,caller,token,false,&record,diagnostic))return false;
    if(record->source_class!=NativeCallbackSource_Item || record->view.program!=frame->expected_program ||
       !record->view.storage || record->storage_owner!=owner || record->view.storage->owner!=owner)
        return error(diagnostic,"Article receiver differs from its captured declaration/program/storage");
    const Item* item=((HSD_GObj*)record->actor)->user_data;
    if((uint32_t)item->native_article_author_kind!=frame->expected_fighter_kind || item->native_script_owner!=owner)
        return error(diagnostic,"Article receiver's immutable author or declaration changed");
    *output=record;return true;
}

bool nativeScriptReceiver_CaptureArticle(const NativeScriptReceiverScope* scope,
    NativeScriptRuntimeContext* caller,uint32_t handle,uint32_t owner,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame)return false;
    if(!output || !article_owner(frame,owner))return error(diagnostic,"Article capture needs its exact owned declaration/root program");
    NativeScriptValue result={.type=NativeScriptType_Object,
        .as.object={0,0,NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|NativeCallbackSource_Item}};
    if(handle!=UINT32_MAX && HSD_GObjPLinkHead) {
        uint32_t visited=0;
        for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];object && visited++<nativeGObjPool.capacity;object=object->next) {
            uint32_t generation;NativeMatchPoolReference reference;
            if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference) || reference.lifetime!=frame->scene_epoch ||
               !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation) || generation!=handle)continue;
            const Item* item=object->user_data;
            if((uint32_t)item->native_article_author_kind!=frame->expected_fighter_kind || item->native_script_owner!=owner)continue;
            NativeScriptReceiverView view={0};
            if(!frame->lookup || !frame->lookup(frame->user,object,NativeCallbackSource_Item,generation,&view) ||
               view.program!=frame->expected_program)continue;
            if(!caller_alive(frame,caller) || nativeMatchPool_Resolve(reference)!=object || object->user_data!=item ||
               !nativeCallbackSource_Alive(object,generation,NativeCallbackSource_Item))
                return error(diagnostic,"Article or caller retired during reference selection");
            if(!retain(scope,caller,object,NativeCallbackSource_Item,&result,diagnostic))return false;
            const NativeScriptReceiverRecord* record;
            if(!nativeScriptReceiver_ValidateArticle(scope,caller,&result,owner,&record,diagnostic))return false;
            if(record->reference.slot!=reference.slot || record->reference.generation!=reference.generation || record->generation!=generation)
                return error(diagnostic,"Article reference changed its selected physical/native generation");
            break;
        }
    }
    if(!selected(scope,caller,diagnostic))return false;
    *output=result;return true;
}

bool nativeScriptReceiver_Import(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    const NativeScriptValue* input,uint64_t epoch,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame)return false;
    if(!input || !output || input->type!=NativeScriptType_Object ||
        (input->as.object.kind&NATIVE_SCRIPT_RECEIVER_KIND_MASK)!=NATIVE_SCRIPT_RECEIVER_ENGINE_KIND ||
        (input->as.object.kind&~(NATIVE_SCRIPT_RECEIVER_ENGINE_KIND|NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)) ||
        (input->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)>NativeCallbackSource_ModelEffect || !epoch || epoch!=frame->scene_epoch)
        return error(diagnostic,"External receiver token/class or pinned input epoch is invalid");
    uint32_t source_class=input->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK;
    if(!input->as.object.identity && !input->as.object.generation) {
        *output=(NativeScriptValue){.type=NativeScriptType_Object,.as.object={0,0,NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|source_class}};return true;
    }
    if(!input->as.object.identity || !input->as.object.generation)return error(diagnostic,"External receiver null token is malformed");
    NativeMatchPoolReference reference={&nativeGObjPool,epoch,input->as.object.identity-1,input->as.object.generation};
    void* actor=nativeMatchPool_Resolve(reference);
    if(!actor)return error(diagnostic,"External receiver pool slot/generation retired");
    return retain(scope,caller,actor,source_class,output,diagnostic);
}

static bool token_record(const NativeScriptReceiverScope* scope,NativeScriptReceiverFrame* frame,
    const NativeScriptValue* value,const NativeScriptReceiverRecord** output,NativeScriptDiagnostic* diagnostic)
{
    if(!value || value->type!=NativeScriptType_Object ||
        (value->as.object.kind&NATIVE_SCRIPT_RECEIVER_KIND_MASK)!=NATIVE_SCRIPT_RECEIVER_RETAINED_KIND ||
        (value->as.object.kind&~(NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)) ||
        (value->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)>NativeCallbackSource_ModelEffect)
        return error(diagnostic,"Value is not an invocation-retained receiver token");
    if(!value->as.object.identity || !value->as.object.generation)return error(diagnostic,"Receiver relation is absent or token malformed");
    uint32_t index=value->as.object.identity-1;
    if(index<frame->first || index>=scope->pool->top)return error(diagnostic,"Receiver token belongs to another invocation or is out of range");
    const NativeScriptReceiverRecord* record=&scope->pool->records[index];
    if(record->serial!=value->as.object.generation || record->frame_serial!=frame->serial ||
        record->source_class!=(value->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK))
        return error(diagnostic,"Receiver token is stale, reused or has a different class");
    *output=record;return true;
}

bool nativeScriptReceiver_Validate(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    const NativeScriptValue* value,bool require_owned,const NativeScriptReceiverRecord** output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame)return false;
    const NativeScriptReceiverRecord* record;
    if(!output || !token_record(scope,frame,value,&record,diagnostic) || !stable(frame,record,diagnostic) ||
        (require_owned && !owned(frame,record,diagnostic)) || !caller_alive(frame,caller))return false;
    *output=record;return true;
}

bool nativeScriptReceiver_Export(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    const NativeScriptValue* input,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame || !input || !output)return false;
    if(input->type==NativeScriptType_Object && !input->as.object.identity && !input->as.object.generation &&
        (input->as.object.kind&NATIVE_SCRIPT_RECEIVER_KIND_MASK)==NATIVE_SCRIPT_RECEIVER_RETAINED_KIND &&
        (input->as.object.kind&~(NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|NATIVE_SCRIPT_RECEIVER_SOURCE_MASK))==0 &&
        (input->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)<=NativeCallbackSource_ModelEffect) {
        if(!frame->scene_epoch)return error(diagnostic,"Receiver export requires a pinned GObj scene");
        *output=*input;output->as.object.kind=NATIVE_SCRIPT_RECEIVER_ENGINE_KIND|(input->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK);return true;
    }
    const NativeScriptReceiverRecord* record;
    if(!nativeScriptReceiver_Validate(scope,caller,input,false,&record,diagnostic))return false;
    *output=(NativeScriptValue){.type=NativeScriptType_Object,.as.object={record->reference.slot+1,
        record->reference.generation,NATIVE_SCRIPT_RECEIVER_ENGINE_KIND|record->source_class}};return true;
}

bool nativeScriptReceiver_ResolveExternal(const NativeScriptRuntimeContext* caller,const NativeScriptValue* input,
    uint64_t epoch,uint32_t expected,void** output,uint32_t* generation,NativeScriptDiagnostic* diagnostic)
{
    NativeMatchPoolReference caller_reference;
    if(!caller || !input || !output || !generation || !epoch || epoch!=nativeGObjPool.lifetime ||
        !nativeMatchPool_Reference(&nativeGObjPool,caller->source,&caller_reference) || caller_reference.lifetime!=epoch ||
        !nativeScriptBindings_SourceAlive((NativeScriptRuntimeContext*)caller) || input->type!=NativeScriptType_Object ||
        (input->as.object.kind&NATIVE_SCRIPT_RECEIVER_KIND_MASK)!=NATIVE_SCRIPT_RECEIVER_ENGINE_KIND ||
        (input->as.object.kind&~(NATIVE_SCRIPT_RECEIVER_ENGINE_KIND|NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)) ||
        (input->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)>NativeCallbackSource_ModelEffect)
        return error(diagnostic,"External native input/caller class or scene is invalid");
    uint32_t source_class=input->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK;
    if(expected!=UINT32_MAX && expected!=source_class)return error(diagnostic,"External native input has a different source class");
    void* actor=NULL;uint32_t actor_generation=0;
    if(input->as.object.identity || input->as.object.generation) {
        if(!input->as.object.identity || !input->as.object.generation)return error(diagnostic,"External input null token is malformed");
        NativeMatchPoolReference reference={&nativeGObjPool,epoch,input->as.object.identity-1,input->as.object.generation};
        actor=nativeMatchPool_Resolve(reference);
        if(!actor || !nativeCallbackSource_Snapshot(actor,source_class,&actor_generation))return error(diagnostic,"External native target slot/generation retired");
    }
    if(nativeMatchPool_Resolve(caller_reference)!=caller->source || !nativeScriptBindings_SourceAlive((NativeScriptRuntimeContext*)caller))
        return error(diagnostic,"External native input caller retired during resolution");
    *output=actor;*generation=actor_generation;return true;
}

bool nativeScriptReceiver_Exists(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    const NativeScriptValue* value,bool* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame || !value || !output)return false;
    if(value->type==NativeScriptType_Object && !value->as.object.identity && !value->as.object.generation &&
        (value->as.object.kind&NATIVE_SCRIPT_RECEIVER_KIND_MASK)==NATIVE_SCRIPT_RECEIVER_RETAINED_KIND &&
        (value->as.object.kind&~(NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|NATIVE_SCRIPT_RECEIVER_SOURCE_MASK))==0 &&
        (value->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK)<=NativeCallbackSource_ModelEffect) {*output=false;return true;}
    const NativeScriptReceiverRecord* record;if(!token_record(scope,frame,value,&record,diagnostic))return false;
    NativeScriptDiagnostic discarded={0};bool result=stable(frame,record,&discarded);
    if(!caller_alive(frame,caller))return error(diagnostic,"Receiver caller retired during existence query");
    *output=result;return true;
}

bool nativeScriptReceiver_Capture(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    NativeScriptReceiverRole role,bool present,bool require_owned,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptReceiverFrame* frame=selected(scope,caller,diagnostic);if(!frame || !output)return false;
    if(!frame->scene_epoch)return error(diagnostic,"Receiver capture requires a pinned nonzero native GObj scene");
    HSD_GObj* target=NULL;HSD_GObj* object=caller->source;
    uint32_t target_class=NativeCallbackSource_Fighter;
    if(caller->source_class==NativeCallbackSource_Item) {
        Item* item=object->user_data;
        HSD_GObj* creator=item->native_script_owner?nativeScriptItem_Creator(object):
            item->native_article_author_kind==Ft_Kind_None?nativeItem_Creator(object):nativeArticle_Creator(object);
        if(role==NativeScriptReceiver_Creator)target=creator;
        else if(role==NativeScriptReceiver_Owner)target=creator && item->owner==creator?creator:NULL;
        else if(role==NativeScriptReceiver_CurrentOwner || role==NativeScriptReceiver_Holder)target=item->owner;
        else if(role==NativeScriptReceiver_AttachmentOwner)target=item->native_script_owner?nativeScriptItem_AttachmentOwner(object):NULL;
        else return error(diagnostic,"Unknown receiver role");
        uint32_t target_generation;
        if(target && !nativeCallbackSource_Snapshot(target,NativeCallbackSource_Fighter,&target_generation))target=NULL;
        if(target && role==NativeScriptReceiver_Holder && ((Fighter*)target->user_data)->item_gobj!=object)target=NULL;
    } else if(caller->source_class==NativeCallbackSource_ModelEffect && role==NativeScriptReceiver_Creator) {
        EF_Effect* effect=nativeModelEffect_Resolve(object,caller->generation);
        target=effect?nativeModelEffect_Creator(effect):NULL;
    } else if(caller->source_class==NativeCallbackSource_Fighter && role==NativeScriptReceiver_ContactTarget) {
        target=((Fighter*)object->user_data)->unk_gobj;
        if(target){
            NativeMatchPoolReference physical;
            if(!nativeMatchPool_Reference(&nativeGObjPool,target,&physical) || physical.lifetime!=frame->scene_epoch)
                return error(diagnostic,"Contact target is outside the pinned GObj scene");
            if(target->classifier==HSD_GOBJ_CLASS_FIGHTER)target_class=NativeCallbackSource_Fighter;
            else if(target->classifier==HSD_GOBJ_CLASS_ITEM)target_class=NativeCallbackSource_Item;
            else return error(diagnostic,"Contact target is not a Fighter or Item actor");
        }
    } else return error(diagnostic,"Receiver relation is unavailable for this source class");
    if(!caller_alive(frame,caller))return error(diagnostic,"Receiver caller retired during relation capture");
    NativeScriptValue result={.type=NativeScriptType_Object,.as.object={0,0,NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|target_class}};
    if(!target) {
        if(present || require_owned)return error(diagnostic,"Required receiver relation is absent");
    } else {
        if(!retain(scope,caller,target,target_class,&result,diagnostic))return false;
        const NativeScriptReceiverRecord* record;
        if(require_owned && !nativeScriptReceiver_Validate(scope,caller,&result,true,&record,diagnostic))return false;
    }
    *output=result;return true;
}

bool nativeScriptReceiver_RunHelper(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
    const NativeScriptValue* receiver,uint32_t function,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptReceiverRecord* record;
    if(!output || !nativeScriptReceiver_Validate(scope,caller,receiver,true,&record,diagnostic))return false;
    NativeScriptReceiverFrame* frame=&scope->pool->frames[scope->frame.slot];
    const NativeScriptProgramIndex* program=frame->expected_program;
    if(!frame->helper || function>=program->function_count || program->functions[function].owner ||
        program->functions[function].callback || !program->functions[function].signature_resolved ||
        program->functions[function].parameter_count!=count || (count&&!arguments))
        return error(diagnostic,"Receiver helper code identity, signature or direct runner is unavailable");
    NativeScriptValue result;
    if(!frame->helper(frame->user,record->actor,record->generation,program,function,arguments,count,&result,diagnostic) ||
        !nativeScriptReceiver_Validate(scope,caller,receiver,true,&record,diagnostic))return false;
    if(result.type!=program->functions[function].result_type)return error(diagnostic,"Receiver helper result type differs from its declaration");
    *output=result;return true;
}
