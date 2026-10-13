// Production constructor versus actual generated full Dr. Mario callbacks.
// Engine stubs observe common primitive calls, not a copied pill policy.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/types.h>
#include <melee/it/itCharItems.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GET_ITEM(g) ((Item*)(g)->user_data)
#define GET_ATTRS(ip) ((itDrMarioPillAttributes*)(ip)->xC4_article_data->x4_specialAttributes)
enum { ESpawn, ERandom, ELife, EState, EManual, EAppearance, ESpin, EPhysics, ECollision, EScale };
typedef struct PillEvent { int operation, index, a; float value; } PillEvent;
typedef struct PillView {
    Vec3 pos, translate;
    Quaternion rotate;
    Vec3 scale;
    float life, half, scl, spin;
    int state, collision, commands[4];
    bool bound, manual, reverse, accessory;
} PillView;
static Fighter fighter, other;
static Item items[2];
static HSD_GObj fg, og, objects[2], *heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static HSD_JObj joints[2], fighter_joint;
static Article article;
static ItemStateArray appearance_states = { .count=7 };
static ItemAttr item_attr;
static itDrMarioPillAttributes attrs;
static const NativeFighterDefinition* module;
static const NativeFighterArticleLogic* logic;
static PillEvent events[128], expected[128];
static PillView snapshots[2][2], expected_snapshots[2][2], final_expected[2];
static SpawnItem spawns[2], expected_spawns[2];
static unsigned event_count, expected_count, created, checks, random_at;
static unsigned random_choices[2];
static bool scripted, inside_stage_callback;
static unsigned invalidate_stage, invalidate_action;
static uint16_t current_offset, previous_offset;
static float pill_lifetime;
static void require(bool c,const char* why)
{ if(!c){fprintf(stderr,"%s at check %u\n",why,checks);exit(1);} }
static int index_of(HSD_GObj* g)
{ require(g==&objects[0]||g==&objects[1],"Known fixture item");return g==&objects[1]; }
static void record(int op,int index,int a,float value)
{ require(event_count<128,"Bounded event buffer");events[event_count++]=(PillEvent){op,index,a,value}; }
static PillView view(unsigned i)
{
    Item* p=&items[i];return (PillView){p->pos,joints[i].translate,joints[i].rotate,joints[i].scale,
        p->xD44_lifeTimer,p->xD48_halfLifeTimer,p->scl,p->xD3C_spinSpeed,p->msid,p->xD0C,
        {p->xDAC_itcmd_var0,p->xDB0_itcmd_var1,p->xDB4_itcmd_var2,p->xDB8_itcmd_var3},
        p->xDC8_word.flags.x13,!p->xDC8_word.flags.x15,p->xDC8_word.flags.x19,p->on_accessory!=NULL};
}
static void compare_view(PillView a,PillView b)
{
    require(!memcmp(&a.pos,&b.pos,sizeof(Vec3))&&!memcmp(&a.translate,&b.translate,sizeof(Vec3)),"Item/model position");
    require(!memcmp(&a.rotate,&b.rotate,sizeof(Quaternion))&&!memcmp(&a.scale,&b.scale,sizeof(Vec3)),"Preserved model channels");
    require(a.life==b.life&&a.half==b.half&&a.scl==b.scl&&a.spin==b.spin,"Lifetime/scale/spin");
    require(a.state==b.state&&a.collision==b.collision&&!memcmp(a.commands,b.commands,sizeof(a.commands)),"State/collision/commands");
    require(a.bound==b.bound&&a.manual==b.manual&&a.reverse==b.reverse&&a.accessory==b.accessory,"Presentation flags/accessory");
}
void __assert(char* f,u32 l,char* e) { fprintf(stderr,"%s:%lu %s\n",f,(unsigned long)l,e);abort(); }
void HSD_JObjSetMtxDirtySub(HSD_JObj* j) { (void)j; }
void it_8026BB68(HSD_GObj* g,Vec3* pos)
{ require(g==&fg,"Spawn owner");*pos=(Vec3){fighter.cur_pos.x+7,fighter.cur_pos.y+13,fighter.cur_pos.z+3}; }
void ftLib_80086644(HSD_GObj* g,Vec3* out) { *out=((Fighter*)g->user_data)->cur_pos; }
s32 HSD_Randi(s32 n)
{ require(random_at<2&&n>0,"Two ordered color draws");unsigned value=random_choices[random_at++]%(unsigned)n;record(ERandom,-1,n,(float)value);return (s32)value; }
static s32 random_excluding(s32 n,s32 first,s32 second)
{ int candidates[9],count=0;require(n==9,"Vitamin catalog size");for(int i=0;i<n;++i)if(i!=first&&i!=second)candidates[count++]=i;return candidates[HSD_Randi(count)]; }
void it_80275158(HSD_GObj* g,float lifetime)
{ int i=index_of(g);record(ELife,i,0,lifetime);items[i].xD44_lifeTimer=lifetime;items[i].xD48_halfLifeTimer=lifetime*.5f; }
void Item_80268E5C(HSD_GObj* g,enum_t state,Item_StateChangeFlags flags)
{
    int i=index_of(g);record(EState,i,flags,(float)state);items[i].msid=state;items[i].on_accessory=NULL;
    if(scripted){items[i].physics_updated=logic->logic->states[state].physics_updated;items[i].collided=logic->logic->states[state].collided;}
}
void it_8026B3A8(HSD_GObj* g)
{ int i=index_of(g);if(!inside_stage_callback)record(EManual,i,0,0);items[i].xDC8_word.flags.x15=false; }
void it_802756D0(HSD_GObj* g) { GET_ITEM(g)->xD0C=2; }
void it_80273670(HSD_GObj* g,int descriptor,float frame)
{ record(EAppearance,index_of(g),descriptor,frame); }
void it_80274740(HSD_GObj* g)
{ int i=index_of(g);record(ESpin,i,0,0);items[i].xD3C_spinSpeed=0;HSD_JObjSetRotationZ(&joints[i],0); }
void it_8026BAE8(HSD_GObj* g,float multiplier)
{ int i=index_of(g);record(EScale,i,0,multiplier);items[i].scl=multiplier*item_attr.x60_scale;Vec3 v={items[i].scl,items[i].scl,items[i].scl};HSD_JObjSetScale(&joints[i],&v); }
static void uniform_scale(HSD_GObj* g,float value)
{ Vec3 v={value,value,value};HSD_JObjSetScale(&joints[index_of(g)],&v); }
void Item_8026AB54(HSD_GObj* g,HSD_GObj* parent,Fighter_Part part)
{ (void)g;(void)parent;(void)part;require(false,"Modes0/1 must never pick up"); }
static void invalidate(HSD_GObj* g)
{
    if(invalidate_action==1){heads[HSD_GOBJ_PLINK_ITEM]=NULL;g->user_data=(void*)(uintptr_t)1;}
    if(invalidate_action==2)++items[index_of(g)].x1C;
}
void Item_802697D4(HSD_GObj* g)
{
    int i=index_of(g);record(EPhysics,i,0,0);snapshots[i][0]=view(i);
    inside_stage_callback=true;if(items[i].physics_updated)items[i].physics_updated(g);inside_stage_callback=false;
    if(invalidate_stage==1)invalidate(g);
}
void Item_80269978(HSD_GObj* g)
{
    int i=index_of(g);record(ECollision,i,0,0);snapshots[i][1]=view(i);
    inside_stage_callback=true;if(items[i].collided)require(!items[i].collided(g),"Presentation collision keeps article");inside_stage_callback=false;
    if(invalidate_stage==2)invalidate(g);
}
static bool incap(HSD_GObj* g);
Fighter_GObj* nativeArticle_Creator(Item_GObj*);
bool nativeItem_BindFighter(Fighter_GObj*,uint32_t,bool);
int32_t nativeArticle_Create(Fighter_GObj*,ItemKind,Vec3*,Vec3*,float,bool,uint32_t,float,bool);
static void Item_8026A810(HSD_GObj*);
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_CustomFirst ? module : NULL; }
Article* nativeFighterRuntime_ArticleAssets(FighterKind kind,ItemKind item)
{ return kind==Ft_Kind_CustomFirst&&item==It_Kind_DrMario_Vitamin?&article:NULL; }
static float root_scale(HSD_GObj* g) { return ((HSD_JObj*)g->hsd_obj)->scale.y; }
static float joint_value(HSD_GObj* g,unsigned model,const char* path,unsigned channel)
{ require(model==0&&!*path&&channel<9,"Root joint scalar");HSD_JObj* j=g->hsd_obj;return channel<3?((float*)&j->translate)[channel]:channel<6?((float*)&j->rotate)[channel-3]:((float*)&j->scale)[channel-6]; }
static void joint_write(HSD_GObj* g,unsigned model,const char* path,unsigned channel,float value)
{ require(model==0&&!*path&&channel<9,"Root joint scalar write");HSD_JObj* j=g->hsd_obj;if(channel<3)((float*)&j->translate)[channel]=value;else if(channel<6)((float*)&j->rotate)[channel-3]=value;else ((float*)&j->scale)[channel-6]=value; }
static HSD_GObj* spawn(SpawnItem* input)
{
    require(created<2,"Two result items");unsigned i=created++;spawns[i]=*input;record(ESpawn,i,input->kind,input->facing_dir);
    items[i].pos=input->pos;items[i].x40_vel=input->vel;items[i].facing_dir=input->facing_dir;items[i].owner=input->x0_parent_gobj;
    return &objects[i];
}
Item_GObj* Item_80268B18(SpawnItem* input) { return spawn(input); }
Item_GObj* Item_CreateAuthored(SpawnItem* input,FighterKind author)
{
    HSD_GObj* g=spawn(input);Item* p=g->user_data;
    p->native_article_author_kind=author;p->xB8_itemLogicTable=logic->logic;
    Item_8026A810(g);return g;
}
static bool signal_owned(HSD_GObj* owner,s32 id,uint32_t hash,float value)
{
    for(unsigned i=0;i<created;++i)if(items[i].x1C==(uint32_t)id&&items[i].owner==owner)
        for(unsigned s=0;s<logic->signal_count;++s)if(logic->signals[s].name_hash==hash){logic->signals[s].callback(&objects[i],value);return true;}
    return false;
}
#include "drmario_results_unused_host.inc"
#include "drmario_results_original.inc"
static bool incap(HSD_GObj* g) { return ftCo_800BF228(g); }
static void reset(bool use_script,float facing,int current,int previous)
{
    scripted=use_script;created=event_count=random_at=0;invalidate_stage=invalidate_action=0;
    memset(&fighter,0,sizeof(fighter));memset(&other,0,sizeof(other));memset(items,0,sizeof(items));memset(heads,0,sizeof(heads));memset(snapshots,0,sizeof(snapshots));
    fg=(HSD_GObj){.user_data=&fighter,.hsd_obj=&fighter_joint,.classifier=HSD_GOBJ_CLASS_FIGHTER};og=(HSD_GObj){.user_data=&other,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    heads[HSD_GOBJ_PLINK_FIGHTER]=&fg;fighter.gobj=&fg;fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);fighter.kind=use_script?Ft_Kind_CustomFirst:Ft_Kind_DrMario;
    fighter.facing_dir=facing;fighter.cur_pos=(Vec3){-19,11,23};fighter.x34_scale.y=1.25f;fighter.co_attrs.model_scaling=.75f;
    fighter.motion_id=ftCo_MS_DeadRight;fighter.u.mr.x222C_vitaminCurr=current;fighter.u.mr.x2230_vitaminPrev=previous;
    memcpy(fighter.native_module_state+current_offset,&current,4);memcpy(fighter.native_module_state+previous_offset,&previous,4);
    fighter_joint=(HSD_JObj){.scale={1,2.5f,3}};item_attr.x60_scale=1.5f;attrs.x8=pill_lifetime;article.x4_specialAttributes=&attrs;article.xC_itemStates=&appearance_states;
    for(unsigned i=0;i<2;++i){objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&joints[i],.classifier=HSD_GOBJ_CLASS_ITEM};
        objects[i].next=i?NULL:&objects[1];items[i].entity=&objects[i];items[i].x1C=100+i;items[i].xCC_item_attr=&item_attr;items[i].xC4_article_data=&article;
        items[i].xDAC_itcmd_var0=11;items[i].xDB0_itcmd_var1=12;items[i].xDB4_itcmd_var2=13;items[i].xDB8_itcmd_var3=14;
        items[i].xDC8_word.flags.x19=true;items[i].xDC8_word.flags.x15=true;items[i].xD3C_spinSpeed=4;
        joints[i]=(HSD_JObj){.flags=JOBJ_MTX_INDEP_SRT,.rotate={.25f,.5f,.75f},.scale={1.5f,1.5f,1.5f}};
        if(!use_script){items[i].physics_updated=itDrMarioPill_Motion6_Phys;items[i].collided=itDrMarioPill_Motion6_Coll;}
    }heads[HSD_GOBJ_PLINK_ITEM]=&objects[0];
}
static void same_events(void)
{ if(event_count!=expected_count){for(unsigned i=0;i<expected_count||i<event_count;++i)fprintf(stderr,"event%u stock %d/%d/%d/%g script %d/%d/%d/%g\n",i,expected[i].operation,expected[i].index,expected[i].a,expected[i].value,events[i].operation,events[i].index,events[i].a,events[i].value);}require(event_count==expected_count,"Primitive order count");for(unsigned i=0;i<event_count;++i)require(events[i].operation==expected[i].operation&&events[i].index==expected[i].index&&events[i].a==expected[i].a&&events[i].value==expected[i].value,"Primitive order/value"); }
typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
typedef int (__cdecl* Upload)(const uint32_t*,unsigned,uint64_t);
typedef unsigned long long (__cdecl* Layout)(void);
int main(int argc,char** argv)
{
    require(argc==3,"module.dll fighter_parameters.fmp");HMODULE dll=LoadLibraryA(argv[1]);require(dll!=NULL,"Load matching generated module");
    Entry entry=(Entry)GetProcAddress(dll,"FreakMelee_GetFighterModule");Upload upload=(Upload)GetProcAddress(dll,"FreakMelee_SetFighterParameters");require(entry&&upload,"Generated exports");
    Layout layout=(Layout)GetProcAddress(dll,"FreakMelee_GetFighterModuleLayout");require(layout&&layout()==nativeFighterModule_LayoutFingerprint(),"Frozen SDK/module layout agreement");
    char bytes[32768];FILE* f=fopen(argv[2],"rb");require(f!=NULL,"FMP open");size_t size=fread(bytes,1,sizeof(bytes),f);fclose(f);
    NativeFighterParameters parameters;unsigned line;require(nativeFighterParameters_Parse(bytes,size,&parameters,&line),"FMP parse");uint32_t values[256];
    for(unsigned i=0;i<parameters.count;++i){values[i]=parameters.fields[i].bits;if(!strcmp(parameters.fields[i].name,"It_Kind_DrMario_Vitamin.lifetime"))memcpy(&pill_lifetime,&values[i],4);}
    require(pill_lifetime>0,"Imported actual pill lifetime");
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.random_int_excluding=random_excluding,
        .create_article_at=nativeArticle_Create,.signal_owned_article=signal_owned,.bind_fighter_item=nativeItem_BindFighter,.item_creator=nativeArticle_Creator,
        .item_change_state_flags=Item_80268E5C,.item_set_lifetime=it_80275158,.item_use_manual_lifetime=it_8026B3A8,
        .item_set_appearance=it_80273670,.item_reset_spin=it_80274740,.item_refresh_physics=it_802750F8,
        .item_scale_by_attribute=it_8026BAE8,.item_set_uniform_scale=uniform_scale,.fighter_incapacitated=incap,.fighter_root_scale_y=root_scale,
        .item_joint_value=joint_value,.item_set_joint_value=joint_write};supply_unused_host(&host);
    MotionState motions[10]={0};NativeFighterDefinition base={.code_key="drmario_clone",.fighter_root="ftDataDrmario",.source_kind=Ft_Kind_DrMario,.joint_count=86,
        .motion_state_first=341,.motion_state_count=10,.motion_states=motions,.special_ground_state={343,345,347,349},.special_air_state={344,346,348,350}};
    module=entry(&base,&host);require(module&&upload(values,parameters.count,parameters.schema_hash),"Generated full module/FMP");require(module->on_demo_enter!=NULL,"Authored demo entry");
    for(unsigned i=0;i<module->state_field_count;++i){if(!strcmp(module->state_fields[i].name,"vitamin_current"))current_offset=module->state_fields[i].offset;if(!strcmp(module->state_fields[i].name,"vitamin_previous"))previous_offset=module->state_fields[i].offset;}
    require(current_offset!=previous_offset,"Both owned vitamin fields");
    for(unsigned i=0;i<module->article_logic_count;++i)if(module->article_logics[i].kind==It_Kind_DrMario_Vitamin)logic=&module->article_logics[i];require(logic&&logic->logic,"Owned pill article");
    for(int current=-1;current<10;++current)for(int previous=-1;previous<10;++previous)for(unsigned draw=0;draw<9;++draw)for(unsigned facing=0;facing<2;++facing){
        random_choices[0]=draw;random_choices[1]=8-draw;reset(false,facing?1:-1,current,previous);DocHelper(&fg,&fighter);
        expected_count=event_count;memcpy(expected,events,event_count*sizeof(*events));memcpy(expected_snapshots,snapshots,sizeof(snapshots));memcpy(expected_spawns,spawns,sizeof(spawns));
        for(unsigned i=0;i<2;++i)final_expected[i]=view(i);int expected_current=fighter.u.mr.x222C_vitaminCurr,expected_previous=fighter.u.mr.x2230_vitaminPrev;
        reset(true,facing?1:-1,current,previous);module->on_demo_enter(&fg,1);same_events();require(created==2&&fighter.item_gobj==&objects[0]&&fighter.x1984_heldItemSpec==&objects[1],"Held/auxiliary binding");
        int actual_current,actual_previous;memcpy(&actual_current,fighter.native_module_state+current_offset,4);memcpy(&actual_previous,fighter.native_module_state+previous_offset,4);
        require(actual_current==expected_current&&actual_previous==expected_previous,"RNG/state policy");
        for(unsigned i=0;i<2;++i){require(!memcmp(&spawns[i].pos,&expected_spawns[i].pos,sizeof(Vec3))&&!memcmp(&spawns[i].prev_pos,&expected_spawns[i].prev_pos,sizeof(Vec3)),"Spawn current/previous origins");require(spawns[i].x40==0&&spawns[i].x44_flag.b0&&spawns[i].x0_parent_gobj==&fg&&spawns[i].x4_parent_gobj2==&fg,"Spawn linkage/group/flag");compare_view(view(i),final_expected[i]);for(unsigned j=0;j<2;++j)compare_view(snapshots[i][j],expected_snapshots[i][j]);}
        ++checks;
    }
    for(unsigned transfer=0;transfer<2;++transfer)for(unsigned mode=0;mode<3;++mode){
        reset(false,-1,0,1);DocHelper(&fg,&fighter);fighter.cur_pos=(Vec3){3,5,7};fighter.facing_dir=1;fighter.motion_id=mode==0?ftCo_MS_DeadRight:mode==1?ftCo_MS_Sleep:ftCo_MS_DeadUpFallHitCameraIce;
        if(transfer)for(unsigned i=0;i<2;++i)items[i].owner=&og;
        for(unsigned i=0;i<2;++i){items[i].xDC8_word.flags.x15=true;items[i].on_accessory(&objects[i]);items[i].xDC8_word.flags.x15=true;require(!itDrMarioPill_Motion6_Anim(&objects[i]),"Stock presentation alive");final_expected[i]=view(i);}
        reset(true,-1,0,1);module->on_demo_enter(&fg,1);fighter.cur_pos=(Vec3){3,5,7};fighter.facing_dir=1;fighter.motion_id=mode==0?ftCo_MS_DeadRight:mode==1?ftCo_MS_Sleep:ftCo_MS_DeadUpFallHitCameraIce;
        if(transfer)for(unsigned i=0;i<2;++i)items[i].owner=&og;
        for(unsigned i=0;i<2;++i){items[i].xDC8_word.flags.x15=true;items[i].on_accessory(&objects[i]);items[i].xDC8_word.flags.x15=true;require(!logic->logic->states[items[i].msid].animated(&objects[i]),"Generated presentation alive");compare_view(view(i),final_expected[i]);}++checks;
    }
    reset(true,1,0,1);module->on_demo_enter(&fg,0);require(!created&&!event_count,"Other demo entry does not construct result pills");++checks;
    reset(true,1,0,1);module->on_demo_enter(&fg,1);heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
    for(unsigned i=0;i<2;++i){PillView before=view(i);items[i].xDC8_word.flags.x15=true;items[i].on_accessory(&objects[i]);items[i].xDC8_word.flags.x15=true;require(!logic->logic->states[items[i].msid].animated(&objects[i]),"Missing creator remains safe");compare_view(view(i),before);}++checks;
    for(unsigned stage=1;stage<=2;++stage)for(unsigned action=1;action<=2;++action){
        reset(true,1,0,1);module->on_demo_enter(&fg,1);event_count=0;invalidate_stage=stage;invalidate_action=action;
        it_802750F8(&objects[0]);require(event_count==(stage==1?1:2),"Refresh stops after source invalidation");require(!items[0].xDCC_flag.b3,"Removed/recycled item skips cached tail write");++checks;
    }
    for(unsigned variant=0;variant<9;++variant){
        reset(true,1,0,1);module->on_demo_enter(&fg,1);fighter.item_gobj=NULL;fighter.x1984_heldItemSpec=NULL;
        uint32_t handle=items[0].x1C;
        if(variant==1)handle=UINT32_MAX;
        if(variant==2)heads[HSD_GOBJ_PLINK_ITEM]=&objects[1];
        if(variant==3)++items[0].x1C;
        if(variant==4)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
        if(variant==5)++fighter.x8_spawnNum;
        if(variant==6)items[0].xDD4_itemVar.native_article.attachment_owner=&og;
        if(variant==7)items[0].native_article_author_kind=Ft_Kind_CustomFirst+1;
        if(variant==8)items[0].owner=&og;
        bool valid=variant==0||variant==8;
        require(nativeItem_BindFighter(&fg,handle,false)==valid,"Production held binding validation");
        require(nativeItem_BindFighter(&fg,handle,true)==valid,"Production auxiliary binding validation");
        require(fighter.item_gobj==(valid?&objects[0]:NULL)&&fighter.x1984_heldItemSpec==(valid?&objects[0]:NULL),"Invalid binding leaves slots untouched");++checks;
    }
    printf("Dr. Mario results constructor/generated callbacks: %u comparisons passed\n",checks);FreeLibrary(dll);return 0;
}
