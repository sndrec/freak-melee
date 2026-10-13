#include <melee/gm/gm_1A45.h>
#include <melee/gm/gm_1A36.h>
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/gm/native_gm_input_snapshot.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <melee/gm/native_snapshot_census.h>
#include <melee/lb/lb_0195.h>
#include <melee/lb/native_input_clock_snapshot.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_controller_snapshot.h>
#include <sysdolphin/baselib/native_random_snapshot.h>
#include <sysdolphin/baselib/random.h>
#include <melee/pl/native_player_snapshot.h>
#include <melee/pl/player_internal.h>
#include <melee/pl/plstale.h>
#include <melee/pl/plattack.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/if/if_2F6E.h>
#include <melee/if/iftime_state_internal.h>
#include <melee/if/ifstock_state_internal.h>
#include <melee/gm/gm_score_state_internal.h>
#include <melee/if/ifmagnify.h>
#include <melee/sfx/crowdsfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned fixture_reads,fixture_motors,fixture_audio,fixture_constructor,fixture_frames;
static unsigned checks,refusals,activity_refusals,replayed;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static _Alignas(16) HSD_GObj objects[16];
static NativeMatchPoolSlot slots[16];
static uint32_t free_order[16];
static HSD_PadData queue[5];
static HSD_PadRumbleListData rumble[12];
static GameSceneInfo scene={GS_VS,NULL,NULL};
static NativeGmLoopSnapshot loop_saved,loop_end;
static NativeGmInputSnapshot mapped_saved,mapped_end;
static NativeInputClockSnapshot clock_saved,clock_end;
static NativeRandomSnapshot rng_saved,rng_end;
static unsigned char pad_saved[8192],pad_end[8192],pool_saved[8192];
static size_t pad_bytes,pad_end_bytes,pool_bytes;
static bool repeated,retirement_test;
static NativePlayerSnapshot player_saved,player_end;
static unsigned player_refusals;
static unsigned char fighter_saved[1024*1024];
static size_t fighter_bytes;
static pl_804D6470_t player_common;
extern unsigned fixture_camera_checks,fixture_camera_refusals;
void fixture_camera_init(void);
void fixture_camera_activity(void);
void fixture_camera_tick(void);
void fixture_camera_sample(unsigned frame,bool repeated,bool retired);
void fixture_camera_retired(void);

#define SAVE(fn, state) do { size_t bytes=0;CHECK(fn(&(state),sizeof(state),&bytes));CHECK(bytes==sizeof(state)); } while(0)

void fixture_activity(void)
{
    fixture_camera_activity();
    NativeGmLoopSnapshot state;
    size_t written=313;
    CHECK(!nativeGmLoop_Save(&state,sizeof(state),&written) && written==313);
    CHECK(!nativeGmLoop_ValidateSnapshot(&loop_saved,sizeof(loop_saved)));
    CHECK(!nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
    NativePlayerSnapshot player;
    CHECK(!nativePlayerSnapshot_Save(&player,sizeof(player),&written) && written==313);
    CHECK(!nativePlayerSnapshot_Validate(&player_saved,sizeof(player_saved)));
    CHECK(!nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
    ++activity_refusals;
}

static bool pause_input(void) { return gm_801A4BA8()%7==3; }
static bool frame_step_input(void) { return gm_801A4BA8()%7==5; }
static void before_process(void) { fixture_activity(); }
static void on_frame(void)
{
    fixture_camera_tick();
    fixture_activity();(void)HSD_Randi(15);
    for(unsigned slot=0;slot<4;++slot){
        Player_SetStocks(slot,99-(int)gm_801A4BA8()/7);
        Player_SetCoins(slot,Player_GetPtrForSlot(slot)->current_coins+1);
        Player_SetFalls(slot,(int)gm_801A4BA8()/7);
        Fighter* actor=player_slots[slot].player_entity[0]->user_data;
        actor->x2068_attackID=2+gm_801A4BA8()%5;
        actor->x206C_attack_instance=plStale_IncrementAttackInstance();
        (void)plAttack_80037B08();
        plStale_UpdateStaleMovesFromFighter(actor->gobj,player_slots[(slot+1)%4].player_entity[0]);
        Vec3 position={(float)gm_801A4BA8(),(float)slot,-3};
        Player_SetSpawnPlatformPos(slot,&position);
    }
}
void fixture_process(void) { fixture_activity(); }
static void initialize(void) { HSD_PadInit(5,queue,12,rumble); }

static void reject_loop(NativeGmLoopSnapshot state)
{
    NativeGmLoopSnapshot before,after;
    SAVE(nativeGmLoop_Save,before);
    CHECK(!nativeGmLoop_ValidateSnapshot(&state,sizeof(state)));
    CHECK(!nativeGmLoop_Load(&state,sizeof(state)));
    SAVE(nativeGmLoop_Save,after);
    CHECK(!memcmp(&before,&after,sizeof(before)));
    ++refusals;
}

static void exercise_refusals(void)
{
    NativeGmLoopSnapshot bad=loop_saved;
    ++bad.epoch;reject_loop(bad);
    bad=loop_saved;++bad.object_epoch;reject_loop(bad);
    bad=loop_saved;bad.scene=(GameSceneInfo*)1;reject_loop(bad);
    bad=loop_saved;bad.loop.unk_C=3;reject_loop(bad);
    bad=loop_saved;bad.loop.unk_10.unk_34=2;reject_loop(bad);
    bad=loop_saved;bad.loop.unk_10.pre_gobj_proc=on_frame;reject_loop(bad);
    bad=loop_saved;bad.loop.unk_10.db_input.check_pause=frame_step_input;reject_loop(bad);
    bad=loop_saved;++bad.config.p_link_max;reject_loop(bad);
    bad=loop_saved;bad.config.unk_2=NULL;reject_loop(bad);
    bad=loop_saved;bad.scheduler_object=(HSD_GObj*)((char*)bad.scheduler_object+1);reject_loop(bad);
    bad=loop_saved;bad.debug_display=objects;reject_loop(bad);
    bad=loop_saved;bad.debug_callback=on_frame;reject_loop(bad);
    // Structurally valid old-slot reference must also fail before any GM write
    // when its generation is not the restored companion generation.
    NativeGmLoopSnapshot before,after;
    SAVE(nativeGmLoop_Save,before);
    bad=loop_saved;++bad.scheduler_reference.generation;bad.loop.unk_0=912;
    CHECK(nativeGmLoop_ValidateSnapshot(&bad,sizeof(bad)));
    CHECK(!nativeGmLoop_Load(&bad,sizeof(bad)));
    SAVE(nativeGmLoop_Save,after);
    CHECK(!memcmp(&before,&after,sizeof(before)));
    ++refusals;
    void* original_enter=scene.enter_data;
    scene.enter_data=&before;
    CHECK(!nativeGmLoop_ValidateSnapshot(&loop_saved,sizeof(loop_saved)));
    CHECK(!nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
    scene.enter_data=original_enter;
    SAVE(nativeGmLoop_Save,after);
    CHECK(!memcmp(&before,&after,sizeof(before)));
    size_t written=314;
    CHECK(!nativeGmLoop_Save(&bad,sizeof(bad)-1,&written) && written==314);
    CHECK(!nativeGmLoop_Save(&bad,sizeof(bad),(size_t*)&bad));
    CHECK(!nativeGmLoop_Save(loop_saved.config.unk_2,sizeof(bad),&written));
    CHECK(!nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)-1));
    CHECK(!nativeGmLoop_Load(NULL,sizeof(loop_saved)));
    NativeGmInputSnapshot mapped=mapped_saved,check;
    mapped.repeat_callback=NULL;
    CHECK(!nativeGmInput_Load(&mapped,sizeof(mapped)));
    SAVE(nativeGmInput_Save,check);
    CHECK(!memcmp(&check,&mapped_saved,sizeof(check)));
    CHECK(!nativeGmInput_Save(&mapped,sizeof(mapped)-1,&written));
    CHECK(!nativeGmInput_Save(&mapped,sizeof(mapped),(size_t*)&mapped));
    CHECK(!nativeGmInput_Load(&mapped_saved,sizeof(mapped_saved)-1));

    // Candidate can describe an old occupied slot, but Load requires its exact
    // generation. Restore the actual companion pool first to resurrect it.
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,pool_saved,sizeof(pool_saved),&pool_bytes));
    CHECK(nativeMatchPool_Free(&nativeGObjPool,loop_saved.scheduler_reference));
    NativeMatchPoolReference replacement_ref;
    HSD_GObj* replacement=nativeMatchPool_Allocate(&nativeGObjPool,&replacement_ref);
    CHECK(replacement==loop_saved.scheduler_object);
    CHECK(nativeGmLoop_ValidateSnapshot(&loop_saved,sizeof(loop_saved)));
    CHECK(!nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,pool_saved,pool_bytes));
    CHECK(nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
}

static void reject_player(NativePlayerSnapshot* bad)
{
    NativePlayerSnapshot before,after;
    SAVE(nativePlayerSnapshot_Save,before);
    CHECK(!nativePlayerSnapshot_Validate(bad,sizeof(*bad)));
    CHECK(!nativePlayerSnapshot_Load(bad,sizeof(*bad)));
    SAVE(nativePlayerSnapshot_Save,after);
    CHECK(!memcmp(&before,&after,sizeof(before)));
    ++player_refusals;
}

static void player_refusal_test(void)
{
    NativePlayerSnapshot bad=player_saved;
    ++bad.header.epoch;reject_player(&bad);
    bad=player_saved;++bad.header.object_epoch;reject_player(&bad);
    bad=player_saved;++bad.header.fighter_epoch;reject_player(&bad);
    bad=player_saved;bad.header.common_data=(void*)1;reject_player(&bad);
    bad=player_saved;bad.header.reserved=1;reject_player(&bad);
    bad=player_saved;bad.header.stale_instance=0;reject_player(&bad);
    bad=player_saved;bad.header.attack_instance=0;reject_player(&bad);
    bad=player_saved;bad.players[3].transformed[0]=2;reject_player(&bad);
    bad=player_saved;bad.players[5].transformed[1]=0;reject_player(&bad);
    bad=player_saved;bad.players[4].stale_moves.current_index=10;reject_player(&bad);
    bad=player_saved;bad.players[1].player_character=ChKind_RuntimeCapacity;reject_player(&bad);
    bad=player_saved;bad.players[2].struct_func=(void(*)(s32))(uintptr_t)1;reject_player(&bad);
    bad=player_saved;bad.players[2].player_entity[0]=(void*)1;reject_player(&bad);
    bad=player_saved;bad.roots[4].object[0]=bad.roots[0].object[0];reject_player(&bad);
    bad=player_saved;bad.roots[2].fighter[0].slot=NATIVE_FIGHTER_POOL_CAPACITY;reject_player(&bad);
    bad=player_saved;bad.players[2].player_entity[0]=bad.players[0].player_entity[0];
    bad.roots[2].object[0]=bad.roots[0].object[0];reject_player(&bad);
    bad=player_saved;bad.roots[2].fighter[0]=bad.roots[0].fighter[0];reject_player(&bad);
    size_t written=992;
    CHECK(!nativePlayerSnapshot_Save(&bad,sizeof(bad)-1,&written) && written==992);
    CHECK(!nativePlayerSnapshot_Save(&bad,sizeof(bad),(size_t*)&bad));
    CHECK(!nativePlayerSnapshot_Save(player_slots,sizeof(bad),&written));
    CHECK(!nativePlayerSnapshot_Save(&bad,sizeof(bad),(size_t*)&staleAttackInstance));
    CHECK(!nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)-1));
    CHECK(!nativePlayerSnapshot_Load(NULL,sizeof(player_saved)));
    ++player_common.x4;
    CHECK(!nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
    --player_common.x4;
    ftMapping mapping=ftMapping_list[CKind_PopoNana];
    ftMapping_list[CKind_PopoNana].extra_internal_id=Ft_Kind_None;
    CHECK(!nativePlayerSnapshot_Validate(&player_saved,sizeof(player_saved)));
    ftMapping_list[CKind_PopoNana]=mapping;

    // Check the LAST actor so failure cannot silently commit earlier slots.
    NativeMatchPoolReference root=player_saved.roots[3].fighter[1];
    CHECK(nativeMatchPool_Free(&nativeFighterPool,root));
    NativeMatchPoolReference replacement;
    CHECK(nativeMatchPool_Allocate(&nativeFighterPool,&replacement));
    CHECK(replacement.slot==root.slot && replacement.generation!=root.generation);
    bad=player_saved;bad.players[0].stocks=1;
    NativePlayerSnapshot before=player_saved;
    CHECK(nativePlayerSnapshot_Validate(&bad,sizeof(bad)));
    CHECK(!nativePlayerSnapshot_Load(&bad,sizeof(bad)));
    CHECK(!memcmp(player_slots,before.players,sizeof(player_slots)));
    CHECK(nativeFighterPool_Load(fighter_saved,fighter_bytes));
    CHECK(nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));

    NativeMatchPoolReference object=player_saved.roots[3].object[1];
    CHECK(nativeMatchPool_Free(&nativeGObjPool,object));
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&replacement));
    CHECK(replacement.slot==object.slot && replacement.generation!=object.generation);
    CHECK(nativePlayerSnapshot_Validate(&player_saved,sizeof(player_saved)));
    CHECK(!nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,pool_saved,pool_bytes));
    CHECK(nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));

    // Transformed physical-role ordering and all extended counter bytes roundtrip.
    bad=player_saved;
    bad.players[0].transformed[0]=1;bad.players[0].transformed[1]=0;
    memset(bad.players[0].xDB0,0xA5,sizeof(bad.players[0].xDB0));
    CHECK(nativePlayerSnapshot_Load(&bad,sizeof(bad)));
    NativePlayerSnapshot after;
    SAVE(nativePlayerSnapshot_Save,after);
    CHECK(!memcmp(&bad,&after,sizeof(bad)));
    CHECK(nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
    staleAttackInstance=UINT16_MAX;unk_804D6480=UINT16_MAX;
    CHECK(plStale_IncrementAttackInstance()==UINT16_MAX && staleAttackInstance==1);
    CHECK(plAttack_80037B08()==UINT16_MAX && unk_804D6480==1);
    CHECK(nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
}

void nativeSnapshotCensus_Sample(uint32_t kind,uint64_t frame,uint64_t sample)
{
    (void)sample;
    CHECK(kind==GS_VS && frame==gm_801A4BA8());
    fixture_camera_sample((unsigned)frame,repeated,retirement_test);
    if(retirement_test){
        CHECK(!nativeGmLoop_ValidateSnapshot(&loop_saved,sizeof(loop_saved)));
        CHECK(!nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
        CHECK(!nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
        gm_801A4B60();return;
    }
    if(frame==8 && !repeated){
        SAVE(nativeGmLoop_Save,loop_saved);
        SAVE(nativeGmInput_Save,mapped_saved);
        SAVE(nativeInputClock_Save,clock_saved);
        SAVE(nativeRandom_Save,rng_saved);
        CHECK(nativeController_Save(pad_saved,sizeof(pad_saved),&pad_bytes));
        SAVE(nativePlayerSnapshot_Save,player_saved);
        CHECK(nativeFighterPool_Save(fighter_saved,sizeof(fighter_saved),&fighter_bytes));
        CHECK(loop_saved.scheduler_reference.generation==1);
        exercise_refusals();
        player_refusal_test();
    }
    if(frame!=24)return;
    NativeGmLoopSnapshot current_loop;
    NativeGmInputSnapshot current_mapped;
    NativeInputClockSnapshot current_clock;
    NativeRandomSnapshot current_rng;
    unsigned char current_pad[8192];size_t current_bytes;
    NativePlayerSnapshot current_player;
    SAVE(nativeGmLoop_Save,current_loop);
    SAVE(nativeGmInput_Save,current_mapped);
    SAVE(nativeInputClock_Save,current_clock);
    SAVE(nativeRandom_Save,current_rng);
    CHECK(nativeController_Save(current_pad,sizeof(current_pad),&current_bytes));
    SAVE(nativePlayerSnapshot_Save,current_player);
    if(!repeated){
        memcpy(&loop_end,&current_loop,sizeof(loop_end));
        memcpy(&mapped_end,&current_mapped,sizeof(mapped_end));
        memcpy(&clock_end,&current_clock,sizeof(clock_end));
        memcpy(&rng_end,&current_rng,sizeof(rng_end));
        memcpy(pad_end,current_pad,current_bytes);pad_end_bytes=current_bytes;
        memcpy(&player_end,&current_player,sizeof(player_end));
        unsigned reads=fixture_reads,motors=fixture_motors;
        unsigned audio=fixture_audio,constructors=fixture_constructor;
        CHECK(nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
        CHECK(nativeGmInput_Load(&mapped_saved,sizeof(mapped_saved)));
        CHECK(nativeInputClock_Load(&clock_saved,sizeof(clock_saved)));
        CHECK(nativeRandom_Load(&rng_saved,sizeof(rng_saved)));
        CHECK(nativeController_Load(pad_saved,pad_bytes));
        CHECK(nativeFighterPool_Load(fighter_saved,fighter_bytes));
        CHECK(nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
        CHECK(fixture_reads==reads && fixture_motors==motors &&
              fixture_audio==audio && fixture_constructor==constructors);
        repeated=true;
    }else{
        CHECK(!memcmp(&loop_end,&current_loop,sizeof(loop_end)));
        CHECK(!memcmp(&mapped_end,&current_mapped,sizeof(mapped_end)));
        CHECK(!memcmp(&clock_end,&current_clock,sizeof(clock_end)));
        CHECK(!memcmp(&rng_end,&current_rng,sizeof(rng_end)));
        CHECK(pad_end_bytes==current_bytes && !memcmp(pad_end,current_pad,current_bytes));
        CHECK(!memcmp(&player_end,&current_player,sizeof(player_end)));
        replayed=16;gm_801A4B60();
    }
}

static void mapping_test(void)
{
    NativeGmInputSnapshot initial,final,repeat;
    SAVE(nativeGmInput_Save,initial);
    CHECK(initial.ports[0].repeat_timer==20);
    for(unsigned pass=0;pass<2;++pass){
        if(pass) CHECK(nativeGmInput_Load(&initial,sizeof(initial)));
        for(unsigned frame=0;frame<128;++frame){
            memset(HSD_PadCopyStatus,0,sizeof(HSD_PadCopyStatus));
            for(unsigned i=0;i<4;++i){
                HSD_PadCopyStatus[i].button=PAD_BUTTON_A|PAD_BUTTON_START|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_STICK_UP;
                if(frame==0)HSD_PadCopyStatus[i].trigger=HSD_PadCopyStatus[i].button;
            }
            gm_EvaluateAllControllerInputs();
            CHECK(gm_GetButtonsPressed(4)&PAD_LRA_START);
            CHECK(gm_GetButtonsPressed(4)&PAD_CONFIRM);
            CHECK(gm_GetButtonsPressed(4)&PAD_ANY_UP);
        }
        if(!pass)SAVE(nativeGmInput_Save,final);else SAVE(nativeGmInput_Save,repeat);
    }
    CHECK(!memcmp(&final,&repeat,sizeof(final)));
    CHECK(final.ports[0].held_frames==100);
}

static void initialize_players(void)
{
    CHECK(nativeMatchScene_Begin());
    CHECK(nativeFighterPool_BeginScene(42));
    pl_804D6470=&player_common;
    plStale_InitAttackInstance();plAttack_80037590();
    for(unsigned slot=0;slot<Gm_Player_NumMax;++slot){
        Player_InitOrResetPlayer(slot);
        plStale_ResetStaleMoveTableForPlayer(slot);
        if(slot>=4)continue;
        Player_SetPlayerCharacter(slot,CKind_PopoNana);
        Player_SetSlottype(slot,Gm_PKind_Cpu);
        Player_SetStocks(slot,99);
        player_slots[slot].player_state=2;
        for(unsigned member=0;member<2;++member){
            NativeMatchPoolReference object_ref,actor_ref;
            HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&object_ref);
            NativeFighterPoolSlot* storage=nativeMatchPool_Allocate(&nativeFighterPool,&actor_ref);
            CHECK(object && storage);
            memset(object,0,sizeof(*object));memset(storage,0,sizeof(*storage));
            Fighter* actor=&storage->actor;
            actor->gobj=object;actor->player_id=(u8)slot;
            actor->kind=member ? Ft_Kind_Nana : Ft_Kind_Popo;
            actor->dat_attrs_backup=storage->attributes;actor->dat_attrs=storage->attributes;
            actor->parts=storage->parts;actor->dobj_list.data=storage->primary_dobjs;
            actor->x203C.data=storage->secondary_dobjs;
            object->classifier=HSD_GOBJ_CLASS_FIGHTER;object->user_data=actor;
            player_slots[slot].player_entity[member]=object;
        }
    }
}

int main(void)
{
    size_t bytes=19;
    CHECK(!nativeGmLoop_Save(&loop_saved,sizeof(loop_saved),&bytes) && bytes==19);
    NativeGmInputSnapshot zero;
    SAVE(nativeGmInput_Save,zero);
    CHECK(!zero.repeat_callback);
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),slots,free_order,
            16,sizeof(objects[0]),_Alignof(HSD_GObj),42));
    initialize_players();
    fixture_camera_init();
    lb_80019AAC(initialize);
    gm_801A4BD4();gm_801A4B88(&scene);
    mapping_test();gm_801A3E88();
    gm_SetDbPauseInputHandlers(pause_input,frame_step_input);
    gm_SetPreGObjProcCallback(before_process);
    gm_801A4B50(1);
    *seed_ptr=0x12345678;
    gm_801A4D34(on_frame,&scene);
    CHECK(replayed==16 && fixture_frames==40 && activity_refusals>80);
    CHECK(!nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
    CHECK(!nativePlayerSnapshot_Load(&player_saved,sizeof(player_saved)));
    // Re-entering the same scene pointer still retires the old loop history.
    retirement_test=true;
    gm_801A4D34(on_frame,&scene);
    CHECK(!nativeGmLoop_Load(&loop_saved,sizeof(loop_saved)));
    fixture_camera_retired();
    printf("{\"status\":\"PASS\",\"checks\":%u,\"camera_checks\":%u,\"camera_refusals\":%u,\"loop_bytes\":%zu,\"mapped_bytes\":%zu,\"player_bytes\":%zu,\"player_refusals\":%u,\"replayed_ticks\":%u,\"refusals\":%u,\"activity_refusals\":%u}\n",
        checks,fixture_camera_checks,fixture_camera_refusals,sizeof(loop_saved),sizeof(mapped_saved),sizeof(player_saved),player_refusals,replayed,refusals,activity_refusals);
    return 0;
}

bool nativeSnapshotCensus_IsResimulating(void) {return false;}
bool nativeSnapshotCensus_UsesReplayJournal(void) {return false;}
u32 it_804D6D10;
u32 it_804D6D14,it_804D6D18;
s8 it_804D6D00;
s32 it_804D6D08,it_804D6D0C;
u8 it_804D6D1C[4];
DamageLogEntry it_804A0E70[15];
Item_FtTrack Item_804A0CCC;
S32Vec3 Item_804A0E24;
ifMagnify ifMagnify_804A1DE0;
HSD_ObjAllocUnk Item_804A0C64;
int ftData_CharacterReferenceCounts[Ft_Kind_RuntimeCapacity];
Element_803F9628 ifStatus_803F9628[8];
IfTimeState if_time_state;
IfStockState if_stock_state;
GmScoreState gm_score_state;
HSD_CObj* devtext_cobj;
CrowdSFX_UnkStruct un_804A2F08;
CrowdSFX_UnkStruct* un_804D7050;
CrowdConfig* gCrowdConfig;
bool nativeSnapshotCensus_BeforeInput(void) {return false;}
bool nativeSnapshotCensus_TimingEnabled(void) {return false;}
void nativeSnapshotCensus_BeginTiming(enum NativeCompactTimingPhase phase) {(void)phase;abort();}
void nativeSnapshotCensus_EndTiming(enum NativeCompactTimingPhase phase) {(void)phase;abort();}
bool nativeSnapshotCensus_HasFailed(void) {return false;}
void HSD_AudioRollbackReset(void) {abort();}
bool HSD_AudioRollbackBeginFrame(uint64_t frame,bool resimulating) {(void)frame;(void)resimulating;abort();}
bool HSD_AudioRollbackEndFrame(void) {abort();}
void HSD_AudioRollbackAbortFrame(void) {abort();}

void nativeSnapshotCensus_Abort(const char* reason) {(void)reason;abort();}
