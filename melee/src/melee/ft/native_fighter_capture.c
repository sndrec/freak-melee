#include "native_fighter_capture.h"
#include "fighter.h"
#include "ftanim.h"
#include "ftcommon.h"
#include "native_source_package.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftCo_Lift.h"
#include "kinds/ftCommon/ftCo_Throw.h"
#include "kinds/ftCommon/ftCo_Thrown.h"
#include "kinds/ftCommon/ftCo_CapturePulled.h"

bool nativeFighterCapture_Current(const NativeFighterCapturePair* pair,bool require_link)
{
    if(!pair || !nativeFighterLifetime_Current(&pair->holder))return false;
    if(!pair->victim.object)return !pair->holder.data->victim_gobj;
    if(!nativeFighterLifetime_Current(&pair->victim) ||
       pair->holder.reference.lifetime!=pair->victim.reference.lifetime)return false;
    return !require_link || (pair->holder.data->victim_gobj==pair->victim.object &&
        pair->holder.data->x1A5C==pair->victim.object &&
        pair->holder.data->native_hitlag_link_generation==pair->victim.generation);
}

bool nativeFighterCapture_Attached(const NativeFighterCapturePair* pair)
{
    return nativeFighterCapture_Current(pair,true) && pair->victim.object &&
        pair->victim.data->victim_gobj==pair->holder.object &&
        pair->victim.data->x1A5C==pair->holder.object &&
        pair->victim.data->native_hitlag_link_generation==pair->holder.generation;
}

bool nativeFighterCapture_Retain(Fighter_GObj* holder,NativeFighterCapturePair* output)
{
    NativeMatchPoolReference physical;
    if(!output || !nativeMatchPool_Reference(&nativeGObjPool,holder,&physical) ||
       holder->classifier!=HSD_GOBJ_CLASS_FIGHTER)return false;
    NativeFighterCapturePair pair={0};
    if(!nativeFighterLifetime_Capture(holder->user_data,&pair.holder))return false;
    Fighter_GObj* victim=pair.holder.data->victim_gobj;
    if(victim){
        if(victim==holder || !nativeMatchPool_Reference(&nativeGObjPool,victim,&physical) ||
           physical.lifetime!=pair.holder.reference.lifetime ||
           victim->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
           !nativeFighterLifetime_Capture(victim->user_data,&pair.victim))return false;
    }
    if(!nativeFighterCapture_Current(&pair,true))return false;
    *output=pair;return true;
}

bool nativeFighterCapture_AdmitMotion(const NativeFighterLifetime* actor,FtMotionId motion)
{
    if(!actor || !nativeFighterLifetime_Current(actor) || (s32)motion<0 ||
       !actor->data->x1C_actionStateList)return false;
    if(actor->source_owned){
        const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(actor->kind);
        const NativeScriptMoveLayout* layout=&actor->program->bank->motion_layout;
        if(!source || source->address!=source || !source->ready || !source->assets.ready ||
           source->target!=actor->kind || actor->program->bank!=&source->compiler.bank ||
           !nativeScriptMoves_Contains(layout,(uint32_t)motion) ||
           actor->data->x18!=(s32)layout->first ||
           actor->data->x1C_actionStateList!=source->assets.common_motions ||
           actor->data->x20_actionStateList!=source->assets.motions)return false;
    }else if((s32)motion>=actor->data->x18)return false;
    return true;
}

bool nativeFighterCapture_Begin(Fighter_GObj* victim,Fighter_GObj* holder,
    FtMotionId motion,bool same_facing,bool force_air,bool track_position,bool holder_animation)
{
    NativeFighterCapturePair pair;
    if(!nativeFighterCapture_Retain(holder,&pair) || pair.victim.object!=victim ||
       !victim || !nativeFighterCapture_AdmitMotion(&pair.victim,motion))return false;
    Fighter* captured=pair.victim.data;Fighter* owner=pair.holder.data;
    ftCommon_8007DB58(victim);
    if(!nativeFighterCapture_Current(&pair,true))return false;
    ftCo_8009750C(victim);
    if(!nativeFighterCapture_Current(&pair,true))return false;
    ftCo_800DD168(victim);
    if(!nativeFighterCapture_Current(&pair,true))return false;
    captured->victim_gobj=captured->x1A5C=holder;
    captured->native_hitlag_link_generation=pair.holder.generation;
    captured->x221B_b5=false;
    captured->facing_dir=same_facing?owner->facing_dir:-owner->facing_dir;
    if(force_air || captured->ground_or_air==GA_Air){
        ftCo_800DB368(owner,captured);captured->x221B_b7=false;
    }else captured->x221B_b7=true;
    if(!nativeFighterCapture_Current(&pair,true))return false;
    Fighter_ChangeMotionState(victim,motion,Ft_MF_None,0,1,0,holder_animation?holder:NULL);
    if(!nativeFighterCapture_Current(&pair,true))return false;
    if(track_position && !captured->x221B_b7)captured->accessory1_cb=ftCo_800DB464;
    if(force_air)ftCommon_8007D5D4(captured);
    ftCommon_8007E2F4(captured,0x1FF);
    ftAnim_8006EBA4(victim);
    if(!nativeFighterCapture_Current(&pair,true))return false;
    ftCommon_8007E2FC(victim);
    return nativeFighterCapture_Current(&pair,true);
}

bool nativeFighterCapture_AttachToVictim(Fighter_GObj* holder)
{
    NativeFighterCapturePair pair;
    if(!nativeFighterCapture_Retain(holder,&pair) || !pair.victim.object)return false;
    ftCo_800DB368(pair.victim.data,pair.holder.data);
    return nativeFighterCapture_Current(&pair,true);
}

void nativeFighterCapture_FollowVictim(Fighter_GObj* holder)
{
    NativeFighterCapturePair pair;
    if(nativeFighterCapture_Retain(holder,&pair) && pair.victim.object)
        pair.holder.data->cur_pos=pair.victim.data->cur_pos;
}

bool nativeFighterCapture_ReleaseThrow(Fighter_GObj* holder)
{
    NativeFighterCapturePair pair;
    if(!nativeFighterCapture_Retain(holder,&pair))return false;
    if(!pair.victim.object)return true;
    // Damage intentionally unlinks the pair. Keep the original identities for
    // the second step instead of reselecting a newly linked/reused victim.
    ftCo_800DE2A8(holder,pair.victim.object);
    if(!nativeFighterCapture_Current(&pair,false))return false;
    ftCo_800DE7C0(pair.victim.object,NULL,false);
    return nativeFighterCapture_Current(&pair,false);
}
