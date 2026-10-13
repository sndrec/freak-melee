#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_RESOURCES_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ITEM_RESOURCES_H

#include "native_script_bindings.h"

// Recipient exports own 0x07. Resources have a separate semantic domain.
#define NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN UINT32_C(0x08000000)
#define NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_ITEM_RESOURCES_PROPERTY UINT32_C(0x08400000)
#define NATIVE_SCRIPT_ITEM_RESOURCES_MATRIX UINT32_C(0x08500000)
#define NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION UINT32_C(0x08600000)
#define NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK UINT32_C(0xfff00000)
#define NATIVE_SCRIPT_ITEM_RESOURCES_INDEX_MASK UINT32_C(0x000fffff)

typedef enum NativeScriptItemResourceSemantic {
    NativeScriptItemResource_Resources=0x08000001,
    NativeScriptItemResource_Begin, NativeScriptItemResource_Release,
    NativeScriptItemResource_Count, NativeScriptItemResource_Resource,
    NativeScriptItemResource_Node, NativeScriptItemResource_Position,
    NativeScriptItemResource_Velocity, NativeScriptItemResource_Collision,
    NativeScriptItemResource_CollisionPosition, NativeScriptItemResource_Previous,
    NativeScriptItemResource_FloorNormal, NativeScriptItemResource_LeftWallNormal,
    NativeScriptItemResource_RightWallNormal, NativeScriptItemResource_CeilingNormal,
    NativeScriptItemResource_Exists, NativeScriptItemResource_SurfaceLine,
    NativeScriptItemResource_Environment, NativeScriptItemResource_FloorLine,
    NativeScriptItemResource_LeftWallLine, NativeScriptItemResource_RightWallLine,
    NativeScriptItemResource_CeilingLine,
    NativeScriptItemResource_Create, NativeScriptItemResource_CreateAt,
    NativeScriptItemResource_LoadModel, NativeScriptItemResource_SetDraw,
    NativeScriptItemResource_BindPart, NativeScriptItemResource_CaptureAnchor,
    NativeScriptItemResource_SweepStage, NativeScriptItemResource_SweepPoint,
    NativeScriptItemResource_FollowFloor, NativeScriptItemResource_LineSpeed,
    NativeScriptItemResource_AttachmentPoint, NativeScriptItemResource_AttachmentMatrix,
    NativeScriptItemResource_WorldMatrix, NativeScriptItemResource_SetWorldMatrix,
    NativeScriptItemResource_EffectPosition, NativeScriptItemResource_EffectFacing,
    NativeScriptItemResource_Joint, NativeScriptItemResource_JointNode,
    NativeScriptItemResource_Translation, NativeScriptItemResource_Rotation,
    NativeScriptItemResource_Scale, NativeScriptItemResource_Quaternion,
    NativeScriptItemResource_AddAnimation, NativeScriptItemResource_RequestAnimation,
    NativeScriptItemResource_AdvanceAnimation, NativeScriptItemResource_RemoveAnimation,
    NativeScriptItemResource_SetAnimationRate,
    NativeScriptItemResource_StaticEnd,
    NativeScriptItemResource_FloatFirst=0x08000100,
} NativeScriptItemResourceSemantic;

enum { NATIVE_SCRIPT_ITEM_RESOURCE_SELECTORS=128,
       NATIVE_SCRIPT_ITEM_RESOURCE_PATH_DEPTH=12 };
typedef struct NativeScriptItemResourceSelector {
    uint32_t semantic, declaration, path_offset, path_length, animation;
    uint8_t path[NATIVE_SCRIPT_ITEM_RESOURCE_PATH_DEPTH];
    uint8_t depth, channel, operation;
} NativeScriptItemResourceSelector;

struct NativeScriptCompileBank;
struct NativeScriptItemProgram;
const NativeScriptBindingDescriptor* nativeScriptBindingItemResources_Descriptor(uint32_t);
bool nativeScriptBindingItemResources_PrepareSelectors(const struct NativeScriptCompileBank*,
    struct NativeScriptItemProgram*, NativeScriptItemResourceSelector*,uint32_t,
    NativeScriptDiagnostic*);
bool nativeScriptBindingItemResources_Read(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingItemResources_Write(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingItemResources_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue*,uint32_t,NativeScriptValue*,uint32_t,
    NativeScriptValue*,NativeScriptDiagnostic*);

#endif
