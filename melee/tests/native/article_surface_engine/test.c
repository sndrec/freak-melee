// Actual extracted production bodies; controlled allocator and effect sink.
#include <melee/it/types.h>
#include <melee/it/itmaplib.h>
#include <melee/it/inlines.h>
#include <melee/mp/forward.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/types.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/lb/lbvector.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/debug.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#undef NDEBUG
#include <assert.h>

static unsigned checks, cases, allocations, releases, dispatches, dirty;
#define CHECK(x) do { ++checks; assert(x); } while (0)
HSD_ObjAllocData efAsync_AllocData;
EF_EffectUsage efLib_Usage;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
static EF_QueuedEffect pool[8];
static bool used[8], fail_allocate;
static HSD_GObj* effect_owner;
static HSD_JObj* effect_joint;
static unsigned effect_id, effect_mode;
static float effect_parameter;

void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
    CHECK(data == &efAsync_AllocData); ++allocations;
    if (fail_allocate) return NULL;
    for (unsigned i = 0; i < 8; ++i) if (!used[i]) {
        used[i] = true; memset(&pool[i], 0xA5, sizeof(pool[i])); return &pool[i];
    }
    assert(!"fixture pool exhausted"); return NULL;
}
void HSD_ObjFree(HSD_ObjAllocData* data, void* object)
{
    CHECK(data == &efAsync_AllocData);
    for (unsigned i = 0; i < 8; ++i) if (object == &pool[i]) {
        CHECK(used[i]); used[i] = false; ++releases; return;
    }
    assert(!"foreign allocation");
}
EF_SpawnResult efSync_Spawn(s32 id, HSD_GObj* owner, ...)
{
    va_list args; va_start(args, owner);
    effect_owner = owner; effect_joint = va_arg(args, HSD_JObj*);
    effect_id = id; ++dispatches;
    if (effect_mode == 3) effect_parameter = *va_arg(args, float*);
    va_end(args); return (EF_SpawnResult){0};
}
void (HSD_JObjSetMtxDirty)(HSD_JObj* joint) { CHECK(joint != NULL); ++dirty; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { CHECK(joint != NULL); ++dirty; }
void __assert(char* file, u32 line, char* message)
{ fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,message);abort(); }
void OSReport(char* format, ...)
{ va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args); }
HSD_JObj* it_80272CC0(Item_GObj* gobj, enum_t index)
{ CHECK(index == 0); return gobj->hsd_obj; }
// Authored deferred model-effect branch is not exercised by legacy item packets.
bool nativeCallbackSource_Snapshot(HSD_GObj* object, uint32_t kind, uint32_t* id)
{ (void)object; (void)kind; (void)id; assert(!"unexpected authored queue branch"); return false; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ (void)kind; assert(!"unexpected authored queue branch"); return NULL; }
void nativeModelEffect_Spawn(Fighter_GObj* object, const NativeFighterModelEffect* policy,
                            s32 part, Vec3* position, bool deferred)
{ (void)object;(void)policy;(void)part;(void)position;(void)deferred;assert(!"unexpected authored branch"); }
void lb_8000B1CC(HSD_JObj* joint, Vec3* offset, Vec3* output)
{ (void)joint;(void)offset;(void)output;assert(!"unexpected position-based queue mode"); }
void Camera_RequestQuake(int id, Vec3* position)
{ (void)id;(void)position;assert(!"unexpected camera queue mode"); }

#include "actual_bodies.h"

static void close_float(float actual, float expected)
{ CHECK(fabsf(actual-expected) <= 2e-6f*fmaxf(1, fmaxf(fabsf(actual),fabsf(expected)))); }
static void surfaces(void)
{
    HSD_GObj object = {0}; Item item = {0}; ItemAttr attr = {0}; HSD_JObj joint = {0};
    object.user_data = &item; object.hsd_obj = &joint; item.xCC_item_attr = &attr;
    CollData* c = &item.x378_itemColl;
    c->left_facing_wall.index=31; c->right_facing_wall.index=47; c->ceiling.index=59;
    c->floor.index=61;
    for (unsigned mask=0; mask<16; ++mask) {
        c->env_flags = (mask&1 ? Collide_LeftWallMask : 0) |
            (mask&2 ? Collide_RightWallMask : 0) |
            (mask&4 ? Collide_CeilingMask : 0) | (mask&8 ? Collide_FloorMask : 0);
        item.xC30=-123;
        CHECK(it_80276308(&object)==(mask&2 ? 4 : mask&1 ? 8 : 0));
        CHECK(item.xC30==(mask&2 ? 47 : mask&1 ? 31 : -123));
        item.xC30=-123; CHECK(it_802763E0(&object)==(mask&4 ? 2 : 0));
        CHECK(item.xC30==(mask&4 ? 59 : -123)); ++cases;
    }
    const float restitution[] = {0,.25f,1,1.5f};
    for (unsigned wall=0; wall<4; ++wall) for (unsigned r=0;r<4;++r)
    for (int direction=-1;direction<=1;++direction) {
        c->env_flags=(wall&1 ? Collide_LeftWallMask:0)|(wall&2 ? Collide_RightWallMask:0);
        c->left_facing_wall.normal=(Vec3){1,0,0};
        c->right_facing_wall.normal=(Vec3){-1,0,0};
        item.x40_vel=(Vec3){direction*4.0f,3,2}; item.xC30=-99; attr.x58=restitution[r];
        float vx=item.x40_vel.x, vy=3, vz=2;
        if (wall) { float nx=wall&2 ? -1:1;
            if (vx*nx<0) vx=-vx; vx*=attr.x58;vy*=attr.x58;vz*=attr.x58; }
        CHECK(it_8027770C(&object)==(wall!=0));
        close_float(item.x40_vel.x,vx);close_float(item.x40_vel.y,vy);close_float(item.x40_vel.z,vz);
        CHECK(item.xC30==(wall&2 ? 47 : wall&1 ? 31:-99)); ++cases;
    }
    for (unsigned axis=0;axis<3;++axis) for(int face=-1;face<=1;face+=2)
    for (int slope=-2;slope<=2;++slope) for(unsigned floor=0;floor<2;++floor) {
        float x=slope*.25f,y=sqrtf(1-x*x);
        c->env_flags=floor ? Collide_FloorMask:0; c->floor.normal=(Vec3){x,y,0};
        item.facing_dir=face; item.xDC8_word.flags.x17=axis;
        joint.flags=JOBJ_MTX_INDEP_SRT; joint.rotate=(Quaternion){.x=7,.y=8,.z=9};
        it_80276CB8(&object);
        float angle=face*acosf(y)*(x<0?-1:1);
        close_float(joint.rotate.x,floor&&axis==1 ? angle:7);
        close_float(joint.rotate.y,floor&&axis==2 ? angle:8);
        close_float(joint.rotate.z,floor&&axis==0 ? -face*angle:9); ++cases;
    }
    // Dot-product gate includes Z, while the actual shared Mirror changes only XY.
    // This deliberately checks that existing engine asymmetry, not a 3D rewrite.
    const Vec3 normals[]={{.6f,.8f,0},{-.6f,.8f,0},{0,0,1},{0,-1,0}};
    const Vec3 velocities[]={{-4,3,2},{4,-3,-2},{0,0,-2},{0,0,2},{0,0,0}};
    for(unsigned n=0;n<4;++n) for(unsigned v=0;v<5;++v) for(unsigned r=0;r<4;++r) {
        c->env_flags=Collide_LeftWallMask; c->left_facing_wall.normal=normals[n];
        Vec3 original=velocities[v],expected=original;item.x40_vel=original;attr.x58=restitution[r];
        Vec3 normal=normals[n];float dot=original.z*normal.z+original.x*normal.x+original.y*normal.y;
        if(dot<0) { float factor=(normal.x*expected.x+normal.y*expected.y)*-2;
            expected.x+=normal.x*factor;expected.y+=normal.y*factor; }
        expected.x*=attr.x58;expected.y*=attr.x58;expected.z*=attr.x58;
        CHECK(it_8027770C(&object));close_float(item.x40_vel.x,expected.x);
        close_float(item.x40_vel.y,expected.y);close_float(item.x40_vel.z,expected.z);++cases;
    }
}

static void packets(void)
{
    Item item={0}; HSD_GObj object={.user_data=&item}; HSD_JObj root={0},child={0},sibling={0};
    root.child=&child;child.next=&sibling;object.hsd_obj=&root;
    HSD_GObjProc proc={.s_link=2}; HSD_GObj_CurrentInvokedProc=&proc;
    const char* paths[]={"","0","1"}; HSD_JObj* joints[]={&root,&child,&sibling};
    EF_QueuedEffect* head=(EF_QueuedEffect*)&item.xBC0;
    for(unsigned mode=0;mode<=3;mode+=3) for(unsigned path=0;path<3;++path) {
        unsigned before=dispatches, freed=releases; item.facing_dir=-.75f;effect_mode=mode;
        nativeArticleModel_SpawnAsyncEffect(&object,777+path,paths[path],mode);
        EF_QueuedEffect* q=head->next; CHECK(q!=NULL);CHECK(dispatches==before);
        CHECK(q->spawn_kind==mode);CHECK(q->gfx_id==777+(int)path);CHECK(q->jobj==joints[path]);
        CHECK(q->native_policy==NULL);CHECK(q->native_author==NULL);CHECK(q->native_creator_generation==0);
        if(mode==3) close_float(q->params.x,-.75f);
        item.facing_dir=1.25f; // Packet must own the scalar; no retained facing pointer.
        efAsync_QueueFlush(&object,head);
        CHECK(head->next==NULL);CHECK(dispatches==before+1);CHECK(releases==freed+1);
        CHECK(effect_owner==&object);CHECK(effect_joint==joints[path]);CHECK(effect_id==777+path);
        if(mode==3) close_float(effect_parameter,-.75f);++cases;
    }
    unsigned before=allocations;
    nativeArticleModel_SpawnAsyncEffect(NULL,1,"",0);
    object.user_data=NULL;nativeArticleModel_SpawnAsyncEffect(&object,1,"",0);object.user_data=&item;
    nativeArticleModel_SpawnAsyncEffect(&object,-1,"",0);
    nativeArticleModel_SpawnAsyncEffect(&object,1,"",1);
    nativeArticleModel_SpawnAsyncEffect(&object,1,"2",0);
    nativeArticleModel_SpawnAsyncEffect(&object,1,"0/",0);
    nativeArticleModel_SpawnAsyncEffect(&object,1,NULL,0);
    object.hsd_obj=NULL;nativeArticleModel_SpawnAsyncEffect(&object,1,"",0);object.hsd_obj=&root;
    CHECK(allocations==before); cases+=8;
    fail_allocate=true;nativeArticleModel_SpawnAsyncEffect(&object,1,"",0);
    CHECK(head->next==NULL);fail_allocate=false;++cases;
    // Queue prepends; clear releases without dispatch; late s_link and NULL proc dispatch immediately.
    nativeArticleModel_SpawnAsyncEffect(&object,10,"0",0);
    nativeArticleModel_SpawnAsyncEffect(&object,11,"1",0);
    CHECK(head->next->gfx_id==11);CHECK(head->next->next->gfx_id==10);
    before=dispatches;efAsync_QueueClear(head);CHECK(dispatches==before);CHECK(head->next==NULL);++cases;
    for(unsigned late=0;late<2;++late) {
        proc.s_link=9;HSD_GObj_CurrentInvokedProc=late ? NULL:&proc;effect_mode=3;
        before=dispatches;item.facing_dir=-1;
        nativeArticleModel_SpawnAsyncEffect(&object,123,"0",3);
        CHECK(dispatches==before+1);CHECK(head->next==NULL);close_float(effect_parameter,-1);++cases;
    }
    for(unsigned i=0;i<8;++i) CHECK(!used[i]);
}
int main(void)
{
    surfaces();packets();
    printf("{\"checks\":%u,\"cases\":%u,\"allocations\":%u,\"releases\":%u,\"effect_dispatches\":%u}\n",
        checks,cases,allocations,releases,dispatches);
    return 0;
}
