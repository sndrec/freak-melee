#include "native_script_effect_dispatch.h"
#include <stddef.h>

// Source-authoritative admission, independent of fighter/species. The focused
// fixture compares every ID against actual dispatcher cases and pointer usage.
static const uint16_t joints[]={
    0x402,0x403,0x409,0x412,0x413,0x414,0x41B,0x422,0x433,0x43D,0x449,0x473,
    0x47C,0x47D,0x47E,0x47F,0x480,0x481,0x482,0x484,0x485,0x487,0x488,0x489,
    0x48A,0x48B,0x48C,0x491,0x496,0x497,0x498,0x499,0x49B,0x49C,0x4A1,0x4A2,
    0x4A3,0x4A5,0x4A6,0x4A7,0x4A8,0x4AD,0x4AE,0x4AF,0x4B4,0x4B5,0x4B6,0x4B7,0x4B8,0x4B9,
    0x4BD,0x4BE,0x4BF,0x4C0,0x4C6,0x4C7,0x4CA,0x4CB,0x4CD,0x4D1,0x4D3,0x4D4,0x4D5,
    0x4D6,0x4D7,0x4DA,0x4DB,0x4DC,0x4DD,0x4DE,0x4DF,0x4E0,0x4E5,0x4E6,0x4E9,0x4EA,0x4EC,0x4ED,0x4EE,
    0x4EF,0x4F0,0x4F2,0x4F3,0x4F4,0x4F5,0x4F6,0x4F7,0x4F8,0x4F9,0x4FA,0x4FB,0x4FC,0x4FD,0x4FE,0x502,
    0x509,0x50A,0x50D,0x511,0x512,
};
static const uint16_t scalars[]={
    0x415,0x423,0x424,0x438,0x439,0x43E,0x452,0x45D,0x45E,0x463,0x464,
    0x468,0x469,0x46A,0x46B,0x46C,0x46D,0x47A,0x48D,0x490,0x492,0x493,0x495,
    0x49A,0x49F,0x4A9,0x4AA,0x4B0,0x4B1,0x4C8,0x4C9,0x4EB,0x4F1,0x507,0x50C,0x50E,0x50F,0x510,
};
static const uint16_t positions[]={
    0x3E9,0x3EA,0x3EB,0x3EC,0x3EE,0x3F3,0x3F4,0x3F6,0x3FA,0x3FB,0x3FC,
    0x405,0x407,0x40C,0x40D,0x40E,0x40F,0x410,0x411,0x416,0x41C,0x41D,
    0x41F,0x420,0x421,0x425,0x426,0x427,0x42D,0x434,0x43A,0x43B,0x43C,
    0x441,0x442,0x444,0x445,0x44C,0x474,0x475,0x476,0x477,
    0x479,0x47B,0x483,0x49E,0x4A0,0x4B2,
    0x4C3,0x4CC,0x4CE,0x4D2,0x4D8,0x4E2,0x4E3,0x4E4,0x4E7,0x4E8,
    0x503,0x504,0x505,0x506,0x508,
};
// Each executed branch consumes one copied Vec3 followed by one copied float.
// The float's meaning belongs to the native recipe (facing, rotation or scale).
static const uint16_t position_scalars[]={
    0x3E8,0x3ED,0x3EF,0x3F0,0x3F1,0x3F2,0x3F5,0x3FE,0x400,0x401,
    0x404,0x406,0x408,0x40A,0x40B,0x42C,0x42E,0x42F,0x430,0x431,0x432,
    0x435,0x436,0x437,0x43F,0x440,0x443,0x447,0x486,0x48E,0x4A4,0x4AC,
    0x4CF,0x4D9,
};
static int contains(const uint16_t* values,size_t count,uint16_t effect)
{
    size_t low=0,high=count;while(low<high){size_t mid=low+(high-low)/2;if(values[mid]<effect)low=mid+1;else high=mid;}
    return low<count&&values[low]==effect;
}
NativeScriptEffectArguments nativeScriptEffectDispatch_Arguments(int32_t effect)
{
    if(effect<0||effect>UINT16_MAX)return NativeScriptEffectArguments_Unsupported;
    // efSync_Spawn routes these numeric generator families before its switch.
    // Both consume exactly one Vec3 and copy it through efLib_CreateGenerator;
    // no creator root, model callback or caller position pointer is retained.
    if(effect<0x250 || effect/1000==0x1E)return NativeScriptEffectArguments_Position;
    switch(effect) {
    case 0x3FF:return NativeScriptEffectArguments_PositionTwoScalar;
    case 0x448:return NativeScriptEffectArguments_JointOffset;
    case 0x48F:case 0x4BB:case 0x4BC:case 0x50B:
        return NativeScriptEffectArguments_TwoJoint;
    }
    if(contains(joints,sizeof(joints)/sizeof(*joints),(uint16_t)effect))return NativeScriptEffectArguments_Joint;
    if(contains(scalars,sizeof(scalars)/sizeof(*scalars),(uint16_t)effect))return NativeScriptEffectArguments_JointScalar;
    if(contains(positions,sizeof(positions)/sizeof(*positions),(uint16_t)effect))return NativeScriptEffectArguments_Position;
    if(contains(position_scalars,sizeof(position_scalars)/sizeof(*position_scalars),(uint16_t)effect))return NativeScriptEffectArguments_PositionScalar;
    return NativeScriptEffectArguments_Unsupported;
}
bool nativeScriptEffectDispatch_PositionRequirements(int32_t effect,uint32_t* requirements)
{
    static const struct {uint16_t effect,requirements;} contexts[]={
        {0x3EB,NativeScriptEffectPosition_CreatesModel},
        {0x3EC,NativeScriptEffectPosition_CreatesModel},
        {0x3EE,NativeScriptEffectPosition_CreatesModel},
        {0x3F6,NativeScriptEffectPosition_CreatesModel},
        {0x3FA,NativeScriptEffectPosition_CreatesModel},
        {0x3FB,NativeScriptEffectPosition_CreatesModel},
        {0x3FC,NativeScriptEffectPosition_CreatesModel},
        {0x40D,NativeScriptEffectPosition_Root},
        {0x41D,NativeScriptEffectPosition_CreatesModel},
        {0x427,NativeScriptEffectPosition_CreatesModel},
        {0x4CC,NativeScriptEffectPosition_Root|NativeScriptEffectPosition_FighterParent|NativeScriptEffectPosition_CreatesModel},
        {0x4D2,NativeScriptEffectPosition_CreatesModel},
        {0x4D8,NativeScriptEffectPosition_Root},
        {0x4E2,NativeScriptEffectPosition_CreatesModel},
        {0x4E8,NativeScriptEffectPosition_CreatesModel},
        {0x504,NativeScriptEffectPosition_Root},
        {0x505,NativeScriptEffectPosition_Root},
    };
    if(!requirements||nativeScriptEffectDispatch_Arguments(effect)!=NativeScriptEffectArguments_Position)return false;
    size_t low=0,high=sizeof(contexts)/sizeof(*contexts);
    while(low<high){size_t mid=low+(high-low)/2;if(contexts[mid].effect<effect)low=mid+1;else high=mid;}
    *requirements=low<sizeof(contexts)/sizeof(*contexts)&&contexts[low].effect==effect?contexts[low].requirements:0;
    return true;
}
bool nativeScriptEffectDispatch_PositionScalarRequirements(int32_t effect,uint32_t* requirements)
{
    if(!requirements||nativeScriptEffectDispatch_Arguments(effect)!=NativeScriptEffectArguments_PositionScalar)return false;
    uint32_t flags=0;
    switch(effect){
    case 0x3E8:case 0x3F5:case 0x404:case 0x406:case 0x42C:case 0x42F:
    case 0x431:case 0x486:case 0x48E:case 0x4A4:case 0x4AC:case 0x4CF:
        flags=NativeScriptEffectPosition_CreatesModel;break;
    case 0x447:
        flags=NativeScriptEffectPosition_CreatesModel|NativeScriptEffectPosition_FighterParent;break;
    case 0x4D9:
        flags=NativeScriptEffectPosition_CreatesModel|NativeScriptEffectPosition_Root;break;
    }
    *requirements=flags;return true;
}
bool nativeScriptEffectDispatch_PositionTwoScalarRequirements(int32_t effect,uint32_t* requirements)
{
    if(!requirements||nativeScriptEffectDispatch_Arguments(effect)!=NativeScriptEffectArguments_PositionTwoScalar)return false;
    // The audited model recipe copies position, facing and rotation during
    // construction. Its shared constructor does not read the creator root.
    *requirements=NativeScriptEffectPosition_CreatesModel;return true;
}
