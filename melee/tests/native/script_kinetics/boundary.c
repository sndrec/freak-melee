#include "engine_fixture.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/gr/stage.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdlib.h>

// This console fixture publishes only a controlled source Fighter, never an
// Item or a stage. Unexercised engine operations remain unavailable.
NativeMatchPool nativeGObjPool;
bool nativeScriptFighter_IsAttached(Fighter_GObj* object)
{
    if(!object || !object->user_data)return false;
    NativeScriptReceiverView view;
    return nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view);
}
bool nativeScriptItem_IsAttached(Item_GObj* object){(void)object;return false;}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object)
{(void)object;abort();}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* object)
{(void)object;abort();}
bool nativeScriptItem_View(Item_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{(void)object;(void)generation;(void)view;abort();}
StKind Stage_80225194(void){abort();}
void lbVector_RotateAboutUnitAxis(Vec3* vector,Vec3* axis,float angle)
{(void)vector;(void)axis;(void)angle;abort();}
void lbVector_Mirror(Vec3* vector,Vec3* axis)
{(void)vector;(void)axis;abort();}
