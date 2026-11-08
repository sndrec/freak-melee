#include "native_studio.h"
#include "native_match_launch.h"
#include "gm_1A45.h"
#include <melee/pl/player.h>
#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/if/ifall.h>
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void nativeStudio_Command(const char* text)
{
    const NativeNetplayLaunchConfig* network=nativeMatchLaunch_Netplay();
    if (!nativeMatchLaunch_IsDirect() || nativeMatchLaunch_IsSpectating() ||
        (network && network->enabled) || nativeMatchLaunch_ReplayPlayback()) {
        OSReport("[studio] Rejected: use an offline --match without replay.\n"); return;
    }
    HSD_GObj* camera=Camera_80030A50();
    if (!camera || !camera->hsd_obj) { OSReport("[studio] Match camera is not ready.\n"); return; }
    unsigned value,port,motion; int end=0;
    float frame,x,y,facing;
    Vec3 eye,target; float fov;
    bool accepted=false;
    if (!strcmp(text,"state")) {
        for (unsigned p=0;p<4;++p) {
            Fighter_GObj* actor=Player_GetEntityAtIndex(p,0);
            if (!actor) continue;
            Fighter* fighter=GET_FIGHTER(actor);
            OSReport("[studio] actor=%u kind=%u motion=%u frame=%.3f position=%.3f,%.3f facing=%.1f\n",
                p,fighter->kind,fighter->motion_id,fighter->cur_anim_frame,
                fighter->cur_pos.x,fighter->cur_pos.y,fighter->facing_dir);
        }
        return;
    }
    if (!strcmp(text,"music,off")) {
        lbAudioAx_800236DC(); accepted=true;
    } else if (!strcmp(text,"reset")) {
        gm_ClearDbPauseFlag(0); Camera_SetModeToStandard(); ifAll_ShowHUD(); accepted=true;
    } else if (sscanf(text,"hud,%u%n",&value,&end)==1 && !text[end] && value<=1) {
        if (value) ifAll_ShowHUD(); else ifAll_HideHUD(); accepted=true;
    } else if (sscanf(text,"pause,%u%n",&value,&end)==1 && !text[end] && value<=1) {
        if (value) gm_SetDbPauseFlag(0); else gm_ClearDbPauseFlag(0); accepted=true;
    } else if (sscanf(text,"camera,%f,%f,%f,%f,%f,%f,%f%n",&eye.x,&eye.y,&eye.z,&target.x,&target.y,&target.z,&fov,&end)==7 && !text[end] &&
        isfinite(eye.x) && isfinite(eye.y) && isfinite(eye.z) && isfinite(target.x) && isfinite(target.y) && isfinite(target.z) &&
        isfinite(fov) && fov>=5 && fov<=100 &&
        (eye.x-target.x)*(eye.x-target.x)+(eye.y-target.y)*(eye.y-target.y)+(eye.z-target.z)*(eye.z-target.z)>1) {
        Camera_8003006C();
        cm_80453004.free_eye_pos=eye; cm_80453004.free_int_pos=target; cm_80453004.free_fov=fov;
        HSD_CObjSetEyePosition(camera->hsd_obj,&eye); HSD_CObjSetInterest(camera->hsd_obj,&target); HSD_CObjSetFov(camera->hsd_obj,fov);
        accepted=true;
    } else if (sscanf(text,"fall,%u,%f,%f%n",&port,&x,&y,&end)==3 && !text[end] && port<4 && isfinite(x) && isfinite(y)) {
        Fighter_GObj* actor=Player_GetEntityAtIndex(port,0);
        if (actor) {
            Fighter* fighter=GET_FIGHTER(actor);
            fighter->cur_pos=fighter->prev_pos=(Vec3){x,y,0};
            fighter->self_vel=(Vec3){0,0,0}; fighter->x8c_kb_vel=(Vec3){0,0,0}; fighter->pos_delta=(Vec3){0,0,0}; fighter->gr_vel=0;
            HSD_JObjSetTranslate(actor->hsd_obj,&fighter->cur_pos);
            ftCommon_8007E2FC(actor); ftCo_Fall_Enter(actor); accepted=true;
        }
    } else if (sscanf(text,"pose,%u,%u,%f,%f,%f,%f%n",&port,&motion,&frame,&x,&y,&facing,&end)==6 && !text[end] &&
        port<4 && motion<1024 && isfinite(frame) && frame>=0 && frame<1024 && isfinite(x) && isfinite(y) && (facing==1 || facing==-1)) {
        Fighter_GObj* actor=Player_GetEntityAtIndex(port,0);
        if (actor) {
            Fighter* fighter=GET_FIGHTER(actor);
            // Motion IDs come from the loaded fighter, rather than archive action indices.
            const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(fighter->kind);
            const NativeFighterDefinition* stock=nativeFighterRuntime_FindStockDefinition(fighter->kind);
            unsigned count=fighter->x18+(source?source->assets.facts.moves.count:stock?stock->motion_state_count:0);
            if (motion<count) {
                fighter->facing_dir=facing;
                fighter->cur_pos=fighter->prev_pos=(Vec3){x,y,0};
                fighter->self_vel=(Vec3){0,0,0}; fighter->x8c_kb_vel=(Vec3){0,0,0}; fighter->pos_delta=(Vec3){0,0,0}; fighter->gr_vel=0;
                HSD_JObjSetTranslate(actor->hsd_obj,&fighter->cur_pos);
                Fighter_ChangeMotionState(actor,motion,Ft_MF_SkipRumble,frame,1,0,NULL);
                accepted=true;
            }
        }
    }
    OSReport("[studio] accepted=%u command=%s\n",accepted,text);
}
