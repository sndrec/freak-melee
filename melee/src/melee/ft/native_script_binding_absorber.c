#include "native_script_binding_absorber.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include "ftcoll.h"
#include "ftparts.h"
#include "ftdat.h"
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool capture(NativeScriptRuntimeContext* c,NativeFighterLifetime* saved,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!c||c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference)||reference.lifetime!=c->scene_epoch||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !((HSD_GObj*)c->source)->user_data||!nativeScriptBindings_SourceAlive(c)||
       !nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,saved)||
       saved->generation!=c->generation||saved->reference.lifetime!=c->scene_epoch)
        return error(d,"Absorber requires its live bound source Fighter");
    return true;
}
bool nativeScriptBindingAbsorber_Read(NativeScriptRuntimeContext* c,uint32_t semantic,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    NativeFighterLifetime saved;if(!out||!capture(c,&saved,d))return false;
    Fighter* f=saved.data;NativeScriptValue value={0};
    switch(semantic) {
    case NativeScriptBinding_AbsorberActive:value.type=NativeScriptType_Bool;value.as.u64=f->x2218_b6;break;
    case NativeScriptBinding_AbsorberHit:value.type=NativeScriptType_Bool;value.as.u64=f->x2218_b7;break;
    case NativeScriptBinding_AbsorberSkipPositionUpdate:value.type=NativeScriptType_Bool;value.as.u64=f->absorb_hit.skip_update_pos;break;
    case NativeScriptBinding_AbsorberDamage:value.type=NativeScriptType_I32;value.as.i64=f->AbsorbAttr.x1A44_damageTaken;break;
    case NativeScriptBinding_AbsorberHits:value.type=NativeScriptType_I32;value.as.i64=f->AbsorbAttr.x1A48_hitsTaken;break;
    case NativeScriptBinding_AbsorberDirection:value.type=NativeScriptType_F32;value.as.f32=f->AbsorbAttr.x1A40_absorbHitDirection;break;
    default:return error(d,"Unknown absorber property");
    }
    if(!nativeFighterLifetime_Current(&saved))return error(d,"Absorber source retired before publication");
    *out=value;return true;
}
bool nativeScriptBindingAbsorber_Write(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!value||value->type!=NativeScriptType_Bool||value->as.u64>1)return error(d,"Absorber write requires a canonical Bool");
    NativeFighterLifetime saved;if(!capture(c,&saved,d))return false;
    switch(semantic) {
    case NativeScriptBinding_AbsorberActive:saved.data->x2218_b6=value->as.u64;break;
    case NativeScriptBinding_AbsorberHit:saved.data->x2218_b7=value->as.u64;break;
    case NativeScriptBinding_AbsorberSkipPositionUpdate:saved.data->absorb_hit.skip_update_pos=value->as.u64;break;
    default:return error(d,"Absorber property is read-only or unknown");
    }
    return nativeFighterLifetime_Current(&saved)||error(d,"Absorber source retired after write");
}
bool nativeScriptBindingAbsorber_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out||semantic!=NativeScriptBinding_AbsorberCreate||count!=5||!args||args[0].type!=NativeScriptType_I32||
       args[0].as.i64<INT32_MIN||args[0].as.i64>INT32_MAX)return error(d,"Absorber creation argument contract is invalid");
    for(unsigned i=1;i<5;++i)if(args[i].type!=NativeScriptType_F32)return error(d,"Absorber creation requires Float coordinates/radius");
    NativeFighterLifetime saved;if(!capture(c,&saved,d)||!saved.source_owned||!nativeScriptBindings_SourceFighterLayout(c,d))return false;
    Fighter* f=saved.data;const NativeSourcePackage* p=nativeFighterRuntime_SourcePackage(f->kind);
    if(!p||!p->ready||p->address!=p||!p->assets.ready||p->target!=f->kind||
       p->program.bank!=saved.program->bank||!ftPartsTable||ftPartsTable[f->kind]!=&p->assets.parts||
       !f->parts||!p->assets.parts.parts_num||p->assets.parts.parts_num>MAX_FT_PARTS||
       p->assets.parts.parts_num!=p->assets.facts.joints+p->assets.facts.omission_count||
       p->assets.parts.part_to_joint!=p->assets.part_to_joint||p->assets.parts.joint_to_part!=p->assets.joint_to_part)
        return error(d,"Absorber imported source rig is malformed or unbound");
    int32_t bone=nativeFighterRig_Resolve(&p->assets.rig,args[0].as.i64);
    // The reference language admission intentionally makes numeric indices
    // outside the owned span a healthy no-op. An admitted missing joint is an
    // invalid rig, rather than permission to dereference an absent bone.
    if(bone>=0&&(uint32_t)bone<p->assets.parts.parts_num) {
        if(!f->parts[bone].joint)return error(d,"Absorber admitted raw joint is absent");
        AbsorbDesc shape={.x0_bone_id=bone,.x4_offset={args[1].as.f32,args[2].as.f32,args[3].as.f32},.x10_size=args[4].as.f32};
        ftColl_CreateAbsorbHit(c->source,&shape);
    }
    if(!nativeFighterLifetime_Current(&saved))return error(d,"Absorber source retired after creation");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
