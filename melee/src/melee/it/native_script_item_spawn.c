#include "native_script_item_spawn.h"
#include "native_script_item_actor.h"
#include "native_item_owner.h"
#include "native_item_lifetime.h"
#include "item.h"
#include "types.h"
#include "inlines.h"
#include "kinds/inlines.h"
#include "it_26B1.h"
#include "it_2725.h"
#include "ithitbox.h"
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_math.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/db/db.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct Caller {
    NativeScriptRuntimeContext* context;
    NativeMatchPoolReference reference;
    NativeScriptReceiverView view;
    uint64_t layout, parameters;
    FighterKind author;
} Caller;
typedef struct Created {Item_GObj* object;NativeMatchPoolReference reference;uint32_t generation;} Created;
static NativeScriptVMStatus error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return NativeScriptVM_Error;}
static bool vector(const Vec3* value)
{return value&&isfinite(value->x)&&isfinite(value->y)&&isfinite(value->z);}
static bool caller_alive(const Caller* c)
{
    NativeScriptReceiverView current;
    return c->reference.lifetime==c->context->scene_epoch && nativeMatchPool_Resolve(c->reference)==c->context->source &&
        nativeScriptBindings_SourceAlive(c->context) &&
        (c->context->source_class==NativeCallbackSource_Fighter?
            nativeScriptFighter_Status(c->context->source):nativeScriptItem_Status(c->context->source))==NativeScriptVM_Ok &&
        nativeScriptActors_Lookup(NULL,c->context->source,c->context->source_class,c->context->generation,&current) &&
        current.program==c->view.program && current.storage==c->view.storage && current.storage && current.storage->live &&
        current.storage->layout->identity==c->layout && current.storage->parameters && current.storage->parameters->identity==c->parameters;
}
static bool capture(NativeScriptRuntimeContext* context,Caller* c)
{
    if(!context || !context->scene_epoch || context->scene_epoch!=nativeGObjPool.lifetime ||
       (context->source_class!=NativeCallbackSource_Fighter&&context->source_class!=NativeCallbackSource_Item) ||
       !nativeScriptBindings_SourceAlive(context) || !nativeMatchPool_Reference(&nativeGObjPool,context->source,&c->reference) ||
       !nativeScriptActors_Lookup(NULL,context->source,context->source_class,context->generation,&c->view) ||
       !c->view.storage || !c->view.storage->live || !c->view.storage->parameters)return false;
    c->context=context;c->layout=c->view.storage->layout->identity;c->parameters=c->view.storage->parameters->identity;
    HSD_GObj* object=context->source;
    if(context->source_class==NativeCallbackSource_Fighter)c->author=((Fighter*)object->user_data)->kind;
    else{const NativeScriptItemProgram* p=nativeScriptItem_Spec(object);if(!p)return false;c->author=p->author_kind;}
    return caller_alive(c);
}
static const NativeScriptItemProgram* selected(const Caller* caller,uint32_t owner)
{
    const NativeScriptItemProgram* p=nativeScriptItem_Program(caller->author,owner);
    return p && p->bank && &p->bank->declarations==caller->view.program && p->article?p:NULL;
}
static bool item_alive(const Created* item)
{
    return nativeMatchPool_Resolve(item->reference)==item->object &&
        nativeCallbackSource_Alive(item->object,item->generation,NativeCallbackSource_Item) &&
        !((Item*)item->object->user_data)->native_removal_in_progress;
}
static NativeScriptVMStatus guard(const Caller* c,const Created* item)
{
    if(!caller_alive(c)||!item_alive(item))return NativeScriptVM_SourceRetired;
    return nativeScriptItem_Status(item->object);
}
static bool created(Item_GObj* object,Created* item)
{
    if(!object || !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&item->generation) ||
       !nativeMatchPool_Reference(&nativeGObjPool,object,&item->reference))return false;
    item->object=object;return item_alive(item);
}
static void parents(const Caller* c,bool damage_owned,SpawnItem* spawn)
{
    HSD_GObj* object=c->context->source;
    spawn->x4_parent_gobj2=object;
    spawn->x0_parent_gobj=damage_owned?(c->context->source_class==NativeCallbackSource_Fighter?object:((Item*)object->user_data)->owner):NULL;
}
static bool fighter(const Caller* c,Fighter_GObj** object,Fighter** data)
{
    if(c->context->source_class!=NativeCallbackSource_Fighter || !caller_alive(c))return false;
    *object=c->context->source;*data=(*object)->user_data;return true;
}
static bool bone(Fighter* fp,uint32_t raw,HSD_JObj** joint)
{
    if((uint32_t)fp->kind>=Ft_Kind_RuntimeCapacity || raw>=MAX_FT_PARTS || !fp->parts || !ftPartsTable ||
       !ftPartsTable[fp->kind] || raw>=ftPartsTable[fp->kind]->parts_num || !fp->parts[raw].joint)return false;
    *joint=fp->parts[raw].joint;return true;
}
static bool provenance(const Created* newborn,float facing,const Vec3* origin,float value)
{
    Item* item=newborn->object->user_data;
    item->xDD4_itemVar.native_article.attachment_facing=facing;item->xDD4_itemVar.native_article.launch_origin=*origin;
    return nativeScriptItem_SetCreationValue(newborn->object,value);
}
static NativeScriptVMStatus notify(const Caller* c,const Created* item,uint32_t hook)
{
    NativeScriptVMStatus status=guard(c,item);if(status!=NativeScriptVM_Ok)return status;
    NativeScriptValue result;status=nativeScriptItem_RunHook(item->object,hook,NULL,0,&result);
    if(status!=NativeScriptVM_Ok)return status;return guard(c,item);
}
typedef struct SpawnFighterRelation {
    Fighter_GObj* object;
    Fighter* data;
    NativeMatchPoolReference reference;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
    uint32_t generation;
    FighterKind kind;
    bool owned;
} SpawnFighterRelation;
static bool relation_current(const SpawnFighterRelation* relation)
{
    if(!relation->object)return true;
    if(nativeMatchPool_Resolve(relation->reference)!=relation->object ||
       !nativeCallbackSource_Alive(relation->object,relation->generation,NativeCallbackSource_Fighter) ||
       relation->object->user_data!=relation->data || relation->data->kind!=relation->kind)return false;
    if(!relation->owned)return true;
    NativeScriptReceiverView view;
    return nativeScriptFighter_Status(relation->object)==NativeScriptVM_Ok &&
        nativeScriptActors_Lookup(NULL,relation->object,NativeCallbackSource_Fighter,relation->generation,&view) &&
        view.program==relation->view.program && view.storage==relation->view.storage && view.storage && view.storage->live &&
        view.storage->layout->identity==relation->layout && view.storage->parameters && view.storage->parameters->identity==relation->parameters;
}
static bool relation_capture(const Caller* caller,Fighter_GObj* object,bool owned,SpawnFighterRelation* relation)
{
    *relation=(SpawnFighterRelation){.object=object,.owned=owned};if(!object)return true;
    if(!nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&relation->generation) ||
       !nativeMatchPool_Reference(&nativeGObjPool,object,&relation->reference) || relation->reference.lifetime!=caller->reference.lifetime)return false;
    relation->data=object->user_data;relation->kind=relation->data->kind;
    if(owned){
        if(relation->data->kind!=caller->author ||
           !nativeScriptActors_Lookup(NULL,object,NativeCallbackSource_Fighter,relation->generation,&relation->view) ||
           relation->view.program!=caller->view.program || !relation->view.storage || !relation->view.storage->live ||
           !relation->view.storage->layout || !relation->view.storage->parameters)return false;
        relation->layout=relation->view.storage->layout->identity;relation->parameters=relation->view.storage->parameters->identity;
    }
    return relation_current(relation);
}
NativeScriptVMStatus nativeScriptItemSpawn_At(NativeScriptRuntimeContext* context,uint32_t owner,
    const Vec3* position,const Vec3* velocity,float facing,bool origin_at_owner,uint32_t collision_group,
    float creation_value,bool initial_collision,bool gameplay_owned,Item_GObj** output,NativeScriptDiagnostic* d)
{
    Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    const NativeScriptItemProgram* program=selected(&caller,owner);
    if(!output || !program || program->recipe || !vector(position) || !vector(velocity) || !isfinite(facing) || !isfinite(creation_value))
        return error(d,"Invalid source-authored positional Article inputs");
    Fighter_GObj* creator=gameplay_owned?context->source:NULL;Fighter_GObj* damage_owner=creator;
    if(gameplay_owned && context->source_class==NativeCallbackSource_Item){
        Item* parent=((Item_GObj*)context->source)->user_data;
        creator=nativeScriptItem_Creator(context->source);damage_owner=parent->owner;
        if(parent->native_creator && !creator)return NativeScriptVM_SourceRetired;
    }
    SpawnFighterRelation original,damage;
    if(!relation_capture(&caller,creator,true,&original) || !relation_capture(&caller,damage_owner,false,&damage))return NativeScriptVM_SourceRetired;
    if(origin_at_owner && !damage_owner)return error(d,"Owner-center origin requires a live current damage owner");
    Vec3 supplied=*position;SpawnItem spawn={0};spawn.kind=program->kind;
    Item_InitSpawn(&spawn,damage_owner,&supplied,facing);
    spawn.x4_parent_gobj2=gameplay_owned?context->source:NULL;
    spawn.vel=*velocity;memcpy(&spawn.x40,&collision_group,sizeof(collision_group));spawn.x44_flag.b0=initial_collision;
    if(origin_at_owner)it_8026BB68(damage_owner,&spawn.pos);
    if(!caller_alive(&caller) || !relation_current(&original) || !relation_current(&damage))return NativeScriptVM_SourceRetired;
    Item_GObj* object=Item_CreateAuthored(&spawn,caller.author,owner);
    if(!caller_alive(&caller) || !relation_current(&original) || !relation_current(&damage))return NativeScriptVM_SourceRetired;
    if(!object){*output=NULL;return NativeScriptVM_Ok;}
    Created item;if(!created(object,&item))return NativeScriptVM_SourceRetired;
    Item* data=object->user_data;
    data->xDD4_itemVar.native_article.attachment_facing=facing;
    data->xDD4_itemVar.native_article.launch_origin=spawn.pos;
    if(!nativeScriptItem_SetCreationValue(object,creation_value))return error(d,"Owned positional creation value could not be initialized");
    NativeScriptVMStatus status=notify(&caller,&item,NativeScriptItemHook_Created);
    if(status!=NativeScriptVM_Ok)return status;
    if(!relation_current(&original) || !relation_current(&damage))return NativeScriptVM_SourceRetired;
    *output=object;return NativeScriptVM_Ok;
}
static bool child_slot_occupied(const Item* parent,uint32_t slot)
{
    if(slot==UINT32_MAX||parent->native_article_links[slot]==UINT32_MAX||!HSD_GObjPLinkHead)return false;
    uint32_t visited=0,id=parent->native_article_links[slot];
    for(Item_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];object&&visited++<nativeGObjPool.capacity;object=object->next) {
        NativeMatchPoolReference reference;uint32_t generation;
        if(nativeMatchPool_Reference(&nativeGObjPool,object,&reference)&&
           nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation)&&generation==id&&
           ((Item*)object->user_data)->native_article_author_kind==parent->native_article_author_kind)return true;
    }
    return false;
}
NativeScriptVMStatus nativeScriptItemSpawn_Child(NativeScriptRuntimeContext* context,uint32_t owner,
    const Vec3* position,float facing,uint32_t damage_owner_handle,uint32_t collision_group,
    float creation_value,uint32_t backlink,uint32_t parent_slot,Item_GObj** output,NativeScriptDiagnostic* d)
{
    Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    const NativeScriptItemProgram* program=selected(&caller,owner);
    if(context->source_class!=NativeCallbackSource_Item || !output || !program || program->recipe ||
       !vector(position) || !isfinite(facing) || !isfinite(creation_value) || backlink>=4 || (parent_slot!=UINT32_MAX&&parent_slot>=4))
        return error(d,"Invalid source child Article inputs or caller");
    Item_GObj* parent=context->source;Item* source=parent->user_data;
    const uint32_t parent_id=(uint32_t)source->x1C;
    if(child_slot_occupied(source,parent_slot)){*output=NULL;return NativeScriptVM_Ok;}
    uint32_t old_link=parent_slot==UINT32_MAX?UINT32_MAX:source->native_article_links[parent_slot];
    Fighter_GObj* creator=nativeScriptItem_Creator(parent);
    Fighter_GObj* attachment=nativeScriptItem_AttachmentOwner(parent);
    if(source->native_creator && !creator)return NativeScriptVM_SourceRetired;
    Fighter_GObj* damage_owner=damage_owner_handle?ftLib_8008741C(damage_owner_handle):NULL;
    if(damage_owner_handle && !damage_owner)return error(d,"Child damage-owner handle is not a live Fighter");
    SpawnFighterRelation original,control,damage;
    if(!relation_capture(&caller,creator,true,&original) || !relation_capture(&caller,attachment,false,&control) ||
       !relation_capture(&caller,damage_owner,false,&damage))return NativeScriptVM_SourceRetired;
    SpawnItem spawn={0};spawn.kind=program->kind;spawn.pos=spawn.prev_pos=*position;
    spawn.facing_dir=facing;spawn.x0_parent_gobj=damage_owner;spawn.x4_parent_gobj2=parent;
    memcpy(&spawn.x40,&collision_group,sizeof(collision_group));spawn.x44_flag.b0=true;
    if(!caller_alive(&caller) || !relation_current(&original) || !relation_current(&control) || !relation_current(&damage))return NativeScriptVM_SourceRetired;
    Item_GObj* object=Item_CreateAuthored(&spawn,caller.author,owner);
    if(!caller_alive(&caller) || !relation_current(&original) || !relation_current(&control) || !relation_current(&damage))return NativeScriptVM_SourceRetired;
    if(!object){*output=NULL;return NativeScriptVM_Ok;}
    Created item;if(!created(object,&item))return NativeScriptVM_SourceRetired;
    if(nativeScriptItem_Spec(object)!=program)return error(d,"Child constructor selected a different source program");
    if(parent_slot!=UINT32_MAX&&source->native_article_links[parent_slot]!=old_link)
        return error(d,"Linked parent slot changed during native construction");
    Item* target=object->user_data;target->native_article_links[backlink]=parent_id;
    if(parent_slot!=UINT32_MAX)source->native_article_links[parent_slot]=target->x1C;
    target->xDD4_itemVar.native_article.attachment_facing=facing;
    target->xDD4_itemVar.native_article.launch_origin=spawn.pos;
    if(!nativeScriptItem_SetCreationValue(object,creation_value))return error(d,"Owned child creation value could not be initialized");
    NativeScriptVMStatus status=notify(&caller,&item,NativeScriptItemHook_Created);
    if(status!=NativeScriptVM_Ok)return status;
    if(!relation_current(&original) || !relation_current(&control) || !relation_current(&damage))return NativeScriptVM_SourceRetired;
    *output=object;return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeScriptItemSpawn_Create(NativeScriptRuntimeContext* context,uint32_t owner,
    const NativeScriptItemSpawnData* data,Item_GObj** output,NativeScriptDiagnostic* d)
{
    NativeScriptVMStatus status;Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    const NativeScriptItemProgram* p=selected(&caller,owner);
    if(!output || !p || p->recipe || !data || !vector(&data->position) || !vector(&data->previous) || !vector(&data->velocity) ||
       !isfinite(data->facing) || !isfinite(data->creation_value) || data->state < -1 ||
       (data->state>=0 && (uint32_t)data->state>=p->state_count))return error(d,"Invalid source Item creation data or owner");
    SpawnItem spawn={0};spawn.kind=p->kind;spawn.pos=data->position;spawn.prev_pos=data->previous;spawn.vel=data->velocity;
    spawn.facing_dir=data->facing;spawn.x40=data->collision_group;spawn.x44_flag.b0=data->initial_collision;parents(&caller,data->damage_owned,&spawn);
    if(data->origin_at_owner){if(context->source_class!=NativeCallbackSource_Fighter)return error(d,"Owner-center origin requires a Fighter source");it_8026BB68(context->source,&spawn.pos);}
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    Item_GObj* object=Item_CreateAuthored(&spawn,caller.author,owner);Created item;
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    if(!created(object,&item))return error(d,"Native source Item constructor did not yield a live instance");
    if(!provenance(&item,data->facing,&spawn.pos,data->creation_value))return error(d,"Owned creation value could not be initialized");
    if(data->state>=0){Item_80268E5C(object,data->state,ITEM_ANIM_UPDATE);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;}
    status=notify(&caller,&item,NativeScriptItemHook_Created);if(status!=NativeScriptVM_Ok)return status;
    if(data->initial_update){if(spawn.x0_parent_gobj){db_80225DD8(object,spawn.x0_parent_gobj);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;}it_802750F8(object);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;}
    *output=object;return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeScriptItemSpawn_Projectile(NativeScriptRuntimeContext* context,uint32_t owner,
    const Vec3* previous,float facing,int32_t variant,const NativeScriptItemLaunchData* launch,Item_GObj** output,NativeScriptDiagnostic* d)
{
    NativeScriptVMStatus status;Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    const NativeScriptItemProgram* p=selected(&caller,owner);Fighter_GObj* parent;Fighter* fp;
    if(!output || !p || !p->projectile || !fighter(&caller,&parent,&fp) || !vector(previous) || !isfinite(facing))return error(d,"Invalid source projectile recipe or launch source");
    const NativeScriptItemProjectile* recipe=p->projectile;
    float speed=launch?launch->speed:recipe->speed.value,angle=launch?launch->angle:recipe->angle.value,lifetime=recipe->lifetime.value;
    uint32_t state=launch?launch->state:recipe->item_state;
    if(!isfinite(speed)||!isfinite(angle)||!isfinite(lifetime)||lifetime<=0||state>=p->state_count)return error(d,"Invalid source projectile launch values");
    if(launch){angle=nativeScriptMath_WrapAngle(angle);if(!launch->preserve_facing)facing=angle<(float)M_PI_2 || angle>(float)(M_PI*1.5)?1:-1;}
    SpawnItem spawn={0};spawn.kind=p->kind;spawn.prev_pos=*previous;spawn.prev_pos.z=0;it_8026BB68(parent,&spawn.pos);
    spawn.facing_dir=facing;spawn.x40=launch?launch->collision_group:0;spawn.x44_flag.b0=1;parents(&caller,true,&spawn);
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    Item_GObj* object=Item_CreateAuthored(&spawn,caller.author,owner);Created item;
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    if(!created(object,&item))return error(d,"Native projectile constructor did not yield a live instance");
    Item* ip=object->user_data;
    if(recipe->initial_velocity)ip->x40_vel=(Vec3){(launch?1:facing)*(speed*cosf(angle)),speed*sinf(angle),0};
    ip->xDD4_itemVar.native_article.launch_angle=launch?angle:nativeScriptMath_WrapAngle(facing>=0?angle:(float)M_PI-angle);
    ip->xDD4_itemVar.native_article.launch_speed=speed;
    ip->xDD4_itemVar.native_article.launch_origin=spawn.pos;
    it_80275158(object,lifetime);Item_80268E5C(object,(int32_t)state,ITEM_ANIM_UPDATE);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    if(!recipe->auto_lifetime)it_8026B3A8(object);
    if(recipe->appearance_state>=0){it_80273670(object,recipe->appearance_state,(float)variant);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;}
    if(!provenance(&item,fp->facing_dir,&spawn.pos,0))return error(d,"Owned projectile creation value could not be initialized");
    status=notify(&caller,&item,NativeScriptItemHook_Created);if(status!=NativeScriptVM_Ok)return status;
    db_80225DD8(object,parent);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    if(recipe->initial_update){it_802750F8(object);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;}
    if(!recipe->auto_lifetime)it_8026B3A8(object);
    status=notify(&caller,&item,NativeScriptItemHook_Launched);if(status!=NativeScriptVM_Ok)return status;
    *output=object;return NativeScriptVM_Ok;
}
typedef struct HeldSource {
    Caller caller;
    NativeFighterLifetime fighter;
    const NativeSourcePackage* package;
    const NativeScriptItemProgram* program;
    FighterBone* parts;
    HSD_JObj* joint;
    const HSD_ClassInfo* joint_class;
    void* fighter_model;
    u8 fighter_object_kind;
    uint32_t raw,part_count;
} HeldSource;
static NativeScriptVMStatus held_current(const HeldSource* s,NativeScriptDiagnostic* d)
{
    if(!caller_alive(&s->caller))return NativeScriptVM_SourceRetired;
    if(!nativeFighterLifetime_Current(&s->fighter) ||
       nativeScriptBindings_SourceFighterLayout(s->caller.context,d)!=&s->fighter.program->bank->motion_layout ||
       nativeFighterRuntime_SourcePackage(s->fighter.kind)!=s->package ||
       !ftPartsTable || ftPartsTable[s->fighter.kind]!=&s->package->assets.parts ||
       s->package->assets.parts.parts_num!=s->part_count || s->fighter.data->parts!=s->parts ||
       s->parts[s->raw].joint!=s->joint || s->joint->object.parent.class_info!=s->joint_class ||
       s->fighter.object->hsd_obj!=s->fighter_model || s->fighter.object->obj_kind!=s->fighter_object_kind ||
       selected(&s->caller,s->program->owner)!=s->program)
        return error(d,"Held Article source, program or raw bone changed during construction");
    return NativeScriptVM_Ok;
}
static NativeScriptVMStatus held_item_current(const HeldSource* s,const NativeItemLifetime* item,NativeScriptDiagnostic* d)
{
    NativeScriptVMStatus status=held_current(s,d);if(status!=NativeScriptVM_Ok)return status;
    if(nativeMatchPool_Resolve(item->reference)!=item->object)return NativeScriptVM_SourceRetired;
    if(item->object->classifier!=HSD_GOBJ_CLASS_ITEM || item->object->user_data!=item->data ||
       item->object->obj_kind!=item->object_kind || item->object->hsd_obj!=item->model)
        return error(d,"Held Article physical payload or model changed during construction");
    status=nativeScriptItem_Status(item->object);if(status!=NativeScriptVM_Ok)return status;
    if(!nativeItemLifetime_Current(item))return error(d,"Held Article identity changed during construction");
    return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeScriptItemSpawn_HeldAtCenter(NativeScriptRuntimeContext* context,uint32_t owner,
    const Vec3* previous,uint32_t raw,float facing,Item_GObj** output,NativeScriptDiagnostic* d)
{
    HeldSource s={.raw=raw};NativeMatchPoolReference reference;
    if(!output || !context || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference) ||
       ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !vector(previous) || !isfinite(facing))return error(d,"Invalid held Article context or coordinates");
    if(!capture(context,&s.caller))return NativeScriptVM_SourceRetired;
    s.program=selected(&s.caller,owner);
    if(!s.program || s.program->recipe || !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&s.fighter) ||
       !s.fighter.source_owned || nativeScriptBindings_SourceFighterLayout(context,d)!=&s.fighter.program->bank->motion_layout)
        return error(d,"Held Article requires its exact owned Fighter program");
    s.package=nativeFighterRuntime_SourcePackage(s.fighter.kind);s.parts=s.fighter.data->parts;
    uint32_t joints=s.package->assets.facts.joints,omissions=s.package->assets.facts.omission_count;
    s.part_count=s.package->assets.parts.parts_num;
    if(!ftPartsTable || ftPartsTable[s.fighter.kind]!=&s.package->assets.parts || !s.parts ||
       !joints || joints>MAX_FT_PARTS || omissions>MAX_FT_PARTS-joints ||
       s.part_count!=joints+omissions || !s.part_count || s.part_count>MAX_FT_PARTS)
        return error(d,"Held Article raw part table differs from its owned rig");
    // The legacy raw-part contract treats an absent bone as no creation.
    if(raw>=s.part_count || !(s.joint=s.parts[raw].joint)){*output=NULL;return NativeScriptVM_Ok;}
    s.joint_class=s.joint->object.parent.class_info;
    s.fighter_model=s.fighter.object->hsd_obj;s.fighter_object_kind=s.fighter.object->obj_kind;
    SpawnItem spawn={0};spawn.kind=s.program->kind;spawn.prev_pos=*previous;
    it_8026BB68(s.fighter.object,&spawn.pos);spawn.facing_dir=facing;spawn.x44_flag.b0=1;
    spawn.x0_parent_gobj=spawn.x4_parent_gobj2=s.fighter.object;
    NativeScriptVMStatus status=held_current(&s,d);if(status!=NativeScriptVM_Ok)return status;
    Item_GObj* object=Item_CreateAuthored(&spawn,s.caller.author,owner);
    status=held_current(&s,d);if(status!=NativeScriptVM_Ok)return status;
    // The constructor cleans unpublished failures, including Spawned removal.
    if(!object){*output=NULL;return NativeScriptVM_Ok;}
    NativeItemLifetime item;
    if(!nativeItemLifetime_Capture(object,&item) || !item.source_owned || item.program!=s.program ||
       item.bank!=s.fighter.program->bank || item.article!=s.program->article || item.kind!=s.program->kind ||
       item.attributes!=s.program->article->x0_common_attr)
        return error(d,"Held constructor did not yield the selected owned Article");
    Created newborn={.object=object,.reference=item.reference,.generation=item.generation};
    if(!provenance(&newborn,facing,&spawn.pos,0))return error(d,"Owned held creation value could not be initialized");
    NativeScriptValue result;
    status=nativeScriptItem_RunHook(object,NativeScriptItemHook_Created,NULL,0,&result);if(status!=NativeScriptVM_Ok)return status;
    status=held_item_current(&s,&item,d);if(status!=NativeScriptVM_Ok)return status;
    Item_8026AB54(object,s.fighter.object,(Fighter_Part)raw);
    status=held_item_current(&s,&item,d);if(status!=NativeScriptVM_Ok)return status;
    s.fighter.data->item_gobj=object;*output=object;return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeScriptItemSpawn_Attached(NativeScriptRuntimeContext* context,uint32_t owner,
    uint32_t raw,Item_GObj** output,NativeScriptDiagnostic* d)
{
    NativeScriptVMStatus status;Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    const NativeScriptItemProgram* p=selected(&caller,owner);Fighter_GObj* parent;Fighter* fp;HSD_JObj* joint;
    if(!output||!p||p->recipe||!fighter(&caller,&parent,&fp)||!bone(fp,raw,&joint))return error(d,"Invalid source attachment recipe or native bone");
    const NativeScriptItemAttachment defaults={.initial_collision=true,.register_as_held=true};
    const NativeScriptItemAttachment* recipe=p->attachment?p->attachment:&defaults;Vec3 previous={0},position={0};
    if((!recipe->origin_at_owner&&!recipe->origin_at_center)||recipe->previous_at_bone){lb_8000B1CC(joint,NULL,&previous);position=previous;}
    if(recipe->origin_at_center)ftLib_80086990(parent,&position);else if(recipe->origin_at_owner)position=fp->cur_pos;
    if(recipe->flatten_z){position.z=0;previous.z=0;}
    SpawnItem spawn={0};spawn.kind=p->kind;Item_InitSpawn(&spawn,parent,&position,fp->facing_dir);
    if(recipe->previous_at_bone)spawn.prev_pos=previous;spawn.x44_flag.b0=recipe->initial_collision;
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    Item_GObj* object=Item_CreateAuthored(&spawn,caller.author,owner);Created item;
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    if(!created(object,&item))return error(d,"Native attachment constructor did not yield a live instance");
    if(!provenance(&item,fp->facing_dir,&spawn.pos,0))return error(d,"Owned attached creation value could not be initialized");status=notify(&caller,&item,NativeScriptItemHook_Created);if(status!=NativeScriptVM_Ok)return status;
    Item_8026AB54(object,parent,(Fighter_Part)raw);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    if(recipe->register_as_held){db_80225DD8(object,parent);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;fp->x1984_heldItemSpec=object;}
    *output=object;return NativeScriptVM_Ok;
}
static void follow_owner(Item_GObj* object)
{
    Item* item=object->user_data;Fighter_GObj* owner=nativeItem_Creator(object);
    if(!owner || item->owner!=owner){item->on_accessory=NULL;return;}
    Vec3 position=((Fighter*)owner->user_data)->cur_pos;item->pos=position;
    HSD_JObjSetTranslate(object->hsd_obj,&position);
    HSD_JObjSetRotationY(object->hsd_obj,(float)M_PI_2*item->xDD4_itemVar.native_article.attachment_facing);
    HSD_JObjSetRotationZ(object->hsd_obj,0);it_8026B3A8(object);
}
NativeScriptVMStatus nativeScriptItemSpawn_AttachedPresentation(NativeScriptRuntimeContext* context,uint32_t owner,
    uint32_t raw,int32_t variant,int32_t appearance,uint32_t initial,uint32_t attached,float lifetime,
    Item_GObj** output,NativeScriptDiagnostic* d)
{
    NativeScriptVMStatus status;Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    const NativeScriptItemProgram* p=selected(&caller,owner);Fighter_GObj* parent;Fighter* fp;HSD_JObj* joint;
    if(!output||!p||p->recipe||!fighter(&caller,&parent,&fp)||!bone(fp,raw,&joint)||appearance<0||
       !p->article->xC_itemStates||(uint32_t)appearance>=p->article->xC_itemStates->count||
       initial>=p->state_count||attached>=p->state_count||!isfinite(lifetime)||lifetime<=0)
        return error(d,"Invalid source presentation article setup");
    Vec3 previous;lb_8000B1CC(joint,NULL,&previous);SpawnItem spawn={0};spawn.kind=p->kind;
    spawn.prev_pos=previous;spawn.prev_pos.z=0;it_8026BB68(parent,&spawn.pos);
    spawn.facing_dir=fp->facing_dir;spawn.x44_flag.b0=1;parents(&caller,true,&spawn);
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    Item_GObj* object=Item_CreateAuthored(&spawn,caller.author,owner);Created item;
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    if(!created(object,&item))return error(d,"Native presentation constructor did not yield a live instance");
    it_80275158(object,lifetime);Item_80268E5C(object,(int32_t)initial,ITEM_ANIM_UPDATE);
    if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    it_8026B3A8(object);it_8026B3A8(object);it_80273670(object,appearance,(float)variant);
    if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    it_8026B3A8(object);Item_80268E5C(object,(int32_t)attached,ITEM_ANIM_UPDATE);
    if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    Item* ip=object->user_data;ip->xDAC_itcmd_var0=ip->xDB0_itcmd_var1=0;ip->xDB4_itcmd_var2=ip->xDB8_itcmd_var3=0;
    ip->xDC8_word.flags.x19=0;if(!provenance(&item,fp->facing_dir,&spawn.pos,0))return error(d,"Owned presentation creation value could not be initialized");
    ip->on_accessory=follow_owner;ip->xDC8_word.flags.x13=1;ip->owner=parent;
    it_80274740(object);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    it_802756D0(object);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;it_8026B3A8(object);
    status=notify(&caller,&item,NativeScriptItemHook_Created);if(status!=NativeScriptVM_Ok)return status;
    db_80225DD8(object,parent);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    it_802750F8(object);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    it_8026B3A8(object);*output=object;return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeScriptItemSpawn_PullHoldable(NativeScriptRuntimeContext* context,uint32_t owner,
    Item_GObj** output,NativeScriptDiagnostic* d)
{
    NativeScriptVMStatus status;Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    const NativeScriptItemProgram* p=selected(&caller,owner);Fighter_GObj* parent;Fighter* fp;HSD_JObj* joint;
    if(!output||!p||!p->recipe||!fighter(&caller,&parent,&fp)||fp->item_gobj||!fp->ft_data||!fp->ft_data->x8)return error(d,"Invalid source holdable recipe or occupied hand");
    uint32_t raw=fp->ft_data->x8->x10;if(!bone(fp,raw,&joint))return error(d,"Source holdable hand bone is absent");
    Vec3 position;lb_8000B1CC(joint,NULL,&position);SpawnItem spawn={0};spawn.kind=p->kind;Item_InitSpawn(&spawn,parent,&position,fp->facing_dir);
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    Item_GObj* object=Item_CreateAuthored(&spawn,caller.author,owner);Created item;
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    if(!created(object,&item))return error(d,"Native holdable constructor did not yield a live instance");
    status=notify(&caller,&item,NativeScriptItemHook_Created);if(status!=NativeScriptVM_Ok)return status;
    fp->item_gobj=object;Item_8026AB54(object,parent,(Fighter_Part)raw);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    ftpickupitem_80094818(parent,false);if((status=guard(&caller,&item))!=NativeScriptVM_Ok)return status;
    *output=object;return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeScriptItemSpawn_Food(NativeScriptRuntimeContext* context,Item_GObj** output,NativeScriptDiagnostic* d)
{
    Caller caller;if(!capture(context,&caller))return NativeScriptVM_SourceRetired;
    if(!output||context->source_class!=NativeCallbackSource_Item)return error(d,"Food spawn requires a live source Item");
    const Item* source=((HSD_GObj*)context->source)->user_data;SpawnItem spawn={0};spawn.kind=It_Kind_Foods;
    Vec3 position=source->pos;Item_InitSpawn(&spawn,NULL,&position,source->facing_dir);spawn.vel.x=source->x40_vel.x*.35f;spawn.vel.y=1.8f;
    Item_GObj* object=Item_CreateEngine(&spawn);Created item;
    if(!caller_alive(&caller))return NativeScriptVM_SourceRetired;
    if(!created(object,&item))return error(d,"Native food constructor did not yield a live instance");
    *output=object;return NativeScriptVM_Ok;
}
