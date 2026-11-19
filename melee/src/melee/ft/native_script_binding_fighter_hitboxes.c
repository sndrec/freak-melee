#include "native_script_binding_fighter_hitboxes.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "fighter.h"
#include "ftcoll.h"
#include "ftchangeparam.h"
#include "ft_0DF0.h"
#include <melee/db/db.h>
#include <melee/lb/lbcollision.h>
#include <melee/gm/forward.h>
#include <melee/pl/player.h>
#include <melee/pl/types.h>
#include <math.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool index(const NativeScriptValue* value,uint32_t* slot,NativeScriptDiagnostic* d)
{
    uint64_t number;
    switch(value->type) {
    case NativeScriptType_Bool:case NativeScriptType_U32:case NativeScriptType_U64:number=value->as.u64;break;
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(value->as.i64<0)return error(d,"Fighter hitbox index must address one of four slots");
        number=(uint64_t)value->as.i64;break;
    default:return error(d,"Fighter hitbox index must be integral");
    }
    if(number>=4)return error(d,"Fighter hitbox index must address one of four slots");
    *slot=(uint32_t)number;return true;
}

static bool capture(NativeScriptRuntimeContext* c,NativeFighterLifetime* source,
    NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!c||c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference)||reference.lifetime!=c->scene_epoch||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !((HSD_GObj*)c->source)->user_data||!nativeScriptBindings_SourceAlive(c))
        return error(d,"Fighter hitbox operation requires its retained physical source");
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,source)||
       !source->source_owned||source->generation!=c->generation||source->reference.lifetime!=c->scene_epoch||
       nativeScriptBindings_SourceFighterLayout(c,d)!=&source->program->bank->motion_layout)
        return error(d,"Fighter hitbox source has retired or changed owned data/program");
    return true;
}

static float* coordinate(Fighter* fighter,uint32_t slot,uint32_t semantic)
{
    HitCapsule* hit=&fighter->x914[slot];
    NativeHitboxLifetime* lifetime = &fighter->native_hitbox_lifetimes[slot];
    Vec3* previous = lifetime->capsule ? &lifetime->previous_start : &hit->x58;
    switch(semantic) {
    case NativeScriptBinding_HitboxCurrentX:return &hit->x4C.x;
    case NativeScriptBinding_HitboxCurrentY:return &hit->x4C.y;
    case NativeScriptBinding_HitboxCurrentZ:return &hit->x4C.z;
    case NativeScriptBinding_HitboxPreviousX:return &previous->x;
    case NativeScriptBinding_HitboxPreviousY:return &previous->y;
    case NativeScriptBinding_HitboxPreviousZ:return &previous->z;
    default:return NULL;
    }
}

bool nativeScriptBindingFighterHitboxes_Read(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out||!selectors||count!=1)return error(d,"Fighter hitbox coordinate requires one selector and output");
    uint32_t slot;if(!index(selectors,&slot,d))return false;
    NativeFighterLifetime source;if(!capture(c,&source,d))return false;
    const float* field=coordinate(source.data,slot,semantic);
    if(!field)return error(d,"Unknown Fighter hitbox coordinate");
    NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=*field};
    if(!nativeFighterLifetime_Current(&source))return error(d,"Fighter hitbox source retired before coordinate publication");
    *out=value;return true;
}

bool nativeScriptBindingFighterHitboxes_Write(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* input,NativeScriptDiagnostic* d)
{
    if(!selectors||count!=1||!input||!nativeScriptExpression_Assignable(NativeScriptType_F32,input->type))
        return error(d,"Fighter hitbox coordinate requires one selector and a numeric value");
    uint32_t slot;if(!index(selectors,&slot,d))return false;
    NativeScriptValue value;if(!nativeScriptValue_Convert(input,NativeScriptType_F32,&value,d))return false;
    NativeFighterLifetime source;if(!capture(c,&source,d))return false;
    float* field=coordinate(source.data,slot,semantic);
    if(!field)return error(d,"Unknown Fighter hitbox coordinate");
    // These are native capsule history fields, not geometry operations. Keep
    // scalar assignments exact; authored code owns any history initialization.
    *field=value.as.f32;
    return nativeFighterLifetime_Current(&source)||error(d,"Fighter hitbox source changed during coordinate write");
}

bool nativeScriptBindingFighterHitboxes_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool state=semantic==NativeScriptBinding_HitboxState;
    bool damage=semantic==NativeScriptBinding_HitboxDamage;
    bool group=semantic==NativeScriptBinding_HitboxGroup;
    bool set_group=semantic==NativeScriptBinding_HitboxSetGroup;
    bool position=semantic==NativeScriptBinding_HitboxSetWorldPosition;
    bool enabled=semantic==NativeScriptBinding_HitboxSetEnabled;
    bool victims=semantic==NativeScriptBinding_HitboxClearVictims;
    bool all=semantic==NativeScriptBinding_HitboxClearAll;
    bool target = semantic == NativeScriptBinding_HitboxLaunchTarget;
    bool follow = semantic == NativeScriptBinding_HitboxLaunchFollow;
    if(!out||(!args&&!all)||(!state&&!damage&&!group&&!set_group&&!position&&!enabled&&!victims&&!all&&!target&&!follow&&semantic!=NativeScriptBinding_HitboxSetDamage)||
       count!=(all?0u:(state||damage||group||victims)?1u:follow?5u:(position||target)?4u:2u)||
       (enabled&&(args[1].type!=NativeScriptType_Bool||args[1].as.u64>1)))
        return error(d,"Fighter hitbox operation has an invalid argument shape");
    uint32_t slot=UINT32_MAX;
    if(enabled||victims) {
        if(!nativeScriptExpression_IsIntegral(args[0].type))return error(d,"Fighter hitbox index must be integral");
        if((args[0].type==NativeScriptType_Bool&&args[0].as.u64>1)||
           (args[0].type==NativeScriptType_U32&&args[0].as.u64>UINT32_MAX)||
           (args[0].type==NativeScriptType_I32&&(args[0].as.i64<INT32_MIN||args[0].as.i64>INT32_MAX)))
            return error(d,"Fighter hitbox index exceeds its declared scalar width");
        bool signed_value=args[0].type==NativeScriptType_I32||args[0].type==NativeScriptType_I64;
        uint64_t number=signed_value?(uint64_t)args[0].as.i64:args[0].as.u64;
        // These two authored mutations have native guarded-index no-op semantics.
        // Keep the original width: an invalid high index must never alias a slot.
        slot=number<4?(uint32_t)number:UINT32_MAX;
    } else if(!all&&!index(&args[0],&slot,d))return false;
    NativeFighterLifetime source;
    if(!capture(c,&source,d))return false;
    Fighter* f=source.data;
    NativeScriptValue result={.type=state?NativeScriptType_I32:damage?NativeScriptType_F32:group?NativeScriptType_U32:NativeScriptType_Void};
    if(all)ftColl_8007AFF8(source.object);
    else if(enabled) {
        if(slot!=UINT32_MAX) {
            if(args[1].as.u64) {
                lbColl_80008434(&f->x914[slot]);
                // Maintain ordinary transition cleanup, including the authored
                // correction for stock Ness's delayed-rehit yo-yo glitch.
                f->x2219_b3=true;
            } else lbColl_80008428(&f->x914[slot]);
        }
    } else if(victims) {
        if(slot!=UINT32_MAX)lbColl_80008440(&f->x914[slot]);
    } else if(state)result.as.i64=f->x914[slot].state;
    else if(damage)result.as.f32=f->x914[slot].damage;
    else if(group)result.as.u64=f->x914[slot].x4;
    else if (target || follow) {
        float parameters[4];
        for (unsigned i = 1; i < count; ++i) {
            NativeScriptValue value;
            if (!nativeScriptValue_Convert(&args[i], NativeScriptType_F32, &value, d)) {
                return false;
            }
            if (!isfinite(value.as.f32)) {
                return error(d, "Hitbox launch parameters must be finite");
            }
            parameters[i - 1] = value.as.f32;
        }
        if (target && parameters[2] < 1.0f) {
            return error(d, "A hitbox launch target needs at least one travel frame");
        }
        if (follow && (parameters[1] < 0.0f ||
                       parameters[3] < 0.0f || parameters[3] >= 360.0f)) {
            return error(d, "Hitbox launch attraction must be nonnegative and ground angle must be in [0, 360)");
        }
        if (f->x914[slot].state != HitCapsule_Disabled) {
            NativeHitboxLifetime* hit = &f->native_hitbox_lifetimes[slot];
            if (target) {
                hit->launch = (NativeHitboxLaunch) {
                    .mode = NativeHitboxLaunch_Target,
                    .target = { parameters[0], parameters[1], parameters[2], 0 },
                };
            } else {
                hit->launch = (NativeHitboxLaunch) {
                    .mode = NativeHitboxLaunch_Follow,
                    .follow = { parameters[0], parameters[1], parameters[2], parameters[3] },
                };
            }
        }
    }
    else if(set_group){
        if(!nativeScriptExpression_IsIntegral(args[1].type)||
           (args[1].type==NativeScriptType_Bool&&args[1].as.u64>1))
            return error(d,"Fighter hitbox group requires an integral value");
        NativeScriptValue value;if(!nativeScriptValue_Convert(&args[1],NativeScriptType_U32,&value,d))return false;
        // Native group changes write this tag directly. They neither clear
        // contact history nor change the capsule's activation state.
        f->x914[slot].x4=(uint32_t)value.as.u64;
    }
    else if(position) {
        NativeScriptValue values[3];
        for(unsigned i=0;i<3;++i)
            if(!nativeScriptExpression_Assignable(NativeScriptType_F32,args[i+1].type)||
               !nativeScriptValue_Convert(&args[i+1],NativeScriptType_F32,&values[i],d))return false;
        Vec3 point={values[0].as.f32,values[1].as.f32,values[2].as.f32};
        ftColl_8007B8A8(&f->x914[slot],&point);
    }
    else {
        NativeScriptValue damage;
        // Integral casts preserve native U32 narrowing; Float casts reject only
        // undefined conversions. The setter applies scale, smash and staleness.
        if(!nativeScriptValue_Convert(&args[1],NativeScriptType_U32,&damage,d))return false;
        float scaled=(float)(uint32_t)damage.as.u64;
        if(f->x34_scale.y!=1.0f) {
            if(!Fighter_804D6524||f->x34_scale.y==0.0f)
                return error(d,"Fighter hitbox scale has no defined native helper input");
            scaled=ftCo_CalcYScaledKnockback(scaled,f->x34_scale.y,Fighter_804D6524->x4);
        }
        scaled=ftCo_800DEEB8(f,scaled);
        NativeScriptValue checked,scalar={.type=NativeScriptType_F32,.as.f32=scaled};
        // Native unk_count casts through s32, before stale damage reduction.
        // Validate that exact conversion rather than clamping authored damage.
        if(!nativeScriptValue_Convert(&scalar,NativeScriptType_I32,&checked,d))return false;
        if(DbLevel<DbLKind_DebugRom) {
            if(f->player_id>=Gm_Player_NumMax)return error(d,"Fighter stale damage player slot is outside native storage");
            StaleMoveTable* table=Player_GetStaleMoveTableIndexPtr(f->player_id);
            if(!table||table->current_index<0||table->current_index>9||
               (f->x2068_attackID!=1&&!Fighter_804D6548))
                return error(d,"Fighter stale damage table or weights are invalid");
        }
        if(!nativeFighterLifetime_Current(&source))return error(d,"Fighter hitbox source changed before setter");
        ftColl_8007ABD0(&f->x914[slot],(uint32_t)damage.as.u64,source.object);
    }
    if(!nativeFighterLifetime_Current(&source))return error(d,"Fighter hitbox source retired before publication");
    *out=result;return true;
}
