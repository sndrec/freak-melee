#include "native_script_binding_vectors.h"
#include "native_script_bindings.h"
#include "native_callback_lifetime.h"
#include <melee/lb/lbvector.h>
#include <stdio.h>
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
bool nativeScriptBindingVectors_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!c||!out||!args||(c->source_class!=NativeCallbackSource_Fighter&&c->source_class!=NativeCallbackSource_Item)||
       !nativeScriptBindings_SourceAlive(c))return error(d,"Vector source is unavailable or retired");
    if(semantic==NativeScriptBinding_VectorsRotate) {
        if(count!=8)return error(d,"Vector rotation requires XYZ, axis, angle and three outputs");
        for(uint32_t i=0;i<count;++i)
            if(args[i].type!=(i==3?NativeScriptType_I32:NativeScriptType_F32))
                return error(d,"Vector rotation needs exact float components and an integer axis");
        // lbVector_Rotate leaves its result uninitialized for other axis codes.
        // Reject those before calling it or publishing any output.
        int axis=(int)args[3].as.i64;
        if(args[3].as.i64!=axis||(axis!=1&&axis!=2&&axis!=4))
            return error(d,"Vector rotation axis must be 1 (X), 2 (Y) or 4 (Z)");
        void* source=c->source;uint32_t generation=c->generation,source_class=c->source_class;
        Vec3 vector={args[0].as.f32,args[1].as.f32,args[2].as.f32};
        lbVector_Rotate(&vector,axis,args[4].as.f32);
        if(c->source!=source||c->generation!=generation||c->source_class!=source_class||!nativeScriptBindings_SourceAlive(c))
            return error(d,"Vector rotation caller retired before result publication");
        args[5].as.f32=vector.x;args[6].as.f32=vector.y;args[7].as.f32=vector.z;
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    bool normalize=semantic==NativeScriptBinding_VectorsNormalize||semantic==NativeScriptBinding_VectorsNormalizeXY;
    if((!normalize&&(semantic<NativeScriptBinding_VectorsAngleXY||semantic>NativeScriptBinding_VectorsCrossNormalizedZ))||
       count!=(normalize?3u:semantic==NativeScriptBinding_VectorsAngleXY?4u:6u))return error(d,"Vector operation/argument count is invalid");
    for(uint32_t i=0;i<count;++i)if(args[i].type!=NativeScriptType_F32)return error(d,"Vector input must be an exact F32 value");
    void* source=c->source;uint32_t generation=c->generation,source_class=c->source_class;
    Vec3 a={args[0].as.f32,args[1].as.f32,count==4?0:args[2].as.f32};
    if(normalize) {
        if(semantic==NativeScriptBinding_VectorsNormalizeXY)lbVector_NormalizeXY(&a);
        else lbVector_Normalize(&a);
        if(c->source!=source||c->generation!=generation||c->source_class!=source_class||!nativeScriptBindings_SourceAlive(c))
            return error(d,"Vector caller retired before result publication");
        args[0].as.f32=a.x;args[1].as.f32=a.y;args[2].as.f32=a.z;
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    unsigned second=count==4?2:3;
    Vec3 b={args[second].as.f32,args[second+1].as.f32,count==4?0:args[second+2].as.f32};
    float value;
    if(semantic==NativeScriptBinding_VectorsAngleXY)value=lbVector_AngleXY(&a,&b);
    else if(semantic==NativeScriptBinding_VectorsAngle)value=lbVector_Angle(&a,&b);
    else {
        Vec3 cross;lbVector_CrossprodNormalized(&a,&b,&cross);
        value=semantic==NativeScriptBinding_VectorsCrossNormalizedX?cross.x:
            semantic==NativeScriptBinding_VectorsCrossNormalizedY?cross.y:cross.z;
    }
    if(c->source!=source||c->generation!=generation||c->source_class!=source_class||!nativeScriptBindings_SourceAlive(c))
        return error(d,"Vector caller retired before result publication");
    *out=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};return true;
}
