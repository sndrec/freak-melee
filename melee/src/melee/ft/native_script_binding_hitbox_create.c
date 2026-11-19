#include "native_script_binding_hitbox_create.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "native_hitbox_lifetime.h"
#include "native_source_package.h"
#include "fighter.h"
#include "ftcoll.h"
#include "ftchangeparam.h"
#include "ft_0DF0.h"
#include "ftparts.h"
#include <melee/it/native_article_resource.h>
#include <melee/db/db.h>
#include <melee/gm/forward.h>
#include <melee/pl/player.h>
#include <melee/pl/types.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool damage_valid(Fighter* f,float damage,NativeScriptDiagnostic* d)
{
    // Preserve common scale/smash/staleness rules, but reject undefined casts
    // before modifying a capsule's activation or victim history.
    float scaled=damage;
    if(f->x34_scale.y!=1.0f){
        if(!Fighter_804D6524||f->x34_scale.y==0.0f)return error(d,"Hitbox damage scale has no defined helper input");
        scaled=ftCo_CalcYScaledKnockback(scaled,f->x34_scale.y,Fighter_804D6524->x4);
    }
    scaled=ftCo_800DEEB8(f,scaled);
    NativeScriptValue checked,scalar={.type=NativeScriptType_F32,.as.f32=scaled};
    if(!nativeScriptValue_Convert(&scalar,NativeScriptType_I32,&checked,d))return false;
    if(DbLevel<DbLKind_DebugRom){
        if(f->player_id>=Gm_Player_NumMax)return error(d,"Hitbox damage player slot is outside native storage");
        StaleMoveTable* table=Player_GetStaleMoveTableIndexPtr(f->player_id);
        if(!table||table->current_index<0||table->current_index>9||
           (f->x2068_attackID!=1&&!Fighter_804D6548))return error(d,"Hitbox damage stale table or weights are invalid");
    }
    return true;
}
bool nativeScriptBindingHitboxCreate_Invoke(NativeScriptRuntimeContext* c,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!c||!out||!args||count!=2||args[1].type!=NativeScriptType_HitboxDefinition||
       args[1].as.u64>=NATIVE_SCRIPT_HITBOX_DEFINITIONS||!nativeScriptExpression_IsIntegral(args[0].type))
        return error(d,"Hitbox creation requires an integral slot and an immutable definition selector");
    uint64_t slot;
    if(args[0].type==NativeScriptType_Bool&&args[0].as.u64>1)return error(d,"Malformed Bool hitbox slot");
    if(args[0].type==NativeScriptType_I32||args[0].type==NativeScriptType_I64){
        if(args[0].as.i64<0)return error(d,"Hitbox slot must be zero through three");slot=(uint64_t)args[0].as.i64;
    }else slot=args[0].as.u64;
    if(slot>=4)return error(d,"Hitbox slot must be zero through three");
    NativeMatchPoolReference reference;
    if(c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference)||reference.lifetime!=c->scene_epoch||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||!((HSD_GObj*)c->source)->user_data||
       !nativeScriptBindings_SourceAlive(c))return error(d,"Hitbox creation requires its retained Fighter source");
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,&source)||!source.source_owned||
       source.generation!=c->generation||source.reference.lifetime!=c->scene_epoch||
       nativeScriptBindings_SourceFighterLayout(c,d)!=&source.program->bank->motion_layout)
        return error(d,"Hitbox source retired or changed its owned program");
    const NativeScriptCompileBank* bank=source.program->bank;
    if(bank->hitbox_definition_count>NATIVE_SCRIPT_HITBOX_DEFINITIONS||args[1].as.u64>=bank->hitbox_definition_count)
        return error(d,"Hitbox selector is not declared by this Fighter program");
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(source.kind);Fighter* f=source.data;
    if(!package||!package->assets.ready||bank!=&package->compiler.bank||
       f->ft_data!=&package->assets.character.data||!ftPartsTable||
       ftPartsTable[source.kind]!=&package->assets.parts||!f->parts||
       !package->assets.parts.parts_num||package->assets.parts.parts_num>MAX_FT_PARTS)
        return error(d,"Hitbox creation has no matching owned Fighter rig");
    const NativeScriptHitboxDefinition* spec=&bank->hitbox_definitions[args[1].as.u64];
    if(spec->bone<0)return error(d,"Hitbox bone ID must be nonnegative");
    int bone=nativeFighterRig_Resolve(&package->assets.rig,spec->bone);
    NativeScriptValue result={.type=NativeScriptType_Bool,.as.u64=0};HSD_JObj* joint=NULL;
    if(bone<0){*out=result;return true;}
    if((unsigned)bone<package->assets.parts.parts_num)joint=f->parts[bone].joint;
    else if(!nativeArticleResource_ResolveBoundPart(source.object,(unsigned)bone,&joint)){*out=result;return true;}
    if(!joint||(spec->require_throw_owner&&!f->x1064_thrownHitbox.owner)){*out=result;return true;}
    if(!p_ftCommonData)return error(d,"Hitbox angle helper has no common engine parameters");
    if(!damage_valid(f,spec->damage,d))return false;
    if(!nativeFighterLifetime_Current(&source))return error(d,"Hitbox source retired before creation");
    nativeHitboxLifetime_Clear(f, (unsigned)slot);
    NativeHitboxLifetime* lifetime = &f->native_hitbox_lifetimes[slot];
    lifetime->exclude_grabbed = spec->exclude_grabbed;
    lifetime->capsule = spec->capsule;
    lifetime->end_offset = (Vec3){spec->end_x, spec->end_y, spec->end_z};
    HitCapsule* hit=&f->x914[slot];
    if(hit->state==HitCapsule_Disabled||hit->x4!=(uint32_t)spec->group){
        hit->x4=(uint32_t)spec->group;hit->state=HitCapsule_Enabled;f->x2219_b3=true;ftColl_800768A0(f,hit);
    }
    hit->jobj=joint;ftColl_SetHitboxDamage(hit,spec->damage,source.object);
    if(!nativeFighterLifetime_Current(&source))return error(d,"Hitbox source retired during damage initialization");
    hit->scale=spec->radius;hit->b_offset=(Vec3){spec->offset_x,spec->offset_y,spec->offset_z};
    ftColl_8007AC9C(hit,spec->angle,source.object);
    if(!nativeFighterLifetime_Current(&source))return error(d,"Hitbox source retired during angle initialization");
    hit->x24=(uint32_t)spec->growth;hit->x28=(uint32_t)spec->weight_knockback;
    hit->x43_b0=spec->item_interaction;hit->x43_b1=spec->ignore_fighter_scale;
    hit->x40_b0=spec->clank;hit->x40_b1=spec->rebound;hit->x2C=(uint32_t)spec->base_knockback;
    hit->element=(uint32_t)spec->element;hit->x34=spec->shield_damage;
    hit->sfx_severity=spec->sound_severity;hit->sfx_kind=spec->sound_kind;
    hit->x40_b2=spec->aerial;hit->x40_b3=spec->grounded;
    hit->x42_b5=1;hit->x42_b7=1;hit->x41_b4=0;hit->x41_b6=0;hit->x41_b5=0;
    hit->x42_b0=0;hit->x42_b4=0;hit->x41_b7=0;hit->hit_grabbed_victim_only=spec->victim_only;
    hit->x42_b1=1;hit->x42_b2=0;hit->x43_b2=0;result.as.u64=1;*out=result;return true;
}
