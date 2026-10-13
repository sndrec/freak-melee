// Actual old45 and current common prediction loops. Geometry and registry are
// controlled boundaries; predicate order, vectors, RNG seed and first hit match.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/it/types.h>
#include <melee/lb/lbcollision.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GET_FIGHTER(o) ((Fighter*)(o)->user_data)

static unsigned checks, pairs, direct_pairs;
static Fighter fighter, other;
static Item projectile;
static HSD_GObj source, owner;
static const NativeFighterDefinition* definition;
static unsigned reject_mode, immunity_mask, geometry_mask, count;
typedef struct Probe {
    unsigned operation, slot;
    Vec3 positions[4];
    float scale, radius;
} Probe;
static Probe probes[16];
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"%s:%d: %s\n", \
    __FILE__,__LINE__,#c); exit(1); } } while (0)
static unsigned slot(HitCapsule* hit)
{
    for(unsigned i=0;i<4;++i) if(hit==&projectile.x5D4_hitboxes[i].hit) return i;
    CHECK(false); return 0;
}
bool ftLib_80086960(HSD_GObj* object)
{ CHECK(object==&owner || object==NULL); return object==&owner; }
bool ftCo_IsAlly(Fighter* f, Fighter* o)
{ CHECK(f==&fighter && o==&other); return reject_mode==2; }
bool ftCo_800A5980(Fighter* object)
{ CHECK(object==(Fighter*)&projectile); return reject_mode==5; }
bool lbColl_8000ACFC(void* object, HitCapsule* hit)
{
    CHECK(object==&fighter && count<16);
    unsigned i=slot(hit); probes[count++]=(Probe){.operation=1,.slot=i};
    return (immunity_mask&(1u<<i))!=0;
}
bool lbColl_80006094(Vec3* a,Vec3* b,Vec3* c,Vec3* d,Vec3* e,Vec3* f,
                    float scale,float radius)
{
    unsigned i=4;
    for(unsigned j=0;j<4;++j) if(a==&projectile.x5D4_hitboxes[j].hit.x4C) i=j;
    CHECK(i<4 && count<16);
    probes[count++]=(Probe){.operation=2,.slot=i,.positions={*a,*b,*c,*d},
                            .scale=scale,.radius=radius};
    *e=(Vec3){1,2,3}; *f=(Vec3){4,5,6};
    return (geometry_mask&(1u<<i))!=0;
}
#include "random_original.h"
#define ftCo_CpuPredictHitboxPosition old_predict
#define ftCo_800BB220 old_prediction
#include "projectile_original.h"
#undef ftCo_800BB220
#undef ftCo_CpuPredictHitboxPosition
static void stock_slot(HSD_GObj* gobj) { (void)gobj; }
#include "base.h"
#include "behavior.generated.c"
static bool alive(HSD_GObj* object,uint32_t generation,uint32_t kind)
{
    return object==&source && source.user_data==&fighter &&
        generation==(uint32_t)fighter.x8_spawnNum && kind==NativeCallbackSource_Fighter;
}
bool nativeCallbackSource_Snapshot(HSD_GObj* object,uint32_t kind,uint32_t* generation)
{
    if(!generation || object!=&source || kind!=NativeCallbackSource_Fighter) return false;
    *generation=(uint32_t)fighter.x8_spawnNum;
    return alive(object,*generation,kind);
}
bool nativeCallbackSource_Alive(HSD_GObj* object,uint32_t generation,uint32_t kind)
{ return alive(object,generation,kind); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_Ness ? definition : NULL; }
#define ftCo_CpuPredictHitboxPosition current_predict
#define ftCo_800BB220 current_prediction
#include "projectile_current.h"
#undef ftCo_800BB220
#undef ftCo_CpuPredictHitboxPosition
static void unexpected(void) { CHECK(false); }
static void parameters(const char* path)
{
    FILE* file=fopen(path,"rb"); CHECK(file);
    char text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY+1];
    size_t size=fread(text,1,sizeof(text)-1,file); CHECK(feof(file)&&!ferror(file)); fclose(file);
    NativeFighterParameters p; CHECK(nativeFighterParameters_Parse(text,size,&p,NULL));
    uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for(unsigned i=0;i<p.count;++i) values[i]=p.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(values,p.count,p.schema_hash));
}
static void reset(int level,int kind,unsigned rejection,unsigned filter,unsigned blocked)
{
    memset(&fighter,0,sizeof(fighter)); memset(&other,0,sizeof(other));
    memset(&projectile,0,sizeof(projectile));
    source=(HSD_GObj){.user_data=&fighter,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    owner=(HSD_GObj){.user_data=&other,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    fighter.gobj=&source; fighter.kind=kind; fighter.cpu.level=level;
    fighter.x8_spawnNum=0x80000099u; fighter.pos_delta=(Vec3){.2f,-.3f,.5f};
    projectile.owner=rejection==1?&source:rejection==2?&owner:NULL;
    projectile.kind=rejection==3?It_Kind_Star:rejection==4?It_Kind_Unk4:It_Kind_Ness_PKFire;
    reject_mode=rejection; immunity_mask=filter==5?5:0;
    for(unsigned i=0;i<4;++i) {
        HitCapsule* h=&projectile.x5D4_hitboxes[i].hit;
        h->state=HitCapsule_Enabled+1;
        h->x4C=(Vec3){1.1f+i,2.2f+i,-.1f*i};
        h->x58=(Vec3){.5f+i,.7f+i,.3f*i};
        h->scale=.5f+.1f*i; h->x42_b0=blocked;
        if(filter==1 && i<3) h->state=HitCapsule_Disabled;
        if(filter==2 && i<3) h->state=HitCapsule_Enabled;
        if(filter==3 && i<3) h->x43_b2=true;
        if(filter==4 && i<3) h->element=HitElement_Inert;
    }
    count=0; memset(probes,0,sizeof(probes));
}
int main(int argc,char** argv)
{
    CHECK(argc==2); NativeFighterHost host={0};
#include "required_host.h"
    host.callback_source_alive=alive;
    definition=FreakMelee_GetFighterModule(&ftNessClone_Definition,&host); CHECK(definition);
    parameters(argv[1]);
    const uint32_t seeds[]={1,0xFFFFFFFFu,0x80000001u,17,7193};
    for(int level=1;level<=9;++level) for(unsigned kind=0;kind<2;++kind)
      for(unsigned s=0;s<5;++s) for(unsigned rejection=0;rejection<6;++rejection)
      for(unsigned filter=0;filter<6;++filter) for(unsigned geometry=0;geometry<5;++geometry)
      for(unsigned blocked=0;blocked<2;++blocked) {
        int fighter_kind=kind?Ft_Kind_Mario:Ft_Kind_Ness;
        Vec3 start={7,13,-4};
        reset(level,fighter_kind,rejection,filter,blocked);
        geometry_mask=geometry==4?0:1u<<geometry; seed=seeds[s];
        int expected=old_prediction(&fighter,&projectile,&start,3.25f);
        uint32_t final_seed=seed; unsigned expected_count=count;
        Probe expected_probes[16]; memcpy(expected_probes,probes,sizeof(probes));
        reset(level,fighter_kind,rejection,filter,blocked); seed=seeds[s];
        int actual=current_prediction(&fighter,&projectile,&start,3.25f);
        CHECK(actual==expected); CHECK(seed==final_seed); CHECK(count==expected_count);
        CHECK(!memcmp(probes,expected_probes,sizeof(probes))); ++pairs;
      }
    // Legal stock levels>3 cannot predict21+ frames. Exercise edited query inputs
    // directly rather than manufacturing an out-of-range RNG result.
    for(int level=1;level<=9;++level) for(int frames=0;frames<=32;++frames)
      for(unsigned blocked=0;blocked<2;++blocked) {
        reset(level,Ft_Kind_Ness,0,0,blocked);
        int actual=definition->on_cpu_projectile_hit(&source,frames,blocked);
        CHECK(actual==(level>3 && frames>=21 && blocked?3:2)); ++direct_pairs;
      }
    printf("Ness composed projectile: %u checks, 0 failures, %u source pairs, %u direct query pairs\n",
           checks,pairs,direct_pairs);
    return 0;
}
