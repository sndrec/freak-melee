#include <melee/ft/native_callback_lifetime.h>
#include "engine_fixture.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ftaction.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftdata.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Generic engine functions below are the actual extracted bodies. Only graph,
// audio, immutable Definition observation and the four animation stage leaves
// are controlled fixture boundaries. No production binary links this unit.
static ftCommonData common;
ftCommonData* p_ftCommonData=&common;
static FighterPartsTable part_table;
static FighterPartsTable* tables[128];
FighterPartsTable** ftPartsTable=tables;
unsigned script_kinetic_stage_count,script_kinetic_stages[16];
unsigned script_kinetic_sounds;
static unsigned retire_stage,retire_mode;
static const NativeFighterDefinition* supplied_definition;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
#ifdef SCRIPT_FIXTURE_SOURCE_ACTOR
static NativeSourcePackage fixture_source;
static NativeScriptFighterProgram fixture_selected;
static NativeScriptStorageInstance fixture_storage;
static NativeScriptStorageInstance* source_storage;
static HSD_GObj* source_object;
static HSD_GObj* source_second_object;
static NativeScriptStorageInstance* source_second_storage;
static MotionState source_motions[1024];
static NativeMatchPoolSlot source_slot[1];
static uint32_t source_free_order[1];
static HSD_GObj* source_fixed_payload;
static uint64_t source_epoch;
static void register_fixed_source(HSD_GObj* object)
{
    NativeMatchPoolReference existing;
    bool fixed=source_fixed_payload==object&&nativeGObjPool.payload==(uint8_t*)object&&nativeGObjPool.capacity==1;
    if(!fixed&&nativeMatchPool_Reference(&nativeGObjPool,object,&existing))return;
    HSD_GObj saved=*object;
    if(source_epoch==UINT64_MAX || !nativeMatchPool_Bind(&nativeGObjPool,object,sizeof(*object),
            source_slot,source_free_order,1,sizeof(*object),_Alignof(HSD_GObj),++source_epoch))abort();
    if(nativeMatchPool_Allocate(&nativeGObjPool,&existing)!=object)abort();
    *object=saved;source_fixed_payload=object;
}
NativeScriptVMStatus script_fixture_source_status;
const NativeScriptProgramIndex* script_fixture_source_view_program;
NativeSourcePackage* scriptKineticsFixture_Source(void){return &fixture_source;}
NativeScriptCompileBank* scriptKineticsFixture_OwnSource(Fighter* actor,
    const NativeScriptCompileBank* bank,NativeScriptStorageInstance* storage)
{
    register_fixed_source(actor->gobj);
    NativeScriptCompileBank copy={0};if(bank)copy=*bank;
    memset(&fixture_source,0,sizeof(fixture_source));
    fixture_source.address=&fixture_source;fixture_source.ready=fixture_source.assets.ready=true;
    fixture_source.target=actor->kind=actor->behavior_kind=Ft_Kind_CustomFirst;
    if(bank)fixture_source.compiler.bank=copy;
    else fixture_source.compiler.bank=(NativeScriptCompileBank){.valid=true,.has_motion_layout=true,
        .motion_layout={.common_count=341,.first=341,.count=8}};
    NativeScriptCompileBank* selected=&fixture_source.compiler.bank;
    selected->storage_layout.program=&selected->declarations;
    fixture_source.assets.facts.moves=selected->motion_layout;
    fixture_source.assets.motions=source_motions;
    fixture_source.program.bank=selected;fixture_source.program.identity=UINT64_C(1234);
    fixture_selected=fixture_source.program;
    fixture_source.assets.character.data.x0=NULL;
    actor->ft_data=&fixture_source.assets.character.data;
    gFtDataList[actor->kind]=actor->ft_data;ftData_CharacterStateTables[actor->kind]=source_motions;
    fixture_storage=(NativeScriptStorageInstance){.layout=&selected->storage_layout,
        .parameters=&selected->storage_parameters,.owner=0,
        .generation=actor->x8_spawnNum,.live=true};
    source_storage=storage?storage:&fixture_storage;source_object=actor->gobj;
    source_second_object=NULL;source_second_storage=NULL;
    script_fixture_source_view_program=&selected->declarations;
    script_fixture_source_status=NativeScriptVM_Ok;
    return selected;
}
void scriptKineticsFixture_AddSource(Fighter* actor,NativeScriptStorageInstance* storage)
{
    actor->kind=actor->behavior_kind=fixture_source.target;
    actor->ft_data=&fixture_source.assets.character.data;
    source_second_object=actor->gobj;source_second_storage=storage;
}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{return fixture_source.ready&&kind==fixture_source.target?&fixture_selected:NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object)
{return object==source_object||object==source_second_object?script_fixture_source_status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{
    NativeScriptStorageInstance* storage=object==source_object?source_storage:object==source_second_object?source_second_storage:NULL;
    if(!view||!storage||!storage->live||storage->generation!=generation)return false;
    *view=(NativeScriptReceiverView){.program=script_fixture_source_view_program,.storage=storage};return true;
}
#endif

void scriptKineticsFixture_Reset(void)
{
    memset(&common,0,sizeof(common));memset(&part_table,0,sizeof(part_table));
    common.friction_when_above_walk_speed=1.5f;common.aerial_friction_oob=0.375f;
    common.x258=0.1875f;common.x88=0.625f;common.x8C=4;
    part_table.parts_num=3;
    for(unsigned i=0;i<128;++i)tables[i]=&part_table;
    script_kinetic_stage_count=script_kinetic_sounds=retire_stage=retire_mode=0;
    supplied_definition=NULL;
#ifdef SCRIPT_FIXTURE_SOURCE_ACTOR
    fixture_source.ready=false;source_object=NULL;source_storage=NULL;
#endif
}

void scriptKineticsFixture_Definition(const NativeFighterDefinition* definition)
{ supplied_definition=definition; }

void scriptKineticsFixture_RetireAt(unsigned stage,unsigned mode)
{ retire_stage=stage;retire_mode=mode;script_kinetic_stage_count=0; }

static void animation_stage(HSD_GObj* object,unsigned stage)
{
    if(script_kinetic_stage_count>=16)abort();
    script_kinetic_stages[script_kinetic_stage_count++]=stage;
    Fighter* fp=object->user_data;
    fp->cur_anim_frame+=(float)stage*0.125f;
    if(stage!=retire_stage)return;
    if(retire_mode==1) { ++fp->x8_spawnNum;return; }
    HSD_GObj** cursor=&HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
    while(*cursor&&*cursor!=object)cursor=&(*cursor)->next;
    if(*cursor==object)*cursor=object->next;
    object->user_data=(void*)1;
}

void ftAnim_8006E9B4(HSD_GObj* object){animation_stage(object,1);}
void ftAction_80073240(HSD_GObj* object){animation_stage(object,2);}
void ftAnim_800707B0(HSD_GObj* object){animation_stage(object,3);}
void ftCo_800DB500(HSD_GObj* object){animation_stage(object,4);}

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ (void)kind;return supplied_definition; }
// This isolated primitive oracle exercises stock/default metadata only.
// Real source-facts selection is covered by package/runtime acceptance tests.
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{
#ifdef SCRIPT_FIXTURE_SOURCE_ACTOR
    return kind==fixture_source.target&&source_object?&fixture_source:NULL;
#else
    (void)kind;return NULL;
#endif
}
float mpColl_8004CA6C(CollData* collision)
{ (void)collision;return 0.625f; }
void ft_PlaySFX(Fighter* fp,enum_t sound,u8 volume,u8 pan)
{ (void)fp;if(sound!=0x96||volume!=0x7F||pan!=0x40)abort();++script_kinetic_sounds; }
bool lb_8000B074(HSD_JObj* joint){return joint&&(joint->flags&1)!=0;}
float lbGetJObjEndFrame(HSD_JObj* joint){return joint?joint->scale.x:0;}
void HSD_AObjSetRate(HSD_AObj* animation,float rate)
{ (void)animation;(void)rate; }
void HSD_ForeachAnim(void* object,HSD_Type type,HSD_TypeMask mask,void* function,
    AObj_Arg_Type argument,...)
{
    if(type!=JOBJ_TYPE||mask!=0xFB7F||function!=(void*)HSD_AObjSetRate||argument!=AOBJ_ARG_AF)abort();
    va_list values;va_start(values,argument);float rate=(float)va_arg(values,double);va_end(values);
    if(object)((HSD_JObj*)object)->scale.y=rate;
}

#include "script_kinetics_actual.h"

// Neutral/Wait requires real actor + fixed-pool lifetime admission. Its actual
// scheduler and authored callbacks are covered by script_fighter_movement_scheduler.
// This primitive-only fixture has no actor runtime: accidental entry fails loudly.
void ft_8008A348(Fighter_GObj* object,float blend)
{(void)object;(void)blend;abort();}
