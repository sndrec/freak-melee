#include "native_fighter_pose.h"
#include "fighter.h"
#include "ftcommon.h"
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>

bool nativeFighterPose_ApplyMapped(Fighter* fighter,int32_t destination,
    const NativeFighterPoseBinding* binding,bool blend,float a,float b)
{
    if(!fighter||!binding||!binding->count||binding->count>MAX_FT_PARTS||
       (unsigned)destination>NativeFighterPose_Blend||!fighter->parts||
       (unsigned)fighter->kind>=Ft_Kind_RuntimeCapacity||!ftPartsTable[fighter->kind]||
       !isfinite(a)||!isfinite(b))return false;
    unsigned count=ftPartsTable[fighter->kind]->parts_num;
    if(!count||count>MAX_FT_PARTS)return false;
    if(!blend&&(!fighter->ft_data||!fighter->ft_data->x0||!fighter->ft_data->x8||
       fighter->ft_data->x8->x10>=count||!isfinite(fighter->ft_data->x0->model_scaling)||
       fighter->ft_data->x0->model_scaling==0))return false;
    for(unsigned n=0;n<binding->count;++n){
        unsigned part=binding->parts[n];
        if(part>=count||!binding->descriptors[n])return false;
        const FighterBone* bone=&fighter->parts[part];
        bool needed=blend?!bone->flags_b0&&!bone->flags_b5:
            bone->flags_b0||!bone->flags_b5||part==fighter->ft_data->x8->x10;
        if(needed&&!(destination?bone->x4_jobj2:bone->joint))return false;
    }
    for(unsigned n=0;n<binding->count;++n){
        unsigned part=binding->parts[n];FighterBone* bone=&fighter->parts[part];
        HSD_Joint* source=binding->descriptors[n];
        HSD_JObj* target=destination?bone->x4_jobj2:bone->joint;
        if(blend){
            if(!bone->flags_b0&&!bone->flags_b5){
                if(bone->flags_b4)lb_8000B4FC(target,source);
                else lb_8000C868(source,target,target,a,b);
            }
        }else if(part==fighter->ft_data->x8->x10){
            if(bone->flags_b0){
                lb_8000B760(target,source);
                if(destination)HSD_JObjClearFlags(target,JOBJ_USE_QUATERNION);
            }else if(!bone->flags_b5)lb_8000B5DC(target,source);
            ftCommon_8007F6A4(fighter,target);
        }else if(bone->flags_b0){
            lb_8000B6A4(target,source);
            if(destination)HSD_JObjClearFlags(target,JOBJ_USE_QUATERNION);
        }else if(!bone->flags_b5)lb_8000B4FC(target,source);
    }
    return true;
}
