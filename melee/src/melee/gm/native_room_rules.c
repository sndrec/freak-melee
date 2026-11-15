#include "native_room_rules.h"
#include "types.h"
#include <string.h>

bool nativeRoomRules_Decode(NativeRoomRules* rules, const uint8_t* bytes, size_t size)
{
    static const uint8_t limits[] = { 2,99,99,2,20,4,99,1,1,1,2,4,1 };
    if (!rules || !bytes || size != NATIVE_ROOM_RULES_BYTES) return false;
    for (unsigned i=0;i<sizeof(limits);++i)
        if (bytes[i]>limits[i] && !(i==11 && bytes[i]==255)) return false;
    if (!bytes[2] || bytes[4]<5 || (bytes[0]!=1 && !bytes[1])) return false;
    NativeRoomRules next = { .mode=bytes[0], .time=bytes[1], .stocks=bytes[2],
        .handicap=bytes[3], .damage=bytes[4], .stage_policy=bytes[5], .stock_time=bytes[6],
        .friendly_fire=bytes[7], .pause=bytes[8], .score=bytes[9], .sd_penalty=bytes[10],
        .item_frequency=bytes[11], .teams=bytes[12] };
    for (unsigned i=13;i<21;++i) next.item_mask=(next.item_mask<<8)|bytes[i];
    for (unsigned i=21;i<25;++i) next.stage_mask=(next.stage_mask<<8)|bytes[i];
    *rules=next;
    return true;
}

bool nativeRoomRules_Encode(const NativeRoomRules* rules, uint8_t bytes[NATIVE_ROOM_RULES_BYTES])
{
    if (!rules || !bytes) return false;
    uint8_t next[NATIVE_ROOM_RULES_BYTES] = { rules->mode,rules->time,rules->stocks,
        rules->handicap,rules->damage,rules->stage_policy,rules->stock_time,
        rules->friendly_fire,rules->pause,rules->score,rules->sd_penalty,rules->item_frequency,rules->teams };
    uint64_t mask=rules->item_mask;
    for (int i=20;i>=13;--i) { next[i]=(uint8_t)mask; mask>>=8; }
    mask=rules->stage_mask;
    for (int i=24;i>=21;--i) { next[i]=(uint8_t)mask; mask>>=8; }
    NativeRoomRules checked;
    if (!nativeRoomRules_Decode(&checked,next,sizeof(next))) return false;
    memcpy(bytes,next,sizeof(next));
    return true;
}

void nativeRoomRules_Capture(NativeRoomRules* output, const GameRules* rules,
                             const struct GamePrefs* prefs, bool teams)
{
    *output=(NativeRoomRules){ .mode=rules->mode, .time=rules->time_limit,
        .stocks=rules->stock_count, .handicap=rules->handicap, .damage=rules->damage_ratio,
        .stage_policy=rules->stage_sel, .stock_time=rules->stock_time_limit,
        .friendly_fire=rules->friendly_fire, .pause=rules->pause, .score=rules->score_display,
        .sd_penalty=rules->unk_xc, .item_frequency=prefs->item_freq, .teams=teams,
        .item_mask=prefs->item_mask, .stage_mask=prefs->stage_mask };
}

void nativeRoomRules_Apply(const NativeRoomRules* source, GameRules* rules, struct GamePrefs* prefs)
{
    rules->mode=source->mode; rules->time_limit=source->time; rules->stock_count=source->stocks;
    rules->handicap=source->handicap; rules->damage_ratio=source->damage; rules->stage_sel=source->stage_policy;
    rules->stock_time_limit=source->stock_time; rules->friendly_fire=source->friendly_fire;
    rules->pause=source->pause; rules->score_display=source->score; rules->unk_xc=source->sd_penalty;
    prefs->item_freq=source->item_frequency; prefs->item_mask=source->item_mask; prefs->stage_mask=source->stage_mask;
}
