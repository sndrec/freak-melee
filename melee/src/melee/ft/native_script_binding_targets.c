#include "native_script_binding_targets.h"
#include "native_script_actor_access.h"
#include "native_script_fighter.h"
#include "native_source_package.h"
#include "native_fighter_target.h"
#include "ftdata.h"
#include "ftparts.h"
#include "ftlib.h"
#include "ftcommon.h"
#include "kinds/ftCommon/ftCo_Guard.h"
#include "kinds/ftCommon/ftCo_0A01.h"
#include "types.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/it/it_26B1.h>
#include <melee/gm/gmvs.h>
#include <melee/lb/lbcollision.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/robj.h>
#include <stdio.h>
#include <float.h>
#include <string.h>

typedef struct Caller {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    const void* data;
    const NativeScriptCompileBank* bank;
    const NativeScriptItemProgram* item;
    const NativeScriptFighterProgram* fighter;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
    uint32_t owner;
} Caller;
typedef struct Target {
    Fighter_GObj* object;
    NativeMatchPoolReference reference;
    const Fighter* data;
    uint32_t generation,kind;
} Target;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool caller_live(const Caller* a)
{
    HSD_GObj* object=a->context.source;NativeScriptRuntimeContext context=a->context;
    NativeScriptReceiverView view;
    if(nativeMatchPool_Resolve(a->reference)!=object || a->reference.lifetime!=context.scene_epoch ||
       object->user_data!=a->data || !nativeScriptBindings_SourceAlive(&context) || !a->bank->valid)return false;
    if(context.source_class==NativeCallbackSource_Item) {
        if(nativeScriptItem_Status(object)!=NativeScriptVM_Ok || nativeScriptItem_Spec(object)!=a->item ||
           a->item->bank!=a->bank || a->item->owner!=a->owner)return false;
    } else if(nativeScriptFighter_Program(((const Fighter*)a->data)->kind)!=a->fighter ||
              a->fighter->bank!=a->bank || !nativeScriptBindings_SourceFighterLayout(&context,NULL))return false;
    return nativeScriptActors_Lookup(NULL,object,context.source_class,context.generation,&view) &&
        view.program==&a->bank->declarations && view.program==a->view.program && view.storage==a->view.storage &&
        view.storage && view.storage->live && view.storage->owner==a->owner && view.storage->generation==context.generation &&
        view.storage->layout==&a->bank->storage_layout && view.storage->layout->identity==a->layout &&
        view.storage->parameters==&a->bank->storage_parameters && view.storage->parameters->identity==a->parameters;
}
static bool caller_capture(NativeScriptRuntimeContext* c,Caller* a,NativeScriptDiagnostic* d)
{
    if(!c || !c->scene_epoch || (c->source_class!=NativeCallbackSource_Fighter&&c->source_class!=NativeCallbackSource_Item) ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&a->reference) || a->reference.lifetime!=c->scene_epoch ||
       !nativeScriptBindings_SourceAlive(c))return error(d,"Target query caller has no live identity in its pinned scene");
    a->context=*c;a->data=((HSD_GObj*)c->source)->user_data;
    if(c->source_class==NativeCallbackSource_Fighter) {
        a->fighter=nativeScriptFighter_Program(((const Fighter*)a->data)->kind);a->bank=a->fighter?a->fighter->bank:NULL;a->owner=0;
    } else {a->item=nativeScriptItem_Spec(c->source);a->bank=a->item?a->item->bank:NULL;a->owner=a->item?a->item->owner:0;}
    if(!a->bank || !nativeScriptActors_Lookup(NULL,c->source,c->source_class,c->generation,&a->view) ||
       !a->view.storage || !a->view.storage->layout || !a->view.storage->parameters)
        return error(d,"Target query caller has no selected owned program/storage");
    a->layout=a->view.storage->layout->identity;a->parameters=a->view.storage->parameters->identity;
    return caller_live(a)?true:error(d,"Target query caller is faulted, retired or has changed schema");
}
static bool handle(const NativeScriptValue* value,uint32_t* out,NativeScriptDiagnostic* d)
{
    uint64_t bits;
    switch(value->type) {
    case NativeScriptType_Bool:bits=value->as.u64!=0;break;
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(value->as.i64<INT32_MIN || value->as.i64>UINT32_MAX)return error(d,"Fighter target handle exceeds native 32-bit width");
        bits=(uint32_t)value->as.i64;break;
    case NativeScriptType_U32:case NativeScriptType_U64:bits=value->as.u64;break;
    default:return error(d,"Fighter target handle must be integral");
    }
    if(bits>UINT32_MAX)return error(d,"Fighter target handle exceeds native 32-bit width");*out=(uint32_t)bits;return true;
}
static bool target_live(const Target* t,uint64_t epoch)
{
    return nativeMatchPool_Resolve(t->reference)==t->object && t->reference.lifetime==epoch &&
        t->object->classifier==HSD_GOBJ_CLASS_FIGHTER && t->object->user_data==t->data &&
        nativeCallbackSource_Alive(t->object,t->generation,NativeCallbackSource_Fighter) &&
        t->data->gobj==t->object && (uint32_t)t->data->kind==t->kind;
}
static bool select(uint32_t handle,uint64_t epoch,Target* t,NativeScriptDiagnostic* d)
{
    if(!handle || !HSD_GObjPLinkHead)return true;
    uint32_t visited=0;
    for(Fighter_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];object;object=object->next) {
        NativeMatchPoolReference reference;uint32_t generation;
        // Admit only actual physical Fighters in this scene before inspecting
        // userdata. Foreign kinds/stock programs are intentionally permitted.
        if(visited++==nativeGObjPool.capacity)return error(d,"Fighter target registry is cyclic or exceeds its physical pool");
        if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference) || reference.lifetime!=epoch)
            return error(d,"Fighter target registry contains a retired or nonphysical object");
        if(object->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
           !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&generation) || generation!=handle)continue;
        const Fighter* data=object->user_data;
        if(!data || data->gobj!=object)return error(d,"Matched Fighter target has malformed native data");
        *t=(Target){.object=object,.reference=reference,.data=data,.generation=generation,.kind=(uint32_t)data->kind};
        return target_live(t,epoch)?true:error(d,"Matched Fighter target changed during identity capture");
    }
    return true;
}

typedef struct ExportTarget {
    Target target;
    const NativeSourcePackage* package;
    const NativeScriptFighterProgram* program;
    const NativeScriptCompileBank* bank;
    NativeScriptReceiverView view;
    uint64_t identity,layout,parameters;
} ExportTarget;
static bool export_schema(const ExportTarget* t)
{
    const NativeScriptStorageInstance* storage=t->view.storage;
    return t->program->bank==t->bank && t->program->identity==t->identity && t->bank->valid &&
        t->view.program==&t->bank->declarations && storage && storage->owner==0 &&
        storage->layout==&t->bank->storage_layout && storage->layout->program==t->view.program &&
        storage->layout->identity==t->layout && storage->parameters==&t->bank->storage_parameters &&
        storage->parameters->identity==t->parameters && storage->parameters->layout_identity==t->layout;
}
static bool export_current(const ExportTarget* t,uint64_t epoch)
{
    NativeScriptReceiverView view;
    NativeScriptRuntimeContext context={.source=t->target.object,.source_class=NativeCallbackSource_Fighter,
        .generation=t->target.generation,.actor_id=t->target.generation,.scene_epoch=epoch};
    return target_live(&t->target,epoch) &&
        nativeFighterRuntime_SourcePackage((FighterKind)t->target.kind)==t->package &&
        nativeScriptFighter_Program((FighterKind)t->target.kind)==t->program && export_schema(t) &&
        t->view.storage->live && t->view.storage->generation==t->target.generation &&
        nativeScriptFighter_Status(t->target.object)==NativeScriptVM_Ok &&
        nativeScriptFighter_View(t->target.object,t->target.generation,&view) &&
        view.program==t->view.program && view.storage==t->view.storage &&
        nativeScriptBindings_SourceFighterLayout(&context,NULL)==&t->bank->motion_layout;
}
static bool export_capture(const Target* target,uint64_t epoch,ExportTarget* t,NativeScriptDiagnostic* d)
{
    t->target=*target;
    // A physical stock Fighter without a direct actor has no source exports.
    if(!target->object || !nativeScriptFighter_IsAttached(target->object))return true;
    if(!target_live(target,epoch) || nativeScriptFighter_Status(target->object)!=NativeScriptVM_Ok)
        return error(d,"Recipient export target is faulted, retired or changed before delivery");
    t->package=nativeFighterRuntime_SourcePackage((FighterKind)target->kind);
    t->program=nativeScriptFighter_Program((FighterKind)target->kind);
    if(!t->package || !t->program || !t->program->bank ||
       !nativeScriptFighter_View(target->object,target->generation,&t->view) ||
       !t->view.storage || !t->view.storage->layout || !t->view.storage->parameters)
        return error(d,"Recipient export target has no selected owned program/storage");
    t->bank=t->program->bank;t->identity=t->program->identity;
    t->layout=t->view.storage->layout->identity;t->parameters=t->view.storage->parameters->identity;
    return export_current(t,epoch)?true:error(d,"Recipient export target has changed program/data/schema");
}
static bool export_retired(const ExportTarget* t,uint64_t epoch)
{
    const Target* physical=&t->target;
    // The retained physical generation is gone. Never inspect/reacquire the
    // replacement, even when its GObj address equals the former recipient.
    if(nativeMatchPool_Resolve(physical->reference)!=physical->object)return true;
    if(physical->reference.lifetime!=epoch || physical->object->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       physical->object->user_data!=physical->data || physical->data->gobj!=physical->object ||
       (uint32_t)physical->data->kind!=physical->kind ||
       nativeFighterRuntime_SourcePackage((FighterKind)physical->kind)!=t->package ||
       nativeScriptFighter_Program((FighterKind)physical->kind)!=t->program || !export_schema(t))return false;
    // NewLife retires the old native creation identity while keeping storage.
    if((uint32_t)physical->data->x8_spawnNum!=physical->generation)return true;
    return !t->view.storage->live && t->view.storage->generation==physical->generation;
}
static const char* export_name(const NativeScriptCompileBank* bank,NativeScriptSourceSpan span)
{
    if(bank->declarations.tree!=&bank->tree || !bank->tree.source || !span.length || span.length>127 ||
       span.offset>bank->tree.source_bytes || span.length>bank->tree.source_bytes-span.offset)return NULL;
    return bank->tree.source+span.offset;
}
static bool export_invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    unsigned operation=(semantic&NATIVE_SCRIPT_TARGET_EXPORT_OPERATION_MASK)>>20;
    uint32_t index=semantic&NATIVE_SCRIPT_TARGET_EXPORT_INDEX_MASK;
    unsigned prepared_count=(semantic&NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK)>>NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT;
    if(!out || !args || count<2 || count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS || count!=prepared_count ||
       operation<1 || operation>4 || args[1].type!=NativeScriptType_Text)
        return error(d,"Recipient export operation/argument contract is invalid");
    for(unsigned a=2;a<count;++a)if(!nativeScriptExpression_IsNumeric(args[a].type))
        return error(d,"Recipient helper arguments must be numeric values");
    Caller caller={0};if(!caller_capture(c,&caller,d))return false;
    const NativeScriptProgramIndex* source=&caller.bank->declarations;
    if(source->export_name_count>NATIVE_SCRIPT_EXPORT_NAMES || index>=source->export_name_count)
        return error(d,"Recipient export name token is not prepared in this caller");
    const NativeScriptExportName* name=&source->export_names[index];
    NativeScriptSourceSpan literal={.offset=args[1].as.text.offset,.length=args[1].as.text.length};
    const char* bytes=export_name(caller.bank,name->name);const char* retained=export_name(caller.bank,literal);
    if(!bytes || !retained || literal.length!=name->name.length || memcmp(bytes,retained,literal.length))
        return error(d,"Recipient export literal differs from its pinned caller name");
    uint32_t id;Target target={0};ExportTarget recipient={0};
    if(!handle(&args[0],&id,d) || !select(id,caller.context.scene_epoch,&target,d) ||
       !export_capture(&target,caller.context.scene_epoch,&recipient,d))return false;
    NativeScriptValue result={.type=operation<=2?NativeScriptType_Bool:operation==3?NativeScriptType_I32:NativeScriptType_F32};
    NativeScriptValue parameters[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS-2];
    const NativeScriptExport* selected=NULL;
    if(recipient.program) {
        const NativeScriptProgramIndex* destination=&recipient.bank->declarations;
        if(destination->export_count>NATIVE_SCRIPT_EXPORTS ||
           destination->function_count>destination->function_capacity ||
           (destination->function_count&&!destination->functions))return error(d,"Recipient export table is malformed");
        unsigned low=0,high=destination->export_count;
        while(low<high){unsigned mid=low+(high-low)/2;if(destination->exports[mid].key<name->key)low=mid+1;else high=mid;}
        for(unsigned i=low;i<destination->export_count&&destination->exports[i].key==name->key;++i) {
            const NativeScriptExport* candidate=&destination->exports[i];const char* candidate_bytes=export_name(recipient.bank,candidate->name);
            if(!candidate_bytes)return error(d,"Recipient export name span is malformed");
            if(candidate->name.length==name->name.length&&!memcmp(candidate_bytes,bytes,name->name.length)){selected=candidate;break;}
        }
        if(selected) {
            if(selected->function>=destination->function_count)return error(d,"Recipient export function is out of range");
            const NativeScriptFunctionDeclaration* function=&destination->functions[selected->function];
            const char* function_name=export_name(recipient.bank,function->name);
            if(function->owner || function->callback || !function->exported || !function->signature_resolved ||
               function->parameter_count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS-2 ||
               destination->parameter_count>destination->parameter_capacity ||
               function->parameter_first>destination->parameter_count ||
               function->parameter_count>destination->parameter_count-function->parameter_first ||
               (function->parameter_count&&!destination->parameters) ||
               function->result_type!=selected->result_type ||
               !function_name || function->name.length!=selected->name.length || memcmp(function_name,bytes,name->name.length) ||
               (selected->result_type!=NativeScriptType_Void&&selected->result_type!=NativeScriptType_Bool&&
                selected->result_type!=NativeScriptType_I32&&selected->result_type!=NativeScriptType_F32))
                return error(d,"Recipient export function has a malformed root ABI");
            if(operation!=1&&selected->result_type!=result.type)selected=NULL;
            if(selected) {
                if(function->parameter_count!=count-2)return error(d,"Recipient helper argument count differs from its declaration");
                for(unsigned a=0;a<function->parameter_count;++a) {
                    NativeScriptIRType type=destination->parameters[function->parameter_first+a].type;
                    if((type!=NativeScriptType_Bool && type!=NativeScriptType_I32 && type!=NativeScriptType_U32 && type!=NativeScriptType_F32) ||
                       !nativeScriptExpression_Assignable(type,args[a+2].type))
                        return error(d,"Recipient helper argument type differs from its declaration");
                    if(!nativeScriptValue_Convert(&args[a+2],type,&parameters[a],d))return false;
                }
            }
        }
    }
    if(selected) {
        NativeScriptValue returned={0};
        NativeScriptVMStatus status=nativeScriptFighter_Helper(target.object,target.generation,selected->function,parameters,count-2,&returned,d);
        if(!caller_live(&caller))return error(d,"Recipient export caller retired/faulted before publication");
        if(status==NativeScriptVM_Error)return false;
        if(status==NativeScriptVM_SourceRetired) {
            if(!export_retired(&recipient,caller.context.scene_epoch))return error(d,"Recipient export live identity/schema was substituted");
        } else if(status!=NativeScriptVM_Ok || !export_current(&recipient,caller.context.scene_epoch) || returned.type!=selected->result_type)
            return error(d,"Recipient export identity/schema or typed result changed before publication");
        if(operation==1)result.as.u64=1;
        else if(status==NativeScriptVM_Ok)result=returned;
    } else {
        if(!caller_live(&caller))return error(d,"Recipient export caller retired/faulted during lookup");
        if(target.object&&(!target_live(&target,caller.context.scene_epoch) ||
           (recipient.program&&!export_current(&recipient,caller.context.scene_epoch))))
            return error(d,"Recipient export target changed during lookup");
    }
    *out=result;return true;
}

typedef struct BoneJoint {
    HSD_JObj* object;
    HSD_JObj* parent;
    HSD_JObj* child;
    HSD_JObj* next;
    HSD_ClassInfo* info;
    HSD_RObj* constraints;
    HSD_AObj* animation;
    void* anchor;
} BoneJoint;
typedef struct BoneTarget {
    Target target;
    const Caller* caller;
    ExportTarget owned;
    const ftData* data;
    FighterPartsTable* table;
    FighterBone* parts;
    const u8* forward;
    const u8* reverse;
    HSD_JObj* root;
    HSD_JObj* selected;
    HSD_JObj* parent;
    HSD_ClassInfo* info;
    const FtCostumeAssets* costume;
    uint32_t count,index;
    uint8_t root_kind,costume_id;
    bool attached;
    BoneJoint evaluated[MAX_FT_PARTS];
    unsigned evaluated_count;
} BoneTarget;

static bool bone_current(const BoneTarget* b)
{
    const Target* t=&b->target;
    if(!caller_live(b->caller) || !target_live(t,b->caller->context.scene_epoch) ||
       t->data->ft_data!=b->data || t->data->parts!=b->parts ||
       !ftPartsTable || ftPartsTable[t->kind]!=b->table ||
       b->table->parts_num!=b->count || b->table->part_to_joint!=b->forward || b->table->joint_to_part!=b->reverse ||
       t->object->hsd_obj!=b->root || t->object->obj_kind!=b->root_kind ||
       t->data->x619_costume_id!=b->costume_id || nativeScriptFighter_IsAttached(t->object)!=b->attached ||
       b->parts[b->index].joint!=b->selected || b->selected->parent!=b->parent ||
       HSD_CLASS_METHOD(b->selected)!=b->info)return false;
    if(b->attached) {
        NativeScriptReceiverView view;
        const ExportTarget* o=&b->owned;
        if(nativeFighterRuntime_SourcePackage((FighterKind)t->kind)!=o->package ||
           nativeScriptFighter_Program((FighterKind)t->kind)!=o->program || !export_schema(o) ||
           !o->view.storage->live || o->view.storage->generation!=t->generation ||
           !nativeScriptFighter_View(t->object,t->generation,&view) ||
           view.program!=o->view.program || view.storage!=o->view.storage)return false;
    }
    for(unsigned i=0;i<b->evaluated_count;++i) {
        const BoneJoint* j=&b->evaluated[i];
        if(j->object->parent!=j->parent || j->object->child!=j->child || j->object->next!=j->next ||
           HSD_CLASS_METHOD(j->object)!=j->info || j->object->robj!=j->constraints ||
           j->object->aobj!=j->animation || (j->animation&&j->animation->hsd_obj!=j->anchor))return false;
    }
    return true;
}
static bool bone_integral(const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!value || !nativeScriptExpression_IsIntegral(value->type) ||
       (value->type==NativeScriptType_Bool&&value->as.u64>1) ||
       (value->type==NativeScriptType_I32&&(value->as.i64<INT32_MIN||value->as.i64>INT32_MAX)) ||
       (value->type==NativeScriptType_U32&&value->as.u64>UINT32_MAX))
        return error(d,"Bone query requires well-formed integral handle and raw part");
    return true;
}
static bool bone_part(const NativeScriptValue* value,uint32_t count,uint32_t* result)
{
    uint64_t index;
    if(value->type==NativeScriptType_I32||value->type==NativeScriptType_I64) {
        if(value->as.i64<0)return false;index=(uint64_t)value->as.i64;
    } else index=value->as.u64;
    if(index>=count)return false;*result=(uint32_t)index;return true;
}
static bool bone_capture(const Caller* caller,const Target* target,const NativeScriptValue* part,
    BoneTarget* b,NativeScriptDiagnostic* d)
{
    b->caller=caller;b->target=*target;b->data=target->data->ft_data;
    if(target->kind>=Ft_Kind_RuntimeCapacity || !ftPartsTable || !ftPartsTable[target->kind] || !target->data->parts)return true;
    b->table=ftPartsTable[target->kind];b->count=b->table->parts_num;b->parts=target->data->parts;
    if(!b->count||b->count>MAX_FT_PARTS)return error(d,"Bone query part count exceeds the actual native rig");
    if(!bone_part(part,b->count,&b->index)||!b->parts[b->index].joint)return true;
    b->forward=b->table->part_to_joint;b->reverse=b->table->joint_to_part;
    b->root=target->object->hsd_obj;b->root_kind=target->object->obj_kind;b->costume_id=target->data->x619_costume_id;
    b->selected=b->parts[b->index].joint;b->parent=b->selected->parent;b->info=HSD_CLASS_METHOD(b->selected);
    b->attached=nativeScriptFighter_IsAttached(target->object);
    if(b->attached) {
        ExportTarget* o=&b->owned;o->target=*target;
        o->package=nativeFighterRuntime_SourcePackage((FighterKind)target->kind);
        o->program=nativeScriptFighter_Program((FighterKind)target->kind);
        if(!o->package || !o->package->ready || !o->package->assets.ready || o->package->target!=(FighterKind)target->kind ||
           !o->program || !o->program->bank || !nativeScriptFighter_View(target->object,target->generation,&o->view) ||
           !o->view.storage || !o->view.storage->layout || !o->view.storage->parameters)
            return error(d,"Bone target has no selected owned program/storage");
        o->bank=o->program->bank;o->identity=o->program->identity;
        o->layout=o->view.storage->layout->identity;o->parameters=o->view.storage->parameters->identity;
        if(o->bank!=&o->package->compiler.bank || b->data!=&o->package->assets.character.data ||
           b->table!=&o->package->assets.parts || b->forward!=o->package->assets.part_to_joint ||
           b->reverse!=o->package->assets.joint_to_part ||
           b->count!=o->package->assets.facts.joints+o->package->assets.facts.omission_count)
            return error(d,"Bone target rig differs from its owned imported assets");
        b->costume=&o->package->assets.costume;
    } else {
        const struct UnkCostumeList* costumes=&CostumeListsForeachCharacter[target->kind];
        if(costumes->costume_list&&b->costume_id<costumes->numCostumes)
            b->costume=costumes->costume_list[b->costume_id].native_assets;
    }
    if(!b->root || b->root_kind!=HSD_GObj_JObjKind || !b->info || b->info->head.obj_size<(int)sizeof(HSD_JObj) ||
       !hsdObjIsDescendantOf((HSD_Obj*)b->selected,HSD_CLASS_INFO(&hsdJObj)))
        return error(d,"Bone target joint lacks actual native model/class extent");
    return bone_current(b)?true:error(d,"Bone target identity/schema changed during rig capture");
}
static bool bone_member(const BoneTarget* b,const HSD_JObj* object)
{
    for(unsigned i=0;i<b->count;++i)if(b->parts[i].joint==object)return true;
    return false;
}
static const HSD_Joint* bone_descriptor(const BoneTarget* b,const HSD_JObj* joint)
{
    if(!b->costume || !joint->id)return NULL;
    const HSD_DatImport* import=&b->costume->import;
    if(import->status!=HSD_DAT_OK || !import->records || import->record_count>import->record_capacity)return NULL;
    for(unsigned i=0;i<import->record_count;++i)
        if(import->records[i].type==DAT_JOINT && (uintptr_t)import->records[i].object==joint->id)return import->records[i].object;
    return NULL;
}
static bool bone_evaluation(BoneTarget* b,HSD_JObj* joint,NativeScriptDiagnostic* d)
{
    if(!joint || !HSD_JObjMtxIsDirty(joint))return true;
    for(unsigned i=0;i<b->evaluated_count;++i)if(b->evaluated[i].object==joint)return true;
    if(b->evaluated_count==MAX_FT_PARTS || !bone_member(b,joint))
        return error(d,"Bone matrix evaluation leaves the retained native rig");
    HSD_ClassInfo* info=HSD_CLASS_METHOD(joint);
    if(!info || info->head.obj_size<(int)sizeof(*joint) || !hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj)))
        return error(d,"Bone matrix dependency lacks native JObj extent");
    if(!bone_current(b))return error(d,"Bone target changed during matrix class admission");
    if(HSD_JOBJ_METHOD(joint)->make_mtx!=HSD_JObjMakeMatrix)
        return error(d,"Bone matrix uses an unsupported custom native matrix method");
    b->evaluated[b->evaluated_count++]=(BoneJoint){.object=joint,.parent=joint->parent,.child=joint->child,.next=joint->next,
        .info=info,.constraints=joint->robj,.animation=joint->aobj,.anchor=joint->aobj?joint->aobj->hsd_obj:NULL};
    if(!bone_evaluation(b,joint->parent,d) || !bone_evaluation(b,joint->aobj?(HSD_JObj*)joint->aobj->hsd_obj:NULL,d))return false;
    // Native IK solvers inspect the two-joint/effector subtree. They are direct
    // HSD math, not authored callbacks; admit their actual rig dependencies.
    if(joint->flags&JOBJ_JOINT) {
        unsigned visits=0;
        for(HSD_JObj* child=joint->child;child;child=child->next) {
            if(++visits>MAX_FT_PARTS || !bone_evaluation(b,child,d))return error(d,"Bone IK subtree is not bounded by its native rig");
        }
    }
    const HSD_Joint* descriptor=NULL;unsigned visits=0;
    for(HSD_RObj* r=joint->robj;r;r=r->next) {
        if(++visits>256)return error(d,"Bone constraints exceed the bounded native chain");
        unsigned type=r->flags&ROBJ_TYPE_MASK;
        if(type==REFTYPE_JOBJ) {if(!bone_evaluation(b,r->u.jobj,d))return false;}
        else if(type==REFTYPE_EXP && (r->flags&0x80000000)) {
            unsigned channel=r->flags&0x0fffffff;
            // EXP supplies scalar ObjData. Path/vector channels need additional
            // closure; effect/UFC channels invoke global native callbacks.
            if(channel==HSD_A_J_PATH || (channel>=HSD_A_J_SETBYTE0&&channel<=0x2a) ||
               (channel>=0x32&&channel<=0x35))
                return error(d,"Bone expression destination requires unsupported path/vector/callback semantics");
            if(!r->u.exp.is_bytecode) {
                if(!HSD_RObjExpressionUsesNativeDummy(r))return error(d,"Bone constraint uses an unsupported native expression callback");
            } else {
                if(!descriptor)descriptor=bone_descriptor(b,joint);
                const HSD_RObjDesc* origin=descriptor?descriptor->robjdesc:NULL;unsigned ordinal=0;
                while(origin&&++ordinal<visits)origin=origin->next;
                if(!origin || (origin->flags&ROBJ_TYPE_MASK)!=REFTYPE_BYTECODE || !origin->u.bcexp ||
                   r->u.exp.expr.bytecode!=origin->u.bcexp->bytecode)
                    return error(d,"Bone expression bytecode has no exact imported descriptor provenance");
            }
            unsigned args=0,rv_count=0;
            for(HSD_Rvalue* v=r->u.exp.rvalue;v;v=v->next) {
                if(++rv_count>256 || !bone_evaluation(b,v->jobj,d))return error(d,"Bone expression references exceed the native rig");
                for(u32 bits=v->flags;bits;bits&=bits-1)++args;
            }
            if(args>100)return error(d,"Bone expression exceeds the native default argument buffer");
        } else if(type!=REFTYPE_EXP && type!=REFTYPE_LIMIT && type!=REFTYPE_IKHINT)
            return error(d,"Bone constraint type is unsupported by native matrix evaluation");
    }
    return true;
}
static bool bone_invoke(NativeScriptRuntimeContext* c,NativeScriptValue* args,uint32_t count,
    NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out||!args||count!=5 || !bone_integral(&args[0],d)||!bone_integral(&args[1],d))
        return error(d,"Bone query argument contract is invalid");
    for(unsigned i=2;i<5;++i)if(args[i].type!=NativeScriptType_F32)return error(d,"Bone query outputs must be F32 destinations");
    Caller caller={0};if(!caller_capture(c,&caller,d))return false;
    uint32_t id;Target target={0};BoneTarget bone={0};Vec3 result;bool found=false;
    if(!handle(&args[0],&id,d) || !select(id,caller.context.scene_epoch,&target,d))return false;
    if(target.object) {
        if(!bone_capture(&caller,&target,&args[1],&bone,d))return false;
        if(bone.selected) {
            // The native root path samples local translation directly; a clean
            // cached matrix likewise performs no constraint/method evaluation.
            if(bone.parent&&!bone_evaluation(&bone,bone.selected,d))return false;
            if(!bone_current(&bone))return error(d,"Bone target changed before native sampling");
            found=nativeFighterTarget_BonePosition(id,bone.index,&result);
            if(!bone_current(&bone))return error(d,"Bone target rig/schema retired or changed during native sampling");
        }
    }
    if(!caller_live(&caller) || (target.object&&!target_live(&target,caller.context.scene_epoch)))
        return error(d,"Bone query caller or captured target retired before publication");
    if(found) {
        args[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=result.x};
        args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=result.y};
        args[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=result.z};
    }
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=found};return true;
}

static bool select_color(const NativeScriptValue* value,Caller* caller,Target* target,NativeScriptDiagnostic* d)
{
    if(value->type!=NativeScriptType_Object) {
        uint32_t id;return handle(value,&id,d) && select(id,caller->context.scene_epoch,target,d);
    }
    void* actor=NULL;uint32_t generation=0;
    if(!nativeScriptReceiver_ResolveExternal(&caller->context,value,caller->context.scene_epoch,
        NativeCallbackSource_Fighter,&actor,&generation,d))return false;
    if(!actor)return true;
    HSD_GObj* object=actor;NativeMatchPoolReference reference;
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference) ||
       reference.lifetime!=caller->context.scene_epoch || object->classifier!=HSD_GOBJ_CLASS_FIGHTER)
        return error(d,"Controller color target is not a physical Fighter in this scene");
    const Fighter* data=object->user_data;
    if(!data || data->gobj!=object)return error(d,"Controller color target has invalid Fighter data");
    *target=(Target){.object=object,.reference=reference,.data=data,.generation=generation,.kind=(uint32_t)data->kind};
    return target_live(target,caller->context.scene_epoch)?true:error(d,"Controller color target retired during capture");
}
static bool geometry_invoke(NativeScriptRuntimeContext* c,uint32_t semantic,NativeScriptValue* args,
    uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool shield=semantic==NativeScriptBinding_TargetsShield;
    unsigned outputs=shield?4:3;
    if(!out || !args || count!=outputs+1)return error(d,"Target geometry argument count is invalid");
    for(unsigned i=1;i<=outputs;++i)
        if(args[i].type!=NativeScriptType_F32)return error(d,"Target geometry outputs must be F32 destinations");
    Caller caller={0};if(!caller_capture(c,&caller,d))return false;
    uint32_t id;Target target={0};BoneTarget bone={0};Vec3 value={0};float radius=0;bool found=false;
    if(!handle(&args[0],&id,d) || !select(id,caller.context.scene_epoch,&target,d))return false;
    if(target.object) {
        const ftData* data=target.data->ft_data;
        const struct ftCo_DatAttrs* attributes=data?data->x0:NULL;
        const struct ftData_x8* parts=data?data->x8:NULL;
        ftCommonData* common=p_ftCommonData;
        if(data && (shield?parts!=NULL:attributes!=NULL)) {
            int32_t index=shield?parts->x11:attributes->camera_zoom_target_bone;
            NativeScriptValue part={.type=NativeScriptType_I32,.as.i64=index};
            if(!bone_capture(&caller,&target,&part,&bone,d))return false;
            if(bone.selected) {
                if(shield && target.kind!=Ft_Kind_Yoshi && !common)
                    return error(d,"Target shield radius requires native common shield data");
                // Camera offsets can force matrix evaluation at the root too.
                // Mirror lb_8000B1CC's actual root translation/SRT decision.
                const Vec3* offset=&target.data->co_attrs.x170;
                bool matrix=bone.parent!=NULL || (!shield && (offset->x || offset->y || offset->z) &&
                    (bone.selected->rotate.x || bone.selected->rotate.y || bone.selected->rotate.z ||
                     bone.selected->scale.x!=1 || bone.selected->scale.y!=1 || bone.selected->scale.z!=1));
                if(matrix && !bone_evaluation(&bone,bone.selected,d))return false;
                if(!bone_current(&bone) || data->x0!=attributes || data->x8!=parts ||
                   (shield?parts->x11:attributes->camera_zoom_target_bone)!=index || (shield&&p_ftCommonData!=common))
                    return error(d,"Target geometry metadata changed before native sampling");
                if(shield)radius=ftCo_80094098(target.object,&value.x);
                else ftLib_800866DC(target.object,&value);
                if(!bone_current(&bone) || data->x0!=attributes || data->x8!=parts ||
                   (shield?parts->x11:attributes->camera_zoom_target_bone)!=index || (shield&&p_ftCommonData!=common))
                    return error(d,"Target geometry rig or metadata changed during native sampling");
                found=true;
            }
        }
    }
    if(!caller_live(&caller) || (target.object&&!target_live(&target,caller.context.scene_epoch)))
        return error(d,"Target geometry caller or captured target retired before publication");
    if(found) {
        args[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value.x};
        args[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value.y};
        args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value.z};
        if(shield)args[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=radius};
    }
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=found};return true;
}
static bool search_fighter_capture(HSD_GObj* object,uint64_t epoch,Target* target,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;uint32_t generation;
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference) || reference.lifetime!=epoch ||
       object->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&generation))
        return error(d,"Ahead search references a retired or nonphysical Fighter");
    const Fighter* data=object->user_data;
    if(!data || data->gobj!=object)return error(d,"Ahead search Fighter data is malformed");
    *target=(Target){.object=object,.reference=reference,.data=data,.generation=generation,.kind=(uint32_t)data->kind};
    return target_live(target,epoch)?true:error(d,"Ahead search Fighter changed during capture");
}

static bool search_camera_admit(const Caller* caller,const Target* target,BoneTarget* bone,NativeScriptDiagnostic* d)
{
    const ftData* data=target->data->ft_data;
    const struct ftCo_DatAttrs* attributes=data?data->x0:NULL;
    if(!attributes)return error(d,"Ahead search Fighter has no camera-target metadata");
    int32_t index=attributes->camera_zoom_target_bone;
    NativeScriptValue part={.type=NativeScriptType_I32,.as.i64=index};
    if(!bone_capture(caller,target,&part,bone,d))return false;
    if(!bone->selected)return error(d,"Ahead search camera-target joint is missing from its actual rig");
    const Vec3* offset=&target->data->co_attrs.x170;
    bool matrix=bone->parent!=NULL || ((offset->x || offset->y || offset->z) &&
        (bone->selected->rotate.x || bone->selected->rotate.y || bone->selected->rotate.z ||
         bone->selected->scale.x!=1 || bone->selected->scale.y!=1 || bone->selected->scale.z!=1));
    if(matrix && !bone_evaluation(bone,bone->selected,d))return false;
    return (bone_current(bone) && data->x0==attributes && attributes->camera_zoom_target_bone==index) ||
        error(d,"Ahead search camera-target metadata changed during admission");
}

static bool ahead_invoke(NativeScriptRuntimeContext* c,uint32_t semantic,NativeScriptValue* args,
    uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out || !args || count!=3)return error(d,"Ahead search requires three Float outputs");
    for(unsigned i=0;i<3;++i)if(args[i].type!=NativeScriptType_F32)
        return error(d,"Ahead search outputs must be Float destinations");
    Caller caller={0};if(!caller_capture(c,&caller,d))return false;
    if(c->source_class!=NativeCallbackSource_Item)return error(d,"Ahead search requires an owned Article");
    const Item* item=caller.data;Vec3 origin=item->pos,value={0};float facing=item->facing_dir;
    HSD_GObj* owner_object=item->owner;
    bool found=false;
    if(HSD_GObjPLinkHead) {
        unsigned visited=0;
        if(semantic==NativeScriptBinding_TargetsReadOpponentAhead) {
            Target owner={0};
            if(owner_object && !search_fighter_capture(owner_object,c->scene_epoch,&owner,d))return false;
            Target selected={0};float minimum=FLT_MAX;
            // Native nearest-opponent policy, with identity/rig admission before
            // each camera sample. Strict comparison retains registry-order ties.
            for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];object;object=object->next) {
                if(visited++==nativeGObjPool.capacity)return error(d,"Ahead search Fighter registry exceeds its physical pool");
                Target target={0};if(!search_fighter_capture(object,c->scene_epoch,&target,d))return false;
                if(ftLib_80086FD4(owner_object,object) || target.data->x221F_b3 ||
                   (gm_8016B168() && owner.object && target.data->team==owner.data->team))continue;
                BoneTarget bone={0};Vec3 sample;
                if(!search_camera_admit(&caller,&target,&bone,d))return false;
                ftLib_800866DC(object,&sample);
                if(!bone_current(&bone) || (owner.object&&!target_live(&owner,c->scene_epoch)))
                    return error(d,"Ahead search source, owner or camera rig retired during sampling");
                if((facing==-1 && sample.x>origin.x) || (facing==1 && sample.x<origin.x))continue;
                float dx=origin.x-sample.x,dy=origin.y-sample.y;
                float distance=dx*dx+dy*dy;
                if(distance<minimum) {minimum=distance;selected=target;}
            }
            if(!caller_live(&caller) || (owner.object&&!target_live(&owner,c->scene_epoch)))
                return error(d,"Ahead search source or owner retired during native sampling");
            if(selected.object) {
                if(!target_live(&selected,c->scene_epoch))return error(d,"Ahead search selected Fighter retired before resampling");
                BoneTarget bone={0};if(!search_camera_admit(&caller,&selected,&bone,d))return false;
                // The original item query resamples the selected camera target.
                ftLib_800866DC(selected.object,&value);
                if(!bone_current(&bone))return error(d,"Ahead search selected camera rig retired before publication");
                found=true;
            }
        } else {
            for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];object;object=object->next) {
                NativeMatchPoolReference reference;uint32_t generation;
                if(visited++==nativeGObjPool.capacity)return error(d,"Ahead search Item registry exceeds its physical pool");
                if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference) || reference.lifetime!=c->scene_epoch ||
                   object->classifier!=HSD_GOBJ_CLASS_ITEM ||
                   !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation))
                    return error(d,"Ahead search Item registry contains a retired or nonphysical object");
            }
            HSD_GObj* selected=it_8026C258(&origin,facing);
            if(selected) {it_8026BB88(selected,&value);found=true;}
        }
    }
    if(!caller_live(&caller) || item->owner!=owner_object)
        return error(d,"Ahead search caller retired before publication");
    if(found)for(unsigned i=0;i<3;++i)
        args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=i==0?value.x:i==1?value.y:value.z};
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=found};return true;
}

static bool overlap_invoke(NativeScriptRuntimeContext* c,NativeScriptValue* args,
    uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!c || c->source_class!=NativeCallbackSource_Item || !out || !args || count!=2 ||
       !bone_integral(&args[0],d) || !bone_integral(&args[1],d))
        return error(d,"Hitbox overlap requires an Article caller, Fighter handle and integral hitbox slot");
    Caller caller={0};if(!caller_capture(c,&caller,d))return false;
    uint32_t id,index;Target target={0};bool found=false;
    Item* item=(Item*)caller.data;
    if(!handle(&args[0],&id,d) || !select(id,c->scene_epoch,&target,d))return false;
    if(target.object && bone_part(&args[1],ARRAY_SIZE(item->x5D4_hitboxes),&index)) {
        Fighter* fighter=(Fighter*)target.data;
        if(fighter->hurt_capsules_len>ARRAY_SIZE(fighter->hurt_capsules))
            return error(d,"Hitbox overlap Fighter hurt capsules exceed native storage");
        if(fighter->hurt_capsules_len && (target.kind>=Ft_Kind_RuntimeCapacity ||
           !ftPartsTable || !ftPartsTable[target.kind] || !fighter->parts ||
           !ftPartsTable[target.kind]->parts_num || ftPartsTable[target.kind]->parts_num>MAX_FT_PARTS))
            return error(d,"Hitbox overlap Fighter lacks a bounded native rig");
        for(unsigned i=0;i<fighter->hurt_capsules_len;++i) {
            HurtCapsule* capsule=&fighter->hurt_capsules[i].capsule;
            unsigned part=0,parts=ftPartsTable[target.kind]->parts_num;
            for(;part<parts;++part)if(fighter->parts[part].joint==capsule->bone)break;
            if(part==parts || !capsule->bone)
                return error(d,"Hitbox overlap hurt capsule bone is outside its native rig");
            BoneTarget bone={0};NativeScriptValue raw={.type=NativeScriptType_I32,.as.i64=part};
            if(!bone_capture(&caller,&target,&raw,&bone,d) || !bone.selected ||
               !bone_evaluation(&bone,bone.selected,d))return false;
            // Geometry only: preserve native endpoint-cache writes, Item radius
            // scaling, Fighter depth and early exit. Combat immunity/state
            // filtering belongs to the caller, not this capsule operation.
            found=lbColl_80008248(&item->x5D4_hitboxes[index].hit,capsule,
                ftCommon_8007F804(fighter),item->scl,fighter->x34_scale.y,fighter->cur_pos.z);
            if(!bone_current(&bone))return error(d,"Hitbox overlap rig or caller retired before publication");
            if(found)break;
        }
    }
    if(!caller_live(&caller) || (target.object&&!target_live(&target,c->scene_epoch)))
        return error(d,"Hitbox overlap caller or target retired before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=found};return true;
}

bool nativeScriptBindingTargets_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_TargetsNearestAvailableFighter) {
        if(!out || count)return error(d,"Nearest available Fighter search takes no arguments");
        if(!c || c->source_class!=NativeCallbackSource_Fighter)
            return error(d,"Nearest available Fighter search requires a Fighter caller");
        // Membership checks used by caller_capture also traverse this list.
        // Admit its physical links first, before any registry-based lifetime
        // check can read a foreign head or follow a cycle.
        if(HSD_GObjPLinkHead) {
            unsigned visited=0;
            for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];object;object=object->next) {
                NativeMatchPoolReference reference;
                if(visited++==nativeGObjPool.capacity ||
                   !nativeMatchPool_Reference(&nativeGObjPool,object,&reference) ||
                   reference.lifetime!=c->scene_epoch || object->classifier!=HSD_GOBJ_CLASS_FIGHTER)
                    return error(d,"Nearest available Fighter registry has a foreign, retired or cyclic link");
            }
        }
        Caller caller={0};if(!caller_capture(c,&caller,d))return false;
        Target selected={0};
        if(HSD_GObjPLinkHead) {
            // The common search reads every candidate, so admit the complete
            // physical registry before invoking its unchanged native policy.
            unsigned visited=0;
            for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];object;object=object->next) {
                if(visited++==nativeGObjPool.capacity)
                    return error(d,"Nearest available Fighter registry is cyclic or exceeds its pool");
                Target target={0};if(!search_fighter_capture(object,c->scene_epoch,&target,d))return false;
            }
            if(!caller_live(&caller))return error(d,"Nearest available Fighter caller retired before search");
            // Includes allies, uses caller CPU body margins and two-dimensional
            // distance, and retains the first tie. This is not nearest enemy.
            Fighter* result=ftCo_800A4A40((Fighter*)caller.data);
            if(result && !search_fighter_capture(result->gobj,c->scene_epoch,&selected,d))return false;
            if(result && selected.data!=result)
                return error(d,"Nearest available Fighter result changed native identity");
        }
        if(!caller_live(&caller) || (selected.object&&!target_live(&selected,c->scene_epoch)))
            return error(d,"Nearest available Fighter source or result retired before publication");
        *out=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=selected.object?(int32_t)selected.generation:0};
        return true;
    }
    if(semantic==NativeScriptBinding_TargetsOverlapsHitbox)return overlap_invoke(c,args,count,out,d);
    if(semantic==NativeScriptBinding_TargetsReadOpponentAhead || semantic==NativeScriptBinding_TargetsReadLockableItemAhead)
        return ahead_invoke(c,semantic,args,count,out,d);
    if(semantic==NativeScriptBinding_TargetsReadBonePosition)return bone_invoke(c,args,count,out,d);
    if(semantic==NativeScriptBinding_TargetsShield || semantic==NativeScriptBinding_TargetsReadCameraTargetPosition)
        return geometry_invoke(c,semantic,args,count,out,d);
    if((semantic&NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN_MASK)==NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN)
        return export_invoke(c,semantic,args,count,out,d);
    bool delta=semantic==NativeScriptBinding_TargetsReadPositionDelta;
    bool position=semantic==NativeScriptBinding_TargetsReadPosition||delta;
    bool motion=semantic==NativeScriptBinding_TargetsMotion;
    bool scalar=semantic==NativeScriptBinding_TargetsDamage || semantic==NativeScriptBinding_TargetsFacing ||
        semantic==NativeScriptBinding_TargetsCpuBodyHalfWidth || semantic==NativeScriptBinding_TargetsModelScale;
    bool guard=semantic==NativeScriptBinding_TargetsGuardActive;
    bool sleeping=semantic==NativeScriptBinding_TargetsSleeping;
    bool floor=semantic==NativeScriptBinding_TargetsFloorLine;
    bool color=semantic==NativeScriptBinding_TargetsControllerColor || semantic==NativeScriptBinding_TargetsControllerColorPresent;
    if(!out || !args || (!position&&!motion&&!scalar&&!guard&&!sleeping&&!floor&&!color&&semantic!=NativeScriptBinding_TargetsAirborne&&semantic!=NativeScriptBinding_TargetsIsFighter&&semantic!=NativeScriptBinding_TargetsSmashChargeActive) ||
       count!=(position?4u:1u))return error(d,"Target query semantic or argument count is invalid");
    if(position)for(unsigned i=1;i<4;++i)if(args[i].type!=NativeScriptType_F32)return error(d,"Target position outputs must be F32 destinations");
    Caller caller={0};if(!caller_capture(c,&caller,d))return false;
    Target target={0};
    if(color) {if(!select_color(&args[0],&caller,&target,d))return false;}
    else {uint32_t id;if(!handle(&args[0],&id,d) || !select(id,caller.context.scene_epoch,&target,d))return false;}
    Vec3 value={0};GXColor rgba={0};bool result=false;int32_t motion_id=-1;float scalar_value=0.0f;
    if(target.object) {
        if(color)result=ftLib_800870BC(target.object,&rgba);
        else if(position){if(delta)ftLib_80086BEC(target.object,&value);else value=target.data->cur_pos;result=true;}
        else if(motion)motion_id=target.data->motion_id;
        else if(floor)motion_id=target.data->coll_data.floor.index;
        else if(sleeping)result=target.data->x221F_b3!=0;
        else if(guard)result=ftLib_80086A18(target.object);
        else if(semantic==NativeScriptBinding_TargetsModelScale)scalar_value=ftLib_800869D4(target.object);
        else if(scalar) {
            const float* field=semantic==NativeScriptBinding_TargetsDamage?&target.data->dmg.x1830_percent:
                semantic==NativeScriptBinding_TargetsFacing?&target.data->facing_dir:&target.data->cpu.x564;
            memcpy(&scalar_value,field,sizeof(scalar_value));
        }
        else if(semantic==NativeScriptBinding_TargetsSmashChargeActive)result=target.data->smash_attrs.state==2;
        else result=semantic==NativeScriptBinding_TargetsIsFighter || target.data->ground_or_air==GA_Air;
    }
    if(!caller_live(&caller))return error(d,"Target query caller retired/faulted before publication");
    if(target.object&&!target_live(&target,caller.context.scene_epoch))return error(d,"Captured Fighter target changed before publication");
    if(position&&result) {
        // No normalization: preserve native F32 position/delta bits, including
        // infinities, NaNs and signed zero. Publish only after all guards.
        args[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value.x};
        args[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value.y};
        args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value.z};
    }
    if(motion||floor)*out=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=motion_id};
    else if(scalar) {
        *out=(NativeScriptValue){.type=NativeScriptType_F32};
        memcpy(&out->as.f32,&scalar_value,sizeof(scalar_value));
    }
    else if(semantic==NativeScriptBinding_TargetsControllerColor) {
        uint32_t bits=((uint32_t)rgba.r<<24)|((uint32_t)rgba.g<<16)|((uint32_t)rgba.b<<8)|rgba.a;
        int32_t packed;memcpy(&packed,&bits,sizeof(packed));
        *out=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=packed};
    } else *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=result};return true;
}
