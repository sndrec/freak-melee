// Original complete Din steering and real generated assembled Zelda callbacks.
// Registry/refs/nearest/position bodies execute; shared stage/game observations
// are deterministic boundaries. This does not simulate the whole CPU scheduler.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/types.h>
#include <melee/it/native_article_refs.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static unsigned checks, pairs, rounding_cases, edited_cases, safety_cases, lifetime_cases;
static unsigned large_cancellation_cases;
static double maximum_threshold_delta;
static double maximum_ordinary_threshold_delta;
typedef struct ThresholdCase {
    int motion;unsigned callback;float enemy_y,projectile_y;
    double original,authored,error;unsigned original_bytes,authored_bytes;
} ThresholdCase;
static ThresholdCase thresholds[128];
static Fighter* fighter;
static Fighter enemies[3];
static HSD_GObj objects[4], item_object, *heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static Item item;
static HSD_JObj primary;
static ftData data;
static const NativeFighterDefinition* definition;
static NativeFighterDefinition base;
static MotionState motions[18];
static NativeFighterHost host;
static int mode, special[3], special_result[3];
static unsigned observations[128], observation_count;
static unsigned mutation_query, mutation_kind, query_count;
static uint32_t selected_id;
static float stage_limit;

#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr,"Din check line%d: %s\n",__LINE__,#x); abort(); } } while(0)
#undef HSD_ASSERTREPORT
#define HSD_ASSERTREPORT(line, condition, message) CHECK(condition)
#define GET_FIGHTER(object) ((Fighter*)(object)->user_data)
#include "generated.inc"
#include "parameters.inc"

static void unexpected(void) { CHECK(!"unexecuted component crossed fixture boundary"); }
bool nativeModelEffect_Snapshot(HSD_GObj* object,uint32_t* generation)
{ (void)object;(void)generation;unexpected();return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object,uint32_t generation)
{ (void)object;(void)generation;unexpected();return NULL; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_CustomFirst ? definition : NULL; }
void __assert(char* file,u32 line,char* expression)
{ fprintf(stderr,"HSD assertion %s:%lu %s\n",file,(unsigned long)line,expression);abort(); }

static void observe(unsigned event)
{ CHECK(observation_count<128);observations[observation_count++]=event; }
bool gm_8016B168(void) { observe(10);return mode==1; }
bool gm_8016B14C(void) { observe(11);return mode==2; }
float Stage_GetBlastZoneLeftOffset(void) { observe(1);return -stage_limit; }
float Stage_GetBlastZoneRightOffset(void) { observe(2);return stage_limit; }
float Stage_GetBlastZoneBottomOffset(void) { observe(3);return -stage_limit; }
float Stage_GetBlastZoneTopOffset(void) { observe(4);return stage_limit; }
bool ft_80087A18(Fighter_GObj* object)
{
    unsigned i=(unsigned)(object-objects)-1;CHECK(i<3);
    observe(20+i);return special[i]!=0;
}
int ft_80087A80(Fighter_GObj* object)
{ unsigned i=(unsigned)(object-objects)-1;CHECK(i<3);observe(30+i);return special_result[i]; }
int ftLib_GetMotionId(HSD_GObj* object)
{ return ((Fighter*)object->user_data)->motion_id; }
#include "queries.inc"
#include "original.inc"

static void invalidate_after_query(unsigned query)
{
    ++query_count;
    if (mutation_query!=query) return;
    mutation_query=0;
    if(mutation_kind==1) {
        heads[HSD_GOBJ_PLINK_FIGHTER]=objects[0].next;
        DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_NOACCESS,&old));
        objects[0].user_data=(void*)1;
    } else if(mutation_kind==2) ++fighter->x8_spawnNum;
    else if(mutation_kind==3) {
        // Selected generation disappears before the second query; retained ID
        // cannot be redirected to a replacement using the same address.
        ++enemies[0].x8_spawnNum;
    }
}
static bool translation(HSD_GObj* source,uint32_t id,ItemKind kind,Vec3* out)
{
    bool found=nativeArticleRef_PrimaryModelTranslation(source,id,kind,out);
    invalidate_after_query(1);return found;
}
static bool nearest(Fighter_GObj* source,const Vec3* point,uint32_t* out)
{
    bool found=ftCo_CpuNearestEligibleTarget(source,point,out);
    selected_id=found?*out:0;invalidate_after_query(2);return found;
}
static bool target_position(uint32_t id,Vec3* out)
{
    bool found=nativeFighterTarget_Position(id,out);
    invalidate_after_query(3);return found;
}
static void load_parameters(const uint32_t* values)
{
    CHECK(FreakMelee_SetFighterParameters(values,
        sizeof(parameter_bits)/sizeof(*parameter_bits),parameter_schema));
}
static unsigned param(const char* name)
{
    // The runner records declaration order after retail import, without
    // guessing any compiler index or depending on anonymous union layout.
    for(unsigned i=0;i<sizeof(parameter_names)/sizeof(*parameter_names);++i)
        if(!strcmp(parameter_names[i],name)) return i;
    CHECK(!"missing owned parameter");return 0;
}
static void handle(int32_t id)
{
    for(unsigned i=0;i<definition->state_field_count;++i) {
        const NativeFighterStateField* field=&definition->state_fields[i];
        if(!strcmp(field->name,"din_handle")) {
            CHECK(field->type==NativeFighterState_Int&&field->stride==4);
            memcpy(fighter->native_module_state+field->offset,&id,4);return;
        }
    }
    CHECK(!"missing din_handle");
}
static void setup(unsigned variant,int motion,Vec3 source,Vec3 point,Vec3 enemy,
                  unsigned prefix,bool projectile)
{
    DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
    memset(fighter,0,sizeof(*fighter));memset(enemies,0,sizeof(enemies));
    memset(objects,0,sizeof(objects));memset(&item,0,sizeof(item));
    memset(&item_object,0,sizeof(item_object));memset(&primary,0,sizeof(primary));
    memset(heads,0,sizeof(heads));memset(special,0,sizeof(special));memset(special_result,0,sizeof(special_result));
    mode=(int)(variant%3);observation_count=query_count=mutation_query=mutation_kind=0;selected_id=0;
    stage_limit=fabsf(enemy.y)>1000?1e10f:1000;
    uint32_t ids[]={UINT32_C(0x80000099),UINT32_C(16777217),UINT32_C(0x80000001),UINT32_C(0xfffffffe)};
    for(unsigned i=0;i<4;++i) {
        Fighter* f=i?&enemies[i-1]:fighter;
        f->gobj=&objects[i];f->kind=Ft_Kind_CustomFirst;f->ft_data=&data;
        f->x8_spawnNum=(s32)ids[i];f->player_id=(s8)i;f->team=(s8)i;
        f->motion_id=ftCo_MS_Wait;objects[i].user_data=f;
        objects[i].classifier=HSD_GOBJ_CLASS_FIGHTER;objects[i].next=i<3?&objects[i+1]:NULL;
    }
    heads[HSD_GOBJ_PLINK_FIGHTER]=objects;
    fighter->motion_id=motion;fighter->cur_pos=source;fighter->cpu.x18=4;fighter->cpu.x1C=7;
    fighter->cpu.x90=19;fighter->cpu.level=(int)(variant%10);fighter->cpu.x44=&enemies[2];
    memset(fighter->cpu.buffer,0xa5,sizeof(fighter->cpu.buffer));
    CHECK(prefix<=sizeof(fighter->cpu.buffer));fighter->cpu.write_pos=fighter->cpu.buffer+prefix;
    enemies[0].cur_pos=enemy;
    enemies[1].cur_pos=(Vec3){enemy.x+100,enemy.y,enemy.z+100};
    // Planner target intentionally has an unrelated location.
    enemies[2].cur_pos=(Vec3){-999,999,999};
    item.entity=&item_object;item.x1C=UINT32_C(0x80000021);
    item.kind=It_Kind_Zelda_DinFire;item.native_article_author_kind=Ft_Kind_CustomFirst;
    const NativeFighterArticleLogic* logic=NULL;
    for(unsigned i=0;i<definition->article_logic_count;++i)
        if(definition->article_logics[i].kind==item.kind) logic=&definition->article_logics[i];
    CHECK(logic);item.xB8_itemLogicTable=logic->logic;
    item.owner=&objects[3]; // reflected/current owner is independent of author
    item.pos=(Vec3){800,700,600};primary.translate=point;
    item_object.user_data=&item;item_object.classifier=HSD_GOBJ_CLASS_ITEM;item_object.hsd_obj=&primary;
    heads[HSD_GOBJ_PLINK_ITEM]=&item_object;
    fighter->u.zd.x222C=projectile?&item_object:NULL; // ORIGINAL ONLY reads this
    handle(projectile?(int32_t)item.x1C:-1);
}
typedef struct Result {
    unsigned bytes;unsigned char buffer[256];int behavior,travel;bool has_target;
    unsigned observations;unsigned events[128];
} Result;
static Result result(void)
{
    Result out={0};out.bytes=(unsigned)(fighter->cpu.write_pos-fighter->cpu.buffer);
    CHECK(out.bytes<=sizeof(out.buffer));memcpy(out.buffer,fighter->cpu.buffer,sizeof(out.buffer));
    out.behavior=fighter->cpu.x18;out.travel=fighter->cpu.x90;out.has_target=fighter->cpu.x44!=NULL;
    out.observations=observation_count;memcpy(out.events,observations,sizeof(out.events));return out;
}
static void compare_case(unsigned variation,int motion,Vec3 source,Vec3 point,
                         Vec3 enemy,unsigned prefix,bool projectile,bool allow_rounding)
{
    setup(variation,motion,source,point,enemy,prefix,projectile);
    bool handled=motion>=344&&motion<=348;
    if(handled) ftCo_800B9504(fighter);
    Result expected=result();
    for(unsigned callback=0;callback<2;++callback) {
        setup(variation,motion,source,point,enemy,prefix,projectile);
        bool actual=callback?definition->on_cpu_travel_decision(&objects[0]):
            definition->on_cpu_attack_decision(&objects[0]);
        Result now=result();CHECK(actual==handled);CHECK(now.travel==19);
        if(memcmp(&expected,&now,sizeof(now))) {
            // Only the original promotion of 15.0+y-z to double is allowed to
            // select a different release command. It is not ignored generally.
            double original=fabs(15.0+(double)enemy.y-(double)point.y);
            float authored=fabsf((15.0f+enemy.y)-point.y);
            CHECK(allow_rounding&&handled&&projectile);
            CHECK((original<15.0)!=(authored<15.0f));
            CHECK(fabsf(point.x-enemy.x)<15&&fabsf(point.x-source.x)<=fabsf(enemy.x-source.x));
            double error=fabs(original-(double)authored);
            float sum=15.0f+enemy.y;
            double sum_ulp=fabs((double)nextafterf(sum,INFINITY)-(double)sum);
            double difference_ulp=fabs((double)nextafterf(sum-point.y,INFINITY)-(double)(sum-point.y));
            CHECK(error<=0.5*(sum_ulp+difference_ulp));
            if(fabsf(enemy.y)>1000) ++large_cancellation_cases;
            else if(error>maximum_ordinary_threshold_delta) maximum_ordinary_threshold_delta=error;
            if(error>maximum_threshold_delta) maximum_threshold_delta=error;
            CHECK(expected.behavior==now.behavior&&expected.travel==now.travel&&expected.has_target==now.has_target);
            CHECK(expected.observations==now.observations&&!memcmp(expected.events,now.events,sizeof(now.events)));
            unsigned release_length=8, stick_length=3;
            CHECK(expected.bytes==prefix+(original<15?release_length:stick_length));
            CHECK(now.bytes==prefix+(authored<15?release_length:stick_length));
            const unsigned char release[]={CpuCmd_ReleaseB,CpuCmd_SetLstickX,0,
                CpuCmd_SetLstickY,0,CpuCmd_WaitFor,1,CpuCmd_Done};
            const unsigned char stick[]={CpuCmd_SetLstickY,
                (15.0f+enemy.y>point.y)?0x7f:0x81,CpuCmd_Done};
            CHECK(!memcmp(expected.buffer+prefix,original<15?release:stick,
                          original<15?sizeof(release):sizeof(stick)));
            CHECK(!memcmp(now.buffer+prefix,authored<15?release:stick,
                          authored<15?sizeof(release):sizeof(stick)));
            CHECK(!memcmp(expected.buffer,now.buffer,prefix));
            for(unsigned i=expected.bytes;i<sizeof(expected.buffer);++i) CHECK(expected.buffer[i]==0xa5);
            for(unsigned i=now.bytes;i<sizeof(now.buffer);++i) CHECK(now.buffer[i]==0xa5);
            CHECK(rounding_cases<sizeof(thresholds)/sizeof(*thresholds));
            thresholds[rounding_cases]=(ThresholdCase){motion,callback,enemy.y,point.y,
                original,authored,error,expected.bytes,now.bytes};
            ++rounding_cases;
        } else CHECK(!memcmp(&expected,&now,sizeof(now)));
        ++pairs;
    }
}
static void matrix(void)
{
    const float dx[]={-30,-15.000001f,-15,-14.999999f,0,14.999999f,15,15.000001f,30};
    const float dy[]={-30.000002f,-30,-29.999998f,-15,0,0.00000095367431640625f,
        14.99999904632568359375f,15,15.00000095367431640625f,30};
    for(int motion=343;motion<=349;++motion) for(unsigned present=0;present<2;++present)
    for(unsigned x=0;x<9;++x) for(unsigned y=0;y<sizeof(dy)/sizeof(*dy);++y) {
        Vec3 source={-50,0,0}, enemy={20,dy[y],5}, point={20+dx[x],15,5};
        compare_case(x+y,motion,source,point,enemy,0,present,true);
    }
    // Deliberately widened controlled stage bounds expose cancellation far
    // beyond ordinary arena positions. These are labelled numeric boundaries,
    // not assertions that a retail stage permits enemies at these heights.
    const float large[]={16777216.0f,33554432.0f,268435456.0f,-16777216.0f,-268435456.0f};
    for(unsigned n=0;n<sizeof(large)/sizeof(*large);++n) for(int delta=-32;delta<=32;delta+=2)
        compare_case(0,344,(Vec3){-50,0,0},(Vec3){20,large[n]+delta,5},
                     (Vec3){20,large[n],5},0,true,true);
    // Prefix and exactly-fitting shared command buffers, negative-side steer,
    // and 3D nearest/list/filter behavior, distinct from CPU planner target.
    for(unsigned v=0;v<96;++v) {
        Vec3 source={(v&1)?50:-50,-10,0},point={(float)((int)(v%11)-5)*7,10,3};
        Vec3 enemy={(float)((int)(v%13)-6)*5,(float)((int)(v%17)-8)*3,2};
        setup(v,346,source,point,enemy,0,true);ftCo_800B9504(fighter);
        unsigned count=result().bytes;CHECK(count<=256);
        compare_case(v,346,source,point,enemy,1,true,false);
        compare_case(v,346,source,point,enemy,256-count,true,false);
    }
    for(unsigned reversed=0;reversed<2;++reversed) for(unsigned filter=0;filter<8;++filter) {
        setup(0,344,(Vec3){-50,0,0},(Vec3){0,0,0},(Vec3){-10,0,0},0,true);
        enemies[1].cur_pos=(Vec3){10,0,0};enemies[2].cur_pos=(Vec3){0,0,100};
        if(reversed) {objects[0].next=&objects[2];objects[2].next=&objects[1];objects[1].next=&objects[3];}
        if(filter&1) enemies[0].player_id=0;
        if(filter&2) {enemies[1].team=0;mode=1;}
        if(filter&4) enemies[2].x2219_b1=true;
        ftCo_800B9504(fighter);Result expected=result();
        fighter->cpu.write_pos=fighter->cpu.buffer;memset(fighter->cpu.buffer,0xa5,256);
        fighter->cpu.x18=4;fighter->cpu.x44=&enemies[2];observation_count=0;
        CHECK(definition->on_cpu_travel_decision(&objects[0]));Result now=result();
        CHECK(!memcmp(&expected,&now,sizeof(now)));++pairs;
    }
    setup(0,344,(Vec3){0},(Vec3){0},(Vec3){10,0,0},0,true);
    for(unsigned i=0;i<3;++i) enemies[i].player_id=0;
    ftCo_800B9504(fighter);Result expected=result();
    fighter->cpu.write_pos=fighter->cpu.buffer;memset(fighter->cpu.buffer,0xa5,256);
    fighter->cpu.x18=4;fighter->cpu.x44=&enemies[2];observation_count=0;
    CHECK(definition->on_cpu_attack_decision(&objects[0]));Result now=result();
    CHECK(!memcmp(&expected,&now,sizeof(now))&&now.behavior==7&&!now.has_target);++pairs;
    const uint32_t ids[]={UINT32_C(16777217),UINT32_C(0x80000001),UINT32_C(0xfffffffe)};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i) {
        setup(0,346,(Vec3){-50,0,0},(Vec3){10,0,0},(Vec3){20,0,0},0,true);
        enemies[0].x8_spawnNum=(s32)ids[i];
        ftCo_800B9504(fighter);Result expected_id=result();
        fighter->cpu.write_pos=fighter->cpu.buffer;memset(fighter->cpu.buffer,0xa5,256);observation_count=0;
        CHECK(definition->on_cpu_travel_decision(&objects[0]));Result actual_id=result();
        CHECK(selected_id==ids[i]);CHECK(!memcmp(&expected_id,&actual_id,sizeof(actual_id)));++pairs;
    }
}
static void edited(void)
{
    uint32_t values[sizeof(parameter_bits)/sizeof(*parameter_bits)];memcpy(values,parameter_bits,sizeof(values));
    float radius=8,height=25;memcpy(&values[param("cpu_din_release_radius")],&radius,4);
    memcpy(&values[param("cpu_din_target_height")],&height,4);
    values[param("cpu_din_wait_frames")]=3;values[param("cpu_din_stick_up")]=63;
    values[param("cpu_din_stick_down")]=(uint32_t)-63;load_parameters(values);
    for(unsigned mode=0;mode<3;++mode) {
        setup(0,344,(Vec3){-50,0,0},(Vec3){20,mode==0?25:mode==1?-20:60,0},(Vec3){20,0,0},0,true);
        CHECK(definition->on_cpu_travel_decision(&objects[0]));Result out=result();
        if(mode==0) CHECK(out.bytes==8&&out.buffer[0]==CpuCmd_ReleaseB&&out.buffer[6]==3);
        else CHECK(out.bytes==3&&out.buffer[0]==CpuCmd_SetLstickY&&out.buffer[1]==(mode==1?63:(unsigned char)-63));
        CHECK(out.travel==19);++edited_cases;
    }
    load_parameters(parameter_bits);
}
static void safety(void)
{
    for(unsigned invalid=0;invalid<6;++invalid) {
        setup(0,344,(Vec3){-50,0,0},(Vec3){10,0,0},(Vec3){20,0,0},0,true);
        switch(invalid) {
        case 0:handle(-1);break;
        case 1:item.x1C++;break;
        case 2:item.native_article_author_kind=Ft_Kind_CustomFirst+1;break;
        case 3:item.xB8_itemLogicTable=NULL;break;
        case 4:item.native_removal_in_progress=true;break;
        case 5:heads[HSD_GOBJ_PLINK_ITEM]=NULL;item_object.user_data=(void*)1;break;
        }
        CHECK(definition->on_cpu_travel_decision(&objects[0]));Result out=result();
        CHECK(out.bytes==1&&out.buffer[0]==CpuCmd_Done&&out.behavior==4&&out.has_target);++safety_cases;
    }
    for(unsigned callback=0;callback<2;++callback) for(unsigned query=1;query<=3;++query)
    for(unsigned change=1;change<=2;++change) {
        setup(0,344,(Vec3){-50,0,0},(Vec3){10,0,0},(Vec3){20,0,0},0,true);
        mutation_query=query;mutation_kind=change;
        bool handled=callback?definition->on_cpu_travel_decision(&objects[0]):definition->on_cpu_attack_decision(&objects[0]);
        CHECK(!handled);CHECK(query_count==query);
        DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
        CHECK(fighter->cpu.write_pos==fighter->cpu.buffer&&fighter->cpu.x90==19);++lifetime_cases;
    }
    setup(0,344,(Vec3){-50,0,0},(Vec3){10,0,0},(Vec3){20,0,0},0,true);
    mutation_query=2;mutation_kind=3;
    CHECK(definition->on_cpu_travel_decision(&objects[0]));Result out=result();
    CHECK(out.bytes==1&&out.buffer[0]==CpuCmd_Done&&out.behavior==7&&!out.has_target);++safety_cases;
}
int main(void)
{
    fighter=VirtualAlloc(NULL,sizeof(*fighter),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(fighter);
    base.fighter_root="ftDataZelda";base.source_kind=Ft_Kind_Zelda;base.animation_source_kind=Ft_Kind_Zelda;
    base.motion_state_first=341;base.motion_state_count=18;base.motion_states=motions;base.joint_count=118;
    base.special_ground_state[1]=344;base.special_air_state[1]=347;
    base.special_ground_state[3]=355;base.special_air_state[3]=357;
#include "hosts.inc"
    host.cpu_command=ftCo_800B463C;host.cpu_command_arg=ftCo_800B46B8;host.abs_float=fabsf;
    host.callback_source_alive=nativeCallbackSource_Alive;
    host.article_ref_primary_translation=translation;host.cpu_nearest_enemy_at=nearest;host.target_position=target_position;
    load_parameters(parameter_bits);definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
    CHECK(definition->on_cpu_attack_decision&&definition->on_cpu_travel_decision);
    matrix();edited();safety();CHECK(VirtualFree(fighter,0,MEM_RELEASE));
    printf("{\"checks\":%u,\"pairs\":%u,\"rounding_cases\":%u,\"large_cancellation_cases\":%u,\"maximum_threshold_delta\":%.17g,\"maximum_ordinary_threshold_delta\":%.17g,\"edited_cases\":%u,\"safety_cases\":%u,\"lifetime_cases\":%u,\"threshold_cases\":[",
        checks,pairs,rounding_cases,large_cancellation_cases,maximum_threshold_delta,
        maximum_ordinary_threshold_delta,edited_cases,safety_cases,lifetime_cases);
    for(unsigned i=0;i<rounding_cases;++i) {
        const ThresholdCase* t=&thresholds[i];
        printf("%s{\"motion\":%d,\"travel\":%s,\"enemy_y\":%.9g,\"projectile_y\":%.9g,\"original_distance\":%.17g,\"authored_distance\":%.17g,\"error\":%.17g,\"original_bytes\":%u,\"authored_bytes\":%u}",
            i?",":"",t->motion,t->callback?"true":"false",t->enemy_y,t->projectile_y,
            t->original,t->authored,t->error,t->original_bytes,t->authored_bytes);
    }
    puts("]}");
    return 0;
}
