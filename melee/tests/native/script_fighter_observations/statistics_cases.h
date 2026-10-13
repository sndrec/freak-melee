#include <melee/pl/player.h>
#include <melee/pl/plbonus.h>
#include <melee/pl/plbonuslib.h>
#include "native_action_count.inc"

static plActionStats action_stats[Gm_Player_NumMax];
static unsigned stat_reads,taunt_calls;
static int taunt_slot,taunt_secondary;
static bool taunt_detach;
static HSD_GObj* taunt_source;

plActionStats* Player_GetActionStats(int slot)
{CHECK(slot>=0&&slot<Gm_Player_NumMax);++stat_reads;return &action_stats[slot];}
void pl_80040120(int slot,int secondary)
{
    ++taunt_calls;taunt_slot=slot;taunt_secondary=secondary;
    if(taunt_detach)nativeScriptFighter_Detach(taunt_source);
}

static void statistics_cases(HSD_GObj* object)
{
    NativeScriptRuntimeContext c=context(object);
    Fighter* f=object->user_data;
    NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99},saved=out;
    const unsigned begin=offsetof(plActionStats,by_attack_hi);
    const unsigned count=(sizeof(plActionStats)-begin)/sizeof(u32);
    for(unsigned slot=0;slot<Gm_Player_NumMax;++slot) {
        for(unsigned id=0;id<count;++id) {
            u32 bits=(slot+1)*UINT32_C(0x192837)+id;
            memcpy((u8*)&action_stats[slot]+begin+id*sizeof(bits),&bits,sizeof(bits));
        }
        for(unsigned id=0;id<=count;++id) {
            NativeScriptValue args[2]={{.type=NativeScriptType_I64,.as.i64=slot},
                {.type=NativeScriptType_U64,.as.u64=id}};
            CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StatsActionCount,NULL,0,args,2,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_U32&&out.as.u64==pl_800386D8(&action_stats[slot],id));
            if(id==0x72)CHECK(out.as.u64==action_stats[slot].x598[6]);
            ++pairs;++cases;
        }
        f->player_id=slot;
        CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_FighterPlayerSlot,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_I32&&out.as.i64==slot);
        for(unsigned secondary=0;secondary<2;++secondary) {
            f->x221F_b4=secondary;
            CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StatsNotifyTaunt,NULL,0,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&taunt_slot==(int)slot&&taunt_secondary==(int)secondary);++cases;
        }
    }
    for(unsigned age=0;age<=UINT8_MAX;++age) {
        f->x682=age;
        CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_InputDownPressAge,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_I32&&out.as.i64==age);++cases;
    }
    f->player_id=2;f->x682=1;f->x221F_b4=0;
    CHECK(helper(object,"ObserveStatistics"));
    CHECK(field(object,"stat_count").as.u64==action_stats[2].x598[6]);
    CHECK(field(object,"stat_slot").as.i64==2&&field(object,"release_age").as.i64==1);++cases;
    const NativeScriptValue invalid[]={
        {.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I32,.as.i64=INT64_MAX},
        {.type=NativeScriptType_I64,.as.i64=-1},{.type=NativeScriptType_U64,.as.u64=UINT64_MAX},
        {.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_Bool,.as.u64=1}};
    unsigned reads=stat_reads;
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)for(unsigned selector=0;selector<2;++selector) {
        NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=0},
            {.type=NativeScriptType_I32,.as.i64=0x72}};args[selector]=invalid[i];out=saved;
        CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_StatsActionCount,NULL,0,args,2,&out,&diagnostic));
        CHECK(!memcmp(&out,&saved,sizeof(out))&&stat_reads==reads);++cases;
    }
    NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=Gm_Player_NumMax},
        {.type=NativeScriptType_I32,.as.i64=0x72}};out=saved;
    CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_StatsActionCount,NULL,0,args,2,&out,&diagnostic));
    CHECK(!memcmp(&out,&saved,sizeof(out))&&stat_reads==reads);++cases;
    f->player_id=Gm_Player_NumMax;out=saved;unsigned calls=taunt_calls;
    CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_StatsNotifyTaunt,NULL,0,NULL,0,&out,&diagnostic));
    CHECK(!memcmp(&out,&saved,sizeof(out))&&taunt_calls==calls);++cases;f->player_id=2;
    taunt_source=actor(0);taunt_detach=true;c=context(taunt_source);out=saved;
    CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_StatsNotifyTaunt,NULL,0,NULL,0,&out,&diagnostic));
    CHECK(!memcmp(&out,&saved,sizeof(out)));++cases;
    taunt_detach=false;HSD_GObjFree(taunt_source);taunt_source=NULL;
}
