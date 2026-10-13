// Actual generated complete modules vs extracted original policies/callers.
// Console only; allocation, animations, CPU world probes and item presentation
// are explicit recorded boundaries. Real callback generations use live plink.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/kinds/ftDonkey/ftdonkey_clone.h>
#include <melee/ft/kinds/ftKoopa/ftkoopa_clone.h>
#include <melee/ft/kinds/ftDonkey/types.h>
#include <melee/ft/kinds/ftKoopa/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftCommon/ftCo_Damage.h>
#include <melee/ft/kinds/ftCommon/ftCo_Throw.h>
#include <melee/ft/kinds/ftCommon/ftCo_Shouldered.h>
#include <melee/ft/kinds/ftCommon/ftCo_CargoWait.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftDonkey/ftdonkeyspecialn.h>
#include <melee/ft/kinds/ftMewtwo/ftmewtwospecialn.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/ftwalkcommon.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/mp/mpisland.h>
#include <melee/gm/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
typedef int (__cdecl* Parameters)(const uint32_t*,unsigned,uint64_t);
typedef struct ClosureEvent { int id,a,b; float x,y,z,w,v,t; } ClosureEvent;
static ClosureEvent events[128],expected[128];
static unsigned calls,expected_calls,comparisons;
static Fighter fighters[2],victims[2];
static HSD_GObj objects[2],caught[2],item;
static HSD_GObj* heads[32]; HSD_GObj** HSD_GObjPLinkHead;
static ftData data[2]; static struct ftCo_DatAttrs common_attrs[2];
static uint32_t raw_attrs[2][512],backup[2][512];
static mp_UnkStruct0 island;
static const NativeFighterDefinition* active;
static unsigned family,current_side;
static bool heavy,blocked,ranged,has_island,suppressed,ground_contact,air_contact;
static int knockback,walk_variant;
static Item_HoldKinds hold_kind;
static float rng;
static bool frames_remaining;
static unsigned invalidate_after_motion;
ftCommonData common_data; ftCommonData* p_ftCommonData=&common_data;
static void unexpected_host_call(void) { fprintf(stderr,"Unexpected closure boundary at %u family %u\n",comparisons,family);abort(); }
static void record(int id,int a,int b,float x,float y,float z,float w,float v,float t)
{ assert(calls<128); events[calls++]=(ClosureEvent){id,a,b,x,y,z,w,v,t}; }
static void simple(int id,int a,int b,float x) { record(id,a,b,x,0,0,0,0,0); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_CustomFirst?active:NULL; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object,uint32_t generation)
{ (void)object;(void)generation;unexpected_host_call();return NULL; }
bool nativeModelEffect_Snapshot(HSD_GObj* object,uint32_t* generation)
{ (void)object;(void)generation;unexpected_host_call();return false; }
float HSD_Randf(void) { simple(50,0,0,rng);return rng; }
void ftCo_800B463C(Fighter* f,u8 cmd) { (void)f;simple(51,cmd,0,0); }
void ftCo_800B46B8(Fighter* f,u8 cmd,u8 arg) { (void)f;simple(52,cmd,arg,0); }
bool ftCo_800A5A90(Fighter* f) { (void)f;return false; }
mp_UnkStruct0* mpIsland_8005AC14(Vec3* pos,float depth)
{ record(53,0,0,pos->x,pos->y,pos->z,depth,0,0);return has_island?&island:NULL; }
void mpIsland_8005ACE8(mp_UnkStruct0* selected,Vec3* left,Vec3* right)
{ assert(selected==&island);simple(54,0,0,0);*left=island.x8;*right=island.x14; }
bool ftCo_CpuRangedAttackUnsafe(Fighter* f,Fighter* target,float distance)
{ assert(target==f->cpu.x44);simple(55,target!=NULL,0,distance);return ranged; }
void ftCo_CpuAddAttackCandidate(Fighter* f,int cmd)
{ if(f->cpu.xEC<8U) f->cpu.xCC_array[f->cpu.xEC++]=cmd; }
bool ftCo_800A0FB0(Vec3* pos,int* line,u32* flags,Vec3* normal,int a,int b,int c,float x,float y,float tx,float ty,float offset)
{ (void)pos;(void)line;(void)flags;(void)normal;assert(a==-1&&b==-1&&c==-1&&offset==0);record(56,0,0,x,y,tx,ty,0,0);return blocked; }
// Raw floor contact and stage exclusion are separate controlled primitives.
bool mpCheckFloor(float x,float y,float tx,float ty,float offset,Vec3* p,int* line,u32* flags,Vec3* normal,int a,int b,int c,bool(*filter)(Fighter_GObj*,int),Fighter_GObj* owner)
{assert(a==-1&&b==-1&&c==-1&&offset==0&&!filter&&!owner);record(56,0,0,x,y,tx,ty,0,0);*p=(Vec3){0};*line=0;*flags=0;*normal=(Vec3){0,1,0};return blocked;}
bool ftCo_800A1B38(enum_t line){assert(line==0);return false;}
bool itIsHeavy(Item_GObj* object) { (void)object;return heavy; }
Item_HoldKinds itGetHoldKind(Item_GObj* object) { (void)object;return hold_kind; }
void ftAnim_80070FB4(Fighter_GObj* object,s32 a,s32 b) { (void)object;simple(1,a,b,0); }
void ftAnim_80070C48(Fighter_GObj* object,s32 a) { (void)object;simple(2,a,0,0); }
void ftAnim_80070CC4(Fighter_GObj* object,int a) { (void)object;simple(3,a,0,0); }
void ftAnim_800704F0(Fighter_GObj* object,int a,float value) { (void)object;simple(4,a,0,value); }
void ftParts_80074A4C(Fighter_GObj* object,int a,int b) { (void)object;simple(5,a,b,0); }
void ftCo_800BFFD0(Fighter* f,s32 color,u8 arg) { (void)f;simple(6,color,arg,0); }
static bool start_color(Fighter* f,enum_t color,bool arg) { ftCo_800BFFD0(f,color,arg);return true; }
FigaTree* ftData_80085E50(Fighter* f,enum_t motion)
{ (void)f;simple(7,motion,0,0);return (FigaTree*)(uintptr_t)motion; }
float lbAnim_8001E8F8(FigaTree* clip) { int motion=(int)(uintptr_t)clip;simple(8,motion,0,0);return (float)(motion-270); }
void it_8026B3F8(Article* data,s32 kind) { (void)data;assert(kind==It_Kind_Koopa_Flame); }
void ftCommon_8007D7FC(Fighter* f) { f->ground_or_air=GA_Ground;simple(9,0,0,0); }
void ftCommon_8007D5D4(Fighter* f) { f->ground_or_air=GA_Air;simple(10,0,0,0); }
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* donor)
{ (void)donor;GET_FIGHTER(object)->motion_id=motion;record(11,motion,flags,frame,rate,blend,GET_FIGHTER(object)->facing_dir,0,0);if(object==&objects[1]) { if(invalidate_after_motion==1)++fighters[1].x8_spawnNum;else if(invalidate_after_motion==2)objects[0].next=NULL; } }
void ftAnim_8006EBA4(Fighter_GObj* object) { (void)object;simple(12,0,0,0); }
void ftCo_8008DCE0(Fighter_GObj* object,int motion,float facing)
{ GET_FIGHTER(object)->motion_id=motion;simple(13,motion,0,facing); }
bool ftCo_8008E984(Fighter* f) { (void)f;simple(14,0,0,0);return suppressed; }
float ftCo_ScaleBy154(float value) { simple(15,0,0,value);return value; }
s32 ftCo_Damage_CalcKnockbackLevel(float value) { simple(16,0,0,value);return knockback; }
void ftCo_8008E908(Fighter_GObj* object,float facing) { (void)object;simple(17,0,0,facing); }
void Item_8026ABD8(Item_GObj* object,Vec3* speed,float arg)
{ assert(object==&item);record(18,0,0,speed->x,speed->y,speed->z,arg,0,0); }
void ft_80084DB0(Fighter_GObj* object) { (void)object;simple(19,0,0,0); }
void ft_80084F3C(Fighter_GObj* object) { (void)object;simple(20,0,0,0); }
bool ft_80082708(Fighter_GObj* object) { (void)object;simple(21,0,0,0);return ground_contact; }
GroundOrAir ft_80081D0C(Fighter_GObj* object) { (void)object;simple(22,0,0,0);return air_contact?GA_Air:GA_Ground; }
void ftWalkCommon_800DFCA4(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,float frame,float a,float b,float c,float d,float e,float f,float multiplier)
{ (void)object;assert(multiplier==1);record(23,motion,flags,frame,a,b,c,0,0);record(24,0,0,d,e,f,multiplier,0,0); }
FtWalkType ftWalkCommon_GetWalkType(Fighter_GObj* object) { (void)object;simple(25,0,0,0);return walk_variant; }
void ftCo_8009C5A4(Fighter_GObj* object,FtMotionId motion) { assert(object==&caught[current_side]);simple(26,motion,0,0); }
void ftCo_800DD398(Fighter_GObj* object,FtMotionId motion,FtMotionId victim_motion,float speed)
{ (void)object;simple(27,motion,victim_motion,speed); }
void ftCo_8009C640(Fighter_GObj* object,int motion)
{ assert(object==&caught[current_side]);simple(28,motion,0,0); }
void ftCommon_8007E2F4(Fighter* f,s16 mask) { f->x1A6A=mask;simple(29,mask,0,0); }
void ftMt_SpecialN_Shoot(Fighter_GObj* object) { assert(GET_FIGHTER(object)->kind!=Ft_Kind_Mewtwo); }
void ftCo_800DD724(Fighter_GObj* object) { (void)object;simple(30,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* object) { (void)object;simple(31,0,0,0);return frames_remaining; }
void ftCommon_8007D92C(Fighter_GObj* object) { (void)object;simple(32,0,0,0); }
static FighterKind animation_family(Fighter* f) { return f->kind; }
bool ft_800827A0(Fighter_GObj* object) { return ft_80082708(object); }
void ftDk_Init_8010D774(Fighter_GObj* object) { (void)object;unexpected_host_call(); }
void ftDk_SpecialN_DestroyAllEffects(Fighter_GObj* object) { (void)object;unexpected_host_call(); }
void efLib_PauseAll(HSD_GObj* object) { (void)object;unexpected_host_call(); }
void efLib_ResumeAll(HSD_GObj* object) { (void)object;unexpected_host_call(); }
#include "donkey_bowser_closure_source_fixture.h"
static const NativeFighterStateField* field(const char* name)
{ for(unsigned i=0;i<active->state_field_count;++i) if(!strcmp(active->state_fields[i].name,name)) return &active->state_fields[i];abort(); }
static void set_int(const char* name,int32_t value) { memcpy(fighters[1].native_module_state+field(name)->offset,&value,4); }
static void set_float(const char* name,float value) { memcpy(fighters[1].native_module_state+field(name)->offset,&value,4); }
static float get_float(const char* name) { float value;memcpy(&value,fighters[1].native_module_state+field(name)->offset,4);return value; }
static int get_int(const char* name) { int value;memcpy(&value,fighters[1].native_module_state+field(name)->offset,4);return value; }
static void reset(void)
{
    memset(fighters,0,sizeof(fighters));memset(victims,0,sizeof(victims));memset(data,0,sizeof(data));
    memset(events,0,sizeof(events));calls=expected_calls=0;current_side=0;
    for(unsigned side=0;side<2;++side) {
        memcpy(raw_attrs[side],closure_stock_attribute_words[family],sizeof(raw_attrs[side]));
        memcpy(backup[side],raw_attrs[side],sizeof(backup[side]));
        data[side].ext_attr=raw_attrs[side];data[side].x0=&common_attrs[side];
        common_attrs[side].weight_independent_throws_mask=15;
        fighters[side].ft_data=&data[side];fighters[side].dat_attrs=backup[side];fighters[side].dat_attrs_backup=backup[side];
        fighters[side].x2CC=(void*)backup[side];fighters[side].gobj=&objects[side];
        fighters[side].kind=side?Ft_Kind_CustomFirst:family?Ft_Kind_Koopa:Ft_Kind_Donkey;
        fighters[side].behavior_kind=fighters[side].kind;
        fighters[side].x5AC.model_num=3;
        fighters[side].x8_spawnNum=UINT32_C(0xFEDCBA98);fighters[side].x34_scale.y=1;
        fighters[side].item_gobj=&item; fighters[side].victim_gobj=&caught[side];
        objects[side]=(HSD_GObj){.user_data=&fighters[side]}; caught[side]=(HSD_GObj){.user_data=&victims[side]};
        fighters[side].cpu.x44=&victims[side];fighters[side].cpu.half_width=10;
        victims[side].cpu.half_width=10; victims[side].ft_data=&data[side];
        fighters[side].facing_dir=1;
    }
    objects[0].next=&objects[1];heads[HSD_GOBJ_PLINK_FIGHTER]=&objects[0];HSD_GObjPLinkHead=heads;
    island.x8=(Vec3){-100,0,0};island.x14=(Vec3){100,0,0};
    heavy=has_island=true;blocked=ranged=suppressed=false;knockback=3;walk_variant=0;rng=.5f;
    frames_remaining=false;invalidate_after_motion=0;
}
static bool input_false(Fighter_GObj* object) { (void)object;return false; }
static bool input_walk(Fighter_GObj* object) { (void)object;return true; }
static ftCo_JumpInput input_jump(Fighter_GObj* object) { (void)object;return 0; }
static void remember(void) { expected_calls=calls;memcpy(expected,events,sizeof(expected));calls=0;memset(events,0,sizeof(events));current_side=1; }
static void compare(void)
{
    if(calls!=expected_calls||memcmp(events,expected,calls*sizeof(events[0]))) {
        fprintf(stderr,"event mismatch at comparison %u (%u vs %u)\n",comparisons,calls,expected_calls);
        for(unsigned i=0;i<calls||i<expected_calls;++i) fprintf(stderr,"%u: %d,%d,%d,%g,%g / %d,%d,%d,%g,%g\n",i,expected[i].id,expected[i].a,expected[i].b,expected[i].x,expected[i].y,events[i].id,events[i].a,events[i].b,events[i].x,events[i].y);
        abort();
    }
    ++comparisons;
}
static void parameters(HMODULE lib,const char* path)
{
    FILE* file=fopen(path,"rb");assert(file);char text[32768];size_t size=fread(text,1,sizeof(text),file);fclose(file);
    NativeFighterParameters values;unsigned line;assert(nativeFighterParameters_Parse(text,size,&values,&line));
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<values.count;++i)words[i]=values.fields[i].bits;
    Parameters bind=(Parameters)GetProcAddress(lib,"FreakMelee_SetFighterParameters");assert(bind&&bind(words,values.count,values.schema_hash));
}
static void lifecycle(void)
{
    HSD_GObjEvent original[]={family?ftKp_Init_OnDeath:ftDk_Init_OnDeath,family?ftKp_Init_OnItemVisible:ftDk_Init_OnItemVisible,family?ftKp_Init_OnItemInvisible:ftDk_Init_OnItemInvisible,family?ftKp_Init_OnKnockbackEnter:ftDk_Init_OnKnockbackEnter,family?ftKp_Init_OnKnockbackExit:ftDk_Init_OnKnockbackExit};
    HSD_GObjEvent authored[]={active->on_death,active->on_item_visible,active->on_item_invisible,active->on_knockback_enter,active->on_knockback_exit};
    for(unsigned seed=0;seed<32;++seed)for(unsigned event=0;event<9;++event) {
        reset();heavy=seed&1;hold_kind=(Item_HoldKinds)((seed>>1)%6);
        if(event<5)original[event](&objects[0]);
        else if(event==5||event==7) { if(family)ftKp_Init_OnItemPickup(&objects[0],seed&1);else ftDk_Init_OnItemPickup(&objects[0],seed&1); }
        else { if(family)ftKp_Init_OnItemDrop(&objects[0],seed&1);else ftDk_Init_OnItemDrop(&objects[0],seed&1); }
        remember();if(event<5)authored[event](&objects[1]);
        else if(event==5)active->on_item_pickup(&objects[1],seed&1);
        else if(event==6)active->on_item_drop(&objects[1],seed&1);
        else if(event==7)active->on_item_pickup_ext(&objects[1],seed&1);
        else active->on_item_drop_ext(&objects[1],seed&1);
        compare();if(event==0) {
            if(family) { assert(fighters[0].dmg.armor0==fighters[1].dmg.armor0);assert(fighters[0].u.kp.x222C==get_float("breath_length"));assert(fighters[0].u.kp.x2230==get_float("breath_size")); }
            else assert(fighters[0].u.dk.x222C==get_int("punch_charge"));
        }
    }
    reset();if(family)ftKp_Init_OnLoad(&objects[0]);else ftDk_Init_OnLoad(&objects[0]);remember();
    active->on_load(&objects[1]);compare();assert(fighters[1].dat_attrs==fighters[1].dat_attrs_backup);
    if(family)assert(fighters[0].x2226_b1==fighters[1].x2226_b1);
    else {
        ftDonkeyAttributes* stock=fighters[0].dat_attrs;
        assert(stock->x8==get_float("hold_walk_slow_length")&&stock->xC==get_float("hold_walk_middle_length")&&stock->x10==get_float("hold_walk_fast_length"));
        assert(!fighters[1].x2222_b0); // Owned policy intentionally has no legacy capability bit.
    }
    reset();memset(backup,0xa5,sizeof(backup));
    if(family)ftKp_Init_LoadSpecialAttrs(&objects[0]);else ftDk_Init_LoadSpecialAttrs(&objects[0]);
    active->on_load_special_attrs(&objects[1]);assert(!memcmp(backup[0],backup[1],family?sizeof(ftKoopaAttributes):sizeof(ftDonkeyAttributes)));++comparisons;
    if(!family)for(int charge=0;charge<12;++charge) {
        reset();fighters[0].u.dk.x222C=charge;set_int("punch_charge",charge);fighters[1].u.dk.x222C=!charge;
        ftDk_Init_UnkMotionStates4(&objects[0]);remember();active->on_color_animation_reset(&objects[1]);compare();
    }
    if(family)for(int motion=339;motion<=348;++motion)for(int length=0;length<4;++length) {
        reset();float value=length*40.f;fighters[0].u.kp.x222C=value;fighters[0].u.kp.x2230=value;
        set_float("breath_length",value);set_float("breath_size",value);fighters[1].u.kp.x222C=-999;fighters[1].u.kp.x2230=-999;
        fighters[0].motion_id=fighters[1].motion_id=motion;ftKp_SpecialLw_80134D78(&objects[0]);active->on_late_update(&objects[1]);
        assert(fighters[0].u.kp.x222C==get_float("breath_length")&&fighters[0].u.kp.x2230==get_float("breath_size"));++comparisons;
    }
}
static void cpu(void)
{
    static const float samples[]={0,.001f,.1f,.25f,.5f,.99f,1};
    for(unsigned sample=0;sample<7;++sample)for(unsigned tick=599;tick<=601;++tick) {
        reset();rng=samples[sample];fighters[0].cpu.x7C=fighters[1].cpu.x7C=tick;
        original_ftCo_800A0CB0(&fighters[0]);remember();production_spacing(&fighters[1]);compare();assert(fighters[0].cpu.x56C==fighters[1].cpu.x56C);
    }
    reset();original_spacing_init(&fighters[0]);active->on_cpu_initialize(&objects[1]);assert(fighters[0].cpu.x56C==fighters[1].cpu.x56C);++comparisons;
    for(int motion=0;motion<=390;++motion) {
        reset();fighters[0].motion_id=fighters[1].motion_id=motion;
        assert(original_ftCo_800B630C(&fighters[0])==production_attacking(&fighters[1]));
        assert(original_ftCo_IsGrabbing(&fighters[0])==production_grabbing(&fighters[1]));comparisons+=2;
    }
    if(!family) {
        for(int motion=349;motion<=378;++motion)for(int charge=0;charge<=10;++charge) {
            reset();fighters[0].motion_id=fighters[1].motion_id=motion;fighters[0].u.dk.x222C=charge;set_int("punch_charge",charge);fighters[1].u.dk.x222C=!charge;
            original_charge_decision(&fighters[0]);remember();bool handled=active->on_cpu_charge_decision(&objects[1]);compare();assert(handled==(motion!=370&&charge==0));
            calls=0;original_charge_attack(&fighters[0]);remember();assert(!active->on_cpu_attack_decision(&objects[1]));compare();
            calls=0;fighters[0].cpu.x18=3;original_ftCo_800ADC28(&fighters[0]);remember();active->on_cpu_charge_cancel(&objects[1]);compare();
        }
        static const float pos[]={-100,-90.001f,-90,-89.999f,-1,0,1,89.999f,90,90.001f,100};
        for(int motion=350;motion<=357;++motion)for(unsigned found=0;found<2;++found)for(unsigned i=0;i<11;++i) {
            reset();has_island=found;fighters[0].motion_id=fighters[1].motion_id=motion;fighters[0].cur_pos.x=fighters[1].cur_pos.x=pos[i];
            original_cargo_decision(&fighters[0]);remember();bool handled=active->on_cpu_capture_decision(&objects[1]);compare();assert(handled==(motion>=351&&motion<=356));
        }
    } else {
        for(unsigned air=0;air<2;++air)for(unsigned floor=0;floor<2;++floor)for(unsigned result=0;result<2;++result)for(unsigned count=0;count<10;++count) {
            reset();ranged=result;for(unsigned side=0;side<2;++side) { fighters[side].ground_or_air=air;fighters[side].cpu.xFA_b5=floor;fighters[side].cpu.xEC=count; }
            original_ranged(&fighters[0]);remember();active->on_cpu_ranged_candidates(&objects[1]);compare();
            assert(fighters[0].cpu.xEC==fighters[1].cpu.xEC&&!memcmp(fighters[0].cpu.xCC_array,fighters[1].cpu.xCC_array,sizeof(fighters[0].cpu.xCC_array)));
            assert(active->on_cpu_ranged_allowed(&objects[1])==floor);++comparisons;
        }
        static const float xs[]={-20,-19.999f,0,19.999f,20};static const float ys[]={-1,0,.1f,34.999f,35,35.001f};
        for(int motion=62;motion<=64;++motion)for(unsigned bomb=0;bomb<3;++bomb)for(unsigned x=0;x<5;++x)for(unsigned y=0;y<6;++y)for(unsigned block=0;block<2;++block) {
            reset();blocked=block;for(unsigned side=0;side<2;++side) { fighters[side].motion_id=bomb?360+bomb:motion;victims[side].cur_pos.x=xs[x];victims[side].cur_pos.y=ys[y]; }
            bool expected_value=original_threat(&victims[0],&fighters[0]);remember();bool value=active->on_cpu_threat(&objects[1],&caught[1]);compare();assert(value==expected_value);
            assert(active->on_cpu_pummel_special(&objects[1])==(fighters[1].motion_id==350));++comparisons;
        }
        for(int motion=348;motion<=356;++motion) { reset();fighters[1].motion_id=motion;assert(!active->on_cpu_capture_decision(&objects[1]));assert(active->on_cpu_pummel_special(&objects[1])==(motion==350));comparisons+=2; }
    }
}
static void heavy_throw(void)
{
    if(family)return;
    for(unsigned air=0;air<2;++air) { reset();fighters[0].ground_or_air=fighters[1].ground_or_air=air;ftDk_MS_341_800DF980(&objects[0]);remember();assert(active->on_heavy_pickup_finish(&objects[1]));compare();assert(fighters[0].motion_id==fighters[1].motion_id); }
    for(unsigned disabled=0;disabled<2;++disabled)for(int level=0;level<6;++level) {
        reset();suppressed=disabled;knockback=level;fighters[0].dmg.kb_applied=fighters[1].dmg.kb_applied=17;
        ftDk_MS_349_800E06D8(&objects[0]);remember();active->on_heavy_item_damage(&objects[1]);compare();
    }
    for(unsigned air=0;air<2;++air)for(unsigned contact=0;contact<2;++contact) {
        reset();fighters[0].ground_or_air=fighters[1].ground_or_air=air;ground_contact=air_contact=contact;
        ftCo_800961D0(&objects[0]);remember();active->on_heavy_throw_enter(&objects[1]);assert(fighters[1].phys_cb&&fighters[1].coll_cb);fighters[1].phys_cb(&objects[1]);compare();
        calls=0;ftCo_80096498(&objects[0]);remember();fighters[1].coll_cb(&objects[1]);compare();assert(fighters[0].ground_or_air==fighters[1].ground_or_air);
    }
    static const float scale[]={.5f,1,2};
    for(unsigned size=0;size<3;++size)for(unsigned variant=0;variant<3;++variant)for(unsigned cargo=0;cargo<2;++cargo) {
        reset();walk_variant=variant;for(unsigned side=0;side<2;++side) {
            fighters[side].x34_scale.y=scale[size];ftDonkeyAttributes* attrs=fighters[side].dat_attrs;
            attrs->x8=26;attrs->xC=27;attrs->x10=28;if(!side){attrs->x14*=scale[size];attrs->x18*=scale[size];attrs->x1C*=scale[size];}
        }
        set_float("hold_walk_slow_length",26);set_float("hold_walk_middle_length",27);set_float("hold_walk_fast_length",28);
        if(cargo)ftCo_8009B6C8(&objects[0],0);else ftDk_MS_341_800DFAE4(&objects[0],0);remember();
        // Use the actual owned state IASA to reach its authored walk helper.
        fighters[1].motion_id=cargo?351:341;fighters[1].input.lstick[0].x=1;
        active->motion_states[cargo?10:0].input_cb(&objects[1]);compare();
    }
}
static void throw_policies(void)
{
    // Actual common resolver retains weight-speed calculation around the owned
    // victim-pose query. Raw stock kinds exercise every current animation family.
    for(int motion=219;motion<=222;++motion)for(int kind=0;kind<Ft_Kind_CustomFirst;++kind)
    for(unsigned independent=0;independent<2;++independent) {
        reset();common_data.x37C=.0175f;
        for(unsigned side=0;side<2;++side) {
            common_attrs[side].weight=80.5f;victims[side].kind=kind;
            common_attrs[side].weight_independent_throws_mask=independent?15:0;
        }
        ftCo_800DD4B0(&objects[0],motion);remember();ftCo_800DD4B0(&objects[1],motion);compare();
    }
    if(family)return;
    for(unsigned remaining=0;remaining<2;++remaining)for(unsigned victim=0;victim<2;++victim) {
        reset();frames_remaining=remaining;fighters[0].x2222_b0=true;
        fighters[0].motion_id=fighters[1].motion_id=219;
        if(!victim) {
            // Stock cargo entry requires an existing caught fighter; no-victim
            // fallback is a deliberate safe boundary of the owned finish hook.
            fighters[1].victim_gobj=NULL;
            assert(!active->on_throw_finish(&objects[1]));++comparisons;continue;
        }
        original_throw_forward(&objects[0]);remember();production_throw_forward(&objects[1]);compare();
        assert(fighters[0].motion_id==fighters[1].motion_id);
    }
    for(int motion=218;motion<=223;++motion) {
        reset();current_side=1;fighters[1].motion_id=motion;
        bool replaced=active->on_throw_finish(&objects[1]);
        assert(replaced==(motion==219));assert(calls==(motion==219?2U:0U));++comparisons;
    }
    // Exact common cargo transition preserves back-throw animation facing,
    // source facing after change, all flag bits, frame and the capture mask.
    for(int motion=361;motion<=368;++motion)for(unsigned reversed=0;reversed<2;++reversed) {
        reset();unsigned air=motion>=365;
        for(unsigned side=0;side<2;++side) {
            fighters[side].motion_id=motion;fighters[side].ground_or_air=air;
            fighters[side].cur_anim_frame=8.25f;fighters[side].facing_dir=reversed?-1:1;fighters[side].facing_dir1=1;
        }
        ground_contact=false;air_contact=true;
        if(air) { assert(ft_80081D0C(&objects[0])==GA_Air);ftCommon_8007D7FC(&fighters[0]); }
        else { assert(!ft_80082708(&objects[0]));ftCommon_8007D5D4(&fighters[0]); }
        cargo_transition_original(&objects[0],motion+(air?-4:4));remember();
        active->motion_states[motion-341].coll_cb(&objects[1]);compare();
        assert(fighters[0].facing_dir==fighters[1].facing_dir&&fighters[0].ground_or_air==fighters[1].ground_or_air&&fighters[0].x1A6A==fighters[1].x1A6A);
    }
    // Recycled source during nested state change must stop before hold-victim
    // initialization, and the common caller must stop before any later work.
    for(unsigned invalidation=1;invalidation<=2;++invalidation) {
        reset();current_side=1;fighters[1].motion_id=219;invalidate_after_motion=invalidation;
        production_throw_forward(&objects[1]);assert(calls==3&&events[2].id==11);++comparisons;
    }
}
static void punch_flags(void)
{
    if(family)return;
    HSD_GObjEvent original[]={ftDk_SpecialN_Coll,ftDk_SpecialAirN_Coll,ftDk_SpecialNFull_Coll,ftDk_SpecialAirNFull_Coll};
    const int motions[]={372,377,373,378};
    for(unsigned index=0;index<4;++index)for(unsigned contact=0;contact<2;++contact) {
        reset();unsigned air=index&1;
        fighters[0].motion_id=fighters[1].motion_id=motions[index];
        fighters[0].ground_or_air=fighters[1].ground_or_air=air;
        fighters[0].cur_anim_frame=fighters[1].cur_anim_frame=12.5f;
        ground_contact=!contact;air_contact=contact;
        original[index](&objects[0]);remember();active->motion_states[motions[index]-341].coll_cb(&objects[1]);compare();
        assert(fighters[0].motion_id==fighters[1].motion_id&&fighters[0].ground_or_air==fighters[1].ground_or_air);
        if(contact)assert(fighters[1].take_dmg_cb&&fighters[1].death2_cb&&fighters[1].take_dmg_2_cb&&fighters[1].pre_hitlag_cb&&fighters[1].post_hitlag_cb);
    }
}
int main(int argc,char** argv)
{
    assert(argc==5);NativeFighterHost host={0};closure_host_traps(&host);
    static MotionState common[ftCo_MS_Count];host.common_motion_states=common;
    host.callback_source_alive=nativeCallbackSource_Alive;
    host.fighter_item_pickup=Fighter_OnItemPickup;host.fighter_item_drop=Fighter_OnItemDrop;
    host.fighter_item_visible=Fighter_OnItemVisible;host.fighter_item_invisible=Fighter_OnItemInvisible;
    host.fighter_knockback_enter=Fighter_OnKnockbackEnter;host.fighter_knockback_exit=Fighter_OnKnockbackExit;
    host.set_visibility_group=ftParts_80074A4C;host.start_color=start_color;host.min_float=fminf;
    host.copy_ext_attributes=copy_ext_attributes;host.reload_ext_attributes=reload_ext_attributes;host.animation_clip=ftData_80085E50;host.clip_length=lbAnim_8001E8F8;
    host.cpu_command=ftCo_800B463C;host.cpu_command_arg=ftCo_800B46B8;host.find_floor_island=mpIsland_8005AC14;host.island_bounds=mpIsland_8005ACE8;
    host.cpu_ranged_unsafe=ftCo_CpuRangedAttackUnsafe;host.cpu_add_candidate=ftCo_CpuAddAttackCandidate;host.stage_read_floor=mpCheckFloor;host.stage_cpu_floor_excluded=ftCo_800A1B38;
    host.change_motion_state=Fighter_ChangeMotionState;host.air_to_ground=ftCommon_8007D7FC;host.ground_to_air=ftCommon_8007D5D4;
    host.enter_damage_motion=ftCo_8008DCE0;host.enter_damage_reaction=ftCo_8008E908;host.damage_reaction_suppressed=ftCo_8008E984;host.scale_knockback=ftCo_ScaleBy154;host.knockback_level=ftCo_Damage_CalcKnockbackLevel;host.release_held_item=Item_8026ABD8;
    host.air_drift=ft_80084DB0;host.ground_friction=ft_80084F3C;host.ground_collision=ft_80082708;host.air_collision=ft_80081D0C;
    host.check_heavy_item_throw=input_false;host.check_turn_input=input_false;host.check_platform_drop=input_false;host.walk_requested=input_walk;host.jump_input=input_jump;
    host.enter_walk=ftWalkCommon_800DFCA4;host.walk_type=ftWalkCommon_GetWalkType;host.set_hold_motion=ftCo_8009C5A4;
    host.initialize_hold=ftCo_8009C640;host.set_capture_mask=ftCommon_8007E2F4;host.animation_family=animation_family;
    host.special_ground_collision=ft_800827A0;host.pause_effects=efLib_PauseAll;host.resume_effects=efLib_ResumeAll;
    const NativeFighterDefinition* bases[]={&ftDonkeyClone_Definition,&ftKoopaClone_Definition};
    for(family=0;family<2;++family) {
        HMODULE lib=LoadLibraryA(argv[1+family*2]);assert(lib);Entry entry=(Entry)GetProcAddress(lib,"FreakMelee_GetFighterModule");assert(entry);parameters(lib,argv[2+family*2]);
        assert(!entry(NULL,&host)&&!entry(bases[family],NULL));comparisons+=2;
        NativeFighterHost missing=host;missing.callback_source_alive=NULL;
        assert(!entry(bases[family],&missing));++comparisons;
        NativeFighterDefinition extended=*bases[family];MotionState extra[47];
        assert(extended.motion_state_count<47);memcpy(extra,extended.motion_states,extended.motion_state_count*sizeof(*extra));
        extra[extended.motion_state_count]=(MotionState){.anim_cb=ftCamera_UpdateCameraBox};
        extended.motion_states=extra;++extended.motion_state_count;
        assert(!entry(&extended,&host));++comparisons; // No authored phase for the extra stock callback.
        active=entry(bases[family],&host);assert(active&&active->source_kind==Ft_Kind_None);
        assert(active->bones==bases[family]->bones&&active->bone_count==(family?48:52)&&active->omission_count==0);
        assert(active->asset_layout.article_mask==(family?1:0)&&active->article_asset_count==family);
        assert(active->demo_motion_names==bases[family]->demo_motion_names&&active->effect_bank==(family?12:8));
        assert(active->animation_source_kind==bases[family]->animation_source_kind&&active->sound_banks==bases[family]->sound_banks);
        assert(active->model_effect_count==1&&active->model_effects&&active->model_effects[0].update&&active->model_effects[0].gfx_id==(family?0x4DA:0x4CC));
        if(family)assert(active->article_logic_count==1&&active->article_logics[0].on_created&&active->article_assets[0].kind==It_Kind_Koopa_Flame);
        for(unsigned i=0;i<active->bone_count;++i) { assert(active->bones[i].part==closure_expected_bones[family][i].part&&active->bones[i].joint==closure_expected_bones[family][i].joint);++comparisons; }
        for(unsigned i=0;i<active->motion_state_count;++i) {
            const MotionState* original=&bases[family]->motion_states[i],*owned=&active->motion_states[i];
            assert(original->anim_id==owned->anim_id&&original->x4_flags==owned->x4_flags);
            assert(original->cam_cb==owned->cam_cb);
            assert(!original->anim_cb||(owned->anim_cb&&owned->anim_cb!=original->anim_cb));
            assert(!original->input_cb||(owned->input_cb&&owned->input_cb!=original->input_cb));
            assert(!original->phys_cb||(owned->phys_cb&&owned->phys_cb!=original->phys_cb));
            assert(!original->coll_cb||(owned->coll_cb&&owned->coll_cb!=original->coll_cb));++comparisons;
        }
        assert(active->on_load&&active->on_load_special_attrs&&active->on_death&&active->on_item_pickup_ext&&active->on_item_drop_ext);
        assert(active->on_item_pickup&&active->on_item_drop&&active->on_item_visible&&active->on_item_invisible&&active->on_knockback_enter&&active->on_knockback_exit);
        assert(active->special_n!=bases[family]->special_n&&active->special_air_n!=bases[family]->special_air_n);
        assert(active->special_s!=bases[family]->special_s&&active->special_air_s!=bases[family]->special_air_s);
        assert(active->special_hi!=bases[family]->special_hi&&active->special_air_hi!=bases[family]->special_air_hi);
        assert(active->special_lw!=bases[family]->special_lw);
        assert(family?active->special_air_lw!=bases[family]->special_air_lw:active->special_air_lw==NULL);
        assert(active->on_load!=ftDk_Init_OnLoad&&active->on_load!=ftKp_Init_OnLoad);
        lifecycle();cpu();heavy_throw();throw_policies();punch_flags();active=NULL;FreeLibrary(lib);
    }
    printf("Donkey/Bowser closure: %u source comparisons, 0 failures\n",comparisons);return 0;
}
