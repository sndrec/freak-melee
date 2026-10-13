#include "native_script_binding_spawn.h"
#include "native_script_actor_access.h"
#include "native_script_fighter.h"
#include "native_callback_lifetime.h"
#include "ftdata.h"
#include "ftdat.h"
#include "ftparts.h"
#include "fighter.h"
#include "types.h"
#include <melee/it/native_script_item_spawn.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/item.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/types.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct Caller {
    NativeScriptRuntimeContext* context;
    NativeMatchPoolReference reference;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
} Caller;
typedef struct Target {
    Item_GObj* object;
    NativeMatchPoolReference reference;
    const NativeScriptItemProgram* program;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
    uint32_t generation;
} Target;
static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool alive(const Caller* c)
{
    NativeScriptReceiverView view;
    return nativeMatchPool_Resolve(c->reference)==c->context->source &&
        c->reference.lifetime==c->context->scene_epoch && nativeScriptBindings_SourceAlive(c->context) &&
        nativeScriptActors_Lookup(NULL,c->context->source,c->context->source_class,c->context->generation,&view) &&
        (c->context->source_class==NativeCallbackSource_Item ? nativeScriptItem_Status(c->context->source)==NativeScriptVM_Ok :
            nativeScriptFighter_Status(c->context->source)==NativeScriptVM_Ok) &&
        view.program==c->view.program && view.storage==c->view.storage && view.storage && view.storage->live &&
        view.storage->layout->identity==c->layout && view.storage->parameters && view.storage->parameters->identity==c->parameters;
}
static bool capture(NativeScriptRuntimeContext* context,Caller* c,NativeScriptDiagnostic* d)
{
    if(!context || !context->scene_epoch || context->scene_epoch!=nativeGObjPool.lifetime ||
       !nativeScriptBindings_SourceAlive(context) ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&c->reference) ||
       !nativeScriptActors_Lookup(NULL,context->source,context->source_class,context->generation,&c->view) ||
       !c->view.program || !c->view.storage || !c->view.storage->live || !c->view.storage->parameters)
        return error(d,"Source entity operation requires a live interpreted actor in its pinned scene");
    c->context=context;c->layout=c->view.storage->layout->identity;c->parameters=c->view.storage->parameters->identity;
    return alive(c)?true:error(d,"Source entity caller/program retired during capture");
}
static bool target(const Caller* c,Item_GObj* object,Target* out)
{
    Target value={.object=object};
    if(!object || !nativeMatchPool_Reference(&nativeGObjPool,object,&value.reference) ||
       value.reference.lifetime!=c->reference.lifetime ||
       !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&value.generation))return false;
    Item* item=object->user_data;
    value.program=nativeScriptItem_Spec(object);
    if(item->native_removal_in_progress || !value.program || !value.program->bank ||
       &value.program->bank->declarations!=c->view.program ||
       item->native_script_owner!=value.program->owner ||
       !nativeScriptItem_View(object,value.generation,&value.view) || value.view.program!=c->view.program ||
       !value.view.storage || !value.view.storage->live || !value.view.storage->parameters)return false;
    value.layout=value.view.storage->layout->identity;value.parameters=value.view.storage->parameters->identity;
    *out=value;return true;
}
static bool target_alive(const Caller* c,const Target* t)
{
    NativeScriptReceiverView view;
    return alive(c) && nativeMatchPool_Resolve(t->reference)==t->object &&
        nativeCallbackSource_Alive(t->object,t->generation,NativeCallbackSource_Item) &&
        !((Item*)t->object->user_data)->native_removal_in_progress && nativeScriptItem_Spec(t->object)==t->program &&
        nativeScriptItem_View(t->object,t->generation,&view) && view.program==t->view.program && view.storage==t->view.storage &&
        view.storage && view.storage->live && view.storage->layout->identity==t->layout && view.storage->parameters &&
        view.storage->parameters->identity==t->parameters;
}
static bool handle(const NativeScriptValue* value,int32_t* result,NativeScriptDiagnostic* d)
{
    double number;
    switch(value->type) {
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(value->as.i64<INT32_MIN || value->as.i64>INT32_MAX)return error(d,"Article handle exceeds signed creation-ID width");
        *result=(int32_t)value->as.i64;return true;
    case NativeScriptType_U32:case NativeScriptType_U64:
        if(value->as.u64>INT32_MAX)return error(d,"Article handle exceeds signed creation-ID width");
        *result=(int32_t)value->as.u64;return true;
    case NativeScriptType_F32:
        number=value->as.f32;
        if(fabs(number)>16777215.0)return error(d,"Float article handle exceeds its exact creation-ID range");
        break;
    case NativeScriptType_F64:number=value->as.f64;break;
    default:return error(d,"Article handle must be a numeric creation ID");
    }
    if(!isfinite(number) || number<INT32_MIN || number>INT32_MAX || trunc(number)!=number)
        return error(d,"Article handle is not an exact signed creation ID");
    *result=(int32_t)number;return true;
}
static bool owned(const Caller* c,const Target* t)
{
    if(!target_alive(c,t) || c->context->source_class!=NativeCallbackSource_Fighter || t->program->recipe)return false;
    Item* item=t->object->user_data;
    return item->owner==c->context->source && nativeItem_Creator(t->object)==c->context->source && target_alive(c,t);
}
static bool find_owned(const Caller* c,int32_t id,Target* result)
{
    if(id<0 || !HSD_GObjPLinkHead)return false;
    uint32_t walked=0;
    for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];object && walked++<nativeGObjPool.capacity;object=object->next) {
        Target current;
        if(target(c,object,&current) && (int32_t)current.generation==id && owned(c,&current)) {*result=current;return true;}
    }
    return false;
}
static bool bone(const Caller* c,uint32_t part,uint32_t* raw,NativeScriptDiagnostic* d)
{
    if(c->context->source_class!=NativeCallbackSource_Fighter)return error(d,"Article bone spawn requires a Fighter source");
    Fighter* fp=((HSD_GObj*)c->context->source)->user_data;
    FighterPartsTable* table=(uint32_t)fp->kind<Ft_Kind_RuntimeCapacity && ftPartsTable?ftPartsTable[fp->kind]:NULL;
    if(part>=FT_COMMON_PART_COUNT || !table || !table->part_to_joint || !fp->parts ||
       !table->parts_num || table->parts_num>MAX_FT_PARTS)return error(d,"Article semantic part/model schema is absent or out of range");
    uint32_t index=(uint32_t)ftParts_GetBoneIndex(fp,(Fighter_Part)part);
    if(index>=table->parts_num || !fp->parts[index].joint)return error(d,"Article semantic part is unmapped");
    *raw=index;return alive(c)?true:error(d,"Article source retired while resolving its bone");
}
static bool publish(const Caller* c,Item_GObj* object,bool scalar,NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    Target created;
    if(!target(c,object,&created) || !target_alive(c,&created))return error(d,"Spawned source Item/program retired before publication");
    if(scalar) {
        // Existing authored state may store this result as Float. Publishing
        // only this exact range prevents a newly-created ID rounding into an
        // older live ID; no persistent Object state or side registry is added.
        if(!owned(c,&created))return error(d,"Spawned scalar article no longer belongs to its captured author");
        if(created.generation>UINT32_C(16777215))return error(d,"Spawned creation ID exceeds the authored Float handle range");
        value->as.i64=(int32_t)created.generation;
    }
    return alive(c)?true:error(d,"Spawn source retired before output publication");
}
static bool native_word(const NativeScriptValue* value,uint32_t* result,NativeScriptDiagnostic* d)
{
    uint64_t group;
    if(value->type==NativeScriptType_I32||value->type==NativeScriptType_I64) {
        if(value->as.i64<0)return error(d,"Article owner/group/slot cannot be negative");group=(uint64_t)value->as.i64;
    } else if(value->type==NativeScriptType_Bool||value->type==NativeScriptType_U32||value->type==NativeScriptType_U64)group=value->as.u64;
    else return error(d,"Article owner/group/slot requires an integral value");
    if(group>UINT32_MAX)return error(d,"Article owner/group/slot exceeds its native word");*result=(uint32_t)group;return true;
}
static bool collision_word(const NativeScriptValue* value,uint32_t* result,NativeScriptDiagnostic* d)
{
    if(value->type==NativeScriptType_I32 || value->type==NativeScriptType_I64) {
        int64_t maximum=value->type==NativeScriptType_I32?INT32_MAX:(int64_t)UINT32_MAX;
        if(value->as.i64<INT32_MIN || value->as.i64>maximum)
            return error(d,"Article collision group exceeds its native word");
        *result=(uint32_t)value->as.i64;return true;
    }
    if(value->type==NativeScriptType_Bool && value->as.u64>1)
        return error(d,"Article collision group Bool is malformed");
    return native_word(value,result,d);
}

bool nativeScriptBindingSpawn_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* d)
{
    Caller caller;if(!result || !capture(context,&caller,d))return false;
    NativeScriptValue value={.type=NativeScriptType_Bool};
    if(semantic==NativeScriptBinding_ItemsHeld && context->source_class==NativeCallbackSource_Fighter) {
        Item_GObj* held=((Fighter*)((HSD_GObj*)context->source)->user_data)->item_gobj;
        NativeMatchPoolReference reference;uint32_t generation;
        value.as.u64=held && nativeMatchPool_Reference(&nativeGObjPool,held,&reference) &&
            reference.lifetime==caller.reference.lifetime && nativeCallbackSource_Snapshot(held,NativeCallbackSource_Item,&generation);
        if(value.as.u64) {
            const Item* item=held->user_data;
            value.as.u64=item->entity==held && !item->native_removal_in_progress && item->owner==context->source;
        }
    } else return error(d,"Source entity property/context is unsupported");
    if(!alive(&caller))return error(d,"Source entity caller retired before property publication");
    *result=value;return true;
}

bool nativeScriptBindingSpawn_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(semantic);
    if(!output || !descriptor || descriptor->argument_count!=count || (count&&!args))return error(d,"Source entity operation signature is invalid");
    Caller caller;if(!capture(context,&caller,d))return false;
    NativeScriptValue value={.type=descriptor->result_type};
    uint32_t base=nativeScriptBindingSpawn_Base(semantic),owner=semantic&NATIVE_SCRIPT_SPAWN_OWNER_MASK;
    if((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN &&
       (base==NativeScriptBinding_ArticlesLaunchAtPosition || base==NativeScriptBinding_ArticlesLaunchProjectile)) {
        bool world=base==NativeScriptBinding_ArticlesLaunchAtPosition;
        // The compiler resolves the declaration name into semantic's owner.
        // An It_Kind_* name can also evaluate to its stock numeric constant;
        // that literal is not the declaration's source-tree index.
        if(context->source_class!=NativeCallbackSource_Fighter || args[0].type!=NativeScriptType_I32 ||
           args[0].as.i64<INT32_MIN || args[0].as.i64>INT32_MAX)
            return error(d,"Projectile launch requires its prepared root Fighter declaration");
        unsigned angle_at=world?4:5,speed_at=world?5:6,state_at=world?6:7;
        unsigned first_float=world?1:2,last_float=world?5:6;
        for(unsigned n=first_float;n<=last_float;++n)
            if(args[n].type!=NativeScriptType_F32 || !isfinite(args[n].as.f32))
                return error(d,"Projectile position, offset, angle and speed require finite F32 values");
        if(args[state_at].type!=NativeScriptType_I32 || args[state_at].as.i64<0 || args[state_at].as.i64>INT32_MAX)
            return error(d,"Projectile launch state requires a nonnegative native Int");
        uint32_t group=0;int32_t variant=0;
        Fighter* fp=((HSD_GObj*)context->source)->user_data;float facing=fp->facing_dir;
        Vec3 position;
        if(world) {
            if(args[7].type!=NativeScriptType_I32 || args[7].as.i64<INT32_MIN || args[7].as.i64>INT32_MAX ||
               args[9].type!=NativeScriptType_F32 || !isfinite(args[9].as.f32))
                return error(d,"Projectile appearance requires a native Int and facing a finite F32");
            if(!collision_word(&args[8],&group,d))return false;
            position=(Vec3){args[1].as.f32,args[2].as.f32,args[3].as.f32};
            variant=(int32_t)args[7].as.i64;facing=args[9].as.f32;
        } else {
            if(args[1].type!=NativeScriptType_U32 || args[1].as.u64>UINT32_MAX)
                return error(d,"Projectile launch bone requires its semantic U32 part");
            uint32_t raw;if(!bone(&caller,(uint32_t)args[1].as.u64,&raw,d))return false;
            Vec3 offset={args[2].as.f32,args[3].as.f32,args[4].as.f32};
            lb_8000B1CC(fp->parts[raw].joint,&offset,&position);
            if(!alive(&caller))return error(d,"Projectile source retired during offset position read");
        }
        NativeScriptItemLaunchData launch={.angle=args[angle_at].as.f32,.speed=args[speed_at].as.f32,
            .state=(uint32_t)args[state_at].as.i64,.collision_group=group,.preserve_facing=world};
        Item_GObj* created=NULL;
        NativeScriptVMStatus status=nativeScriptItemSpawn_Projectile(context,owner,&position,facing,variant,&launch,&created,d);
        if(status!=NativeScriptVM_Ok || !publish(&caller,created,false,&value,d))
            return error(d,"Explicit projectile launch or its source/item program retired");
        *output=value;return true;
    }
    if((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN &&
       base==NativeScriptBinding_ArticlesCreateHeldAtCenter) {
        if(context->source_class!=NativeCallbackSource_Fighter || count!=6 ||
            args[0].type!=NativeScriptType_I32 || args[0].as.i64<INT32_MIN || args[0].as.i64>INT32_MAX ||
           args[1].type!=NativeScriptType_F32 || args[2].type!=NativeScriptType_F32 ||
           args[3].type!=NativeScriptType_F32 || args[5].type!=NativeScriptType_F32)
            return error(d,"Held-at-center Article requires its prepared owner and exact scalar inputs");
        uint32_t raw;
        if(args[4].type==NativeScriptType_I32 || args[4].type==NativeScriptType_I64) {
            if(args[4].as.i64<INT32_MIN || args[4].as.i64>INT32_MAX)
                return error(d,"Held Article signed raw part exceeds native width");
            raw=(uint32_t)args[4].as.i64;
        } else if(args[4].type==NativeScriptType_U32 || args[4].type==NativeScriptType_U64 || args[4].type==NativeScriptType_Bool) {
            if(args[4].as.u64>UINT32_MAX || (args[4].type==NativeScriptType_Bool && args[4].as.u64>1))
                return error(d,"Held Article unsigned raw part exceeds native width");
            raw=(uint32_t)args[4].as.u64;
        } else return error(d,"Held Article raw part requires an integral value");
        Vec3 previous={args[1].as.f32,args[2].as.f32,args[3].as.f32};Item_GObj* object=NULL;
        NativeScriptVMStatus status=nativeScriptItemSpawn_HeldAtCenter(context,owner,&previous,
            raw,args[5].as.f32,&object,d);
        if(status!=NativeScriptVM_Ok)return false;
        value.as.i64=-1;
        if(object) {
            Target child;if(!target(&caller,object,&child) || !target_alive(&caller,&child))
                return error(d,"Held Article retired before handle publication");
            int32_t handle_bits;memcpy(&handle_bits,&child.generation,sizeof(handle_bits));
            value.as.i64=handle_bits;
        }
        if(!alive(&caller))return error(d,"Held Article source retired before publication");
        *output=value;return true;
    }
    if((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN&&base==NativeScriptBinding_ItemCreateLinked) {
        if(context->source_class!=NativeCallbackSource_Item||count!=7||args[0].type!=NativeScriptType_I32||
           args[1].type!=NativeScriptType_I32||args[2].type!=NativeScriptType_I32||
           args[3].type!=NativeScriptType_F32||args[4].type!=NativeScriptType_F32||
           args[5].type!=NativeScriptType_F32||args[6].type!=NativeScriptType_F32)
            return error(d,"Linked Article factory requires its exact Item source and signature");
        uint32_t slot,backlink;
        if(!native_word(&args[1],&slot,d)||!native_word(&args[2],&backlink,d)||slot>=4||backlink>=4)
            return error(d,"Linked Article slots must be integral values from 0 to 3");
        Item* source=((HSD_GObj*)context->source)->user_data;uint32_t damage_owner=0;
        if(source->owner&&!nativeCallbackSource_Snapshot(source->owner,NativeCallbackSource_Fighter,&damage_owner))
            return error(d,"Linked Article damage owner is not a live Fighter");
        Vec3 position={args[3].as.f32,args[4].as.f32,args[5].as.f32};Item_GObj* object=NULL;
        NativeScriptVMStatus status=nativeScriptItemSpawn_Child(context,owner,&position,args[6].as.f32,
            damage_owner,0,0,backlink,slot,&object,d);
        if(status!=NativeScriptVM_Ok||!alive(&caller))return error(d,"Linked Article factory or caller retired before publication");
        value.as.i64=-1;
        if(object) {
            Target child;
            if(!target(&caller,object,&child)||!target_alive(&caller,&child)||child.program->owner!=owner)
                return error(d,"Linked Article retired before handle publication");
            int32_t bits;memcpy(&bits,&child.generation,sizeof(bits));value.as.i64=bits;
        }
        *output=value;return true;
    }
    if((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN &&
       (base==NativeScriptBinding_ArticlesCreateAtPosition||base==NativeScriptBinding_ArticlesCreateWithVelocity||base==NativeScriptBinding_ArticlesCreateUnownedAtPosition||
        base==NativeScriptBinding_ItemCreateAtPosition||base==NativeScriptBinding_ItemCreateChild)) {
        bool item_factory=base==NativeScriptBinding_ItemCreateAtPosition||base==NativeScriptBinding_ItemCreateChild;
        if(context->source_class!=(item_factory?NativeCallbackSource_Item:NativeCallbackSource_Fighter))
            return error(d,"Positional Article factory requires its load-resolved source class");
        bool moving=base==NativeScriptBinding_ArticlesCreateWithVelocity;
        bool unowned=base==NativeScriptBinding_ArticlesCreateUnownedAtPosition;
        bool position_data=((semantic&NATIVE_SCRIPT_SPAWN_OPERATION_MASK)>>20)==NativeScriptSpawn_AtPositionData;
        if((!item_factory && !unowned && (args[moving?8:5].type!=NativeScriptType_Bool || args[moving?8:5].as.u64>1)) ||
           (moving && (args[11].type!=NativeScriptType_Bool || args[11].as.u64>1)))
            return error(d,"Positional Article flags require canonical Bool values");
        Vec3 position={args[1].as.f32,args[2].as.f32,args[3].as.f32},velocity={0};
        unsigned facing_index=(moving||unowned)?7:4;uint32_t group=0;float creation_value=0;
        if(moving){velocity=(Vec3){args[4].as.f32,args[5].as.f32,args[6].as.f32};creation_value=args[10].as.f32;
            if(!collision_word(&args[9],&group,d))return false;}
        else if(unowned)velocity=(Vec3){args[4].as.f32,args[5].as.f32,args[6].as.f32};
        else if(position_data){creation_value=args[7].as.f32;if(!collision_word(&args[6],&group,d))return false;}
        Item_GObj* object=NULL;
        NativeScriptVMStatus status;
        if(base==NativeScriptBinding_ItemCreateChild) {
            uint32_t damage_owner,backlink;
            if(!native_word(&args[5],&damage_owner,d) || !collision_word(&args[6],&group,d) ||
               !native_word(&args[8],&backlink,d))return false;
            if(backlink>3)return error(d,"Child backlink must select command slot 0 through 3");
            status=nativeScriptItemSpawn_Child(context,owner,&position,args[4].as.f32,damage_owner,group,args[7].as.f32,backlink,UINT32_MAX,&object,d);
        } else status=nativeScriptItemSpawn_At(context,owner,&position,&velocity,args[facing_index].as.f32,
            !item_factory && !unowned && args[facing_index+1].as.u64!=0,group,creation_value,!moving || args[11].as.u64!=0,!unowned,&object,d);
        if(status!=NativeScriptVM_Ok || !alive(&caller))return error(d,"Positional Article factory or captured relationships retired");
        value.as.i64=-1;
        if(object){Target created;
            if(!target(&caller,object,&created)||!target_alive(&caller,&created)||created.program->owner!=owner||
               created.generation>INT32_MAX)return error(d,"Positional Article/program retired or its handle exceeds signed width");
            // Damage ownership is mutable; it is not the identity of this
            // same-bank source declaration or a condition for handle publication.
            value.as.i64=(int32_t)created.generation;
        }
    } else if((semantic&NATIVE_SCRIPT_SPAWN_DOMAIN_MASK)==NATIVE_SCRIPT_SPAWN_DOMAIN) {
        uint32_t raw;if(!bone(&caller,(uint32_t)args[1].as.u64,&raw,d))return false;
        Item_GObj* created=NULL;NativeScriptVMStatus status;
        if(base==NativeScriptBinding_SpawnAuthoredProjectile) {
            Fighter* fp=((HSD_GObj*)context->source)->user_data;Vec3 position;
            lb_8000B1CC(fp->parts[raw].joint,NULL,&position);
            if(!alive(&caller))return error(d,"Projectile source retired during bone position read");
            status=nativeScriptItemSpawn_Projectile(context,owner,&position,fp->facing_dir,(int32_t)args[2].as.i64,NULL,&created,d);
        } else if(base==NativeScriptBinding_SpawnBoneAttached)
            status=nativeScriptItemSpawn_Attached(context,owner,raw,&created,d);
        else status=nativeScriptItemSpawn_AttachedPresentation(context,owner,raw,(int32_t)args[2].as.i64,
            (int32_t)args[3].as.i64,(uint32_t)args[4].as.u64,(uint32_t)args[5].as.u64,args[6].as.f32,&created,d);
        if(status!=NativeScriptVM_Ok || !publish(&caller,created,base!=NativeScriptBinding_SpawnAuthoredProjectile,&value,d))return false;
    } else if(base>=NativeScriptBinding_ArticlesOwned && base<=NativeScriptBinding_ArticlesRemoveOwned) {
        int32_t id;Target selected;
        if(context->source_class!=NativeCallbackSource_Fighter || !handle(&args[0],&id,d))return false;
        bool present=find_owned(&caller,id,&selected);
        if(!alive(&caller))return error(d,"Article caller retired during owned lookup");
        value.as.u64=present;
        if(present && base!=NativeScriptBinding_ArticlesOwned) {
            if(base==NativeScriptBinding_ArticlesPauseOwned)it_8026B724(selected.object);
            else if(base==NativeScriptBinding_ArticlesResumeOwned)it_8026B73C(selected.object);
            else Item_8026A8EC(selected.object);
            if(!alive(&caller))return error(d,"Article caller retired during owned operation");
            if(base!=NativeScriptBinding_ArticlesRemoveOwned && !owned(&caller,&selected))return error(d,"Captured article retired or changed ownership during operation");
        }
    } else if(base==NativeScriptBinding_ItemsPull) {
        if(context->source_class!=NativeCallbackSource_Fighter)return error(d,"Holdable pull requires a Fighter source");
        Fighter* fp=((HSD_GObj*)context->source)->user_data;
        const NativeScriptFighterProgram* program=nativeScriptFighter_Program(fp->kind);
        uint32_t index=(uint32_t)args[0].as.u64;
        if(!program || !program->bank || &program->bank->declarations!=caller.view.program || index>=program->items.recipe_count)
            return error(d,"Holdable recipe index or pinned root program is invalid");
        const NativeScriptItemProgram* recipe=NULL;
        for(uint32_t i=0;i<program->items.count;++i)if(program->items.programs[i].recipe && program->items.programs[i].recipe_index==index) {
            if(recipe)return error(d,"Immutable holdable recipe index is ambiguous");recipe=&program->items.programs[i];
        }
        if(!recipe)return error(d,"Immutable holdable recipe index is absent");
        Item_GObj* created=NULL;
        if(nativeScriptItemSpawn_PullHoldable(context,recipe->owner,&created,d)!=NativeScriptVM_Ok || !publish(&caller,created,false,&value,d))return false;
        if(fp->item_gobj!=created || ((Item*)created->user_data)->owner!=context->source || nativeItem_Creator(created)!=context->source)
            return error(d,"Pulled Item no longer has the captured author/holder relation");
        value.as.u64=true;
    } else if(base==NativeScriptBinding_ItemSpawnFood) {
        Item_GObj* created=NULL;
        if(nativeScriptItemSpawn_Food(context,&created,d)!=NativeScriptVM_Ok || !alive(&caller))return false;
    } else if(base==NativeScriptBinding_ItemChangeState || base==NativeScriptBinding_ItemChangeStateFlags ||
              base==NativeScriptBinding_ItemActivateAttached || base==NativeScriptBinding_ItemInteract) {
        if(context->source_class!=NativeCallbackSource_Item)return error(d,"Item lifecycle operation requires an Item source");
        const NativeScriptItemProgram* program=nativeScriptItem_Spec(context->source);
        if(!program || !program->bank || &program->bank->declarations!=caller.view.program)return error(d,"Item lifecycle program/schema does not match its caller");
        if(base==NativeScriptBinding_ItemChangeState || base==NativeScriptBinding_ItemChangeStateFlags) {
            if(args[0].type!=NativeScriptType_I32 || args[0].as.i64<INT32_MIN || args[0].as.i64>INT32_MAX ||
               (base==NativeScriptBinding_ItemChangeStateFlags &&
                (args[1].type!=NativeScriptType_U32 || args[1].as.u64>UINT32_MAX)))
                return error(d,"Item state transition requires canonical I32 state and U32 flags");
            int64_t next=args[0].as.i64;uint32_t flags=base==NativeScriptBinding_ItemChangeState?ITEM_ANIM_UPDATE:(uint32_t)args[1].as.u64;
            if(next<0 || (uint64_t)next>=program->state_count)return error(d,"Item state exceeds its exact owned state table");
            NativeItemLifetime item;
            if(!nativeItemLifetime_Capture(context->source,&item) || !item.source_owned ||
               item.program!=program || !item.article || !item.attributes || !item.model ||
               nativeScriptItem_Revision(context->source)==UINT64_MAX)
                return error(d,"Item state transition requires its retained owned asset/model and available revision");
            Item_80268E5C(context->source,(int32_t)next,(Item_StateChangeFlags)flags);
            // A nested transition may select another state and revision. It is
            // valid while the same actor, immutable assets and storage survive.
            if(!nativeItemLifetime_Current(&item) || !alive(&caller))
                return error(d,"Item retired or changed owned identity during state transition");
        } else if(base==NativeScriptBinding_ItemActivateAttached) {
            Item_802694CC(context->source);
            if(!alive(&caller))return error(d,"Item retired during attached animation update");
            it_80274574(context->source);
        } else {
            void* other;uint32_t generation;NativeMatchPoolReference reference;
            if(!nativeScriptReceiver_ResolveExternal(context,&args[0],context->scene_epoch,UINT32_MAX,&other,&generation,d) ||
               !other || !nativeMatchPool_Reference(&nativeGObjPool,other,&reference))return error(d,"Item interaction target is absent or retired");
            uint32_t source_class=args[0].as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK;
            value.as.u64=it_8026B894(context->source,other);
            if(nativeMatchPool_Resolve(reference)!=other || !nativeCallbackSource_Alive(other,generation,source_class))
                return error(d,"Item interaction target retired before publication");
        }
    } else return error(d,"Source entity factory was not load-resolved or operation is unsupported");
    if(!alive(&caller))return error(d,"Source entity caller/program retired before output publication");
    *output=value;return true;
}
