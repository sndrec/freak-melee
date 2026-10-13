#include <melee/gm/native_match_control_history.h>
#include "native_material_texture_snapshot.h"
#include "mobj.h"
#include "native_gobj_pool_storage.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <string.h>

static bool boundary(void)
{return nativeGObjPool.lifetime && nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle();}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool independent(const void* data,size_t bytes)
{
    if(!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data ||
       overlap(data,bytes,&tobj_shadows,sizeof(tobj_shadows)) ||
       overlap(data,bytes,&tobj_toon,sizeof(tobj_toon)))return false;
    HSD_TObj* roots[2]={tobj_shadows,tobj_toon};
    for(unsigned i=0;i<2;++i)if(roots[i] && overlap(data,bytes,roots[i],sizeof(HSD_TObj)))return false;
    return true;
}
static bool live_chain(HSD_TObj* head,uint32_t* count)
{
    uint32_t n=0;
    for(HSD_TObj* node=head;node;node=node->next) {
        if(n==NATIVE_MATERIAL_SHADOW_LIMIT || !nativeClass_Reference(node,sizeof(*node)).bytes ||
           !nativeClass_TypeKnown(HSD_CLASS_METHOD(node),sizeof(*node)))return false;
        ++n;
    }
    *count=n;return true;
}
bool nativeMaterialTexture_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes!=sizeof(NativeMaterialTextureSnapshot) || !independent(data,bytes))return false;
    NativeMaterialTextureSnapshot s;memcpy(&s,data,sizeof(s));
    if(s.object_epoch!=nativeGObjPool.lifetime || s.reserved || s.shadows>NATIVE_MATERIAL_SHADOW_LIMIT ||
       (s.roots[0]!=NULL)!=(s.shadows!=0))return false;
    static const NativeClassReference zero={0};
    for(unsigned i=0;i<2;++i) {
        if(!s.roots[i]) {
            if(s.types[i] || memcmp(&s.backing[i],&zero,sizeof(zero)))return false;
        } else if(s.backing[i].bytes!=sizeof(HSD_TObj) || nativeClass_Resolve(s.backing[i])!=s.roots[i] ||
                  !nativeClass_TypeKnown(s.types[i],sizeof(HSD_TObj)) ||
                  overlap(data,bytes,s.roots[i],sizeof(HSD_TObj)) ||
                  overlap(data,bytes,s.types[i],s.types[i]->head.info_size))return false;
    }
    return true;
}
bool nativeMaterialTexture_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeMaterialTextureSnapshot);
    if(!boundary() || capacity<bytes || !independent(data,bytes) || !independent(written,sizeof(*written)) ||
       overlap(data,bytes,written,sizeof(*written)))return false;
    NativeMaterialTextureSnapshot s={0};s.object_epoch=nativeGObjPool.lifetime;
    s.roots[0]=tobj_shadows;s.roots[1]=tobj_toon;
    if(!live_chain(s.roots[0],&s.shadows))return false;
    for(unsigned i=0;i<2;++i)if(s.roots[i]) {
        s.backing[i]=nativeClass_Reference(s.roots[i],sizeof(HSD_TObj));
        if(!s.backing[i].bytes)return false;
        s.types[i]=HSD_CLASS_METHOD(s.roots[i]);
    }
    if(!nativeMaterialTexture_Validate(&s,sizeof(s)))return false;
    memcpy(data,&s,bytes);*written=bytes;return true;
}
bool nativeMaterialTexture_ValidateClass(const void* data,size_t bytes,NativeClassSnapshotView view)
{
    if(!nativeMaterialTexture_Validate(data,bytes))return false;
    NativeMaterialTextureSnapshot s;memcpy(&s,data,sizeof(s));
    for(unsigned i=0;i<2;++i)
        if(s.roots[i] && !nativeClass_SnapshotObjectType(view,s.backing[i],s.types[i]))return false;
    HSD_TObj* node=s.roots[0];
    for(uint32_t i=0;i<s.shadows;++i) {
        NativeClassReference backing=nativeClass_Reference(node,sizeof(HSD_TObj));
        const void* payload=nativeClass_ReadSnapshot(view,backing);
        if(!payload || nativeClass_ResolveSnapshot(view,backing)!=node)return false;
        HSD_TObj saved;memcpy(&saved,payload,sizeof(saved));
        if(!nativeClass_SnapshotObjectType(view,backing,HSD_CLASS_METHOD(&saved)))return false;
        node=saved.next;
    }
    return node==NULL;
}
bool nativeMaterialTexture_Load(const void* data,size_t bytes)
{
    if(!nativeMaterialTexture_Validate(data,bytes))return false;
    NativeMaterialTextureSnapshot s;memcpy(&s,data,sizeof(s));
    for(unsigned i=0;i<2;++i)
        if(s.roots[i] && HSD_CLASS_METHOD(s.roots[i])!=s.types[i])return false;
    uint32_t count;if(!live_chain(s.roots[0],&count) || count!=s.shadows)return false;
    // Class companions already restored the links/payloads. Restore roots
    // directly; add/delete would change those links or invoke source rendering.
    tobj_shadows=s.roots[0];tobj_toon=s.roots[1];return true;
}

#ifdef MELEE_NATIVE
bool nativeMaterialTexture_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(tobj_shadows);
    ROOT(tobj_toon);
#undef ROOT
    return true;
}
#endif
