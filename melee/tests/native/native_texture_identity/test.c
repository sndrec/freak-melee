#include <melee/gm/native_texture_identity_snapshot.h>
#include <dolphin/gx/GXTexture.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern uint32_t sNextTexObjId,sNextTlutObjId;
uint32_t next_tex_obj_id(void);
uint32_t next_tlut_obj_id(void);
static bool loop_idle=true,script_idle=true;
static unsigned checks;
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"line %u: %s\n",__LINE__,#c);exit(1);}}while(0)
bool nativeGmLoop_SnapshotBoundary(void) {return loop_idle;}
bool nativeScriptRuntime_Idle(void) {return script_idle;}
int main(void)
{
    CHECK(sizeof(NativeTextureIdentitySnapshot)==8);
    GXTextureStateRegion regions[2];CHECK(GXGetTextureStateRegions(regions,2)==2);
    CHECK(regions[0].memory==&sNextTexObjId && regions[1].memory==&sNextTlutObjId);
    CHECK(regions[0].size==4 && regions[1].size==4);
    NativeTextureIdentitySnapshot saved,end,repeat,unchanged={999,888};size_t bytes=777;
    CHECK(nativeTextureIdentity_Save(&saved,sizeof(saved),&bytes));
    CHECK(bytes==8 && saved.texture==1 && saved.palette==1);
    uint32_t identities[256];
    for(unsigned pass=0;pass<2;++pass) {
        CHECK(nativeTextureIdentity_Load(&saved,sizeof(saved)));
        for(unsigned i=0;i<128;++i) {
            uint32_t texture=next_tex_obj_id(),palette=next_tlut_obj_id();
            if(!pass) {identities[i*2]=texture;identities[i*2+1]=palette;}
            else CHECK(texture==identities[i*2] && palette==identities[i*2+1]);
        }
        CHECK(nativeTextureIdentity_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)) && end.texture==129 && end.palette==129);
    bytes=777;
    CHECK(!nativeTextureIdentity_Save(&unchanged,7,&bytes));
    CHECK(bytes==777 && unchanged.texture==999 && unchanged.palette==888);
    CHECK(!nativeTextureIdentity_Save(&sNextTexObjId,8,&bytes));
    CHECK(!nativeTextureIdentity_Save(&unchanged,sizeof(unchanged),(size_t*)&sNextTlutObjId));
    CHECK(!nativeTextureIdentity_Save(&unchanged,sizeof(unchanged),(size_t*)&unchanged));
    CHECK(!nativeTextureIdentity_Validate(&sNextTexObjId,8));
    CHECK(!nativeTextureIdentity_Load(&saved,7));
    CHECK(sNextTexObjId==129 && sNextTlutObjId==129);
    loop_idle=false;CHECK(!nativeTextureIdentity_Save(&unchanged,8,&bytes));
    CHECK(!nativeTextureIdentity_Load(&saved,8));loop_idle=true;
    script_idle=false;CHECK(!nativeTextureIdentity_Validate(&saved,8));
    CHECK(!nativeTextureIdentity_Load(&saved,8));script_idle=true;
    CHECK(!nativeTextureIdentity_Validate(NULL,8));
    CHECK(!nativeTextureIdentity_Save(&unchanged,8,NULL));
    CHECK(!nativeTextureIdentity_Load((const void*)(UINTPTR_MAX-3),8));
    CHECK(unchanged.texture==999 && unchanged.palette==888 && bytes==777);
    // Preserve the source identity allocation/exhaustion contract exactly.
    NativeTextureIdentitySnapshot edge={UINT32_MAX,UINT32_MAX};
    CHECK(nativeTextureIdentity_Load(&edge,8));
    CHECK(next_tex_obj_id()==UINT32_MAX && next_tlut_obj_id()==UINT32_MAX);
    CHECK(nativeTextureIdentity_Save(&end,8,&bytes));
    CHECK(end.texture==0 && end.palette==0);
    CHECK(nativeTextureIdentity_Load(&saved,8));CHECK(nativeTextureIdentity_Load(&end,8));
    CHECK(sNextTexObjId==0 && sNextTlutObjId==0);
    printf("{\"checks\":%u,\"bytes\":8,\"repeated_identity_pairs\":128}\n",checks);
    return 0;
}
