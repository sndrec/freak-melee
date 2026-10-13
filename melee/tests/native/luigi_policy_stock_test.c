// Actual generated Luigi policies and production common callers.
// Metadata expectations describe console semantic bytes independently of
// native union packing; the old malformed native output is checked separately.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/pl/plattack.h>
#include <melee/it/it_26B1.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/lb/forward.h>
#include <melee/mp/forward.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter fighter,target;
static HSD_GObj source,*heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static const NativeFighterDefinition *module,*active;
static bool unsafe,heavy;
static unsigned queries,counter_calls,checks,commands;
static unsigned char bytes[32],expected_bytes[32];
static void require(bool valid,const char* why)
{if(!valid){fprintf(stderr,"%s at comparison%u\n",why,checks);exit(1);}}
void __assert(char* file,u32 line,char* condition)
{fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,condition);abort();}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?active:NULL;}
bool ftCo_CpuRangedAttackUnsafe(Fighter* fp,Fighter* victim,float distance)
{require(fp==&fighter&&victim==fighter.cpu.x44&&distance==60,"Exact ordered ranged query");++queries;return unsafe;}
void ftCo_800B463C(Fighter* fp,u8 command)
{require(fp==&fighter&&commands<sizeof(bytes),"Bounded controller queue");bytes[commands++]=command;}
void ftCo_800B46B8(Fighter* fp,u8 command,u8 arg)
{ftCo_800B463C(fp,command);ftCo_800B463C(fp,arg);}
u16 plAttack_80037B08(void){++counter_calls;return 0xCAFE;}
bool it_8026B6C8(Item_GObj* object){require(object==(void*)(uintptr_t)7,"Held item identity");return heavy;}
#include "luigi_policy_unused_host.inc"
#include "luigi_policy_original.inc"
#include "luigi_policy_rig.inc"

static void reset(bool authored)
{
    memset(&fighter,0,sizeof(fighter));memset(heads,0,sizeof(heads));
    source=(HSD_GObj){.user_data=&fighter,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    fighter.gobj=&source;fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);
    fighter.kind=authored?Ft_Kind_CustomFirst:Ft_Kind_Luigi;
    fighter.behavior_kind=fighter.kind;heads[HSD_GOBJ_PLINK_FIGHTER]=&source;
    active=authored?module:NULL;queries=commands=counter_calls=0;
    memset(bytes,0,sizeof(bytes));
}
static void metadata_equal(uint32_t canonical)
{
    // Four console bytes: leading byte, category/animation byte, statistics
    // byte, move byte. Reverse bitfield allocation is never assumed here.
    unsigned char console[4];
    for(unsigned i=0;i<4;++i)console[i]=(unsigned char)(canonical>>(8*(3-i)));
    union Struct2070* v=&fighter.x2070;
    require((unsigned char)v->x2070==console[0]&&v->x2071_b0_3==console[1]/16&&v->x2073==console[3],"Canonical action bytes");
    unsigned animation=v->x2071_b4*8+v->x2071_b5*4+v->x2071_b6*2+v->x2071_b7;
    unsigned statistics=v->x2072_b0*128+v->x2072_b1*64+v->x2072_b2*32+v->count_x1A4*16+v->count_thrown_items*8+v->count_aerials*4+v->count_x1A0*2+v->count_specials;
    require(animation==console[1]%16&&statistics==console[2],"Semantic animation/statistics flags");
}
static unsigned invalidation;
static int32_t invalidate_query(Fighter_GObj* g,int32_t metadata)
{
    require(g==&source&&counter_calls==1,"Counter precedes metadata callback");
    if(invalidation==1){heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;g->user_data=(void*)(uintptr_t)1;}
    else ++fighter.x8_spawnNum;
    return metadata;
}
typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
typedef int (__cdecl* Upload)(const uint32_t*,unsigned,uint64_t);
typedef unsigned long long (__cdecl* Layout)(void);
int main(int argc,char** argv)
{
    require(argc==3,"Generated DLL and imported FMP required");HMODULE dll=LoadLibraryA(argv[1]);require(dll!=NULL,"Load generated Luigi module");
    Entry entry=(Entry)GetProcAddress(dll,"FreakMelee_GetFighterModule");Upload upload=(Upload)GetProcAddress(dll,"FreakMelee_SetFighterParameters");Layout layout=(Layout)GetProcAddress(dll,"FreakMelee_GetFighterModuleLayout");
    require(entry&&upload&&layout&&layout()==nativeFighterModule_LayoutFingerprint(),"Matching module layout/exports");
    char text[32768];FILE* input=fopen(argv[2],"rb");require(input!=NULL,"Imported parameters open");size_t size=fread(text,1,sizeof(text),input);fclose(input);
    NativeFighterParameters parameters;unsigned line;require(nativeFighterParameters_Parse(text,size,&parameters,&line)&&parameters.count<=256,"Imported parameters parse/bounds");
    uint32_t values[256];for(unsigned i=0;i<parameters.count;++i)values[i]=parameters.fields[i].bits;
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.cpu_command=ftCo_800B463C,.cpu_command_arg=ftCo_800B46B8,.cpu_ranged_unsafe=ftCo_CpuRangedAttackUnsafe,.cpu_add_candidate=ftCo_CpuAddAttackCandidate};supply_unused_host(&host);
    MotionState motions[18]={0};NativeFighterDefinition base={.code_key="luigi_clone",.source_kind=Ft_Kind_Luigi,.fighter_root="ftDataLuigi",.joint_count=61,.bone_count=sizeof(ftLuigiClone_Bones)/sizeof(ftLuigiClone_Bones[0]),.bones=ftLuigiClone_Bones,.motion_states=motions,.motion_state_count=18,.motion_state_first=341,.special_ground_state={341,343,355,357},.special_air_state={342,349,356,358}};
    module=entry(&base,&host);require(module&&upload(values,parameters.count,parameters.schema_hash),"Full generated module and owned FMP");
    require(module->source_kind==Ft_Kind_None&&module->on_cpu_ranged_candidates&&module->on_cpu_recovery&&module->on_cpu_falling_destination_reached&&module->on_action_metadata,"Independent decision callback closure");
    for(unsigned count=0;count<=8;++count)for(unsigned air=0;air<2;++air)for(unsigned forced=0;forced<2;++forced)for(unsigned exists=0;exists<2;++exists)for(unsigned result=0;result<2;++result){
        unsafe=result;reset(false);fighter.ground_or_air=air;fighter.cpu.xFA_b5=forced;fighter.cpu.x44=exists?&target:NULL;fighter.cpu.xEC=count;
        for(unsigned i=0;i<count;++i)fighter.cpu.xCC_array[i]=52;
        original_ranged(&fighter);u8 expected_count=fighter.cpu.xEC;u8 candidates[8];memcpy(candidates,fighter.cpu.xCC_array,8);require(queries==1,"Original range queried even when full");
        reset(true);fighter.ground_or_air=air;fighter.cpu.xFA_b5=forced;fighter.cpu.x44=exists?&target:NULL;fighter.cpu.xEC=count;for(unsigned i=0;i<count;++i)fighter.cpu.xCC_array[i]=52;
        module->on_cpu_ranged_candidates(&source);require(queries==1&&fighter.cpu.xEC==expected_count&&!memcmp(candidates,fighter.cpu.xCC_array,8),"Candidate order/capacity/policy");++checks;
    }
    for(unsigned side=0;side<2;++side){reset(false);fighter.cpu.x54.x=side?80:-80;original_recovery(&fighter);unsigned count=commands;memcpy(expected_bytes,bytes,count);reset(true);fighter.cpu.x54.x=side?80:-80;module->on_cpu_recovery(&source);require(commands==count&&!memcmp(bytes,expected_bytes,count),"Exact destination-directed Side-B bytecode");++checks;}
    for(unsigned air=0;air<2;++air)for(unsigned forced=0;forced<2;++forced)for(unsigned wall=0;wall<2;++wall)for(int delta=-1;delta<=1;++delta)for(unsigned above=0;above<2;++above){
        reset(false);fighter.ground_or_air=air;fighter.cpu.xFA_b5=forced;fighter.coll_data.env_flags=wall?Collide_WallMask:0;fighter.pos_delta.y=delta;fighter.cpu.x54.y=above?100:-100;
        bool expected=original_destination(&fighter);fighter.kind=Ft_Kind_CustomFirst;fighter.behavior_kind=Ft_Kind_CustomFirst;active=module;
        require(production_destination(&fighter)==expected,"Falling destination shared gates/strict negative delta");++checks;
    }
    unsigned moves[]={0,98,113,255};
    for(unsigned move=0;move<4;++move)for(unsigned byte=0;byte<256;++byte)for(unsigned previous=0;previous<2;++previous)for(unsigned is_heavy=0;is_heavy<2;++is_heavy){
        uint32_t canonical=(UINT32_C(0xA5)<<24)|((uint32_t)byte<<16)|((uint32_t)(255-byte)<<8)|moves[move];
        reset(true);heavy=is_heavy;fighter.item_gobj=(void*)(uintptr_t)7;fighter.x2070.x2073=previous?moves[move]:99;
        unsigned counters=moves[move]==0||fighter.x2070.x2073!=moves[move];fighter.dmg.x1830_percent=46.5f;fighter.is_metal=true;fighter.x221D_b6=true;fighter.x2220_b6=true;
        ft_800895E0(&fighter,(int32_t)canonical);uint32_t expected=moves[move]==113?UINT32_C(0x240063):moves[move]==98&&heavy?UINT32_C(0x44003D):canonical;
        metadata_equal(expected);require(counter_calls==counters&&(!counters||fighter.x2074.x2088==0xCAFE),"Original decoded move controls attack counter");
        require(*(s32*)&fighter.x2074.x2074_vec.x==46&&fighter.x2074.x2084_b1&&fighter.x2074.x2084_b3&&fighter.x2074.x2084_b6,"Common damage/status snapshots preserved");++checks;
    }
    reset(false);malformed_native_original(&fighter,0x71);require(fighter.x2070.x2073==0,"Known malformed native taunt baseline recorded");
    ft_800895E0(&fighter,0x71);metadata_equal(0x240063);++checks;
    const char* tuned_names[]={"taunt_attack_move_id","taunt_attack_category","taunt_animation_flags","taunt_statistics_flags"};
    unsigned indices[4]={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
    for(unsigned i=0;i<parameters.count;++i)for(unsigned j=0;j<4;++j)if(!strcmp(parameters.fields[i].name,tuned_names[j]))indices[j]=i;
    for(unsigned i=0;i<4;++i)require(indices[i]!=UINT32_MAX,"Owned integer metadata operands");
    int masks[]={-1,0,4,256,511};
    for(unsigned a=0;a<5;++a)for(unsigned b=0;b<5;++b)for(unsigned c=0;c<5;++c)for(unsigned d=0;d<5;++d){
        unsigned selected[]={a,b,c,d};uint32_t changed[256];memcpy(changed,values,parameters.count*4);
        for(unsigned i=0;i<4;++i)changed[indices[i]]=(uint32_t)masks[selected[i]];
        require(upload(changed,parameters.count,parameters.schema_hash),"Upload integer metadata tuning");reset(true);
        uint32_t expected=((uint32_t)masks[a]%256)+((uint32_t)masks[b]%16)*UINT32_C(1048576)+((uint32_t)masks[c]%16)*UINT32_C(65536)+((uint32_t)masks[d]%256)*UINT32_C(256);
        require((uint32_t)module->on_action_metadata(&source,0x71)==expected,"Actual generated builder masks each integer operand");++checks;
    }
    require(upload(values,parameters.count,parameters.schema_hash),"Restore default owned tuning");
    for(invalidation=1;invalidation<=2;++invalidation){reset(true);NativeFighterDefinition interrupted=*module;interrupted.on_action_metadata=invalidate_query;active=&interrupted;
        fighter.x2070.x2073=12;fighter.x2074.x2074_vec.x=8;ft_800895E0(&fighter,0x71);require(fighter.x2070.x2073==12&&fighter.x2074.x2074_vec.x==8,"Removed/recycled callback source skips cached continuation");++checks;}
    printf("Luigi CPU/generated canonical metadata: %u comparisons passed\n",checks);FreeLibrary(dll);return 0;
}
