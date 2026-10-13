#include "engine_fixture.h"
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftdata.h>
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>
#include <stdlib.h>
#include <stdarg.h>

HSD_JObjInfo hsdJObj;
// Exact null-safe getter from baselib/aobj.c.
u32 HSD_AObjGetFlags(HSD_AObj* object){return object?object->flags:0;}
bool nativeScriptFighter_IsAttached(Fighter_GObj* object){(void)object;return false;}

int script_presentation_log[32][5];unsigned script_presentation_log_count;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){(void)kind;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object){(void)object;return NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{(void)object;(void)generation;(void)view;return false;}
static const NativeFighterDefinition* supplied;static HSD_GObj* retiring;static bool replacement;
static NativeSourcePackage* supplied_source;
void scriptPresentationFixture_Source(NativeSourcePackage* source){supplied_source=source;}
void scriptPresentationFixture_Reset(const NativeFighterDefinition* definition)
{HSD_CLASS_INFO(&hsdJObj)->head.flags=1;HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);supplied=definition;supplied_source=NULL;retiring=NULL;script_presentation_log_count=0;}
void scriptPresentationFixture_RetireOnMatrix(HSD_GObj* object,bool replace)
{retiring=object;replacement=replace;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{(void)kind;return supplied;}
// Audio/rotation comparisons here use controlled stock/default metadata.
// No prepared source package or source-facts branch is claimed by this fixture.
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return supplied_source&&supplied_source->target==kind?supplied_source:NULL;}
static void log_sound(int call,int sound,int volume,int pan,int track)
{if(script_presentation_log_count==32)abort();int* row=script_presentation_log[script_presentation_log_count++];row[0]=call;row[1]=sound;row[2]=volume;row[3]=pan;row[4]=track;}
int lbAudioAx_800237A8(enum_t sound,int volume,int pan){log_sound(1,sound,volume,pan,0);return 77;}
int lbAudioAx_80023870(int sound,int volume,int pan,int track){log_sound(2,sound,volume,pan,track);return sound==0x83D61?-1:77;}
int lbAudioAx_80024B94(int voice,int pitch){log_sound(3,voice,pitch,0,0);return 0;}
int lbAudioAx_800233EC(int sound){return sound;}
int lbAudioAx_80023130(int sound){return sound/10000;}
int lbAudioAx_800230C8(int bank,int* start,int* unused){(void)unused;*start=bank*10000;return 0;}
int lbAudioAx_80023220(int bank){(void)bank;return 100;}
bool ftCommon_80080144(Fighter* fp){return fp->x619_costume_id!=0;}
void __assert(char* file,u32 line,char* expression){(void)file;(void)line;(void)expression;abort();}
void OSReport(char* format,...){(void)format;}
void HSD_JObjSetupMatrixSub(HSD_JObj* object)
{
 object->flags&=~JOBJ_MTX_DIRTY;
 if(!retiring)return;
 if(replacement){++((Fighter*)retiring->user_data)->x8_spawnNum;return;}
 HSD_GObj** cursor=&HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];while(*cursor&&*cursor!=retiring)cursor=&(*cursor)->next;
 if(*cursor==retiring)*cursor=retiring->next;retiring->user_data=(void*)1;
}
#define ASSERTMSGLINE(line,condition,message) do{(void)(line);(void)(message);if(!(condition))abort();}while(0)
#include "script_presentation_actual.h"
