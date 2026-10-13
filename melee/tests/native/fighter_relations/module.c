#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include RELATIONS_MODULE
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
__declspec(dllimport) int __stdcall VirtualFree(void*,size_t,unsigned long);
static void* pages;
static HSD_GObj* object;
static Fighter* fighter;
static const NativeFighterDefinition* definition;
static unsigned checks, mode, link_calls, copies, reads, releases;
static bool live, bone_found;
static uint32_t source_id;
#define PEER UINT32_C(0x80000003)
#define CHECK(v) do { ++checks; assert(v); } while (0)
static bool alive(HSD_GObj* source,uint32_t id,uint32_t cls)
{
    CHECK(source==object && id==source_id && cls==NativeCallbackSource_Fighter);
    return live && fighter->x8_spawnNum==source_id;
}
static uint32_t member(HSD_GObj* source,unsigned index) { CHECK(source==object && index==1); return PEER; }
static bool link(HSD_GObj* source,uint32_t id) { CHECK(source==object && (id==PEER || id==0)); ++link_calls; return true; }
static uint32_t linked(HSD_GObj* source) { CHECK(source==object); return PEER; }
static bool copy(HSD_GObj* source,uint32_t id) { CHECK(source==object && id==PEER); ++copies; return true; }
static bool bone(uint32_t id,unsigned part,Vec3* point)
{
    CHECK(id==PEER && part==2); ++reads;
    if (mode==1) { live=false; unsigned long old; CHECK(VirtualProtect(pages,65536,1,&old)); }
    if (mode==2) ++fighter->x8_spawnNum;
    *point=(Vec3){10,20,30}; return bone_found;
}
static void release(HSD_GObj* source) { CHECK(source==object); ++releases; }
static unsigned offset(const char* name)
{
    for (unsigned i=0; i<definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name,name)) return definition->state_fields[i].offset;
    assert(0); return 0;
}
static int32_t integer(const char* name) { int32_t v; memcpy(&v,fighter->native_module_state+offset(name),4); return v; }
static bool flag(const char* name) { return fighter->native_module_state[offset(name)]!=0; }
static float scalar(const char* name) { float v; memcpy(&v,fighter->native_module_state+offset(name),4); return v; }
static void set_scalar(const char* name,float v) { memcpy(fighter->native_module_state+offset(name),&v,4); }
static void reset(void)
{
    unsigned long old; CHECK(VirtualProtect(pages,65536,4,&old)); memset(pages,0,65536);
    object=pages; fighter=(Fighter*)((unsigned char*)pages+4096); object->user_data=fighter;
    source_id=UINT32_C(0x80000001); fighter->x8_spawnNum=source_id; live=true;
    link_calls=copies=reads=releases=0; fighter->x2219_b5=true; fighter->x221F_b3=true;
    fighter->cpu.x90=12; set_scalar("x",1); set_scalar("y",2); set_scalar("z",3);
}
int main(void)
{
    _Static_assert(sizeof(Fighter)<=65536-4096,"Protected Fighter allocation");
    pages=VirtualAlloc(NULL,65536,0x3000,4); CHECK(pages);
    MotionState motion={0}; NativeFighterDefinition base={.fighter_root="ftDataTest",
        .motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.family_member_id=member,.link_hitlag=link,.linked_hitlag_id=linked,
        .copy_collision_from=copy,.target_bone_position=bone,.request_hitlag_release=release,.callback_source_alive=alive};
    definition=FreakMelee_GetFighterModule(&base,&host); CHECK(definition && definition->on_cpu_pending_action);
    NativeFighterHost bad=host; bad.target_bone_position=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&bad));
    bad=host; bad.family_member_id=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&bad));
    CHECK(FreakMelee_GetFighterModule(&base,&host));
    for (mode=0; mode<3; ++mode) for (unsigned outcome=0; outcome<2; ++outcome) {
        reset(); bone_found=outcome!=0;
#if RELATIONS_CALLBACKS
        definition->motion_states[0].phys_cb(object);
#else
        definition->on_load(object);
#endif
        unsigned long old; CHECK(VirtualProtect(pages,65536,4,&old));
        CHECK((uint32_t)integer("peer")==PEER && (uint32_t)integer("linked")==PEER);
        CHECK(flag("bound") && flag("copied") && flag("active") && flag("incapacitated"));
        CHECK(copies==1 && reads==1 && link_calls==(mode?1:2) && releases==(mode?0:1));
        CHECK(integer("after")== (mode?0:99)); CHECK(flag("found")==(!mode && bone_found));
        CHECK(scalar("x")==(!mode && bone_found?10:1));
        CHECK(scalar("y")==(!mode && bone_found?20:2));
        CHECK(scalar("z")==(!mode && bone_found?30:3));
        CHECK(fighter->ground_or_air==GA_Air && fighter->x40==7.25f && fighter->cpu.x90==13 && fighter->cpu.xF8_b5);
    }
    reset(); CHECK(!definition->on_cpu_pending_action(object)); fighter->cpu.xF8_b5=true; CHECK(definition->on_cpu_pending_action(object));
    CHECK(VirtualFree(pages,0,0x8000)); printf("PASS %u generated paired relations checks\n",checks);
}
