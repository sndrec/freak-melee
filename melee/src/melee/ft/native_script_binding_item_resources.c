#include "native_script_binding_item_resources.h"
#include "native_script_compile.h"
#include "native_script_item_program.h"
#include <melee/it/native_article_resource_internal.h>
#include <melee/it/native_article_resource_animation.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_item_lifetime.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define ID(n) NativeScriptItemResource_##n

typedef struct ResourceSource {
    NativeScriptRuntimeContext context;
    NativeItemLifetime item;
    ResourceCapture node;
    ResourceCollection* collection;
    uint8_t* state;
    uint32_t state_capacity;
    uint32_t collection_generation, index;
    bool present, indexed;
} ResourceSource;
static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool current(const ResourceSource* s,const NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{
    if(!c||c->source!=s->context.source||c->source_class!=s->context.source_class||c->generation!=s->context.generation||c->scene_epoch!=s->context.scene_epoch||
       !nativeItemLifetime_Current(&s->item)||s->item.view.storage->state!=s->state||
       s->item.view.storage->capacity!=s->state_capacity)return error(d,"Resource caller, owned program or storage retired before publication");
    return true;
}
static bool integral(const NativeScriptValue* v)
{return v&&(v->type==NativeScriptType_Bool||v->type==NativeScriptType_I32||v->type==NativeScriptType_U32||v->type==NativeScriptType_I64||v->type==NativeScriptType_U64);}
static bool unsigned_value(const NativeScriptValue* v,unsigned* out)
{
    if(!integral(v))return false;
    if((v->type==NativeScriptType_I32||v->type==NativeScriptType_I64)&&v->as.i64<0)return false;
    if(v->as.u64>UINT_MAX)return false;*out=(unsigned)v->as.u64;return true;
}
static bool signed_value(const NativeScriptValue* v,int32_t* out)
{
    if(!integral(v))return false;
    if(v->type==NativeScriptType_I32||v->type==NativeScriptType_I64){if(v->as.i64<INT32_MIN||v->as.i64>INT32_MAX)return false;*out=(int32_t)v->as.i64;}
    else {if(v->as.u64>INT32_MAX)return false;*out=(int32_t)v->as.u64;}return true;
}
static bool capture(NativeScriptRuntimeContext* c,ResourceSource* s,const NativeScriptValue* selectors,uint32_t count,bool indexed,NativeScriptDiagnostic* d)
{
    if(!c||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||!nativeItemLifetime_Capture(c->source,&s->item)||
       !s->item.source_owned||s->item.reference.lifetime!=c->scene_epoch||s->item.generation!=c->generation)
        return error(d,"Resources require a live exact source-owned Item actor");
    s->context=*c;s->present=false;s->indexed=indexed;s->index=UINT_MAX;
    s->state=s->item.view.storage->state;s->state_capacity=s->item.view.storage->capacity;
    if(indexed){if(!selectors||!count||!integral(&selectors[0]))return error(d,"Resource index must be an integral selector");
        // Negative/wide indices are absent, never aliases of low-bit slots.
        unsigned_value(&selectors[0],&s->index);
        s->present=nativeArticleResource_Capture(c->source,s->index,&s->node);
    }else if(count)return error(d,"Collection operation has unexpected resource selectors");
    s->collection=nativeArticleResource_Collection(c->source);s->collection_generation=s->collection?s->collection->generation:0;
    return current(s,c,d);
}
static bool node_current(const ResourceSource* s,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{
    if(!current(s,c,d))return false;
    if(s->present&&!nativeArticleResource_Current(&s->node))return error(d,"Captured resource node or graph retired before publication");
    ResourceCollection* collection=nativeArticleResource_Collection(c->source);
    if(collection!=s->collection||(collection&&collection->generation!=s->collection_generation))return error(d,"Resource collection changed before publication");
    return true;
}
static const NativeScriptItemResourceSelector* recipe(const ResourceSource* s,uint32_t semantic,NativeScriptDiagnostic* d)
{
    const NativeScriptItemProgram* p=s->item.program;
    if(p->resource_selector_count>NATIVE_SCRIPT_ITEM_RESOURCE_SELECTORS||(p->resource_selector_count&&!p->resource_selectors)){error(d,"Resource prepared recipe table is invalid");return NULL;}
    unsigned lo=0,hi=p->resource_selector_count;while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(p->resource_selectors[mid].semantic<semantic)lo=mid+1;else hi=mid;}
    if(lo==p->resource_selector_count||p->resource_selectors[lo].semantic!=semantic){error(d,"Resource recipe is not prepared for this exact source owner");return NULL;}
    return &p->resource_selectors[lo];
}
static bool joint_recipe(const ResourceSource* s,uint32_t semantic,const NativeScriptValue* selectors,unsigned count,const NativeScriptItemResourceSelector** out,NativeScriptDiagnostic* d)
{
    const NativeScriptItemResourceSelector* r=recipe(s,semantic,d);
    if(!r||count!=2||!selectors||selectors[1].type!=NativeScriptType_Text||selectors[1].as.text.offset!=r->path_offset||selectors[1].as.text.length!=r->path_length||r->depth>NATIVE_SCRIPT_ITEM_RESOURCE_PATH_DEPTH||r->channel>15)
        return error(d,"Resource joint selectors differ from their immutable path recipe");
    *out=r;return true;
}
static int int_channel(uint32_t semantic)
{
    switch(semantic){case ID(SurfaceLine):return NativeArticleResource_SurfaceLine;
    case ID(Environment):return NativeArticleResource_EnvironmentFlags;case ID(FloorLine):return NativeArticleResource_FloorLine;
    case ID(LeftWallLine):return NativeArticleResource_LeftWallLine;case ID(RightWallLine):return NativeArticleResource_RightWallLine;
    case ID(CeilingLine):return NativeArticleResource_CeilingLine;default:return -1;}
}
bool nativeScriptBindingItemResources_Read(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out)return error(d,"Resource read output is absent");ResourceSource s;bool indexed=semantic!=ID(Count);
    if(!capture(c,&s,selectors,count,indexed,d))return false;NativeScriptValue value={0};
    if(semantic==ID(Count))value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=nativeArticleResource_Count(c->source)};
    else if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_PROPERTY){const NativeScriptItemResourceSelector* r;
        if(!joint_recipe(&s,semantic,selectors,count,&r,d))return false;
        float result=nativeArticleResource_ReadJoint(c->source,s.index,r->path,r->depth,r->channel);
        value=(r->channel==9||r->channel==10||r->channel==15)?(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=result!=0}:(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=result};
    }else{
        if(count!=1)return error(d,"Resource scalar read has unexpected selectors");int channel=int_channel(semantic);
        if(semantic==ID(Exists))value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=nativeArticleResource_Exists(c->source,s.index)};
        else if(semantic>=ID(FloatFirst)&&semantic<ID(FloatFirst)+NativeArticleResource_FloatCount)value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=nativeArticleResource_ReadFloat(c->source,s.index,(NativeArticleResourceFloat)(semantic-ID(FloatFirst)))};
        else if(channel>=0)value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=nativeArticleResource_ReadInt(c->source,s.index,(NativeArticleResourceInt)channel)};
        else return error(d,"Unsupported resource scalar read");
    }
    if(!node_current(&s,c,d))return false;*out=value;return true;
}
bool nativeScriptBindingItemResources_Write(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!value)return error(d,"Resource write value is absent");ResourceSource s;if(!capture(c,&s,selectors,count,true,d))return false;
    if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_PROPERTY){const NativeScriptItemResourceSelector* r;
        if(!joint_recipe(&s,semantic,selectors,count,&r,d))return false;
        bool boolean=r->channel==10||r->channel==15;
        if(r->channel==9||value->type!=(boolean?NativeScriptType_Bool:NativeScriptType_F32))return error(d,"Resource joint write type/access differs");
        nativeArticleResource_WriteJoint(c->source,s.index,r->path,r->depth,r->channel,boolean?(float)(value->as.u64!=0):value->as.f32);
    }else{
        if(count!=1)return error(d,"Resource scalar write has unexpected selectors");
        if(semantic==ID(SurfaceLine)){if(value->type!=NativeScriptType_I32)return error(d,"Resource surface line write needs Int");nativeArticleResource_WriteInt(c->source,s.index,NativeArticleResource_SurfaceLine,(int32_t)value->as.i64);}
        else if(semantic>=ID(FloatFirst)&&semantic<ID(FloatFirst)+NativeArticleResource_FloorNormalX){if(value->type!=NativeScriptType_F32)return error(d,"Resource scalar write needs Float");nativeArticleResource_WriteFloat(c->source,s.index,(NativeArticleResourceFloat)(semantic-ID(FloatFirst)),value->as.f32);}
        else return error(d,"Resource property is read-only");
    }
    // Missing resource paths retain the native no-op contract. Captured live
    // paths still require exact node/graph retention after any native operation.
    return node_current(&s,c,d);
}
static bool changed_model_current(const ResourceSource* s,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{
    if(!current(s,c,d))return false;
    ResourceCollection* set=nativeArticleResource_Collection(c->source);
    if(set!=s->collection||(set&&set->generation!=s->collection_generation))return error(d,"Resource collection retired during node/model construction");
    if(s->present){ResourceCapture now;if(!nativeArticleResource_Capture(c->source,s->index,&now)||now.collection!=s->node.collection||now.generation!=s->node.generation||now.node!=s->node.node||now.link!=s->node.link||
        now.payload.pool!=s->node.payload.pool||now.payload.lifetime!=s->node.payload.lifetime||now.payload.slot!=s->node.payload.slot||now.payload.generation!=s->node.payload.generation||
        now.object.pool!=s->node.object.pool||now.object.lifetime!=s->node.object.lifetime||now.object.slot!=s->node.object.slot||now.object.generation!=s->node.object.generation)return error(d,"Resource physical node replaced during model construction");}
    return true;
}
static bool matrix_storage(ResourceSource* s,const NativeScriptItemResourceSelector* r,NativeScriptValue* arguments,unsigned count,uint8_t** bytes,NativeScriptDiagnostic* d)
{
    const NativeScriptStorageInstance* storage=s->item.view.storage;const NativeScriptStorageLayout* layout=storage->layout;
    if(count!=1||!arguments||arguments[0].type!=NativeScriptType_I32||arguments[0].as.i64!=r->declaration||r->declaration>=layout->field_count||r->declaration>=s->item.bank->declarations.value_count)return error(d,"Resource matrix declaration token differs");
    const NativeScriptStorageField* field=&layout->fields[r->declaration];const NativeScriptValueDeclaration* declaration=&s->item.bank->declarations.values[r->declaration];
    if(!field->array||field->parameter||field->type!=NativeScriptType_F32||field->count!=12||field->owner!=s->item.program->owner||field->offset>storage->capacity||48>storage->capacity-field->offset||!storage->state||
       declaration->owner!=field->owner||!declaration->array||declaration->parameter||declaration->type!=NativeScriptType_F32||declaration->count!=12||declaration->byte_offset!=field->offset)
        return error(d,"Resource matrix requires its exact owned Float[12] storage layout");
    *bytes=storage->state+field->offset;return true;
}
bool nativeScriptBindingItemResources_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptValue* arguments,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out)return error(d,"Resource call output is absent");bool matrix=(semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_MATRIX;
    bool animation=(semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION;
    bool collection=semantic==ID(Begin)||semantic==ID(Release);ResourceSource s;
    if(!capture(c,&s,selectors,selector_count,!collection,d))return false;
    const NativeScriptItemResourceSelector* prepared=NULL;
    if(matrix){prepared=recipe(&s,semantic,d);if(!prepared)return false;semantic=ID(Resources)+prepared->operation;}
    if(animation){if(!joint_recipe(&s,semantic,selectors,selector_count,&prepared,d))return false;semantic=ID(Resources)+prepared->operation;if(semantic<ID(AddAnimation)||semantic>ID(SetAnimationRate))return error(d,"Resource animation recipe operation differs");}
    const NativeScriptBindingDescriptor* spec=nativeScriptBindingItemResources_Descriptor(semantic);
    if(!spec||spec->kind!=NativeScriptBinding_Intrinsic||spec->result_type==NativeScriptType_Namespace||count!=spec->argument_count||(!arguments&&count)||(!collection&&selector_count!=(animation?2u:1u)))return error(d,"Resource call semantic/arity/selectors differ");
    for(unsigned i=0;i<count;++i){const NativeScriptBindingArgument* formal=&spec->arguments[i];if(formal->numeric_any){if(!integral(&arguments[i]))return error(d,"Resource native integral argument differs");}
        else if(arguments[i].type!=formal->type)return error(d,"Resource native formal argument differs");}
    bool result=false,changed=false;unsigned a=0,b=0;int32_t signed_arg=0;Vec3 vector={0},position={0};Mtx matrix_value={{0}};uint8_t* matrix_bytes=NULL;
    if(matrix&&!matrix_storage(&s,prepared,arguments,count,&matrix_bytes,d))return false;
    switch(semantic){
    case ID(AddAnimation):
        if(arguments[0].as.i64<0||(uint64_t)arguments[0].as.i64!=prepared->animation)return error(d,"Resource animation argument differs from its prepared bundle");
        result=nativeArticleResource_AddAnimation(c->source,s.index,prepared->path,prepared->depth,prepared->animation);break;
    case ID(RequestAnimation):result=nativeArticleResource_RequestAnimation(c->source,s.index,prepared->path,prepared->depth,arguments[0].as.f32);break;
    case ID(AdvanceAnimation):result=nativeArticleResource_AdvanceAnimation(c->source,s.index,prepared->path,prepared->depth);break;
    case ID(RemoveAnimation):result=nativeArticleResource_RemoveAnimation(c->source,s.index,prepared->path,prepared->depth);break;
    case ID(SetAnimationRate):result=nativeArticleResource_SetAnimationRate(c->source,s.index,prepared->path,prepared->depth,arguments[0].as.f32);break;
    case ID(Begin):if(unsigned_value(&arguments[0],&a))result=nativeArticleResource_Begin(c->source,a);break;
    case ID(Release):nativeArticleResource_Release(c->source);break;
    case ID(Create):case ID(CreateAt):{
        if(!unsigned_value(&arguments[0],&a)||!unsigned_value(&arguments[2],&b))break;
        NativeArticleResourceInit init={.attachment_part=a,.radius=arguments[1].as.f32,.collision_category=b};
        if(count==6)init.position=(Vec3){arguments[3].as.f32,arguments[4].as.f32,arguments[5].as.f32};
        result=nativeArticleResource_CreateNode(c->source,s.index,&init);changed=result;break;}
    case ID(LoadModel):if(arguments[0].as.i64<0||(uint64_t)arguments[0].as.i64>=s.item.program->model_root_count)return error(d,"Resource root ordinal is outside its immutable owner table");result=nativeArticleResource_LoadModel(c->source,s.index,(uint32_t)arguments[0].as.i64);changed=result;break;
    case ID(SetDraw):result=nativeArticleResource_SetDraw(c->source,s.index,arguments[0].as.u64!=0);break;
    case ID(BindPart):if(unsigned_value(&arguments[0],&a))result=nativeArticleResource_BindExternalPart(c->source,s.index,a);break;
    case ID(CaptureAnchor):result=nativeArticleResource_SetCaptureAnchor(c->source,s.index);break;
    case ID(SweepStage):result=nativeArticleResource_SweepStage(c->source,s.index);break;
    case ID(SweepPoint):result=nativeArticleResource_SweepPoint(c->source,s.index);break;
    case ID(FollowFloor):result=nativeArticleResource_FollowFloor(c->source,s.index);break;
    case ID(LineSpeed):if(signed_value(&arguments[0],&signed_arg))result=nativeArticleResource_ReadLineSpeed(c->source,s.index,signed_arg,&vector);break;
    case ID(AttachmentPoint):position=(Vec3){arguments[0].as.f32,arguments[1].as.f32,arguments[2].as.f32};result=nativeArticleResource_ReadAttachmentPoint(c->source,s.index,&position,&vector);break;
    case ID(AttachmentMatrix):result=nativeArticleResource_ReadAttachmentMatrix(c->source,s.index,matrix_value);break;
    case ID(WorldMatrix):result=nativeArticleResource_ReadWorldMatrix(c->source,s.index,matrix_value);break;
    case ID(SetWorldMatrix):memcpy(matrix_value,matrix_bytes,sizeof(matrix_value));result=nativeArticleResource_SetWorldMatrix(c->source,s.index,matrix_value);break;
    case ID(EffectPosition):case ID(EffectFacing):
        if(!signed_value(&arguments[0],&signed_arg))return error(d,"Resource effect ID does not fit native signed width");
        position=(Vec3){arguments[1].as.f32,arguments[2].as.f32,arguments[3].as.f32};
        result=semantic==ID(EffectPosition)?nativeArticleResource_SpawnEffectAtPosition(c->source,s.index,signed_arg,&position):nativeArticleResource_SpawnEffectFacingAtPosition(c->source,s.index,signed_arg,&position,arguments[4].as.f32);break;
    default:return error(d,"Unsupported resource invocation");
    }
    if(collection){if(!current(&s,c,d))return false;}
    else if(changed){if(!changed_model_current(&s,c,d))return false;}
    else if(!node_current(&s,c,d))return false;
    // Healthy native false uses the old zero-initialized query result. No
    // output or state-array byte is published after retirement or substitution.
    if(semantic==ID(LineSpeed)||semantic==ID(AttachmentPoint)){unsigned first=semantic==ID(LineSpeed)?1:3;arguments[first]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=vector.x};arguments[first+1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=vector.y};arguments[first+2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=vector.z};}
    if(semantic==ID(AttachmentMatrix)||semantic==ID(WorldMatrix))memcpy(matrix_bytes,matrix_value,sizeof(matrix_value));
    *out=spec->result_type==NativeScriptType_Void?(NativeScriptValue){.type=NativeScriptType_Void}:(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=result};return true;
}
