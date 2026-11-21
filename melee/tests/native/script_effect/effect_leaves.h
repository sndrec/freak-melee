#include <melee/ef/eflib.h>
#include <melee/ef/efdata.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efalt.h>
#include <melee/ef/efasync.h>
#include <melee/ef/native_script_effect_actor.h>
#include <melee/ef/native_script_effect_snapshot.h>
#include <melee/ef/native_model_effect.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/lb_00B0.h>
#include <melee/cm/camera.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <stdarg.h>
#include <melee/ef/inlines.h>
#define EFALT_VA_ARG(type) va_arg(vlist_arg,type)
HSD_ObjAllocData efLib_AllocData;
HSD_ObjAllocData efAsync_AllocData;
void efAsync_Spawn(HSD_GObj* object,void* queue,u32 kind,u32 id,HSD_JObj* joint,...){(void)object;(void)queue;(void)kind;(void)id;(void)joint;abort();}
void Camera_RequestQuake(CmQuakeKind kind,Vec3* position){(void)kind;(void)position;abort();}
static EF_QueuedEffect queue_payloads[32];static bool queue_live[32];
unsigned effect_queue_frees;
static void* queue_allocate(void)
{for(unsigned i=0;i<32;++i)if(!queue_live[i]){queue_live[i]=true;return &queue_payloads[i];}return NULL;}
static void queue_free(void* object)
{for(unsigned i=0;i<32;++i)if(object==&queue_payloads[i]&&queue_live[i]){queue_live[i]=false;++effect_queue_frees;return;}abort();}
u32 efLib_LoadKind;
s32 efLib_AnimCount;
u32 efLib_EffectCount;
u32 efLib_AllocationSerial;
EF_EffectUsage efLib_Usage;
void nativeWorkshopPractice_ScriptFault(const char* kind, unsigned generation,
    unsigned line, unsigned column, const char* message)
{ fprintf(stderr, "%s %u at %u:%u: %s\n", kind, generation, line, column, message); }
HSD_JObj* efLib_AnimQueue[32];
EF_DAT_Entry efAsync_DatEntries[51];
static EF_Effect effect_payloads[520];static bool effect_live[520];static HSD_JObj effect_joints[520][3];
static unsigned effect_slot;
extern HSD_GObj* spawn_caller;
extern unsigned effect_models,effect_frees,effect_animations,effect_retire_on_create,effect_retire_on_anim,effect_fail_model_at;
static void* effect_allocate(void){for(unsigned i=0;i<520;++i)if(!effect_live[i]){effect_slot=i;effect_live[i]=true;memset(&effect_payloads[i],0xCC,sizeof(EF_Effect));return &effect_payloads[i];}return NULL;}
static void effect_free(void* data){for(unsigned i=0;i<520;++i)if(data==&effect_payloads[i]){effect_live[i]=false;++effect_frees;return;}abort();}
void efLib_RemoveLast(void){abort();}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* desc){(void)desc;HSD_JObj* joints=effect_joints[effect_slot];memset(joints,0,sizeof(effect_joints[0]));joints[0].child=&joints[1];joints[1].next=&joints[2];joints[1].parent=joints[2].parent=&joints[0];joints[0].scale=(Vec3){1,1,1};if(++effect_models==effect_fail_model_at)return NULL;if(effect_retire_on_create){effect_retire_on_create=0;nativeScriptFighter_Detach(spawn_caller);}return joints;}
void HSD_JObjAnimAll(HSD_JObj* joint){(void)joint;++effect_animations;if(effect_retire_on_anim){effect_retire_on_anim=0;nativeScriptFighter_Detach(spawn_caller);}}
void HSD_JObjAddAnimAll(HSD_JObj* j,HSD_AnimJoint* a,HSD_MatAnimJoint* m,HSD_ShapeAnimJoint* s){(void)j;(void)a;(void)m;(void)s;abort();}
void HSD_JObjReqAnimAll(HSD_JObj* j,float frame){(void)j;(void)frame;abort();}
void HSD_MtxGetScale(Mtx matrix,Vec3* scale){(void)matrix;*scale=(Vec3){1,1,1};}
void lb_80011C18(HSD_JObj* j,u32 flags){(void)j;(void)flags;abort();}
void lb_8000C1C0(HSD_JObj* j,HSD_JObj* attach){(void)j;(void)attach;}
void lb_8000C290(HSD_JObj* j,HSD_JObj* attach){(void)j;(void)attach;if(effect_retire_on_create){effect_retire_on_create=0;nativeScriptFighter_Detach(spawn_caller);}}
void efLib_Cb_ftMr_SpecialLw(EF_Effect* effect){(void)effect;abort();}
void efLib_Cb_ftKp_SpecialHi(EF_Effect* effect){(void)effect;abort();}
HSD_Generator* efLib_CreateGenerator(s32 id,Vec3* p){(void)id;(void)p;return NULL;}
EF_SpawnResult efAsync_Dispatch(s32 id,HSD_GObj* parent,va_list list){(void)id;(void)parent;(void)list;return(EF_SpawnResult){0};}
void HSD_JObjSetupMatrixSub(HSD_JObj* object){(void)object;abort();}
void HSD_JObjSetFlagsAll(HSD_JObj* joint,u32 flags){if(!joint)return;joint->flags|=flags;if(!(joint->flags&JOBJ_INSTANCE)){HSD_JObjSetFlagsAll(joint->child,flags);for(HSD_JObj* next=joint->child?joint->child->next:NULL;next;next=next->next)HSD_JObjSetFlagsAll(next,flags);}}
void HSD_JObjClearFlags(HSD_JObj* joint,u32 flags){if(joint)joint->flags&=~flags;}
void HSD_JObjClearFlagsAll(HSD_JObj* joint,u32 flags){if(!joint)return;joint->flags&=~flags;if(!(joint->flags&JOBJ_INSTANCE)){HSD_JObjClearFlagsAll(joint->child,flags);for(HSD_JObj* next=joint->child?joint->child->next:NULL;next;next=next->next)HSD_JObjClearFlagsAll(next,flags);}}
bool ftAnim_IsFramesRemaining(HSD_GObj* object){(void)object;abort();}
float ftAnim_8006F484(HSD_GObj* object){(void)object;abort();}
bool ft_80082708(HSD_GObj* object){(void)object;abort();}
bool ft_800827A0(HSD_GObj* object){(void)object;abort();}
GroundOrAir ft_80081D0C(HSD_GObj* object){(void)object;abort();}
void effect_peer_checkpoint(bool restore)
{
    static EF_Effect saved_payloads[520];static bool saved_live[520];static HSD_JObj saved_joints[520][3];
    static u32 saved_serial;static s32 saved_count;
    if(restore){memcpy(effect_payloads,saved_payloads,sizeof(effect_payloads));memcpy(effect_live,saved_live,sizeof(effect_live));memcpy(effect_joints,saved_joints,sizeof(effect_joints));efLib_AllocationSerial=saved_serial;efLib_EffectCount=saved_count;}
    else{memcpy(saved_payloads,effect_payloads,sizeof(effect_payloads));memcpy(saved_live,effect_live,sizeof(effect_live));memcpy(saved_joints,effect_joints,sizeof(effect_joints));saved_serial=efLib_AllocationSerial;saved_count=efLib_EffectCount;}
}
