#include "native_script_binding_fighter_observations.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "fighter.h"
#include "ftwalkcommon.h"
#include "ftcommon.h"
#include <melee/db/db.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/mp/mplib.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonus.h>
#include <melee/pl/plbonuslib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

static bool current(NativeScriptRuntimeContext* c,const NativeFighterLifetime* f,NativeScriptDiagnostic* d)
{
    if(c->source!=f->object || c->source_class!=NativeCallbackSource_Fighter ||
       c->generation!=f->generation || c->scene_epoch!=f->reference.lifetime ||
       !nativeFighterLifetime_Current(f) || !nativeScriptBindings_SourceAlive(c))
        return error(d,"Fighter observation identity or owned storage retired");
    return true;
}

static bool capture(NativeScriptRuntimeContext* c,NativeFighterLifetime* f,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!c || c->source_class!=NativeCallbackSource_Fighter ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference) || reference.lifetime!=c->scene_epoch ||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER || !nativeScriptBindings_SourceAlive(c) ||
       !nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,f))
        return error(d,"Fighter observation requires its live physical Fighter context");
    return current(c,f,d);
}

bool nativeScriptBindingFighterObservations_Read(NativeScriptRuntimeContext* c,uint32_t semantic,
    NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(!output || (semantic!=NativeScriptBinding_MovementSpecialLandingLag && semantic!=NativeScriptBinding_EngineDebugLevel &&
       semantic!=NativeScriptBinding_ActionCategory && semantic!=NativeScriptBinding_ActionCurrentMetadata && semantic!=NativeScriptBinding_FighterIncapacitated &&
       semantic!=NativeScriptBinding_MotionAllowsControllerSync && semantic!=NativeScriptBinding_SpawnHorizontalOffset &&
       semantic!=NativeScriptBinding_GroundMidWalkPoint && semantic!=NativeScriptBinding_FloorIsPlatform && semantic!=NativeScriptBinding_FloorFriction && semantic!=NativeScriptBinding_FacingAnimationDirection && semantic!=NativeScriptBinding_WalkType &&
       semantic!=NativeScriptBinding_ControlsThrowHorizontalThreshold && semantic!=NativeScriptBinding_ControlsThrowUpThreshold &&
       semantic!=NativeScriptBinding_ControlsThrowDownThreshold &&
       semantic!=NativeScriptBinding_FighterPlayerSlot && semantic!=NativeScriptBinding_InputDownPressAge &&
       !(semantic>=NativeScriptBinding_GroundWalkAccelerationMultiplier && semantic<=NativeScriptBinding_AirJumpHorizontalMax) &&
       !(semantic>=NativeScriptBinding_CollisionPositionX && semantic<=NativeScriptBinding_CollisionPositionZ)))
        return error(d,"Unknown Fighter observation or missing output");
    NativeFighterLifetime f;if(!capture(c,&f,d))return false;
    NativeScriptValue value={0};
    if(semantic>=NativeScriptBinding_CollisionPositionX && semantic<=NativeScriptBinding_CollisionPositionZ) {
        const Vec3* position=&f.data->coll_data.cur_pos;
        const float* component=semantic==NativeScriptBinding_CollisionPositionX?&position->x:
            semantic==NativeScriptBinding_CollisionPositionY?&position->y:&position->z;
        value.type=NativeScriptType_F32;memcpy(&value.as.f32,component,sizeof(value.as.f32));
    } else if(semantic==NativeScriptBinding_FighterPlayerSlot) {
        value.type=NativeScriptType_I32;value.as.i64=f.data->player_id;
    } else if(semantic==NativeScriptBinding_InputDownPressAge) {
        value.type=NativeScriptType_I32;value.as.i64=f.data->x682;
    } else if(semantic==NativeScriptBinding_FloorIsPlatform) {
        value.type=NativeScriptType_Bool;value.as.u64=(f.data->coll_data.floor.flags&LINE_FLAG_PLATFORM)!=0;
    } else if(semantic==NativeScriptBinding_FloorFriction) {
        // Native stage material tables have twenty entries; the low byte is
        // the material index and the remaining bits are collision flags.
        uint32_t flags=f.data->coll_data.floor.flags;
        if((uint8_t)flags>=20)return error(d,"Floor material index exceeds its native table");
        value.type=NativeScriptType_F32;value.as.f32=mpLib_800569EC(flags);
    } else if(semantic==NativeScriptBinding_ControlsThrowHorizontalThreshold ||
              semantic==NativeScriptBinding_ControlsThrowUpThreshold || semantic==NativeScriptBinding_ControlsThrowDownThreshold) {
        if(!p_ftCommonData)return error(d,"Shared throw input data is unavailable");
        const float* threshold=semantic==NativeScriptBinding_ControlsThrowHorizontalThreshold?&p_ftCommonData->x98:
            semantic==NativeScriptBinding_ControlsThrowUpThreshold?&p_ftCommonData->attackhi3_stick_threshold_y:&p_ftCommonData->xB0;
        value.type=NativeScriptType_F32;memcpy(&value.as.f32,threshold,sizeof(value.as.f32));
    } else if(semantic==NativeScriptBinding_WalkType) {
        if(!p_ftCommonData || !isfinite(f.data->gr_vel) || !isfinite(f.data->mv.co.walk.accel_mul) ||
           !isfinite(f.data->co_attrs.walk_max_vel) || !isfinite(p_ftCommonData->walk_fast_stick_threshold) ||
           !isfinite(p_ftCommonData->walk_middle_animation_stick_threshold))
            return error(d,"Walk classification requires finite velocity and native thresholds");
        value.type=NativeScriptType_I32;value.as.i64=ftWalkCommon_GetWalkType(f.object);
    } else if(semantic==NativeScriptBinding_FacingAnimationDirection) {
        value.type=NativeScriptType_F32;
        memcpy(&value.as.f32,&f.data->facing_dir1,sizeof(value.as.f32));
    } else if(semantic==NativeScriptBinding_MovementSpecialLandingLag) {
        value.type=NativeScriptType_F32;
        memcpy(&value.as.f32,&f.data->x2EC,sizeof(value.as.f32));
    } else if(semantic==NativeScriptBinding_ActionCurrentMetadata) {
        // Invert decode_action_metadata in ft_0892.c, including counters which
        // the engine can update after entering a motion. Never copy the host
        // union's bytes: its bitfield allocation is not the console convention.
        const union Struct2070* m=&f.data->x2070;
        uint32_t bits=((uint32_t)(uint8_t)m->x2070<<24)|
            ((uint32_t)m->x2071_b0_3<<20)|((uint32_t)m->x2071_b4<<19)|
            ((uint32_t)m->x2071_b5<<18)|((uint32_t)m->x2071_b6<<17)|
            ((uint32_t)m->x2071_b7<<16)|((uint32_t)m->x2072_b0<<15)|
            ((uint32_t)m->x2072_b1<<14)|((uint32_t)m->x2072_b2<<13)|
            ((uint32_t)m->count_x1A4<<12)|((uint32_t)m->count_thrown_items<<11)|
            ((uint32_t)m->count_aerials<<10)|((uint32_t)m->count_x1A0<<9)|
            ((uint32_t)m->count_specials<<8)|m->x2073;
        int32_t signed_bits;memcpy(&signed_bits,&bits,sizeof(signed_bits));
        value.type=NativeScriptType_I32;value.as.i64=signed_bits;
    } else if(semantic==NativeScriptBinding_ActionCategory) {
        // Semantic native field: canonical metadata word bit positions do not
        // describe the host compiler's Struct2070 bitfield memory layout.
        value.type=NativeScriptType_I32;value.as.i64=f.data->x2070.x2071_b0_3;
    } else if(semantic==NativeScriptBinding_FighterIncapacitated) {
        value.type=NativeScriptType_Bool;value.as.u64=f.data->x221F_b3!=0;
    } else if(semantic==NativeScriptBinding_MotionAllowsControllerSync) {
        value.type=NativeScriptType_Bool;value.as.u64=f.data->x2225_b3!=0;
    } else if(semantic==NativeScriptBinding_SpawnHorizontalOffset) {
        value.type=NativeScriptType_F32;
        memcpy(&value.as.f32,&f.data->x40,sizeof(value.as.f32));
    } else if(semantic>=NativeScriptBinding_GroundWalkAccelerationMultiplier && semantic<=NativeScriptBinding_AirJumpHorizontalMax) {
        value.type=NativeScriptType_F32;
        switch(semantic) {
        case NativeScriptBinding_GroundWalkAccelerationMultiplier:value.as.f32=f.data->co_attrs.walk_accel_mul;break;
        case NativeScriptBinding_GroundWalkMaxSpeed:value.as.f32=f.data->co_attrs.walk_max_vel;break;
        case NativeScriptBinding_AirGroundJumpMomentumMultiplier:value.as.f32=f.data->co_attrs.ground_to_air_jump_momentum_multiplier;break;
        case NativeScriptBinding_AirJumpHorizontalMax:value.as.f32=f.data->co_attrs.jump_h_max_velocity;break;
        default:return error(d,"Unknown locomotion attribute");
        }
    } else if(semantic==NativeScriptBinding_GroundMidWalkPoint) {
        value.type=NativeScriptType_F32;
        memcpy(&value.as.f32,&f.data->co_attrs.mid_walk_point,sizeof(value.as.f32));
    } else {
        _Static_assert(sizeof(DbLevel)==sizeof(int32_t),"Native debug level must be a 32-bit value");
        int32_t level;memcpy(&level,&DbLevel,sizeof(level));
        value.type=NativeScriptType_I32;value.as.i64=level;
    }
    if(!current(c,&f,d))return false;
    *output=value;return true;
}

bool nativeScriptBindingFighterObservations_Write(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* input,NativeScriptDiagnostic* d)
{
    if(semantic!=NativeScriptBinding_SpawnHorizontalOffset || !input || input->type!=NativeScriptType_F32)
        return error(d,"Spawn horizontal offset requires an F32 value");
    NativeFighterLifetime f;if(!capture(c,&f,d))return false;
    memcpy(&f.data->x40,&input->as.f32,sizeof(f.data->x40));
    return current(c,&f,d);
}

bool nativeScriptBindingFighterObservations_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    bool query=semantic==NativeScriptBinding_StatsActionCount;
    if(!output || count!=(query?2u:0u) || (query && !arguments) ||
       (!query && semantic!=NativeScriptBinding_StatsCountAsThrownItem &&
        semantic!=NativeScriptBinding_StatsRefreshAttack && semantic!=NativeScriptBinding_StatsRecordAttack &&
        semantic!=NativeScriptBinding_StatsNotifyTaunt))
        return error(d,"Invalid action statistics operation or argument count");
    uint32_t selectors[2]={0};
    if(query)for(unsigned i=0;i<2;++i) {
        const NativeScriptValue* a=&arguments[i];
        uint64_t bits;
        if(a->type==NativeScriptType_I32 || a->type==NativeScriptType_I64) {
            if(a->as.i64<0 || (a->type==NativeScriptType_I32 && a->as.i64>INT32_MAX))
                return error(d,"Action statistics selector is outside its declared integral width");
            bits=(uint64_t)a->as.i64;
        } else if(a->type==NativeScriptType_U32 || a->type==NativeScriptType_U64)bits=a->as.u64;
        else return error(d,"Action statistics selectors require integral values");
        if(bits>UINT32_MAX || (i==0 && bits>=Gm_Player_NumMax))
            return error(d,"Action statistics selector exceeds native storage");
        selectors[i]=(uint32_t)bits;
    }
    NativeFighterLifetime f;if(!capture(c,&f,d))return false;
    NativeScriptValue value={.type=NativeScriptType_Void};
    if(query) {
        value.type=NativeScriptType_U32;
        value.as.u64=pl_800386D8(Player_GetActionStats(selectors[0]),selectors[1]);
    } else if(semantic==NativeScriptBinding_StatsNotifyTaunt) {
        if(f.data->player_id>=Gm_Player_NumMax)return error(d,"Taunt notification has an invalid player slot");
        pl_80040120(f.data->player_id,f.data->x221F_b4);
    } else if(semantic==NativeScriptBinding_StatsRefreshAttack)ft_80089824(f.object);
    else if(semantic==NativeScriptBinding_StatsRecordAttack)ft_800892A0(f.object);
    else f.data->x2070.count_thrown_items=true;
    if(!current(c,&f,d))return false;
    *output=value;return true;
}
