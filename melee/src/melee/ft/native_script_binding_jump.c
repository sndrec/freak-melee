#include "native_script_binding_jump.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include "native_fighter_jump.h"
#include "native_source_package.h"
#include "fighter.h"
#include "ftparts.h"
#include "ftchangeparam.h"
#include "ftwalljump.h"
#include "kinds/ftCommon/ftCo_JumpAerial.h"
#include "kinds/ftCommon/ftCo_Jump.h"
#include "kinds/ftCommon/ftCo_KneeBend.h"
#include "kinds/ftCommon/forward.h"
#include "types.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* text)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",text);return false;}
static bool present(NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{
    return (context && context->source_class==NativeCallbackSource_Fighter &&
        nativeScriptBindings_SourceAlive(context)) || error(diagnostic,"Jump source is unavailable or retired.");
}

static bool capture(NativeScriptRuntimeContext* context,NativeFighterLifetime* saved,
    NativeScriptDiagnostic* diagnostic)
{
    NativeMatchPoolReference reference;
    if(!context||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference)||
       reference.lifetime!=context->scene_epoch||
       ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !((HSD_GObj*)context->source)->user_data||!nativeScriptBindings_SourceAlive(context)||
       !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,saved)||
       !saved->source_owned||saved->generation!=context->generation||
       saved->reference.lifetime!=context->scene_epoch||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved->program->bank->motion_layout||
       !nativeFighterLifetime_Current(saved))
        return error(diagnostic,"Jump update requires its exact live owned Fighter.");
    return true;
}

bool nativeScriptBindingJump_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_MotionFallAnimation) {
        NativeFighterLifetime saved;if(!result||!capture(context,&saved,diagnostic))return false;
        Fighter* f=saved.data;
        *result=(NativeScriptValue){.type=NativeScriptType_I32,
            .as.i64=(f->motion_id==ftCo_MS_Fall||f->motion_id==ftCo_MS_FallAerial)?f->mv.co.fall.smid:-1};
        return true;
    }
    if(!result || !present(context,diagnostic))return false;
    Fighter* fighter=((HSD_GObj*)context->source)->user_data;
    NativeScriptValue value={.type=NativeScriptType_I32};
    if(semantic==NativeScriptBinding_JumpsUsed)value.as.i64=fighter->x1968_jumpsUsed;
    else if(semantic==NativeScriptBinding_JumpsMax)value.as.i64=fighter->co_attrs.max_jumps;
    else if(semantic==NativeScriptBinding_CollisionBasicLandingVelocityThreshold) {
        if(!p_ftCommonData || !Fighter_804D6524)return error(diagnostic,"Common landing-velocity scale data is unavailable.");
        value.type=NativeScriptType_F32;value.as.f32=ftCo_800D0EC8(fighter);
    } else return error(diagnostic,"Unknown generic jump property.");
    *result=value;return true;
}

bool nativeScriptBindingJump_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_MotionFallAnimation) {
        if(!value||value->type!=NativeScriptType_I32||value->as.i64<INT32_MIN||value->as.i64>INT32_MAX)
            return error(diagnostic,"Fall animation selector requires a native signed Int.");
        NativeFighterLifetime saved;if(!capture(context,&saved,diagnostic))return false;
        if(saved.data->motion_id==ftCo_MS_Fall||saved.data->motion_id==ftCo_MS_FallAerial)
            saved.data->mv.co.fall.smid=(FtMotionId)value->as.i64;
        return true;
    }
    if(!value || !present(context,diagnostic))return false;
    if(semantic!=NativeScriptBinding_JumpsUsed || value->type!=NativeScriptType_I32 ||
       value->as.i64<0 || value->as.i64>UINT8_MAX)
        return error(diagnostic,"Used jumps must fit the native unsigned eight-bit counter.");
    ((Fighter*)((HSD_GObj*)context->source)->user_data)->x1968_jumpsUsed=(u8)value->as.i64;
    return true;
}

bool nativeScriptBindingJump_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_JumpsConfigureMulti || semantic==NativeScriptBinding_JumpsMultiVerticalSpeed) {
        bool configure=semantic==NativeScriptBinding_JumpsConfigureMulti;
        if(!result || !arguments || count!=(configure?13:1))
            return error(diagnostic,"Multi-jump setup takes thirteen tuning values; speed takes one Int index.");
        for(unsigned i=0;i<count;++i) {
            bool integral=!configure || i==0 || i>=10;
            if(integral ? (arguments[i].type!=NativeScriptType_I32 || arguments[i].as.i64<INT32_MIN || arguments[i].as.i64>INT32_MAX):
               (arguments[i].type!=NativeScriptType_F32 || !isfinite(arguments[i].as.f32)))
                return error(diagnostic,"Multi-jump tuning requires native Int fields and finite Float impulses.");
        }
        NativeFighterLifetime saved;if(!capture(context,&saved,diagnostic))return false;
        Fighter* fighter=saved.data;
        // The native common jump/scaling code consumes this instance-owned
        // table. Never read or update a donor DAT's extended attributes.
        if(!fighter->can_multijump || !fighter->x2D0 || fighter->x2D0!=fighter->dat_attrs) {
            *result=configure?(NativeScriptValue){.type=NativeScriptType_Bool}:
                (NativeScriptValue){.type=NativeScriptType_F32};return true;
        }
        if(!fighter->dat_attrs_backup || fighter->dat_attrs!=fighter->dat_attrs_backup)
            return error(diagnostic,"Multi-jump tuning requires an enabled instance-owned jump table.");
        void* table=fighter->x2D0;
        NativeScriptValue value;
        if(configure) {
            NativeFighterMultiJumpStats stats={.turn_frames=(int)arguments[0].as.i64,
                .reverse_threshold=arguments[1].as.f32,.horizontal_impulse=arguments[2].as.f32,
                .acceleration_multiplier=arguments[3].as.f32,.speed_multiplier=arguments[4].as.f32,
                .count=(int)arguments[10].as.i64,.first_motion=(int)arguments[11].as.i64,
                .helmet_first_motion=(int)arguments[12].as.i64};
            for(unsigned i=0;i<5;++i)stats.vertical_impulses[i]=arguments[5+i].as.f32;
            value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=nativeFighterJump_ConfigureMulti(fighter,&stats)};
        } else value=(NativeScriptValue){.type=NativeScriptType_F32,
            .as.f32=nativeFighterJump_MultiVerticalSpeed(fighter,(int)arguments[0].as.i64)};
        if(!nativeFighterLifetime_Current(&saved) || context->source!=saved.object ||
           context->generation!=saved.generation || context->scene_epoch!=saved.reference.lifetime ||
           fighter->x2D0!=table || fighter->dat_attrs!=table || fighter->dat_attrs_backup!=table ||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout)
            return error(diagnostic,"Multi-jump source or table retired before publication.");
        *result=value;return true;
    }
    if(semantic==NativeScriptBinding_JumpPrepare || semantic==NativeScriptBinding_JumpCheckShortHop ||
       semantic==NativeScriptBinding_JumpLaunch || semantic==NativeScriptBinding_PhysicsJumpMotion){
        bool prepare=semantic==NativeScriptBinding_JumpPrepare,launch=semantic==NativeScriptBinding_JumpLaunch;
        if(!result || count!=((prepare||launch)?2:0) ||
           (prepare && (!arguments || arguments[0].type!=NativeScriptType_I32 ||
            arguments[0].as.i64<JumpInput_None || arguments[0].as.i64>JumpInput_XY || arguments[1].type!=NativeScriptType_Bool)) ||
           (launch && (!arguments || arguments[0].type!=NativeScriptType_Bool || arguments[1].type!=NativeScriptType_F32 ||
            !isfinite(arguments[1].as.f32))))
            return error(diagnostic,"Jump preparation requires input Int 0..3 and Bool; launch requires Bool and finite Float; other jump steps take no arguments.");
        NativeFighterLifetime saved;if(!capture(context,&saved,diagnostic))return false;
        if(!prepare && !p_ftCommonData)return error(diagnostic,"Common jump data is unavailable.");
        if(launch && arguments[0].as.u64 && (!saved.data->ft_data || !saved.data->ft_data->x4C_sfx))
            return error(diagnostic,"Audible jump launch requires its owned fighter sound table.");
        if(prepare){
            saved.data->mv.co.kneebend.jump_input=(ftCo_JumpInput)arguments[0].as.i64;
            saved.data->mv.co.kneebend.is_short_hop=arguments[1].as.u64!=0;
        }else if(launch)ftCo_800CB110(saved.object,arguments[0].as.u64!=0,arguments[1].as.f32);
        else if(semantic==NativeScriptBinding_JumpCheckShortHop)ftCo_KneeBend_Check_ShortHop(saved.object);
        else ftCo_Jump_Phys_Inner(saved.object);
        if(!nativeFighterLifetime_Current(&saved) || context->source!=saved.object ||
           context->generation!=saved.generation || context->scene_epoch!=saved.reference.lifetime ||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout)
            return error(diagnostic,"Common jump source retired before publication.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_JumpUpdateAnimation) {
        if(!result||count)return error(diagnostic,"Aerial jump animation update takes no arguments.");
        NativeFighterLifetime saved;if(!capture(context,&saved,diagnostic))return false;
        const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(saved.kind);
        if(!package||!ftPartsTable||ftPartsTable[saved.kind]!=&package->assets.parts||
           !saved.data->parts||!package->assets.parts.parts_num||
           package->assets.parts.parts_num>MAX_FT_PARTS||
           package->assets.parts.parts_num!=package->assets.facts.joints+package->assets.facts.omission_count)
            return error(diagnostic,"Jump animation update requires its owned costume rig.");
        for(unsigned i=0;i<package->assets.parts.parts_num;++i) {
            FighterBone* part=&saved.data->parts[i];
            if(part->flags_b1&&!part->flags_b0&&!part->flags_b5&&
               !(saved.data->x8A4_animBlendFrames==0?part->joint:part->x4_jobj2))
                return error(diagnostic,"Jump animation query has a missing active pose joint.");
        }
        ftCo_JumpAerial_Anim(saved.object);
        if(!nativeFighterLifetime_Current(&saved)||!nativeScriptBindings_SourceFighterLayout(context,diagnostic))
            return error(diagnostic,"Aerial jump animation source retired before publication.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(!result || !present(context,diagnostic))return false;
    const NativeScriptMoveLayout* layout=nativeScriptBindings_SourceFighterLayout(context,diagnostic);
    if(!layout)return false;
    HSD_GObj* object=context->source;Fighter* fighter=object->user_data;
    NativeScriptValue value={.type=NativeScriptType_Void};
    if(semantic==NativeScriptBinding_JumpBegin) {
        if(count!=7 || !arguments || arguments[0].type!=NativeScriptType_I32 ||
           arguments[1].type!=NativeScriptType_F32 || arguments[2].type!=NativeScriptType_F32 ||
           arguments[3].type!=NativeScriptType_F32 || arguments[4].type!=NativeScriptType_Bool ||
           arguments[5].type!=NativeScriptType_Bool || arguments[6].type!=NativeScriptType_Bool)
            return error(diagnostic,"Invalid typed aerial-jump arguments.");
        if(arguments[0].as.i64<0 || arguments[0].as.i64>UINT16_MAX ||
           !nativeScriptMoves_Contains(layout,(uint32_t)arguments[0].as.i64))
            return error(diagnostic,"Aerial jump target is outside the owned motion layout.");
        NativeFighterLifetime source;
        if(!nativeFighterLifetime_Capture(fighter,&source) || !source.source_owned ||
           source.reference.lifetime!=context->scene_epoch ||
           &source.program->bank->motion_layout!=layout)
            return error(diagnostic,"Aerial jump requires its retained owned Fighter.");
        if(arguments[5].as.u64 &&
           !nativeScriptFighter_HasHook(object,NativeScriptFighter_AnimationDrivenJumpPhysics))
            return error(diagnostic,"Animation-driven jump requires on animation_driven_jump_physics before entry.");
        if(!fighter->ft_data->x4C_sfx)
            return error(diagnostic,"Aerial jump sound data is unavailable.");
        Vec3 velocity={arguments[1].as.f32,arguments[2].as.f32,arguments[3].as.f32};
        ftCo_JumpAerial_Begin(object,(FtMotionId)arguments[0].as.i64,&velocity,
            arguments[4].as.u64!=0,arguments[5].as.u64!=0,arguments[6].as.u64!=0);
        if(!nativeFighterLifetime_Current(&source))
            return error(diagnostic,"Aerial jump source retired during entry.");
    } else if(semantic==NativeScriptBinding_BonesRootAddRotationY) {
        if(count!=1 || !arguments || arguments[0].type!=NativeScriptType_F32)
            return error(diagnostic,"Root add-rotation requires one Float argument.");
        const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
        if(!package || !ftPartsTable || ftPartsTable[fighter->kind]!=&package->assets.parts ||
           !package->assets.parts.parts_num || !fighter->parts || !fighter->parts[0].joint ||
           fighter->parts[0].joint!=object->hsd_obj ||
           (fighter->parts[0].joint->flags&JOBJ_USE_QUATERNION))
            return error(diagnostic,"Owned root rotation requires the selected Euler root joint.");
        HSD_JObjAddRotationY(fighter->parts[0].joint,arguments[0].as.f32);
    } else if(semantic==NativeScriptBinding_ControlsAerialJumpActions) {
        if(count)return error(diagnostic,"Aerial jump actions take no arguments.");
        ftCo_JumpAerial_IASA(object);
    } else if(semantic==NativeScriptBinding_CollisionTryWallJump) {
        if(count)return error(diagnostic,"Walljump query takes no arguments.");
        value.type=NativeScriptType_Bool;value.as.u64=ftWallJump_8008169C(object);
    } else return error(diagnostic,"Unknown generic jump operation.");
    if(!nativeScriptBindings_SourceFighterLayout(context,diagnostic))return false;
    *result=value;return true;
}
