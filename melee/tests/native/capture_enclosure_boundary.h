// Algorithm-reference boundary: real physical pool references; stock actor
// registration is controlled and source programs are explicitly unavailable.
// The script_capture_scheduler fixture supplies actual owned actor/VM coverage.
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_lifetime.h>
NativeMatchPool nativeGObjPool;
static NativeMatchPoolSlot enclosure_pool_slots[3];
static uint32_t enclosure_pool_order[3];
static uint64_t enclosure_pool_epoch;
static FighterPartsTable enclosure_reference_parts;
static FighterPartsTable* enclosure_reference_tables[Ft_Kind_RuntimeCapacity];
static u8 enclosure_reference_map[MAX_FT_PARTS];
FighterPartsTable** ftPartsTable=enclosure_reference_tables;
bool nativeCallbackSource_Snapshot(HSD_GObj* object,uint32_t source,uint32_t* generation)
{
    NativeMatchPoolReference reference;
    if(source!=NativeCallbackSource_Fighter||!generation||
       !nativeMatchPool_Reference(&nativeGObjPool,object,&reference)||!object->user_data)return false;
    *generation=((Fighter*)object->user_data)->x8_spawnNum;return true;
}
bool nativeCallbackSource_Alive(HSD_GObj* object,uint32_t generation,uint32_t source)
{uint32_t current;return nativeCallbackSource_Snapshot(object,source,&current)&&current==generation;}
bool nativeScriptFighter_IsAttached(Fighter_GObj* object){(void)object;return false;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){(void)kind;abort();}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object){(void)object;abort();}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{(void)object;(void)generation;(void)view;abort();}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){(void)kind;abort();}
