#include "native_script_binding_air_inputs.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "native_script_moves.h"
#include "ftcommon.h"
#include "types.h"
#include "fighter.h"
#include "ft_0CDD.h"
#include "ft_0DF1.h"
#include "kinds/ftCommon/ftCo_AttackAir.h"
#include "kinds/ftCommon/ftCo_ItemThrow.h"
#include "kinds/ftCommon/ftCo_SpecialAir.h"
#include <melee/it/native_item_lifetime.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

static bool current(NativeScriptRuntimeContext* c,const NativeFighterLifetime* source,NativeScriptDiagnostic* d)
{
    if(c->source!=source->object || c->source_class!=NativeCallbackSource_Fighter ||
       c->generation!=source->generation || c->scene_epoch!=source->reference.lifetime ||
       !nativeFighterLifetime_Current(source) || !nativeScriptBindings_SourceAlive(c) ||
       nativeScriptBindings_SourceFighterLayout(c,d)!=&source->program->bank->motion_layout)
        return error(d,"Air input source, owned storage or imported motion layout retired");
    return true;
}

static bool motions(const NativeFighterLifetime* source,const uint16_t* ids,uint32_t count,NativeScriptDiagnostic* d)
{
    for(uint32_t i=0;i<count;++i)
        if(!nativeScriptMoves_Contains(&source->program->bank->motion_layout,ids[i]))
            return error(d,"Air input common motion is outside its published layout");
    return true;
}

// Retirement ends an Item continuation, but does not undo a handled input.
// A recycled physical slot is never inspected or adopted here.
static bool item_completed(const NativeItemLifetime* s)
{
    if(nativeMatchPool_Resolve(s->reference)!=s->object)return true;
    if(nativeItemLifetime_Current(s))return true;
    if(!s->source_owned || s->object->classifier!=HSD_GOBJ_CLASS_ITEM ||
       s->object->user_data!=s->data || s->object->obj_kind!=s->object_kind || s->object->hsd_obj!=s->model ||
       !nativeCallbackSource_Alive(s->object,s->generation,NativeCallbackSource_Item) ||
       s->data->entity!=s->object || s->data->kind!=s->kind || s->data->xC4_article_data!=s->article ||
       s->data->xCC_item_attr!=s->attributes || nativeScriptItem_Spec(s->object)!=s->program ||
       s->program->identity!=s->program_identity || s->program->bank!=s->bank || !s->bank->valid ||
       s->view.storage->owner!=s->program->owner || s->view.storage->generation!=s->generation ||
       s->view.storage->layout!=&s->bank->storage_layout || s->view.storage->layout->identity!=s->layout_identity ||
       s->view.storage->parameters!=&s->bank->storage_parameters ||
       s->view.storage->parameters->identity!=s->parameter_identity)return false;
    return nativeScriptItem_Status(s->object)==NativeScriptVM_SourceRetired;
}

bool nativeScriptBindingAirInputs_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    (void)arguments;
    if(semantic<NativeScriptBinding_ControlsAerialAttackMotion || semantic>NativeScriptBinding_ControlsTryAirSpecial)
        return error(d,"Unknown air input operation");
    if(!c || !output || count || c->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(c))return error(d,"Air input requires a live Fighter and zero arguments");
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,&source) ||
       !source.source_owned || !current(c,&source,d))return error(d,"Air input requires its exact owned Fighter program");
    if(!p_ftCommonData)return error(d,"Air input common data is unavailable");
    NativeScriptValue result={.type=NativeScriptType_Void};
    if(semantic==NativeScriptBinding_ControlsAerialAttackMotion) {
        result.type=NativeScriptType_I32;result.as.i64=ftCo_AttackAir_GetMsidFromCStick(source.data);
    } else if(semantic==NativeScriptBinding_InputCStickAttackEdge) {
        result.type=NativeScriptType_Bool;result.as.u64=ftCo_800DF478(source.data);
    } else if(semantic==NativeScriptBinding_ControlsTryAirSpecial) {
        result.type=NativeScriptType_Bool;result.as.u64=ftCo_SpecialAir_CheckInput(source.object);
    } else {
        Item_GObj* held=source.data->item_gobj;
        if(semantic==NativeScriptBinding_ControlsTryAirItemThrow)result.type=NativeScriptType_Bool;
        if(held) {
            NativeItemLifetime item;
            if(!nativeItemLifetime_Capture(held,&item) || item.reference.lifetime!=source.reference.lifetime ||
               !item.attributes)return error(d,"Air input held Item identity or attributes are unavailable");
            if(semantic==NativeScriptBinding_StateAirGun) {
                uint16_t ids[2];uint32_t n=0;
                switch(item.kind) {
                case It_Kind_L_Gun:ids[n++]=ftCo_MS_LGunShootAir;ids[n++]=ftCo_MS_LGunShootAirEmpty;break;
                case It_Kind_F_Flower:ids[n++]=ftCo_MS_FireFlowerShootAir;break;
                case It_Kind_S_Scope:ids[n++]=ftCo_MS_ItemScopeAirStart;ids[n++]=ftCo_MS_ItemScopeAirStartEmpty;break;
                default:break;
                }
                if(!motions(&source,ids,n,d))return false;
                ftCo_800CDDA0(source.object);
            } else {
                static const uint16_t ids[]={0x64,0x65,0x66,0x67,0x70,0x71,0x72,0x73};
                if(!motions(&source,ids,sizeof(ids)/sizeof(ids[0]),d))return false;
                NativeItemLifetime selected={0};
                result.as.u64=ftCo_80095328(source.object,NULL,&selected);
                if(!current(c,&source,d))return false;
                if(selected.object && (!selected.data || !item_completed(&selected)))return error(d,"Air input chosen Item faulted or changed identity");
            }
            if(semantic==NativeScriptBinding_StateAirGun && !item_completed(&item))return error(d,"Air input retained Item faulted or changed identity");
        }
    }
    if(!current(c,&source,d))return false;
    *output=result;return true;
}
