#include "native_replay_item_fields.h"
#ifdef MELEE_NATIVE
#include <melee/it/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Host-only diagnostics, excluded from gameplay .mstate and its snapshots.
#pragma clang section data = ""
static struct {
    uint64_t first[16],last[16];
    unsigned count;
    bool initialized;
} selected;

static bool requested(uint64_t sample)
{
    if(!selected.initialized) {
        selected.initialized=true;
        const char* cursor=getenv("FREAK_MELEE_ITEM_FIELDS");
        while(cursor && *cursor && selected.count<16) {
            char* end;uint64_t first=strtoull(cursor,&end,10);
            if(end==cursor || *end!=':')break;
            cursor=end+1;uint64_t last=strtoull(cursor,&end,10);
            if(end==cursor || first>last || (*end && *end!=','))break;
            unsigned slot=selected.count++;
            selected.first[slot]=first;selected.last[slot]=last;
            cursor=*end?end+1:end;
        }
    }
    for(unsigned i=0;i<selected.count;++i)
        if(sample>=selected.first[i] && sample<=selected.last[i])return true;
    return false;
}
typedef struct TraceContext {
    uint64_t sample;unsigned index;const char* group;
} TraceContext;
static void value(const TraceContext* t,const char* name,uint32_t bits)
{
    fprintf(stderr,"[item-value] sample %llu I%u %s.%s=%08x\n",
        (unsigned long long)t->sample,t->index,t->group,name,bits);
}
static uint32_t float_bits(float f){uint32_t bits;memcpy(&bits,&f,sizeof(bits));return bits;}
#define U(name,expression) value(t,name,(uint32_t)(expression))
#define F(name,expression) value(t,name,float_bits(expression))
#define V(name,expression) do{F(name ".x",(expression).x);F(name ".y",(expression).y);F(name ".z",(expression).z);}while(0)
#define V2(name,expression) do{F(name ".x",(expression).x);F(name ".y",(expression).y);}while(0)
#define ECB(name,expression) do{V2(name ".top",(expression).top);V2(name ".bottom",(expression).bottom);V2(name ".right",(expression).right);V2(name ".left",(expression).left);}while(0)
#define SURFACE(name,expression) do{U(name ".index",(expression).index);U(name ".flags",(expression).flags);V(name ".normal",(expression).normal);}while(0)

static void collision(const TraceContext* t,const CollData* c)
{
    V("cur_pos",c->cur_pos);V("prev_pos",c->prev_pos);V("last_pos",c->last_pos);V("x28_vec",c->x28_vec);
#define FLAG(member) U(#member,c->member)
    FLAG(x34_flags.b0);FLAG(x34_flags.b1234);FLAG(x34_flags.b5);FLAG(x34_flags.b6);FLAG(x34_flags.b7);
    FLAG(x35_flags.b0);FLAG(x35_flags.b1234);FLAG(x35_flags.b5);FLAG(x35_flags.b6);FLAG(x35_flags.b7);
#undef FLAG
    U("facing_dir",c->facing_dir);U("floor_skip",c->floor_skip);
    U("ledge_id_right",c->ledge_id_right);U("ledge_id_left",c->ledge_id_left);
    U("joint_id_skip",c->joint_id_skip);U("joint_id_only",c->joint_id_only);
    ECB("x64_ecb",c->x64_ecb);ECB("desired_ecb",c->desired_ecb);ECB("ecb",c->ecb);ECB("prev_ecb",c->prev_ecb);
    U("x130_flags",c->x130_flags);U("env_flags",c->env_flags);U("prev_env_flags",c->prev_env_flags);
    V("contact",c->contact);SURFACE("floor",c->floor);SURFACE("left_facing_wall",c->left_facing_wall);
    SURFACE("right_facing_wall",c->right_facing_wall);SURFACE("ceiling",c->ceiling);
}
static void hit(const TraceContext* t,const HitCapsule* h,
    const NativeSemanticObjectMap* objects)
{
#define INTEGER(member) U(#member,h->member)
#define FLOAT(member) F(#member,h->member)
    INTEGER(state);INTEGER(x4);INTEGER(unk_count);FLOAT(damage);V("b_offset",h->b_offset);FLOAT(scale);
    INTEGER(kb_angle);INTEGER(x24);INTEGER(x28);INTEGER(x2C);INTEGER(element);
    INTEGER(x34);INTEGER(sfx_severity);INTEGER(sfx_kind);
    INTEGER(x40_b0);INTEGER(x40_b1);INTEGER(x40_b2);INTEGER(x40_b3);INTEGER(x40_b4);
    INTEGER(x41_b4);INTEGER(x41_b5);INTEGER(x41_b6);INTEGER(x41_b7);
    INTEGER(x42_b0);INTEGER(x42_b1);INTEGER(x42_b2);INTEGER(x42_b3);
    INTEGER(x42_b4);INTEGER(x42_b5);INTEGER(x42_b6);INTEGER(x42_b7);
    INTEGER(x43_b0);INTEGER(x43_b1);INTEGER(x43_b2);INTEGER(x43_b3);
    INTEGER(x43_b4);INTEGER(x43_b5);INTEGER(x43_b6);INTEGER(x43_b7);INTEGER(x44);INTEGER(x45);
    V("x4C",h->x4C);V("x58",h->x58);V("hurt_coll_pos",h->hurt_coll_pos);FLOAT(coll_distance);
    for(unsigned i=0;i<12;++i) {
        char name[40];
        snprintf(name,sizeof(name),"victims_1.%u.x4",i);U(name,h->victims_1[i].x4);
        snprintf(name,sizeof(name),"victims_2.%u.x4",i);U(name,h->victims_2[i].x4);
        snprintf(name,sizeof(name),"victims_1.%u.object",i);U(name,nativeSemanticState_ObjectId(objects,h->victims_1[i].victim));
        snprintf(name,sizeof(name),"victims_2.%u.object",i);U(name,nativeSemanticState_ObjectId(objects,h->victims_2[i].victim));
    }
    U("owner",nativeSemanticState_ObjectId(objects,h->owner));
#undef INTEGER
#undef FLOAT
}
void nativeReplayItemFields_Trace(uint64_t sample,unsigned index,const Item* item,
    const NativeSemanticObjectMap* objects,const NativeSemanticItemTrace* fields)
{
    if(!requested(sample))return;
    TraceContext context={sample,index,"motion"};const TraceContext* t=&context;
    V("velocity",item->x40_vel);V("position",item->pos);V("x58_vec_unk",item->x58_vec_unk);
    V("x64_vec_unk2",item->x64_vec_unk2);V("nudge",item->x70_nudge);U("ground_or_air",item->ground_or_air);
    context.group="collision";collision(t,&item->x378_itemColl);
    context.group="animation";
    F("command_timer",item->x524_cmd.timer);
    F("command_frame",item->x524_cmd.frame_count);
    F("frame",item->x5CC_currentAnimFrame);
    F("speed",item->x5D0_animFrameSpeed);
    U("command_active",item->x524_cmd.u!=NULL);
    for(unsigned i=0;i<4;++i) {
        char group[16];snprintf(group,sizeof(group),"hit%u",i);context.group=group;
        hit(t,&item->x5D4_hitboxes[i].hit,objects);U("x138",item->x5D4_hitboxes[i].x138);
    }
    context.group="relations";
#define OBJECT(member) U(#member,nativeSemanticState_ObjectId(objects,item->member))
    OBJECT(entity);OBJECT(owner);OBJECT(x51C);OBJECT(xC64_reflectGObj);OBJECT(xC90_absorbGObj);
    OBJECT(xCEC_fighterGObj);OBJECT(xCF0_itemGObj);OBJECT(xCF4_fighterGObjUnk);OBJECT(toucher);
    OBJECT(xCFC);OBJECT(grab_victim);OBJECT(atk_victim);
#undef OBJECT
    context.group="scalar";
    static const char* names[]={"damageDealt","xC9C","xCA0","xCA8","angle","hitlag","knockback",
        "spinSpeed","xD40","lifeTimer","halfLifeTimer","xD4C","landNum","throwNum","destroy_type",
        "attackID","attack_instance"};
    _Static_assert(sizeof(names)/sizeof(*names)==NATIVE_SEMANTIC_ITEM_SCALAR_COUNT,"scalar names");
    for(unsigned i=0;i<NATIVE_SEMANTIC_ITEM_SCALAR_COUNT;++i)U(names[i],fields->scalar[i]);
}
#undef U
#undef F
#undef V
#undef V2
#undef ECB
#undef SURFACE
#endif
