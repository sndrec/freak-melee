#include <melee/gm/native_match_control_history.h>
#include "native_versus_control_snapshot.h"
#include "gmvs_state_internal.h"
#include "native_gm_loop_snapshot.h"
#include <melee/if/ifstatus_state_internal.h>
#include <melee/if/ifmagnify.h>
#include <melee/it/itspawn_state_internal.h>
#include <melee/it/it_3F14.h>
#include <melee/it/item.h>
#include <melee/ft/ftdata.h>
#include <melee/if/if_2F6E.h>
#include <melee/if/ifnametag_state_internal.h>
#include <melee/if/textdraw.h>
#include <sysdolphin/baselib/cobj.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdlib.h>
#include <string.h>

// Definition/geometry lease only; actual gameplay state remains in its owners.
static struct {
    uint64_t epoch;
    NativeMatchPool objects;
    StartMeleeRules rules;
    u32* values[2];
    HudIndex hud;
    Element_803F9628 messages[8];
    IfTimeState timer_definitions;
    IfStockState stock_definitions;
    CrowdConfig* crowd_config;
    CrowdSFX_UnkStruct* crowd_source;
    ItemPickTable tables[3];
    bool active;
} scene;
static const ItemPickTable* table(unsigned i)
{return i==0 ? &it_804A0E30.pick_table : i==1 ? &it_804A0E50 : &it_804A0E60;}
static bool table_binding(const ItemPickTable* a,const ItemPickTable* b)
{return a->count==b->count && a->kinds==b->kinds && a->cumulative_weights==b->cumulative_weights;}
static bool rules_binding(const StartMeleeRules* a,const StartMeleeRules* b)
{
    return a->on_unpause_override==b->on_unpause_override && a->on_pause_override==b->on_pause_override &&
        a->check_for_pauser_override==b->check_for_pauser_override && a->on_match_start==b->on_match_start &&
        a->on_frame_start==b->on_frame_start && a->on_frame_end==b->on_frame_end &&
        a->on_match_end==b->on_match_end && a->x54==b->x54;
}
static bool hud_binding(const HudIndex* a,const HudIndex* b)
{
    return a->unk258==b->unk258 && a->damage_anims==b->damage_anims &&
        a->damage_matanims==b->damage_matanims && a->damage_shapeanims==b->damage_shapeanims &&
        a->unk268==b->unk268 && a->unk26C==b->unk26C && a->unk270==b->unk270 && a->unk274==b->unk274;
}
void nativeVersusControl_BeginScene(void)
{
    if(scene.epoch==UINT64_MAX)abort();++scene.epoch;scene.active=false;
    if(!nativeMatchPool_Validate(&nativeGObjPool))return;
    scene.objects=nativeGObjPool;scene.rules=vs_scene_controller.start;
    scene.values[0]=vs_scene_controller.unk_1C;scene.values[1]=vs_scene_controller.unk_20;
    scene.hud=ifStatus_HudInfo;
    if(un_804D7050 && un_804D7050!=&un_804A2F08)return;
    scene.crowd_config=gCrowdConfig;scene.crowd_source=un_804D7050;
    memcpy(scene.messages,ifStatus_803F9628,sizeof(scene.messages));
    scene.timer_definitions=if_time_state;
    scene.stock_definitions=if_stock_state;
    for(unsigned i=0;i<3;++i) {
        scene.tables[i]=*table(i);
        if(table(i)->count && (!table(i)->kinds || !table(i)->cumulative_weights))return;
    }
    scene.active=true;
}
void nativeVersusControl_EndScene(void) {scene.active=false;}
static bool boundary(void)
{
    return scene.active && nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle() &&
        nativeGObjPool.lifetime==scene.objects.lifetime && nativeGObjPool.payload==scene.objects.payload &&
        nativeGObjPool.stride==scene.objects.stride && nativeGObjPool.capacity==scene.objects.capacity &&
        gCrowdConfig==scene.crowd_config && un_804D7050==scene.crowd_source;
}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool independent(const void* data,size_t bytes)
{
    if(!data || !bytes || overlap(data,bytes,&scene,sizeof(scene)))return false;
#define EXCLUDE(v) if(overlap(data,bytes,&(v),sizeof(v)))return false
    EXCLUDE(vs_scene_controller);EXCLUDE(it_804A0E30);EXCLUDE(it_804A0E50);EXCLUDE(it_804A0E60);
    EXCLUDE(it_804D6D10);
    EXCLUDE(Item_804A0C64);EXCLUDE(ftData_CharacterReferenceCounts);
    EXCLUDE(ifStatus_803F9628);
    EXCLUDE(if_stock_indicators);
    EXCLUDE(if_time_state);
    EXCLUDE(if_stock_state);
    EXCLUDE(gm_score_state);
    EXCLUDE(ifAll_804A0FD8);EXCLUDE(hud_hidden);
    EXCLUDE(nametag_hidden);EXCLUDE(nametag_force_visible);
    EXCLUDE(devtext_cobj);
    EXCLUDE(un_804A2F08);EXCLUDE(un_804D7050);EXCLUDE(gCrowdConfig);
    EXCLUDE(ifStatus_HudInfo);EXCLUDE(hud_damage_display_count);EXCLUDE(hud_rounding_toggle);
#undef EXCLUDE
    for(unsigned i=0;i<3;++i) {
        const ItemPickTable* t=&scene.tables[i];
        if(t->count && (overlap(data,bytes,t->kinds,t->count) ||
            overlap(data,bytes,t->cumulative_weights,(size_t)t->count*sizeof(u16))))return false;
    }
    for(unsigned i=0;i<GM_MAX_PLAYERS;++i)if(ifStatus_HudInfo.players[i].HUD_parent_entity)
        for(unsigned j=0;j<HUD_PLACE_MAX;++j) {
            HSD_JObj* object=ifStatus_HudInfo.players[i].jobjs[j];
            if(!object)continue;
            NativeClassReference reference=nativeClass_Reference(object,sizeof(*object));
            if(!reference.bytes)return false;
            HSD_ClassInfo* type=HSD_CLASS_METHOD(object);
            if(!nativeClass_TypeKnown(type,sizeof(*object)) || overlap(data,bytes,object,sizeof(*object)) ||
               overlap(data,bytes,type,type->head.info_size))return false;
        }
    const NativeMatchPool* p=&scene.objects;
    return !overlap(data,bytes,p->payload,p->capacity*p->stride) &&
        !overlap(data,bytes,p->slots,p->capacity*sizeof(*p->slots)) &&
        !overlap(data,bytes,p->free_order,p->capacity*sizeof(*p->free_order)) && nativeObjAlloc_BufferIndependent(data,bytes);
}
static bool definitions(const NativeVersusControlSnapshot* s)
{
    if(s->timers.countdown_timer_models!=scene.timer_definitions.countdown_timer_models ||
        memcmp(s->timers.digits,scene.timer_definitions.digits,sizeof(s->timers.digits)))return false;
    if(s->stock_hud.hud.x0!=scene.stock_definitions.hud.x0 ||
        s->stock_hud.hud.x4!=scene.stock_definitions.hud.x4)return false;
    for(unsigned i=0;i<8;++i) {
        const Element_803F9628* a=&s->messages[i];const Element_803F9628* b=&scene.messages[i];
        if(a->x8!=b->x8 || a->xC!=b->xC || a->x10!=b->x10 || a->x14!=b->x14)return false;
    }
    return rules_binding(&s->versus.start,&scene.rules) &&
        s->versus.unk_1C==scene.values[0] && s->versus.unk_20==scene.values[1] &&
        hud_binding(&s->hud,&scene.hud) && table_binding(&s->spawner.pick_table,&scene.tables[0]) &&
        table_binding(&s->picks[0],&scene.tables[1]) && table_binding(&s->picks[1],&scene.tables[2]);
}
static void capture(NativeVersusControlSnapshot* s)
{
    // Item construction increments this identity even when the item is removed
    // before capture. Occupied pool records cannot recover the next serial.
    s->item_serial=it_804D6D10;
    s->score_state=gm_score_state;
    s->item_population=Item_804A0C64;
    memcpy(s->fighter_references,ftData_CharacterReferenceCounts,sizeof(s->fighter_references));
    memcpy(s->messages,ifStatus_803F9628,sizeof(s->messages));
    memcpy(s->stock_objects,if_stock_indicators+1,sizeof(s->stock_objects));
    s->timers=if_time_state;
    s->stock_hud=if_stock_state;
    s->hud_timer_position=ifAll_804A0FD8.xC;s->hud_hidden=hud_hidden;
    s->nametag_hidden=nametag_hidden;
    s->debug_text_camera=devtext_cobj;
    s->crowd=un_804A2F08;
    memcpy(s->nametag_force_visible,nametag_force_visible,sizeof(s->nametag_force_visible));
    memcpy(s->hud_player_positions,ifAll_804A0FD8.x18,sizeof(s->hud_player_positions));
    memcpy(s->hud_three_positions,ifAll_804A0FD8.x60,sizeof(s->hud_three_positions));
    memcpy(s->hud_two_positions,ifAll_804A0FD8.x84,sizeof(s->hud_two_positions));
    s->versus=vs_scene_controller;s->spawner=it_804A0E30;s->picks[0]=it_804A0E50;s->picks[1]=it_804A0E60;
    s->hud=ifStatus_HudInfo;s->hud_count=hud_damage_display_count;s->rounding=hud_rounding_toggle;
}
static bool object_shape(const void* object,NativeMatchPoolReference r)
{
    static const NativeMatchPoolReference zero={0};
    if(!object)return !memcmp(&r,&zero,sizeof(r));
    return r.pool==&nativeGObjPool && r.lifetime==scene.objects.lifetime && r.generation &&
        r.slot<scene.objects.capacity && object==scene.objects.payload+(size_t)r.slot*scene.objects.stride;
}
static HSD_GObj* stock_object(const IfStockState* s,unsigned i)
{
    if(i<6)return s->hud.player[i].x0;
    i-=6;if(i==0)return s->hud.gobj;
    --i;if(i<16)return s->stage_indicators[i];
    i-=16;if(i<131)return s->score_queue.x10C[i];
    i-=131;if(i==0)return s->fall_queue.x108;
    return s->fall_queue.x10C[i-1];
}
bool nativeVersusControl_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes!=sizeof(NativeVersusControlSnapshot) || !independent(data,bytes))return false;
    NativeVersusControlSnapshot s;memcpy(&s,data,sizeof(s));
    if(s.epoch!=scene.epoch || s.hud_hidden>1 || !definitions(&s))return false;
    if(s.score_state.flags.initialized>1 || s.score_state.flags.track_leaders>1 ||
        s.score_state.predicates_disabled>1)return false;
    for(unsigned i=0;i<6;++i)if(s.score_state.flags.players[i].zero_score>1 ||
        s.score_state.flags.players[i].leader>1)return false;
    if(s.nametag_hidden>1)return false;
    for(unsigned i=0;i<Gm_Player_NumMax;++i)if(s.nametag_force_visible[i]>1)return false;
    if(s.debug_text_camera) {
        if(s.debug_text_camera_backing.bytes!=sizeof(HSD_CObj) ||
           nativeClass_Resolve(s.debug_text_camera_backing)!=s.debug_text_camera ||
           !nativeClass_TypeKnown(s.debug_text_camera_type,sizeof(HSD_CObj)) ||
           overlap(data,bytes,s.debug_text_camera,sizeof(HSD_CObj)))return false;
    } else {
        static const NativeClassReference zero={0};
        if(s.debug_text_camera_type || memcmp(&s.debug_text_camera_backing,&zero,sizeof(zero)))return false;
    }
    for(unsigned i=0;i<8;++i)if(!object_shape(s.messages[i].x0,s.message_objects[i]))return false;
    for(unsigned i=0;i<12;++i)if(!object_shape(s.stock_objects[i],s.stock_object_refs[i]))return false;
    if(!object_shape(s.timers.match_timer,s.timer_objects[0]) ||
       !object_shape(s.timers.countdown_timer,s.timer_objects[1]))return false;
    for(unsigned i=0;i<IfStock_ObjectCount;++i)
        if(!object_shape(stock_object(&s.stock_hud,i),s.stock_hud_objects[i]))return false;
    for(unsigned i=0;i<Ft_Kind_RuntimeCapacity;++i)if(s.fighter_references[i]<0)return false;
    for(unsigned i=0;i<GM_MAX_PLAYERS;++i) {
        const IfDamageState* p=&s.hud.players[i];
        if(!object_shape(p->HUD_parent_entity,s.objects[i][0]) || !object_shape(p->next,s.objects[i][1]))return false;
        for(unsigned j=0;j<HUD_PLACE_MAX;++j) {
            NativeClassReference r=s.joints[i][j];
            if(!p->HUD_parent_entity || !p->jobjs[j]) {
                static const NativeClassReference zero={0};
                if(s.joint_types[i][j] || memcmp(&r,&zero,sizeof(r)))return false;
            } else if(r.bytes!=sizeof(HSD_JObj) || nativeClass_Resolve(r)!=p->jobjs[j] ||
                !nativeClass_TypeKnown(s.joint_types[i][j],sizeof(HSD_JObj)) ||
                overlap(data,bytes,p->jobjs[j],sizeof(HSD_JObj)))return false;
        }
    }
    return true;
}
bool nativeVersusControl_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeVersusControlSnapshot);
    if(!boundary() || capacity<bytes || !independent(data,bytes) || !independent(written,sizeof(*written)) ||
       overlap(data,bytes,written,sizeof(*written)))return false;
    NativeVersusControlSnapshot s;memset(&s,0,sizeof(s));s.epoch=scene.epoch;capture(&s);
    if(!definitions(&s))return false;
    if(s.debug_text_camera) {
        s.debug_text_camera_backing=nativeClass_Reference(s.debug_text_camera,sizeof(HSD_CObj));
        if(!s.debug_text_camera_backing.bytes)return false;
        s.debug_text_camera_type=HSD_CLASS_METHOD(s.debug_text_camera);
    }
    for(unsigned i=0;i<8;++i)
        if(s.messages[i].x0 && !nativeMatchPool_Reference(&nativeGObjPool,s.messages[i].x0,&s.message_objects[i]))return false;
    for(unsigned i=0;i<12;++i)
        if(s.stock_objects[i] && !nativeMatchPool_Reference(&nativeGObjPool,s.stock_objects[i],&s.stock_object_refs[i]))return false;
    HSD_GObj* timers[2]={s.timers.match_timer,s.timers.countdown_timer};
    for(unsigned i=0;i<2;++i)
        if(timers[i] && !nativeMatchPool_Reference(&nativeGObjPool,timers[i],&s.timer_objects[i]))return false;
    for(unsigned i=0;i<IfStock_ObjectCount;++i) {
        HSD_GObj* object=stock_object(&s.stock_hud,i);
        if(object && !nativeMatchPool_Reference(&nativeGObjPool,object,&s.stock_hud_objects[i]))return false;
    }
    for(unsigned i=0;i<3;++i) {
        const ItemPickTable* t=table(i);
        if(t->count) {memcpy(s.kinds[i],t->kinds,t->count);memcpy(s.weights[i],t->cumulative_weights,(size_t)t->count*sizeof(u16));}
    }
    for(unsigned i=0;i<GM_MAX_PLAYERS;++i) {
        const IfDamageState* p=&s.hud.players[i];
        if(p->HUD_parent_entity && !nativeMatchPool_Reference(&nativeGObjPool,p->HUD_parent_entity,&s.objects[i][0]))return false;
        if(p->next && !nativeMatchPool_Reference(&nativeGObjPool,p->next,&s.objects[i][1]))return false;
        if(p->HUD_parent_entity)for(unsigned j=0;j<HUD_PLACE_MAX;++j)if(p->jobjs[j]) {
            s.joints[i][j]=nativeClass_Reference(p->jobjs[j],sizeof(HSD_JObj));if(!s.joints[i][j].bytes)return false;
            s.joint_types[i][j]=HSD_CLASS_METHOD(p->jobjs[j]);
        }
    }
    if(!nativeVersusControl_Validate(&s,bytes))return false;
    memcpy(data,&s,bytes);*written=bytes;return true;
}
bool nativeVersusControl_ValidateClass(const void* data,size_t bytes,NativeClassSnapshotView view)
{
    if(!nativeVersusControl_Validate(data,bytes))return false;
    NativeVersusControlSnapshot s;memcpy(&s,data,sizeof(s));
    if(s.debug_text_camera && !nativeClass_SnapshotObjectType(view,s.debug_text_camera_backing,s.debug_text_camera_type))return false;
    for(unsigned i=0;i<GM_MAX_PLAYERS;++i)for(unsigned j=0;j<HUD_PLACE_MAX;++j)
        if(s.joints[i][j].bytes && !nativeClass_SnapshotObjectType(view,s.joints[i][j],s.joint_types[i][j]))return false;
    return true;
}
bool nativeVersusControl_Load(const void* data,size_t bytes)
{
    if(!nativeVersusControl_Validate(data,bytes))return false;
    NativeVersusControlSnapshot s;memcpy(&s,data,sizeof(s));
    NativeVersusControlSnapshot current;memset(&current,0,sizeof(current));capture(&current);
    if(!definitions(&current))return false;
    if(s.debug_text_camera && HSD_CLASS_METHOD(s.debug_text_camera)!=s.debug_text_camera_type)return false;
    for(unsigned i=0;i<8;++i)if(s.messages[i].x0) {
        HSD_GObj* object=nativeMatchPool_Resolve(s.message_objects[i]);
        if(object!=s.messages[i].x0 || object->classifier!=0xE || object->p_link!=0xE ||
           object->obj_kind!=HSD_GObj_JObjKind)return false;
    }
    for(unsigned i=0;i<12;++i)if(s.stock_objects[i] &&
        nativeMatchPool_Resolve(s.stock_object_refs[i])!=s.stock_objects[i])return false;
    HSD_GObj* timers[2]={s.timers.match_timer,s.timers.countdown_timer};
    for(unsigned i=0;i<2;++i)if(timers[i] && nativeMatchPool_Resolve(s.timer_objects[i])!=timers[i])return false;
    for(unsigned i=0;i<IfStock_ObjectCount;++i) {
        HSD_GObj* object=stock_object(&s.stock_hud,i);
        if(object && nativeMatchPool_Resolve(s.stock_hud_objects[i])!=object)return false;
    }
    for(unsigned i=0;i<GM_MAX_PLAYERS;++i) {
        const IfDamageState* p=&s.hud.players[i];
        if(p->HUD_parent_entity && nativeMatchPool_Resolve(s.objects[i][0])!=p->HUD_parent_entity)return false;
        if(p->next && nativeMatchPool_Resolve(s.objects[i][1])!=p->next)return false;
        for(unsigned j=0;j<HUD_PLACE_MAX;++j)if(s.joints[i][j].bytes && HSD_CLASS_METHOD(p->jobjs[j])!=s.joint_types[i][j])return false;
    }
    vs_scene_controller=s.versus;it_804A0E30=s.spawner;it_804A0E50=s.picks[0];it_804A0E60=s.picks[1];
    gm_score_state=s.score_state;
    it_804D6D10=s.item_serial;
    Item_804A0C64=s.item_population;
    memcpy(ftData_CharacterReferenceCounts,s.fighter_references,sizeof(s.fighter_references));
    memcpy(ifStatus_803F9628,s.messages,sizeof(s.messages));
    memcpy(if_stock_indicators+1,s.stock_objects,sizeof(s.stock_objects));
    if_time_state=s.timers;
    if_stock_state=s.stock_hud;
    ifAll_804A0FD8.xC=s.hud_timer_position;hud_hidden=s.hud_hidden;
    nametag_hidden=s.nametag_hidden;
    devtext_cobj=s.debug_text_camera;
    un_804A2F08=s.crowd;
    memcpy(nametag_force_visible,s.nametag_force_visible,sizeof(nametag_force_visible));
    memcpy(ifAll_804A0FD8.x18,s.hud_player_positions,sizeof(s.hud_player_positions));
    memcpy(ifAll_804A0FD8.x60,s.hud_three_positions,sizeof(s.hud_three_positions));
    memcpy(ifAll_804A0FD8.x84,s.hud_two_positions,sizeof(s.hud_two_positions));
    ifStatus_HudInfo=s.hud;hud_damage_display_count=s.hud_count;hud_rounding_toggle=s.rounding;
    for(unsigned i=0;i<3;++i) {
        const ItemPickTable* t=table(i);
        if(t->count) {memcpy(t->kinds,s.kinds[i],t->count);memcpy(t->cumulative_weights,s.weights[i],(size_t)t->count*sizeof(u16));}
    }
    return true;
}

#ifdef MELEE_NATIVE
bool nativeVersusControl_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(vs_scene_controller);
    ROOT(gm_score_state);
    ROOT(it_804A0E30);
    ROOT(it_804A0E50);
    ROOT(it_804A0E60);
    ROOT(it_804D6D10);
    ROOT(Item_804A0C64);
    ROOT(Item_804A0CCC);
    ROOT(Item_804A0E24);
    ROOT(it_804A0E70);
    ROOT(it_804D6D00);
    ROOT(it_804D6D08);
    ROOT(it_804D6D0C);
    ROOT(it_804D6D14);
    ROOT(it_804D6D18);
    ROOT(it_804D6D1C);
    ROOT(ftData_CharacterReferenceCounts);
    ROOT(ifStatus_803F9628);
    ROOT(if_time_state);
    ROOT(if_stock_state);
    if(!nativeMatchArena_AddRoot(arena,if_stock_indicators+1,12*sizeof(void*)))return false;
    ROOT(ifAll_804A0FD8.xC);
    ROOT(hud_hidden);
    ROOT(nametag_hidden);
    ROOT(devtext_cobj);
    ROOT(un_804A2F08);
    ROOT(nametag_force_visible);
    ROOT(ifAll_804A0FD8.x18);
    ROOT(ifAll_804A0FD8.x60);
    ROOT(ifAll_804A0FD8.x84);
    ROOT(ifStatus_HudInfo);
    ROOT(hud_damage_display_count);
    ROOT(hud_rounding_toggle);
    // Drawn magnifier state is also read by fighter offscreen damage/recovery
    // and player statistics on the following simulation tick.
    ROOT(ifMagnify_804A1DE0);
    for(unsigned i=0;i<3;++i){const ItemPickTable* t=table(i);
        if(t->count && (!nativeMatchArena_AddRoot(arena,t->kinds,t->count) ||
            !nativeMatchArena_AddRoot(arena,t->cumulative_weights,t->count*sizeof(*t->cumulative_weights))))return false;
    }
#undef ROOT
    return true;
}
#endif
