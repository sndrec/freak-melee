#undef NDEBUG
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/lb/lb_00B0.h>
#ifdef CAPSULE_STATE_ROUTE
#include "generated_state.inc"
#else
#include "generated.inc"
#endif
#include "actual.inc"

static unsigned calls;
static int32_t probe_index, read_index;
static s32 random_integer(s32 bound)
{
    ++calls;
    if (calls == 1) { assert(bound == 8); return probe_index; }
    if ((uint32_t)probe_index < 4 && calls == 2) { assert(bound == 100); return 17; }
    assert(bound == 8); return read_index;
}
// Controlled joint-to-collision-space boundary for the complete update body.
void lb_8000B1CC(HSD_JObj* joint, Vec3* offset, Vec3* world)
{
    assert(joint); world->x=joint->translate.x+offset->x;
    world->y=joint->translate.y+offset->y;world->z=joint->translate.z+offset->z;
}
static unsigned offset(const NativeFighterDefinition* def, const char* name)
{
    for (unsigned i=0;i<def->state_field_count;++i)
        if (!strcmp(name,def->state_fields[i].name)) return def->state_fields[i].offset;
    assert(0);return 0;
}
static void set(const NativeFighterDefinition* def,Fighter* fighter,const char* name,const void* value)
{ memcpy(fighter->native_module_state+offset(def,name),value,4); }
static float get(const NativeFighterDefinition* def,Fighter* fighter,const char* name)
{ float value;memcpy(&value,fighter->native_module_state+offset(def,name),4);return value; }
int main(void)
{
    Fighter* fighter=calloc(1,sizeof(*fighter));assert(fighter);
    HSD_GObj object={.user_data=fighter};HSD_JObj joint={0};
    fighter->x8_spawnNum=UINT32_C(0xFEDCBA98);
    MotionState motion={0};NativeFighterDefinition base={.code_key="capsule_positions",
        .fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.random_int=random_integer};
    const NativeFighterDefinition* def=FreakMelee_GetFighterModule(&base,&host);assert(def);
#ifdef CAPSULE_STATE_ROUTE
    assert(def->motion_state_first == 341 && def->motion_state_count == 1);
    HSD_GObjEvent callback = def->motion_states[0].phys_cb;
#else
    HSD_GObjEvent callback = def->on_tick;
#endif
    assert(callback);
    const int32_t indices[]={-1,0,1,2,3,4,INT32_MIN,INT32_MAX,(int32_t)UINT32_C(0xFEDCBA98)};
    const float values[]={-7,0,3.5f,FLT_MAX,NAN,INFINITY,-INFINITY};unsigned cases=0,checks=0;
    for(unsigned ii=0;ii<sizeof(indices)/sizeof(*indices);++ii)
      for(unsigned v=0;v<sizeof(values)/sizeof(*values);++v)
        for(unsigned invalid_probe=0;invalid_probe<2;++invalid_probe) {
        HitCapsule expected[4];
        for(unsigned h=0;h<4;++h) {
            memset(&fighter->x914[h],0,sizeof(HitCapsule));HitCapsule* hit=&fighter->x914[h];
            hit->state=HitCapsule_Unk3;hit->jobj=&joint;hit->b_offset=(Vec3){9,10,11};
            hit->x4C=(Vec3){(float)(h+1),-2,3};hit->x58=(Vec3){4,(float)(h+5),6};
            hit->damage=12;hit->x4=57;hit->x44=3;hit->x45=2;hit->victims_1[0].victim=&object;
            expected[h]=*hit;
        }
        int32_t index=indices[ii];float value=values[v];
        set(def,fighter,"index",&index);set(def,fighter,"value",&value);
        if ((uint32_t)index<4) for(unsigned period=0;period<2;++period) {
            Vec3* vector=period?&expected[index].x58:&expected[index].x4C;
            float* components=&vector->x;
            for(unsigned axis=0;axis<3;++axis) {float coordinate=value+(float)(period*3+axis);if(isfinite(coordinate)) components[axis]=coordinate;}
        }
        float readings[6]={0};
        if ((uint32_t)index<4) {memcpy(readings,&expected[index].x4C,12);memcpy(readings+3,&expected[index].x58,12);}
        probe_index=invalid_probe?INT32_MIN:2;read_index=invalid_probe?INT32_MAX:1;calls=0;
        if (!invalid_probe) expected[2].x4C.x=17;
        float sample=invalid_probe?0:expected[1].x58.y;
        callback(&object);
        assert(calls==(invalid_probe?2:3));++checks;
        assert(!memcmp(expected,fighter->x914,sizeof(expected)));++checks;
        for(unsigned period=0;period<2;++period)for(unsigned axis=0;axis<3;++axis) {
            char name[32];snprintf(name,sizeof(name),"%s_%c",period?"previous":"current",'x'+axis);
            float read=get(def,fighter,name);assert(!memcmp(&read,&readings[period*3+axis],4));++checks;
        }
        assert(get(def,fighter,"sample")==sample);++checks;++cases;
    }
    HitCapsule capsule={0};capsule.jobj=&joint;capsule.x4C=(Vec3){1,2,3};capsule.x58=(Vec3){4,5,6};
    capsule.b_offset=(Vec3){7,8,9};capsule.damage=10;capsule.state=HitCapsule_Unk3;
    HitCapsule expected=capsule;expected.jobj=NULL;Vec3 position={12,13,14};expected.b_offset=position;
    ftColl_8007B8A8(&capsule,&position);assert(!memcmp(&expected,&capsule,sizeof(capsule)));++checks;
    for(unsigned state=0;state<4;++state)for(unsigned compensate=0;compensate<2;++compensate) {
        memset(&capsule,0,sizeof(capsule));capsule.state=(HitCapsuleState)state;capsule.jobj=&joint;
        capsule.x4C=(Vec3){1,2,3};capsule.x58=(Vec3){4,5,6};capsule.b_offset=(Vec3){2,4,6};
        capsule.x43_b1=compensate;fighter->x34_scale.y=2;joint.translate=(Vec3){10,20,30};
        ftColl_8007AD18(fighter,&capsule);
        if(state==HitCapsule_Disabled) {assert(capsule.x4C.x==1&&capsule.x58.x==4);}
        else {float scale=compensate?0.5f:1;
            assert(capsule.x4C.x==10+2*scale&&capsule.x4C.y==20+4*scale&&capsule.x4C.z==30+6*scale);
            if(state==HitCapsule_Enabled) assert(!memcmp(&capsule.x4C,&capsule.x58,12));
            else assert(capsule.x58.x==1&&capsule.x58.y==2&&capsule.x58.z==3);
        }
        ++checks;
    }
    free(fighter);
    printf("PASS %u generated coordinate cases/%u checks; exact all-axis fields, invalid/full32 indices, finite writes, expression order/count, capsule neighbors; actual AD18/B8A8 bodies\n",cases,checks);
    return 0;
}
