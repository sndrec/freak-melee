#include "native_script_binding_receiver.h"
#include "native_script_expression.h"
#include "native_script_binding_article_reference_resolve.h"
#include "native_script_binding_article_reference.h"
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan source,const char* message)
{if(diagnostic){diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);}return false;}
static bool word(const NativeScriptIRArena* arena,const NativeScriptIRNode* node,const char* text)
{return node->text_length==strlen(text) && !memcmp(arena->source+node->text_offset,text,node->text_length);}
static uint32_t category(uint32_t semantic){return semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK;}
static uint32_t payload(uint32_t semantic){return semantic&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK;}
static bool receiver(uint32_t semantic){return (semantic&NATIVE_SCRIPT_RECEIVER_DOMAIN_MASK)==NATIVE_SCRIPT_RECEIVER_DOMAIN;}
static bool space(uint32_t semantic){return (semantic&UINT32_C(0xff000000))==NATIVE_SCRIPT_RECEIVER_NATIVE_NAMESPACE;}

static bool output(NativeScriptIRResolution* result,NativeScriptIRType type,uint32_t semantic,uint16_t flags)
{*result=(NativeScriptIRResolution){.type=type,.semantic=semantic,.flags=flags};return true;}
static const NativeScriptValueDeclaration* field(const NativeScriptProgramIndex* program,uint32_t index,bool parameter)
{
    if(!program || index>=program->value_count)return NULL;
    const NativeScriptValueDeclaration* declaration=&program->values[index];
    return declaration->owner==0 && declaration->parameter==parameter?declaration:NULL;
}
static const NativeScriptBindingDescriptor* native_descriptor(uint32_t id)
{
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(id);
    return descriptor && descriptor->name && (descriptor->contexts&NativeScriptBindings_Fighter) &&
        !(descriptor->flags&NativeScriptFlag_SourceOnly)?descriptor:NULL;
}
static bool native_result(uint32_t id,NativeScriptIRResolution* result)
{
    const NativeScriptBindingDescriptor* descriptor=native_descriptor(id);if(!descriptor)return false;
    if(descriptor->kind==NativeScriptBinding_Namespace)
        return output(result,NativeScriptType_Namespace,NATIVE_SCRIPT_RECEIVER_NATIVE_NAMESPACE|id,0);
    uint16_t flags=descriptor->flags;
    if(descriptor->kind==NativeScriptBinding_Intrinsic) {
        flags|=NativeScriptFlag_Callable|NativeScriptFlag_Readable;
        for(unsigned i=0;i<descriptor->argument_count;++i)
            if(descriptor->arguments[i].mode==NativeScriptArgument_CodeRef)flags|=NativeScriptFlag_ReceiverOwned;
        return output(result,NativeScriptType_Callable,NATIVE_SCRIPT_RECEIVER_NATIVE|id,flags);
    }
    return output(result,(NativeScriptIRType)descriptor->result_type,NATIVE_SCRIPT_RECEIVER_NATIVE|id,flags);
}
static bool root_child(const NativeScriptBindingReceiverResolver* context,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* result)
{
    (void)context;
    if(word(arena,node,"exists"))return output(result,NativeScriptType_Bool,NATIVE_SCRIPT_RECEIVER_EXISTS,NativeScriptFlag_Readable|NativeScriptFlag_Pure);
    static const char* const identity[]={"is_fighter","is_item","is_effect","kind"};
    for(unsigned i=0;i<sizeof(identity)/sizeof(*identity);++i)
        if(word(arena,node,identity[i]))return output(result,
            i==NativeScriptReceiverIdentity_Kind?NativeScriptType_I32:NativeScriptType_Bool,
            NATIVE_SCRIPT_RECEIVER_IDENTITY|i,NativeScriptFlag_Readable|NativeScriptFlag_Pure);
    if(word(arena,node,"state"))return output(result,NativeScriptType_Namespace,NATIVE_SCRIPT_RECEIVER_STATE_NAMESPACE,0);
    if(word(arena,node,"attrs") || word(arena,node,"params"))return output(result,NativeScriptType_Namespace,NATIVE_SCRIPT_RECEIVER_PARAMETER_NAMESPACE,0);
    if(word(arena,node,"helpers"))return output(result,NativeScriptType_Namespace,NATIVE_SCRIPT_RECEIVER_HELPER_NAMESPACE,0);
    if(word(arena,node,"motion"))return native_result(NativeScriptBinding_MotionID,result);
    if(word(arena,node,"facing"))return native_result(NativeScriptBinding_FacingDirection,result);
    if(word(arena,node,"clear_damage_death"))return native_result(NativeScriptBinding_CallbackClearDamageDeath,result);
    if(node->text_length==8 && !memcmp(arena->source+node->text_offset,"command",7) &&
        arena->source[node->text_offset+7]>='0' && arena->source[node->text_offset+7]<='3')
        return output(result,NativeScriptType_I32,NATIVE_SCRIPT_RECEIVER_COMMAND|(uint32_t)(arena->source[node->text_offset+7]-'0'),
            NativeScriptFlag_Readable|NativeScriptFlag_Addressable|NativeScriptFlag_Writable);
    for(uint32_t id=1;id<NativeScriptBinding_Count;++id) {
        const NativeScriptBindingDescriptor* descriptor=native_descriptor(id);
        if(descriptor && !descriptor->parent && word(arena,node,descriptor->name))return native_result(id,result);
    }
    return false;
}

int nativeScriptBindingReceiver_TryResolve(void* opaque,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptBindingReceiverResolver* context=opaque;
    if(!context || !arena || !node || !result || !arena->source ||
        node->text_offset>arena->source_bytes || node->text_length>arena->source_bytes-node->text_offset)
        {error(diagnostic,node?node->source:(NativeScriptSourceSpan){0},"Invalid receiver resolver/source context");return -1;}
    if(node->kind==NativeScriptIR_Name) {
        const char* const names[]={"creator","owner","current_owner","holder","attachment_owner","target"};
        for(unsigned i=0;i<sizeof(names)/sizeof(*names);++i)if(word(arena,node,names[i])) {
            if(!context->program){error(diagnostic,node->source,"Receiver root needs an indexed source program");return -1;}
            bool contact=i+1==NativeScriptReceiver_ContactTarget;
            if((contact && context->source_class!=NativeCallbackSource_Fighter) ||
                (!contact && context->source_class!=NativeCallbackSource_Item &&
                !(context->source_class==NativeCallbackSource_ModelEffect && i==0))) {
                error(diagnostic,node->source,"Receiver relation is not available in this source class");return -1;
            }
            output(result,NativeScriptType_Object,NATIVE_SCRIPT_RECEIVER_ROOT+i+1,
                NativeScriptFlag_Readable|NativeScriptFlag_ReceiverCapture);return 1;
        }
        return 0;
    }
    if(node->a>=arena->node_count || !arena->nodes)return 0;
    const NativeScriptIRNode* parent=&arena->nodes[node->a];uint32_t base=parent->semantic;
    if(nativeScriptBindingArticleReference_Handles(base)||nativeScriptBindingArticleReference_Namespace(base)) {
        error(diagnostic,node->source,"Article reference projection requires its read-only declared-Article resolver");return -1;
    }
    bool captured_object=parent->type==NativeScriptType_Object && (parent->flags&NativeScriptFlag_ReceiverCapture);
    bool related=receiver(base) || space(base) || captured_object;
    if(!related)return 0;
    if(!context->program){error(diagnostic,node->source,"Receiver projection needs an indexed source program");return -1;}
    if(node->kind==NativeScriptIR_Member) {
        if(captured_object) {
            if(root_child(context,arena,node,result))return 1;
        } else if(base==NATIVE_SCRIPT_RECEIVER_STATE_NAMESPACE || base==NATIVE_SCRIPT_RECEIVER_PARAMETER_NAMESPACE) {
            bool parameter=base==NATIVE_SCRIPT_RECEIVER_PARAMETER_NAMESPACE;
            for(uint32_t i=0;i<context->program->value_count;++i) {
                const NativeScriptValueDeclaration* declaration=field(context->program,i,parameter);
                if(declaration && declaration->name.length==node->text_length &&
                    !memcmp(context->program->tree->source+declaration->name.offset,arena->source+node->text_offset,node->text_length)) {
                    uint16_t flags=NativeScriptFlag_Readable|NativeScriptFlag_ReceiverOwned;
                    if(!parameter && !declaration->array)flags|=NativeScriptFlag_Writable|NativeScriptFlag_Addressable;
                    output(result,declaration->array?NativeScriptType_Object:declaration->type,
                        (parameter?NATIVE_SCRIPT_RECEIVER_PARAMETER:NATIVE_SCRIPT_RECEIVER_STATE)|i,flags);return 1;
                }
            }
            if(!parameter)for(uint32_t id=1;id<NativeScriptBinding_Count;++id) {
                const NativeScriptBindingDescriptor* descriptor=native_descriptor(id);
                if(descriptor && descriptor->parent==NATIVE_SCRIPT_STATE_NAMESPACE && word(arena,node,descriptor->name)) {
                    native_result(id,result);return 1;
                }
            }
        } else if(base==NATIVE_SCRIPT_RECEIVER_HELPER_NAMESPACE) {
            for(uint32_t i=0;i<context->program->function_count;++i) {
                const NativeScriptFunctionDeclaration* declaration=&context->program->functions[i];
                if(!declaration->owner && !declaration->callback && declaration->signature_resolved &&
                    declaration->name.length==node->text_length &&
                    !memcmp(context->program->tree->source+declaration->name.offset,arena->source+node->text_offset,node->text_length)) {
                    output(result,NativeScriptType_Callable,NATIVE_SCRIPT_RECEIVER_HELPER|i,
                        NativeScriptFlag_Readable|NativeScriptFlag_Callable|NativeScriptFlag_ReceiverOwned);return 1;
                }
            }
        } else {
            uint32_t native_parent=UINT32_MAX;
            if(category(base)==NATIVE_SCRIPT_RECEIVER_NATIVE_NAMESPACE)native_parent=payload(base);
            if(base==(NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_MotionID))native_parent=NativeScriptBinding_Motion;
            if(base==(NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_FacingDirection))native_parent=NativeScriptBinding_Facing;
            for(uint32_t id=1;id<NativeScriptBinding_Count;++id) {
                const NativeScriptBindingDescriptor* descriptor=native_descriptor(id);
                if(descriptor && descriptor->parent==native_parent && word(arena,node,descriptor->name)) {
                    native_result(id,result);return 1;
                }
            }
        }
    } else if(node->kind==NativeScriptIR_Index && category(base)==NATIVE_SCRIPT_RECEIVER_STATE) {
        const NativeScriptValueDeclaration* declaration=field(context->program,payload(base),false);
        if(declaration && declaration->array && node->b<arena->node_count &&
            nativeScriptExpression_IsIntegral(arena->nodes[node->b].type)) {
            output(result,declaration->type,base,NativeScriptFlag_Readable|NativeScriptFlag_Addressable|
                NativeScriptFlag_Writable|NativeScriptFlag_ReceiverOwned);return 1;
        }
    } else if(node->kind==NativeScriptIR_Call) {
        if(node->argument_first>arena->argument_count || node->argument_count>arena->argument_count-node->argument_first) {
            error(diagnostic,node->source,"Receiver call argument range is invalid");return -1;
        }
        if(category(base)==NATIVE_SCRIPT_RECEIVER_HELPER) {
            uint32_t function=payload(base);
            if(function<context->program->function_count) {
                const NativeScriptFunctionDeclaration* declaration=&context->program->functions[function];
                if(node->argument_count==declaration->parameter_count) {
                    for(uint32_t i=0;i<node->argument_count;++i) {
                        uint32_t argument=arena->arguments[node->argument_first+i];
                        if(argument>=arena->node_count || !nativeScriptExpression_Assignable(
                            context->program->parameters[declaration->parameter_first+i].type,arena->nodes[argument].type)) {
                            error(diagnostic,node->source,"Receiver helper argument type does not match its signature");return -1;
                        }
                    }
                    output(result,declaration->result_type,base,NativeScriptFlag_Readable|NativeScriptFlag_ReceiverOwned|
                        NativeScriptFlag_MayInvalidateSource);return 1;
                }
            }
        } else if(category(base)==NATIVE_SCRIPT_RECEIVER_NATIVE) {
            NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};
            NativeScriptIRResolution resolved;
            if(!nativeScriptBindings_ResolveCall(&binding,arena,node,payload(base),&resolved,diagnostic))return -1;
            resolved.semantic=NATIVE_SCRIPT_RECEIVER_NATIVE|resolved.semantic;
            resolved.flags|=parent->flags&NativeScriptFlag_ReceiverOwned;*result=resolved;return 1;
        }
    }
    error(diagnostic,node->source,"Unknown or unsupported typed receiver member, index or call");return -1;
}

bool nativeScriptBindingReceiver_DescribeCall(void* opaque,uint32_t semantic,NativeScriptCallLayout* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptBindingReceiverResolver* context=opaque;
    if(!context || !output || !receiver(semantic))return error(diagnostic,(NativeScriptSourceSpan){0},"Invalid receiver call-layout request");
    if(category(semantic)==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT||category(semantic)==NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION||
       category(semantic)==NATIVE_SCRIPT_RECEIVER_ARTICLE_BONE_POSITION) {
        NativeScriptBindingResolverContext binding={context->source_class==NativeCallbackSource_Fighter?NativeScriptBindings_Fighter:
            context->source_class==NativeCallbackSource_Item?NativeScriptBindings_Article:NativeScriptBindings_Effect};
        return nativeScriptBindings_DescribeCall(&binding,semantic,output,diagnostic);
    }
    if(category(semantic)==NATIVE_SCRIPT_RECEIVER_HELPER)
        return nativeScriptProgram_DescribeHelper((void*)context->program,NATIVE_SCRIPT_FUNCTION_SEMANTIC|payload(semantic),output,diagnostic);
    if(category(semantic)==NATIVE_SCRIPT_RECEIVER_NATIVE) {
        NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};
        return nativeScriptBindings_DescribeCall(&binding,payload(semantic),output,diagnostic);
    }
    return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver semantic is not a callable projection");
}

static bool target(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptReceiverRecord** record,NativeScriptDiagnostic* diagnostic)
{
    if(!selectors || !count)return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver projection has no captured receiver selector");
    bool owned=category(semantic)==NATIVE_SCRIPT_RECEIVER_STATE || category(semantic)==NATIVE_SCRIPT_RECEIVER_PARAMETER ||
        category(semantic)==NATIVE_SCRIPT_RECEIVER_HELPER;
    if(!nativeScriptReceiver_Validate(scope,caller,&selectors[0],owned,record,diagnostic))return false;
    if((*record)->source_class!=NativeCallbackSource_Fighter)
        return error(diagnostic,(NativeScriptSourceSpan){0},"This typed receiver projection requires a Fighter actor");
    return true;
}
static NativeScriptRuntimeContext native_context(const NativeScriptReceiverRecord* record,const NativeScriptRuntimeContext* caller)
{return (NativeScriptRuntimeContext){.source=record->actor,.source_class=record->source_class,.generation=record->generation,
    .actor_id=record->generation,.frame=caller->frame,.scene_epoch=caller->scene_epoch};}
static uint32_t storage_semantic(uint32_t semantic)
{return (category(semantic)==NATIVE_SCRIPT_RECEIVER_STATE?NATIVE_SCRIPT_STATE_SEMANTIC:NATIVE_SCRIPT_PARAMETER_SEMANTIC)|payload(semantic);}

bool nativeScriptBindingReceiver_Read(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    if(!output || !receiver(semantic))return error(diagnostic,(NativeScriptSourceSpan){0},"Invalid receiver read");
    if(nativeScriptBindingArticleReference_Handles(semantic))
        return nativeScriptBindingArticleReference_Read(scope,caller,semantic,selectors,count,output,diagnostic);
    uint32_t kind=category(semantic),id=payload(semantic);NativeScriptValue result;
    if(kind==NATIVE_SCRIPT_RECEIVER_ROOT) {
        if(count || (id&~UINT32_C(0x3ff)) || (id&255)<NativeScriptReceiver_Creator ||
           (id&255)>NativeScriptReceiver_ContactTarget)return error(diagnostic,(NativeScriptSourceSpan){0},"Invalid receiver root capture selectors/role");
        return nativeScriptReceiver_Capture(scope,caller,(NativeScriptReceiverRole)(id&255),
            (id&NATIVE_SCRIPT_RECEIVER_REQUIRE_PRESENT)!=0,(id&NATIVE_SCRIPT_RECEIVER_REQUIRE_OWNED)!=0,output,diagnostic);
    }
    if(kind==NATIVE_SCRIPT_RECEIVER_EXISTS) {
        bool exists;if(count!=1 || !selectors || !nativeScriptReceiver_Exists(scope,caller,&selectors[0],&exists,diagnostic))return false;
        *output=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=exists};return true;
    }
    if(kind==NATIVE_SCRIPT_RECEIVER_IDENTITY){
        const NativeScriptReceiverRecord* record;
        if(count!=1 || !selectors || id>NativeScriptReceiverIdentity_Kind ||
           !nativeScriptReceiver_Validate(scope,caller,&selectors[0],false,&record,diagnostic))return false;
        if(id==NativeScriptReceiverIdentity_Kind)
            result=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=(int32_t)record->kind};
        else result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=record->source_class==
            (id==NativeScriptReceiverIdentity_IsFighter?NativeCallbackSource_Fighter:
             id==NativeScriptReceiverIdentity_IsItem?NativeCallbackSource_Item:NativeCallbackSource_ModelEffect)};
        *output=result;return true;
    }
    const NativeScriptReceiverRecord* record;if(!target(scope,caller,semantic,selectors,count,&record,diagnostic))return false;
    if(kind==NATIVE_SCRIPT_RECEIVER_STATE || kind==NATIVE_SCRIPT_RECEIVER_PARAMETER) {
        if(!nativeScriptStorage_Read(record->view.storage,storage_semantic(semantic),selectors+1,count-1,&result,diagnostic))return false;
    } else if(kind==NATIVE_SCRIPT_RECEIVER_NATIVE || kind==NATIVE_SCRIPT_RECEIVER_COMMAND) {
        NativeScriptRuntimeContext native=native_context(record,caller);
        NativeScriptValue index={.type=NativeScriptType_I32,.as.i64=id};
        if(kind==NATIVE_SCRIPT_RECEIVER_COMMAND && (count!=1 || id>3))return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver command selector is invalid");
        if(!nativeScriptBindings_Read(&native,kind==NATIVE_SCRIPT_RECEIVER_COMMAND?NativeScriptBinding_Command:id,
            kind==NATIVE_SCRIPT_RECEIVER_COMMAND?&index:selectors+1,kind==NATIVE_SCRIPT_RECEIVER_COMMAND?1:count-1,&result,diagnostic))return false;
    } else if(kind==NATIVE_SCRIPT_RECEIVER_HELPER) {
        const NativeScriptProgramIndex* program=record->view.program;
        if(count!=1 || id>=program->function_count || program->functions[id].owner || program->functions[id].callback)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver helper code identity is invalid");
        result=(NativeScriptValue){.type=NativeScriptType_Callable,.as.u64=id};
    } else return error(diagnostic,(NativeScriptSourceSpan){0},"Unsupported receiver read class");
    if(!target(scope,caller,semantic,selectors,count,&record,diagnostic))return false;*output=result;return true;
}

bool nativeScriptBindingReceiver_ValidateTarget(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptDiagnostic* diagnostic)
{
    if(nativeScriptBindingArticleReference_Handles(semantic))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Article references are read-only");
    const NativeScriptReceiverRecord* record;if(!receiver(semantic)||!target(scope,caller,semantic,selectors,count,&record,diagnostic))return false;
    uint32_t kind=category(semantic),id=payload(semantic);
    if(kind==NATIVE_SCRIPT_RECEIVER_PARAMETER)return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver parameters are read-only");
    if(kind==NATIVE_SCRIPT_RECEIVER_STATE) {
        const NativeScriptValueDeclaration* declaration=field(record->view.program,id,false);
        if(!declaration || count!=(declaration->array?2u:1u))return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver state target owner/selector shape is invalid");
        if(declaration->array) {
            const NativeScriptValue* index=&selectors[1];uint64_t value;
            if(!nativeScriptExpression_IsIntegral(index->type))return error(diagnostic,declaration->name,"Receiver state index must be integral");
            if(index->type==NativeScriptType_I32||index->type==NativeScriptType_I64) {
                if(index->as.i64<0)return error(diagnostic,declaration->name,"Receiver state index is negative");value=(uint64_t)index->as.i64;
            } else value=index->as.u64;
            if(value>=declaration->count)return error(diagnostic,declaration->name,"Receiver state index exceeds its declared bounds");
        }
        return true;
    }
    const NativeScriptBindingDescriptor* descriptor=native_descriptor(kind==NATIVE_SCRIPT_RECEIVER_COMMAND?NativeScriptBinding_Command:id);
    if((kind!=NATIVE_SCRIPT_RECEIVER_NATIVE && kind!=NATIVE_SCRIPT_RECEIVER_COMMAND) || !descriptor ||
        !(descriptor->flags&NativeScriptFlag_Writable) || !(descriptor->flags&NativeScriptFlag_Addressable))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver native target is not writable/addressable");
    if(kind==NATIVE_SCRIPT_RECEIVER_COMMAND)return count==1 && id<4?true:error(diagnostic,(NativeScriptSourceSpan){0},"Receiver command index is invalid");
    if(id==NativeScriptBinding_Command) {
        if(count!=2 || !nativeScriptExpression_IsIntegral(selectors[1].type))return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver command target needs one integral index");
        uint64_t index=selectors[1].type==NativeScriptType_I32||selectors[1].type==NativeScriptType_I64?
            (uint64_t)selectors[1].as.i64:selectors[1].as.u64;
        return index<4?true:error(diagnostic,(NativeScriptSourceSpan){0},"Receiver command index is out of bounds");
    }
    if(count!=1 || descriptor->kind!=NativeScriptBinding_Property)return error(diagnostic,(NativeScriptSourceSpan){0},"Indexed native receiver assignment protocol is unsupported");
    return true;
}

bool nativeScriptBindingReceiver_Write(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    if(!value || !nativeScriptBindingReceiver_ValidateTarget(scope,caller,semantic,selectors,count,diagnostic))return false;
    const NativeScriptReceiverRecord* record;if(!target(scope,caller,semantic,selectors,count,&record,diagnostic))return false;
    uint32_t kind=category(semantic),id=payload(semantic);
    if(kind==NATIVE_SCRIPT_RECEIVER_STATE) {
        if(!nativeScriptStorage_Write(record->view.storage,storage_semantic(semantic),selectors+1,count-1,value,diagnostic))return false;
    } else {
        NativeScriptRuntimeContext native=native_context(record,caller);NativeScriptValue index={.type=NativeScriptType_I32,.as.i64=id};
        if(!nativeScriptBindings_Write(&native,kind==NATIVE_SCRIPT_RECEIVER_COMMAND?NativeScriptBinding_Command:id,
            kind==NATIVE_SCRIPT_RECEIVER_COMMAND?&index:selectors+1,kind==NATIVE_SCRIPT_RECEIVER_COMMAND?1:count-1,value,diagnostic))return false;
    }
    return target(scope,caller,semantic,selectors,count,&record,diagnostic);
}

bool nativeScriptBindingReceiver_Invoke(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    if(nativeScriptBindingArticleReference_Handles(semantic))
        return nativeScriptBindingArticleReference_Invoke(scope,caller,semantic,selectors,selector_count,arguments,count,output,diagnostic);
    const NativeScriptReceiverRecord* record;
    if(!output || !receiver(semantic) || !target(scope,caller,semantic,selectors,selector_count,&record,diagnostic))return false;
    if(category(semantic)==NATIVE_SCRIPT_RECEIVER_HELPER) {
        if(selector_count!=1)return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver helper has unsupported selectors");
        return nativeScriptReceiver_RunHelper(scope,caller,&selectors[0],payload(semantic),arguments,count,output,diagnostic);
    }
    if(category(semantic)!=NATIVE_SCRIPT_RECEIVER_NATIVE)return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver semantic is not invocable");
    uint32_t id=payload(semantic);const NativeScriptBindingDescriptor* descriptor=native_descriptor(id);
    if(!descriptor)return error(diagnostic,(NativeScriptSourceSpan){0},"Receiver native callee is unavailable");
    for(unsigned i=0;i<descriptor->argument_count;++i)if(descriptor->arguments[i].mode==NativeScriptArgument_CodeRef) {
        if(!nativeScriptReceiver_Validate(scope,caller,&selectors[0],true,&record,diagnostic))return false;
    }
    NativeScriptRuntimeContext native=native_context(record,caller);NativeScriptValue result;
    if(!nativeScriptBindings_Invoke(&native,id,selectors+1,selector_count-1,arguments,count,&result,diagnostic) ||
        !target(scope,caller,semantic,selectors,selector_count,&record,diagnostic))return false;
    *output=result;return true;
}
