#include "native_match_control_history.h"
#include "native_snapshot_census.h"
#include "native_match_snapshot.h"
#include "native_match_history.h"
#include "native_match_save_profile.h"
#include "native_match_state_admission.h"
#include "native_replay.h"
#include "native_match_launch.h"
#include "native_snapshot_endpoint.h"
#include "native_match_snapshot_pool.h"
#include "forward.h"
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/mp/native_collision_state.h>
#include <melee/mp/native_collision_state_internal.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/audio_rollback.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/gr/native_stage_start_snapshot.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_item_link_pool.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/ef/native_script_effect_actor.h>
#include <melee/it/native_article_resource_scene.h>
#include <melee/it/native_article_resource_model.h>
#include <sysdolphin/baselib/native_gobj_snapshot.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL_timer.h>

enum { ScratchBytes = 64 * 1024 * 1024, SampleInterval = 120,
       Owners = NativeMatchSnapshot_Count, EndpointBytes=512*1024*1024, RewindSample=240,
       };
typedef struct OwnerCensus {
    const char* name;
    size_t peak_bytes;
    uint32_t attempts, saved, validated;
    uint64_t query_ns, save_ns, validate_ns;
} OwnerCensus;

// MELEE_ROLLBACK_HOST_STATE: one transition-time reservation, no tick allocation.
static struct {
    void* backing;
    void* comparison;
    void* rewind_backing;
    void* endpoint_backing;
    NativeSnapshotEndpoint endpoint,rewind_endpoint;
    NativeMatchSnapshotPart rewind_parts[Owners];
    NativeReplayActionTraceCheckpoint trace_rewind,trace_endpoint;
    uint64_t rewind_sample;
    bool rewind,rewind_pending,resimulate,resimulating;
    bool correction;
    unsigned correction_pass;
    unsigned rewind_depth,rewind_rounds,completed_rounds;
    unsigned target_sample;
    bool history_mode;
    bool before_input;
    bool benchmark_compact;
    bool timing_enabled;
    uint64_t timing_start,timing_restore,timing_history;
    uint64_t phase_start[NativeCompactTiming_Count],phase_ns[NativeCompactTiming_Count];
    unsigned phase_calls[NativeCompactTiming_Count],history_calls;
    void* history_backing;
    NativeMatchHistory history;
    HSD_AudioRollbackStats correction_audio;
    unsigned resim_compared,resim_ticks;
    FILE* resim_raw;
    FILE* resim_parts;
    unsigned rewind_compared,rewind_loads,endpoint_restores;
    size_t cursor;
    NativeMatchSnapshotPart parts[Owners],comparison_parts[Owners];
    NativeMatchSnapshotMeasurement measurements[Owners];
    FILE* output;
    OwnerCensus owners[Owners];
    uint64_t last_sample, samples;
    bool active, failed, self_restore;
    NativeMatchArenaFrame arena_frame;
    uint32_t restored, compared;
} census;

bool nativeSnapshotCensus_TimingEnabled(void) {return census.active && census.timing_enabled;}
void nativeSnapshotCensus_BeginTiming(enum NativeCompactTimingPhase phase)
{
    if(census.timing_enabled && census.resimulating && (unsigned)phase<NativeCompactTiming_Count)
        census.phase_start[phase]=SDL_GetTicksNS();
}
void nativeSnapshotCensus_EndTiming(enum NativeCompactTimingPhase phase)
{
    if(census.timing_enabled && census.resimulating && (unsigned)phase<NativeCompactTiming_Count) {
        census.phase_ns[phase]+=SDL_GetTicksNS()-census.phase_start[phase];
        ++census.phase_calls[phase];
    }
}
static void begin_replay_timing(uint64_t restore)
{
    if(!census.timing_enabled)return;
    memset(census.phase_ns,0,sizeof(census.phase_ns));
    memset(census.phase_calls,0,sizeof(census.phase_calls));
    census.timing_history=0;census.history_calls=0;census.timing_restore=restore;
#if defined(MELEE_PROFILE_MATCH_SAVE)
    nativeMatchSaveProfile_Begin();
#endif
    census.timing_start=SDL_GetTicksNS();
}

void nativeSnapshotCensus_Abort(const char* reason)
{
    census.failed = true;
    if(census.output)fflush(census.output);
    fprintf(stderr, "[snapshot-census] error: %s\n", reason);
}

static bool configure_unsigned(const char* name,unsigned fallback,unsigned maximum,unsigned* value)
{
    const char* text=getenv(name);
    if(!text || !*text) {*value=fallback;return true;}
    unsigned parsed=0;
    for(const char* at=text;*at;++at) {
        if(*at<'0' || *at>'9' || parsed>maximum/10)return false;
        parsed=parsed*10+(unsigned)(*at-'0');
        if(parsed>maximum)return false;
    }
    *value=parsed;return true;
}

static void report_pool_capacities(const char* phase)
{
    const NativeMatchPool* pools[]={&nativeFighterPool,&nativeGroundPool,
        &nativeItemPool,&nativeItemLinkPool,&nativeGObjPool,&nativeGObjProcPool,
        &nativeAObjPool,&nativeFObjPool};
    const char* names[]={"fighter","ground","item","item-link","gobj",
        "gobj-proc","aobj","fobj"};
    for(unsigned i=0;i<sizeof(pools)/sizeof(*pools);++i) {
        const NativeMatchPool* pool=pools[i];
        fprintf(census.output,"[snapshot-census] pool-capacity phase=%s owner=%s capacity=%u used=%u peak=%u initialized=%u stride=%zu bytes=%zu\n",
            phase,names[i],pool->capacity,pool->used,pool->peak,
            pool->initialized_count,pool->stride,pool->stride*pool->capacity);
    }
}

bool nativeSnapshotCensus_Init(void)
{
#if defined(MELEE_PROFILE_MATCH_SAVE)
    const char* save_profile=getenv("FREAK_MELEE_PROFILE_MATCH_SAVE");
    nativeMatchSaveProfile.enabled=save_profile && !strcmp(save_profile,"1");
    const char* detail_profile=getenv("FREAK_MELEE_PROFILE_MATCH_SAVE_DETAIL");
    nativeMatchSaveProfile.detailed=detail_profile && !strcmp(detail_profile,"1");
#else
    const char* save_profile=getenv("FREAK_MELEE_PROFILE_MATCH_SAVE");
    if(save_profile && !strcmp(save_profile,"1")) {
        nativeSnapshotCensus_Abort("save profiler requires MELEE_PROFILE_MATCH_SAVE build option");
        return false;
    }
#endif
    if (census.active || census.backing || census.output) {
        nativeSnapshotCensus_Abort("already initialized");
        return false;
    }
    const char* path = getenv("FREAK_MELEE_SNAPSHOT_CENSUS");
    census.output = path && *path ? fopen(path, "wb") : stdout;
    if (!census.output) {
        nativeSnapshotCensus_Abort("cannot open diagnostic output");
        return false;
    }
    const char* restore=getenv("FREAK_MELEE_SNAPSHOT_SELF_RESTORE");
    const char* rewind=getenv("FREAK_MELEE_SNAPSHOT_REWIND");
    const char* resimulate=getenv("FREAK_MELEE_SNAPSHOT_RESIMULATE");
    census.resimulate=resimulate && !strcmp(resimulate,"1");
    const char* corrected=getenv("FREAK_MELEE_SNAPSHOT_CORRECTED_INPUTS");
    census.correction=corrected && *corrected;
    const char* history=getenv("FREAK_MELEE_SNAPSHOT_HISTORY");
    census.history_mode=history && !strcmp(history,"1");
    const char* phase=getenv("FREAK_MELEE_SNAPSHOT_BEFORE_INPUT");
    census.before_input=phase && !strcmp(phase,"1");
    const char* benchmark=getenv("FREAK_MELEE_COMPACT_SNAPSHOT_BENCHMARK");
    census.benchmark_compact=benchmark && !strcmp(benchmark,"1");
    const char* timing=getenv("FREAK_MELEE_COMPACT_REPLAY_TIMING");
    census.timing_enabled=timing && !strcmp(timing,"1");
    if(census.before_input && !census.history_mode) {
        nativeSnapshotCensus_Abort("pre-input experiment requires per-frame history");return false;
    }
    if(census.history_mode && !census.resimulate) {
        nativeSnapshotCensus_Abort("history experiment requires compact resimulation");return false;
    }
    // Fifteen frames fit the existing sixteen-frame audio intent history.
    if(!configure_unsigned("FREAK_MELEE_SNAPSHOT_REWIND_DEPTH",8,15,&census.rewind_depth) ||
       !configure_unsigned("FREAK_MELEE_SNAPSHOT_REWIND_ROUNDS",1,64,&census.rewind_rounds) ||
       !configure_unsigned("FREAK_MELEE_SNAPSHOT_REWIND_TARGET",RewindSample,1000000,&census.target_sample) ||
       !census.rewind_depth || !census.rewind_rounds ||
       (census.correction && census.rewind_rounds!=1)) {
        nativeSnapshotCensus_Abort("invalid depth/round count; corrected reference currently covers one interval");return false;
    }
    if(census.correction && (!census.resimulate ||
        !nativeReplay_PrepareAlternateInputs(corrected))) {
        nativeSnapshotCensus_Abort("correction requires a prepared matching inputs-only replay");return false;
    }
    census.rewind=census.resimulate || (rewind && !strcmp(rewind,"1"));
    if(census.resimulate && (nativeMatchLaunch_RollbackProbe() || nativeMatchLaunch_RollbackBenchmark() || nativeMatchLaunch_RollbackCensus() || nativeMatchLaunch_Netplay())) {
        nativeSnapshotCensus_Abort("compact resimulation requires an exclusive offline replay");return false;
    }
    if(census.resimulate) {
        const char* raw=getenv("FREAK_MELEE_SNAPSHOT_RESIMULATION_RAW");
        census.resim_raw=raw && *raw ? fopen(raw,"wb") : NULL;
        if(!census.resim_raw) {nativeSnapshotCensus_Abort("resimulation requires independent raw output path");return false;}
        char parts_path[4096];int length=snprintf(parts_path,sizeof(parts_path),"%s.parts",raw);
        census.resim_parts=length>0 && (size_t)length<sizeof(parts_path) ? fopen(parts_path,"wb") : NULL;
        if(!census.resim_parts) {nativeSnapshotCensus_Abort("cannot reserve resimulation mismatch output");return false;}
    }
    census.self_restore=census.rewind || (restore && !strcmp(restore,"1"));
    census.backing=malloc(ScratchBytes);
    census.comparison=census.self_restore ? malloc(ScratchBytes) : NULL;
    census.rewind_backing=census.rewind ? malloc(ScratchBytes) : NULL;
    census.endpoint_backing=census.rewind ? malloc(EndpointBytes) : NULL;
    if (!census.backing || (census.self_restore && !census.comparison) ||
        (census.rewind && (!census.rewind_backing || !census.endpoint_backing))) {
        nativeSnapshotCensus_Abort("cannot reserve host scratch");
        return false;
    }
    // Touch before gameplay; first-use host page faults must not be measured as
    // serializer work. This buffer does not use HSD/OS allocators or MEM1.
    memset(census.backing,0,ScratchBytes);
    if(census.comparison)memset(census.comparison,0,ScratchBytes);
    census.active = true;
    fprintf(census.output,
        "[snapshot-census] partial-components scratch_bytes=%u interval=%u "
        "restore=%s drawing=retained validation=current-live\n",
        ScratchBytes,SampleInterval,census.self_restore ? "same-boundary" : "none");
    return true;
}

void nativeSnapshotCensus_AdmitArenas(uint32_t scene)
{
    if(!census.active || !census.self_restore || scene!=GS_VS)return;
    if(!nativeMatchState_Admit(census.backing,ScratchBytes)){
        nativeSnapshotCensus_Abort("HSD arena admission refused at scene construction");return;
    }
    // Isolated warm span measurement at construction, not replay-loop timing.
    // Scratch is reused by the component oracle afterwards; retain no frame.
    NativeMatchArena* arenas[]={&nativeHsdArena,&nativeMatchSceneArena};
    const char* names[]={"hsd","scene"};
    report_pool_capacities("construction");
    for(unsigned span=0;span<2;++span){
        NativeMatchArena* arena=arenas[span];
        NativeMatchArenaFrame* frame=&census.arena_frame;
        if(!nativeMatchArena_BindFrame(arena,frame,census.comparison,ScratchBytes)){
            nativeSnapshotCensus_Abort("arena history binding refused at scene construction");return;
        }
        uint64_t start=SDL_GetTicksNS();
        for(unsigned i=0;i<32;++i)if(!nativeMatchArena_Save(frame)){
            nativeSnapshotCensus_Abort("arena capture refused");return;
        }
        uint64_t saves=SDL_GetTicksNS()-start;start=SDL_GetTicksNS();
        for(unsigned i=0;i<32;++i)if(!nativeMatchArena_Load(frame)){
            nativeSnapshotCensus_Abort("arena restore refused");return;
        }
        uint64_t loads=SDL_GetTicksNS()-start;
        size_t copied=frame->bytes;
        for(unsigned i=0;i<arena->root_count;++i)copied+=frame->root_bytes[i];
        fprintf(census.output,"[snapshot-census] arena-construction span=%s prefix_bytes=%zu copied_bytes=%zu roots=%u "
            "samples=32 save_us=%.3f load_us=%.3f scope=warm-single-span\n",
            names[span],frame->bytes,copied,arena->root_count,saves/32000.0,loads/32000.0);
        if(span==1){
            // Exercise real interpreted storage/cache references after construction,
            // without executing any callbacks while those fields are perturbed.
            size_t before_bytes,after_bytes;
            if(!nativeScriptFighter_Save(census.backing,ScratchBytes/2,&before_bytes)){
                nativeSnapshotCensus_Abort("fighter VM copy probe capture refused");return;
            }
            NativeScriptFighterSnapshotHeader header;memcpy(&header,census.backing,sizeof(header));
            const uint8_t* cursor=(const uint8_t*)census.backing+sizeof(header);
            size_t state_bytes=0;
            for(unsigned i=0;i<header.count;++i){
                NativeScriptFighterSnapshotRecord record;memcpy(&record,cursor,sizeof(record));
                NativeScriptReceiverView view;HSD_GObj* object=nativeMatchPool_Resolve(record.object);
                if(!object || !nativeScriptFighter_View(object,record.generation,&view)){
                    nativeSnapshotCensus_Abort("fighter VM copy probe view refused");return;
                }
                if(record.state_bytes){view.storage->state[0]^=0xFF;state_bytes+=record.state_bytes;}
                view.storage->identity^=UINT32_C(0x1234);view.storage->state=NULL;
                cursor+=sizeof(record)+record.state_bytes;
            }
            if(!nativeMatchArena_Load(frame) ||
               !nativeScriptFighter_Save((uint8_t*)census.backing+ScratchBytes/2,ScratchBytes/2,&after_bytes) ||
               before_bytes!=after_bytes || memcmp(census.backing,(uint8_t*)census.backing+ScratchBytes/2,before_bytes)){
                nativeSnapshotCensus_Abort("fighter VM direct copy differs");return;
            }
            fprintf(census.output,"[snapshot-census] fighter-vm-copy actors=%u state_bytes=%zu match=1\n",header.count,state_bytes);
        }

        // Prove every admitted span against the existing complete component
        // oracle. No callbacks/validation run while live bytes are perturbed;
        // the private frame restores them directly before recapture.
        size_t before_bytes,after_bytes;
        if(!nativeMatchSnapshot_Capture(census.parts,census.backing,ScratchBytes/2,&before_bytes,NULL) ||
           !nativeMatchArena_Save(frame)){
            nativeSnapshotCensus_Abort("arena copy probe baseline refused");return;
        }
        memset(arena->memory,0xA5,frame->bytes);
        for(unsigned i=0;i<arena->root_count;++i)
            memset(arena->roots[i].address,0x5A,frame->root_bytes[i]);
        if(!nativeMatchArena_Load(frame) ||
           !nativeMatchSnapshot_Capture(census.comparison_parts,(uint8_t*)census.backing+ScratchBytes/2,
                ScratchBytes/2,&after_bytes,NULL) || before_bytes!=after_bytes ||
           memcmp(census.backing,(uint8_t*)census.backing+ScratchBytes/2,before_bytes)){
            nativeSnapshotCensus_Abort("arena direct copy differs from component oracle");return;
        }
        fprintf(census.output,"[snapshot-census] arena-copy-probe span=%s roots=%u oracle_bytes=%zu match=1\n",
            names[span],arena->root_count,before_bytes);
        frame->valid=false;
    }
    if(census.history_mode) {
        size_t slot_bytes=nativeMatchHistory_SlotBytes(arenas,2);
        if(!slot_bytes || slot_bytes>SIZE_MAX/NativeMatchHistory_Capacity) {
            nativeSnapshotCensus_Abort("invalid copy history capacity");return;
        }
        size_t bytes=slot_bytes*NativeMatchHistory_Capacity;
        census.history_backing=malloc(bytes);
        if(!census.history_backing){nativeSnapshotCensus_Abort("cannot reserve copy history");return;}
        memset(census.history_backing,0,bytes);
        if(!nativeMatchHistory_Bind(&census.history,arenas,2,census.history_backing,bytes,
                NativeMatchHistory_Capacity,nativeGObjPool.lifetime)) {
            nativeSnapshotCensus_Abort("cannot bind copy scene history");return;
        }
        fprintf(census.output,"[snapshot-census] history-copy-bound spans=2 reserved_bytes=%zu slot_bytes=%zu\n",bytes,slot_bytes);
    }
}

static void report_capture(void)
{
    for(unsigned index=0;index<Owners;++index) {
        const NativeMatchSnapshotPart* part=&census.parts[index];
        const NativeMatchSnapshotMeasurement* m=&census.measurements[index];
        OwnerCensus* owner=&census.owners[index];
        owner->name=part->name; ++owner->attempts;owner->saved+=m->saved;
        owner->validated+=part->validated;
        if(m->saved && part->bytes>owner->peak_bytes)owner->peak_bytes=part->bytes;
        owner->query_ns+=m->query_ns;owner->save_ns+=m->save_ns;owner->validate_ns+=m->validate_ns;
        fprintf(census.output,
            "[snapshot-census] owner=%s size_known=%u expected_bytes=%zu bytes=%zu "
            "save=%s validation=%s query_us=%.3f save_us=%.3f validate_us=%.3f\n",
            part->name ? part->name : "unavailable",m->size_known,m->expected,part->bytes,
            m->saved ? "ok" : "rejected",!m->saved ? "skipped" : part->validated ? "ok" : "rejected",
            m->query_ns/1000.0,m->save_ns/1000.0,m->validate_ns/1000.0);
    }
    if(census.parts[12].validated) {
        NativeScriptFighterSnapshotHeader header;memcpy(&header,census.parts[12].data,sizeof(header));
        fprintf(census.output,"[snapshot-census] scripted_fighters=%u\n",header.count);
    }
}
static void report_comparison(const NativeMatchSnapshotPart* expected,bool rewind)
{
    for(unsigned index=0;index<Owners;++index) {
        const NativeMatchSnapshotPart* actual=&census.comparison_parts[index];
        const NativeMatchSnapshotPart* part=&expected[index];
        bool match=actual->validated && actual->bytes==part->bytes &&
            !memcmp(actual->data,part->data,part->bytes);
        fprintf(census.output,"[snapshot-census] %s owner=%s match=%u bytes=%zu\n",
            rewind ? "rewind-compare" : "self-restore-compare",part->name,match,actual->bytes);
        if(rewind)census.rewind_compared+=match;else census.compared+=match;
        if(!match)nativeSnapshotCensus_Abort("component recapture differs");
    }
}
static void report_load(const NativeMatchSnapshotPart* parts,const NativeMatchSnapshotLoadReport* report,bool loaded)
{
    fprintf(census.output,"[snapshot-census] actor-graph valid=%u fighters=%u ground=%u items=%u\n",
        report->actors_valid,report->fighters,report->ground,report->items);
    fprintf(census.output,"[snapshot-census] class-graph valid=%u blocks=%u types=%u\n",
        report->classes_valid,report->class_blocks,report->class_types);
    if(!loaded) {
        if(report->failed_owner<Owners)fprintf(census.output,"[snapshot-census] self-restore-load owner=%s restored=0\n",parts[report->failed_owner].name);
        return;
    }
    for(unsigned i=0;i<Owners;++i)
        fprintf(census.output,"[snapshot-census] self-restore-load owner=%s restored=1 elapsed_us=%.3f\n",
            parts[i].name,report->owner_ns[i]/1000.0);
    fprintf(census.output,"[snapshot-census] self-restore-load-total owners=%u elapsed_us=%.3f\n",Owners,report->total_ns/1000.0);
    fprintf(census.output,"[snapshot-census] self-restore-load-preflight owners=%u elapsed_us=%.3f\n",Owners,report->preflight_ns/1000.0);
}

static void rewind_object_activity(const NativeMatchSnapshotPart* old,
                                    const NativeMatchSnapshotPart* current)
{
    NativeGObjSnapshotHeader a,b;memcpy(&a,old->data,sizeof(a));memcpy(&b,current->data,sizeof(b));
    NativeMatchPoolSnapshotView before=nativeMatchSnapshotPool_View(&nativeGObjPool,
        (const uint8_t*)old->data+old->bytes-a.object_bytes-a.proc_bytes,a.object_bytes);
    NativeMatchPoolSnapshotView after=nativeMatchSnapshotPool_View(&nativeGObjPool,
        (const uint8_t*)current->data+current->bytes-b.object_bytes-b.proc_bytes,b.object_bytes);
    if(!before.records || !after.records) {nativeSnapshotCensus_Abort("rewind object activity views refused");return;}
    unsigned created=0,destroyed=0,reused=0;
    for(unsigned i=0;i<before.header.capacity;++i) {
        uint32_t old_generation,new_generation;
        memcpy(&old_generation,before.generations+(size_t)i*4,4);memcpy(&new_generation,after.generations+(size_t)i*4,4);
        bool old_live=nativeMatchSnapshotPool_Find(before,i)!=NULL;
        bool new_live=nativeMatchSnapshotPool_Find(after,i)!=NULL;
        bool changed=old_generation!=new_generation;
        destroyed+=old_live && (!new_live || changed);
        created+=new_live && (!old_live || changed);
        reused+=old_live && new_live && changed;
    }
    fprintf(census.output,"[snapshot-census] rewind-object-activity created=%u destroyed=%u reused=%u\n",created,destroyed,reused);
}

static void finish_resimulation(uint64_t sample)
{
    if(census.timing_enabled) {
        uint64_t elapsed=SDL_GetTicksNS()-census.timing_start;
        fprintf(census.output,"[snapshot-census] replay-timing start=%llu ticks=%llu pass=%u "
            "restore_us=%.3f history_us=%.3f history_calls=%u input_us=%.3f simulation_us=%.3f drawing_us=%.3f "
            "input_calls=%u simulation_calls=%u drawing_calls=%u render_begin_us=%.3f hsd_draw_us=%.3f render_end_us=%.3f "
            "render_begin_calls=%u hsd_draw_calls=%u render_end_calls=%u elapsed_us=%.3f total_us=%.3f instrumentation=%s\n",
            (unsigned long long)census.rewind_sample,(unsigned long long)(sample-census.rewind_sample),census.correction_pass,
            census.timing_restore/1000.0,census.timing_history/1000.0,census.history_calls,
            census.phase_ns[0]/1000.0,census.phase_ns[1]/1000.0,census.phase_ns[2]/1000.0,
            census.phase_calls[0],census.phase_calls[1],census.phase_calls[2],
            census.phase_ns[3]/1000.0,census.phase_ns[4]/1000.0,census.phase_ns[5]/1000.0,
            census.phase_calls[3],census.phase_calls[4],census.phase_calls[5],elapsed/1000.0,
            (elapsed+census.timing_restore)/1000.0,
            !nativeReplay_StateObserversEnabled() && !nativeMatchLaunch_ObjectCensus() ?
                "endpoint_only" : nativeMatchLaunch_TraceActions() &&
                nativeMatchLaunch_TraceFighterState() && nativeMatchLaunch_ObjectCensus() ?
                "retained" : "partial");
    }
    census.resim_ticks=(unsigned)(sample-census.rewind_sample);
#if defined(MELEE_PROFILE_MATCH_SAVE)
    nativeMatchSaveProfile_Report(census.output,census.timing_history,
        &census.history,census.comparison,ScratchBytes);
#endif
    size_t bytes;bool captured=nativeMatchSnapshot_Capture(census.comparison_parts,census.comparison,ScratchBytes,&bytes,NULL);
    if(census.correction && census.correction_pass==1) {
        unsigned changed=0;
        for(unsigned i=0;i<Owners;++i) {
            const NativeMatchSnapshotPart* old=&census.parts[i];
            const NativeMatchSnapshotPart* actual=&census.comparison_parts[i];
            bool differs=captured && (actual->bytes!=old->bytes ||
                memcmp(actual->data,old->data,old->bytes));
            changed+=differs;
            fprintf(census.output,"[snapshot-census] correction-delta owner=%s changed=%u\n",old->name,differs);
        }
        fflush(stdout);fprintf(stderr,"[snapshot-census] correction-trace-first-end\n");
        // C is the first corrected result, not the predicted endpoint B. Retain
        // both compact C and an independent complete C oracle before repeating.
        if(!captured || !changed || !nativeSnapshotEndpoint_Save(&census.rewind_endpoint,
            census.endpoint_backing,EndpointBytes/2)) {
            nativeSnapshotCensus_Abort("cannot retain changed corrected endpoint");
        } else {
            memcpy(census.backing,census.comparison,bytes);
            for(unsigned i=0;i<Owners;++i) {
                census.parts[i]=census.comparison_parts[i];
                census.parts[i].data=(const uint8_t*)census.backing+
                    ((const uint8_t*)census.comparison_parts[i].data-(const uint8_t*)census.comparison);
            }
            census.correction_audio=HSD_AudioRollbackGetStats();
            uint64_t load_start=census.timing_enabled ? SDL_GetTicksNS() : 0;
            bool loaded=census.history_mode ? nativeMatchHistory_Load(&census.history,census.rewind_sample,nativeGObjPool.lifetime) : nativeMatchSnapshot_Load(census.rewind_parts,NULL);
            uint64_t load_ns=census.timing_enabled ? SDL_GetTicksNS()-load_start : 0;
            bool rewound=loaded && nativeReplay_RewindToSample(census.rewind_sample);
            fprintf(census.output,"[snapshot-census] correction-repeat reset=%u replay_rewound=%u changed_owners=%u\n",loaded,rewound,changed);
            if(rewound) {
                nativeReplay_RestoreActionTrace(&census.trace_rewind);
                census.correction_pass=2;
                fflush(stdout);fprintf(stderr,"[snapshot-census] correction-trace-second-start\n");
                begin_replay_timing(load_ns);
                return;
            }
            nativeSnapshotCensus_Abort("cannot repeat corrected interval from compact state");
        }
        // Abort recovery is allowed only after the correction attempt failed.
        nativeSnapshotEndpoint_Restore(&census.endpoint);
        nativeReplay_RestoreActionTrace(&census.trace_endpoint);
        census.resimulating=false;
        return;
    }
    for(unsigned i=0;i<Owners;++i) {
        const NativeMatchSnapshotPart* expected=&census.parts[i];
        const NativeMatchSnapshotPart* actual=&census.comparison_parts[i];
        bool match=captured && actual->validated && expected->bytes==actual->bytes &&
            !memcmp(expected->data,actual->data,expected->bytes);
        census.resim_compared+=match;
        if(!match && captured) {
            uint32_t owner=i;uint64_t lengths[2]={expected->bytes,actual->bytes};
            bool dumped=fwrite(&owner,sizeof(owner),1,census.resim_parts)==1 &&
                fwrite(lengths,sizeof(lengths),1,census.resim_parts)==1 &&
                fwrite(expected->data,expected->bytes,1,census.resim_parts)==1 &&
                fwrite(actual->data,actual->bytes,1,census.resim_parts)==1;
            if(!dumped)nativeSnapshotCensus_Abort("cannot write resimulation mismatch records");
        }
        size_t first=0;
        if(!match && captured) {
            size_t common=expected->bytes<actual->bytes ? expected->bytes : actual->bytes;
            const unsigned char* old=expected->data;const unsigned char* current=actual->data;
            while(first<common && old[first]==current[first])++first;
        }
        fprintf(census.output,"[snapshot-census] resim-compare owner=%s match=%u bytes=%zu first_difference=%zu\n",
            expected->name,match,actual->bytes,first);
    }
    const NativeSnapshotEndpointPool pools[]={
        {"gobj",&nativeGObjPool},{"gobj-proc",&nativeGObjProcPool},
        {"fighter",&nativeFighterPool},{"ground",&nativeGroundPool},
        {"item",&nativeItemPool},{"item-link",&nativeItemLinkPool},
        {"aobj",&nativeAObjPool},{"fobj",&nativeFObjPool}};
    const NativeSnapshotEndpoint* expected_endpoint=census.correction ? &census.rewind_endpoint : &census.endpoint;
    size_t differences=nativeSnapshotEndpoint_ReportDifferences(expected_endpoint,
        pools,sizeof(pools)/sizeof(pools[0]),census.resim_raw);
    fprintf(census.output,"[snapshot-census] resim-raw differing_bytes=%zu\n",differences);
    HSD_AudioRollbackStats audio=HSD_AudioRollbackGetStats();
    if(census.correction) {
        audio.reused_starts-=census.correction_audio.reused_starts;
        audio.new_starts-=census.correction_audio.new_starts;
        audio.canceled_starts-=census.correction_audio.canceled_starts;
    }
    fprintf(census.output,"[snapshot-census] resim-audio reused=%u new=%u canceled=%u\n",audio.reused_starts,audio.new_starts,audio.canceled_starts);
    nativeSnapshotEndpoint_Restore(&census.endpoint);
    bool restored=nativeSnapshotEndpoint_Matches(&census.endpoint);
    census.endpoint_restores+=restored;census.resimulating=false;
    ++census.completed_rounds;
    nativeReplay_RestoreActionTrace(&census.trace_endpoint);
    if(census.correction && !nativeReplay_UseAlternateInputs(false))
        nativeSnapshotCensus_Abort("cannot recover original input stream");
    if(census.history_mode && census.correction) {
        nativeMatchHistory_Discard(&census.history);
        if(!nativeMatchHistory_Save(&census.history,sample,nativeGObjPool.lifetime))
            nativeSnapshotCensus_Abort("cannot capture recovered predicted boundary");
    }
    fprintf(census.output,"[snapshot-census] resim-end ticks=%u compared=%u expected=%u endpoint_restored=%u\n",
        census.resim_ticks,census.resim_compared,Owners,restored);
    fflush(stdout);fprintf(stderr,census.correction ? "[snapshot-census] correction-trace-second-end\n" : "[snapshot-census] resimulation-trace-end\n");
    if(!restored || !captured || census.resim_compared!=Owners || differences==SIZE_MAX || census.resim_ticks!=census.rewind_depth)
        nativeSnapshotCensus_Abort("compact resimulation differs or recovery failed");
    if(fflush(census.resim_raw) || fflush(census.resim_parts))nativeSnapshotCensus_Abort("cannot flush resimulation evidence");
}

static void rewind_experiment(void)
{
    census.rewind_pending=false;
    census.resim_compared=0;
    if(census.resimulate) {fflush(stdout);fprintf(stderr,"[snapshot-census] resimulation-reference-end\n");}
    if(!nativeMatchSnapshot_Capture(census.parts,census.backing,ScratchBytes,&census.cursor,NULL)) {
        nativeSnapshotCensus_Abort("current component capture refused before rewind");return;
    }
    NativeMatchSnapshotPart current[Owners];memcpy(current,census.parts,sizeof(current));
    if(census.resimulate)nativeReplay_CaptureActionTrace(&census.trace_endpoint);
    for(unsigned i=0;i<Owners;++i)if(!current[i].validated) {nativeSnapshotCensus_Abort("current component capture refused before rewind");return;}
    if(census.failed || !nativeSnapshotEndpoint_Save(&census.endpoint,(uint8_t*)census.endpoint_backing+EndpointBytes/2,EndpointBytes/2)) {
        nativeSnapshotCensus_Abort("cannot preserve current endpoint for rewind experiment");return;
    }
    unsigned changed=0;
    for(unsigned i=0;i<Owners;++i) {
        const NativeMatchSnapshotPart* old=&census.rewind_parts[i];
        bool differs=current[i].bytes!=old->bytes || memcmp(current[i].data,old->data,old->bytes);
        changed+=differs;
        fprintf(census.output,"[snapshot-census] rewind-delta owner=%s changed=%u\n",old->name,differs);
    }
    fprintf(census.output,"[snapshot-census] rewind-begin target_sample=%llu depth=%u changed_owners=%u endpoint_bytes=%zu\n",
        (unsigned long long)census.rewind_sample,census.rewind_depth,changed,census.endpoint.bytes);
    rewind_object_activity(&census.rewind_parts[0],&current[0]);
    NativeMatchSnapshotLoadReport report={.failed_owner=UINT32_MAX};
    bool loaded=census.history_mode ? nativeMatchHistory_Load(&census.history,census.rewind_sample,nativeGObjPool.lifetime) : nativeMatchSnapshot_Load(census.rewind_parts,&report);
    if(census.history_mode && loaded) {
        unsigned invalidated=0;
        for(unsigned i=1;i<=census.rewind_depth;++i)
            invalidated+=nativeMatchHistory_Find(&census.history,census.rewind_sample+i)==NULL;
        fprintf(census.output,"[snapshot-census] history-load target=%llu future_invalidated=%u expected=%u stale_refused=%u\n",(unsigned long long)census.rewind_sample,invalidated,census.rewind_depth,
            nativeMatchHistory_Find(&census.history,census.rewind_sample-NativeMatchHistory_Capacity)==NULL);
        if(invalidated!=census.rewind_depth)nativeSnapshotCensus_Abort("future history remained published after restore");
    }
    if(census.history_mode)
        fprintf(census.output,"[snapshot-census] history-copy-load restored=%u spans=%u\n",loaded,census.history.arena_count);
    else report_load(census.rewind_parts,&report,loaded);
    if(loaded) {
        census.rewind_loads+=census.history_mode ? census.history.arena_count : Owners;
        size_t comparison_bytes;
        if(!nativeMatchSnapshot_Capture(census.comparison_parts,census.comparison,ScratchBytes,&comparison_bytes,NULL))
            nativeSnapshotCensus_Abort("older recapture refused");
        report_comparison(census.rewind_parts,true);
    }
    if(loaded) {
        const NativeSnapshotEndpointPool pools[]={
            {"gobj",&nativeGObjPool},{"gobj-proc",&nativeGObjProcPool},
            {"fighter",&nativeFighterPool},{"ground",&nativeGroundPool},
            {"item",&nativeItemPool},{"item-link",&nativeItemLinkPool},
            {"aobj",&nativeAObjPool},{"fobj",&nativeFObjPool}};
        size_t differences=nativeSnapshotEndpoint_ReportDifferences(&census.rewind_endpoint,
            pools,sizeof(pools)/sizeof(pools[0]),census.output);
        if(differences==SIZE_MAX)nativeSnapshotCensus_Abort("invalid rewind pool coverage registration");
        fprintf(census.output,"[snapshot-census] rewind-raw-summary differing_bytes=%zu\n",differences);
    }
    if(census.resimulate && loaded && !census.failed) {
        uint64_t load_start=census.timing_enabled ? SDL_GetTicksNS() : 0;
        bool reset=census.history_mode ? nativeMatchHistory_Load(&census.history,census.rewind_sample,nativeGObjPool.lifetime) : nativeMatchSnapshot_Load(census.rewind_parts,NULL);
        uint64_t load_ns=census.timing_enabled ? SDL_GetTicksNS()-load_start : 0;
        bool rewound=reset && nativeReplay_RewindToSample(census.rewind_sample) &&
            (!census.correction || nativeReplay_UseAlternateInputs(true));
        fprintf(census.output,"[snapshot-census] resim-null-load restored=%u replay_rewound=%u\n",reset,rewound);
        if(rewound) {
            nativeReplay_RestoreActionTrace(&census.trace_rewind);
            census.resimulating=true;
            census.correction_pass=census.correction ? 1 : 0;
            fprintf(census.output,"[snapshot-census] resim-start sample=%llu target=%llu depth=%u drawing=retained\n",
                (unsigned long long)census.rewind_sample,(unsigned long long)(census.rewind_sample+census.rewind_depth),census.rewind_depth);
            fflush(stdout);fprintf(stderr,census.correction ? "[snapshot-census] correction-trace-first-start\n" : "[snapshot-census] resimulation-trace-start\n");
            begin_replay_timing(load_ns);
            return;
        }
        nativeSnapshotCensus_Abort("cannot reset compact state and rewind replay cursor");
    }
    // Never tick/render the partial older state. The complete known endpoint is
    // restored even after a component refusal or mismatched recapture.
    nativeSnapshotEndpoint_Restore(&census.endpoint);
    bool restored=nativeSnapshotEndpoint_Matches(&census.endpoint);
    census.endpoint_restores+=restored;
    ++census.completed_rounds;
    memcpy(census.parts,current,sizeof(census.parts));
    fprintf(census.output,"[snapshot-census] rewind-end loaded=%u compared=%u endpoint_restored=%u\n",
        loaded,census.rewind_compared,restored);
    if(!loaded || !restored)nativeSnapshotCensus_Abort("rewind experiment or endpoint recovery failed");
}

static void benchmark_compact_snapshots(uint64_t sample)
{
    enum { Iterations=16 };
    uint64_t saves[Iterations],loads[Iterations],save_sum=0,load_sum=0;
    // Scratch/definitions are reserved during startup. No timers, queries or
    // file output run inside these complete direct capture/load calls.
    for(unsigned i=0;i<Iterations;++i) {
        size_t bytes;uint64_t start=SDL_GetTicksNS();
        bool saved=nativeMatchSnapshot_Capture(census.comparison_parts,census.comparison,ScratchBytes,&bytes,NULL);
        saves[i]=SDL_GetTicksNS()-start;
        if(!saved) {nativeSnapshotCensus_Abort("compact benchmark capture refused");return;}
        start=SDL_GetTicksNS();bool loaded=nativeMatchSnapshot_Load(census.parts,NULL);
        loads[i]=SDL_GetTicksNS()-start;
        if(!loaded) {nativeSnapshotCensus_Abort("compact benchmark load refused");return;}
        save_sum+=saves[i];load_sum+=loads[i];
    }
    // Check the resulting state outside measurement. This benchmark does not
    // advance simulation or replace the separate rendered replay proof.
    size_t bytes;
    bool exact=nativeMatchSnapshot_Capture(census.comparison_parts,census.comparison,ScratchBytes,&bytes,NULL);
    for(unsigned i=0;exact && i<Owners;++i)
        exact=census.parts[i].bytes==census.comparison_parts[i].bytes &&
            !memcmp(census.parts[i].data,census.comparison_parts[i].data,census.parts[i].bytes);
    if(!exact) {nativeSnapshotCensus_Abort("compact benchmark changed captured state");return;}
    // Fixed-size insertion sorting happens after the measured calls.
    for(unsigned i=1;i<Iterations;++i) {
        uint64_t value=saves[i];unsigned j=i;
        while(j && saves[j-1]>value) {saves[j]=saves[j-1];--j;}saves[j]=value;
        value=loads[i];j=i;
        while(j && loads[j-1]>value) {loads[j]=loads[j-1];--j;}loads[j]=value;
    }
    fprintf(census.output,"[snapshot-census] compact-benchmark sample=%llu iterations=%u bytes=%zu exact=1 "
        "save_min_us=%.3f save_median_us=%.3f save_mean_us=%.3f save_max_us=%.3f "
        "load_min_us=%.3f load_median_us=%.3f load_mean_us=%.3f load_max_us=%.3f\n",
        (unsigned long long)sample,Iterations,bytes,
        saves[0]/1000.0,saves[Iterations/2]/1000.0,save_sum/(1000.0*Iterations),saves[Iterations-1]/1000.0,
        loads[0]/1000.0,loads[Iterations/2]/1000.0,load_sum/(1000.0*Iterations),loads[Iterations-1]/1000.0);
}

static void retain_rewind_target(uint64_t input_sample)
{
    if(!nativeMatchSnapshot_Capture(census.parts,census.backing,ScratchBytes,&census.cursor,NULL) ||
       !nativeSnapshotEndpoint_Save(&census.rewind_endpoint,census.endpoint_backing,EndpointBytes/2)) {
        nativeSnapshotCensus_Abort("cannot retain target compact capture and oracle");return;
    }
    memcpy(census.rewind_backing,census.backing,census.cursor);
    for(unsigned i=0;i<Owners;++i) {
        census.rewind_parts[i]=census.parts[i];
        census.rewind_parts[i].data=(const uint8_t*)census.rewind_backing+
            ((const uint8_t*)census.parts[i].data-(const uint8_t*)census.backing);
    }
    if(census.history_mode) {
        const NativeMatchHistoryFrame* saved=nativeMatchHistory_Find(&census.history,input_sample);
        if(!saved) {nativeSnapshotCensus_Abort("target history boundary absent");return;}
        // The packed target retained above is an independent diagnostic oracle.
    }
    census.rewind_sample=input_sample;census.rewind_pending=true;
    if(census.resimulate)nativeReplay_CaptureActionTrace(&census.trace_rewind);
    if(census.resimulate) {fflush(stdout);fprintf(stderr,"[snapshot-census] resimulation-reference-start\n");}
}

void nativeSnapshotCensus_Sample(uint32_t scene, uint64_t frame,
                                 uint64_t input_sample)
{
    if (!census.active || scene != GS_VS)return;
    if(census.before_input && !input_sample && !census.failed) {
        size_t bytes=0,after=0;
        bool saved=nativeStageStart_Save(census.comparison,ScratchBytes,&bytes);
        bool valid=saved && nativeStageStart_Validate(census.comparison,bytes);
        bool restored=valid && nativeStageStart_Load(census.comparison,bytes);
        bool same=restored && nativeStageStart_Save(census.backing,ScratchBytes,&after) &&
            bytes==after && !memcmp(census.comparison,census.backing,bytes);
        NativeStageStartSnapshotHeader h={0};if(saved)memcpy(&h,census.comparison,sizeof(h));
        fprintf(census.output,"[snapshot-census] stage-start-cold pending=%u started=%u bytes=%zu saved=%u validated=%u restored=%u match=%u\n",h.count,h.started,bytes,saved,valid,restored,same);
        if(!same) {nativeSnapshotCensus_Abort("pending stage callback snapshot refused or changed");return;}
    }
    if(census.history_mode && (census.before_input || input_sample>=SampleInterval) && !census.failed) {
        uint64_t save_start=census.timing_enabled && census.resimulating ? SDL_GetTicksNS() : 0;
        bool saved=nativeMatchHistory_Save(&census.history,input_sample,nativeGObjPool.lifetime);
        if(census.timing_enabled && census.resimulating) {
            census.timing_history+=SDL_GetTicksNS()-save_start;++census.history_calls;
        }
        if(!saved) {
            fprintf(census.output,"[snapshot-census] history-copy-rejected sample=%llu\n",(unsigned long long)input_sample);
            nativeSnapshotCensus_Abort("per-boundary compact history capture refused");return;
        }
    }
    bool target=census.rewind && !census.resimulating && !census.rewind_pending &&
        census.completed_rounds<census.rewind_rounds &&
        input_sample==census.target_sample+(uint64_t)census.completed_rounds*SampleInterval;
    if(!input_sample) {
        if(target && !census.failed)retain_rewind_target(input_sample);
        return;
    }
    if(census.resimulating) {
        if(input_sample>=census.rewind_sample+census.rewind_depth)finish_resimulation(input_sample);
        if(fflush(census.output))nativeSnapshotCensus_Abort("cannot flush resimulation report");
        return;
    }
    if(census.rewind_pending && input_sample==census.rewind_sample+census.rewind_depth) {
        rewind_experiment();if(fflush(census.output))nativeSnapshotCensus_Abort("cannot flush rewind diagnostic");return;
    }
    if(input_sample%SampleInterval || census.last_sample==input_sample) {
        if(target && !census.failed)retain_rewind_target(input_sample);
        return;
    }
    census.last_sample = input_sample;
    ++census.samples;
    report_pool_capacities("sample");
    fprintf(census.output,
        "[snapshot-census] sample=%llu frame=%llu input_sample=%llu "
        "fighters=%u items=%u ground=%u\n",
        (unsigned long long)census.samples, (unsigned long long)frame,
        (unsigned long long)input_sample, nativeFighterPool.used,
        nativeItemPool.used, nativeGroundPool.used);
    fprintf(census.output,
        "[snapshot-census] readiness script_idle=%u construction_active=%u "
        "proc_active=%u proc_object_active=%u draw_active=%u render_depth=%u render_context_present=%u "
        "deferred_flags=%u next_proc_present=%u "
        "collision_epoch_matches=%u collision_source_matches=%u "
        "collision_description_valid=%u article_scene_ready=%u article_model_ready=%u\n",
        nativeScriptRuntime_Idle(), nativeHsdReservation_Active(),
        HSD_GObj_CurrentInvokedProc != NULL, HSD_GObj_CurrentInvokedProcGObj != NULL,
        HSD_GObj_804D7818 != NULL, (unsigned)HSD_GObj_RenderCallbackDepth, HSD_GObj_804D7814 != NULL,
        (unsigned)HSD_GObj_DelayedProcInfo.flags, HSD_GObj_NextInvokedProc != NULL,
        nativeCollisionScene.object_epoch == nativeGObjPool.lifetime,
        nativeCollisionScene.source == mpLib_804D64B4,
        nativeCollisionState_ValidDescription(mpLib_804D64B4),
        nativeArticleResourceScene_SnapshotIdentity() != 0,
        nativeArticleResourceModel_SnapshotIdentity() != 0);
    if(!nativeMatchSnapshot_Capture(census.parts,census.backing,ScratchBytes,&census.cursor,census.measurements))
        nativeSnapshotCensus_Abort("component capture refused");
    report_capture();
    if(census.self_restore && !census.failed) {
        fprintf(census.output,"[snapshot-census] self-restore-boundary owners=%u bytes=%zu\n",Owners,census.cursor);
        NativeMatchSnapshotLoadReport report;
        bool loaded=nativeMatchSnapshot_Load(census.parts,&report);
        report_load(census.parts,&report,loaded);
        if(loaded) {
            census.restored+=Owners;size_t comparison_bytes;
            if(!nativeMatchSnapshot_Capture(census.comparison_parts,census.comparison,ScratchBytes,&comparison_bytes,NULL))
                nativeSnapshotCensus_Abort("same-boundary recapture refused");
            report_comparison(census.parts,false);
        } else nativeSnapshotCensus_Abort("same-boundary restore refused");
    }
    if(census.samples==1 && !census.failed) {
        NativeMatchSnapshotPart directory[Owners];
        NativeMatchSnapshotMeasurement measure[Owners];
        memset(directory,0xA5,sizeof(directory));memset(measure,0x5A,sizeof(measure));
        NativeMatchSnapshotPart unchanged[Owners];memcpy(unchanged,directory,sizeof(directory));
        size_t bytes=SIZE_MAX;
        bool refused=
            !nativeMatchSnapshot_Capture(directory,census.comparison ? census.comparison : census.backing,0,&bytes,measure) &&
            !nativeMatchSnapshot_Capture(directory,directory,sizeof(directory),&bytes,measure) &&
            !nativeMatchSnapshot_Capture(directory,census.backing,ScratchBytes,(size_t*)census.backing,measure) &&
            !nativeMatchSnapshot_Capture(directory,census.backing,ScratchBytes,&bytes,(void*)directory) &&
            !nativeMatchSnapshot_Capture(directory,(void*)(UINTPTR_MAX-3),8,&bytes,measure) &&
            !memcmp(directory,unchanged,sizeof(directory)) && bytes==SIZE_MAX;
        for(size_t i=0;i<sizeof(measure);++i)refused=refused && ((const unsigned char*)measure)[i]==0x5A;
        fprintf(census.output,"[snapshot-census] aggregate-buffer-tests refused=%u metadata_unchanged=%u\n",refused,bytes==SIZE_MAX);
        if(!refused)nativeSnapshotCensus_Abort("aggregate buffer refusal changed metadata");
    }
    if(census.benchmark_compact && !census.failed)benchmark_compact_snapshots(input_sample);
    if(target && !census.failed)retain_rewind_target(input_sample);
    if (fflush(census.output) != 0)
        nativeSnapshotCensus_Abort("cannot flush diagnostic output");
}

void nativeSnapshotCensus_Close(void)
{
    if (census.output) {
        fprintf(census.output, "[snapshot-census] summary samples=%llu\n",
                (unsigned long long)census.samples);
        for (unsigned i = 0; i < Owners; ++i) {
            const OwnerCensus* owner = &census.owners[i];
            if (!owner->attempts) continue;
            fprintf(census.output,
                "[snapshot-census] summary owner=%s attempts=%u saved=%u "
                "validated=%u peak_bytes=%zu mean_query_us=%.3f "
                "mean_save_us=%.3f mean_validate_us=%.3f\n",
                owner->name, owner->attempts, owner->saved, owner->validated,
                owner->peak_bytes, owner->query_ns / (1000.0 * owner->attempts),
                owner->save_ns / (1000.0 * owner->attempts),
                owner->validate_ns / (1000.0 * owner->attempts));
        }
        if(census.self_restore)
            fprintf(census.output,"[snapshot-census] self-restore-summary restored=%u compared=%u expected=%llu\n",
                census.restored,census.compared,(unsigned long long)census.samples*Owners);
        if(census.rewind)
            fprintf(census.output,"[snapshot-census] rewind-summary loaded=%u compared=%u endpoint_restores=%u pending=%u\n",
                census.rewind_loads,census.rewind_compared,census.endpoint_restores,census.rewind_pending);
        if(census.rewind) {
            fprintf(census.output,"[snapshot-census] history-summary rounds=%u expected=%u depth=%u\n",census.completed_rounds,census.rewind_rounds,census.rewind_depth);
            if(census.completed_rounds!=census.rewind_rounds)nativeSnapshotCensus_Abort("requested history intervals did not complete");
        }
        if(census.history_mode)
            fprintf(census.output,"[snapshot-census] history-storage captures=%llu restores=%llu peak_bytes=%zu slots=%u slot_bytes=%zu failed=%u\n",(unsigned long long)census.history.captures,(unsigned long long)census.history.restores,census.history.peak_bytes,census.history.capacity,census.history.slot_bytes,census.history.failed);
        if (fflush(census.output) != 0) nativeSnapshotCensus_Abort("cannot flush summary");
        if (census.output != stdout && fclose(census.output) != 0)
            nativeSnapshotCensus_Abort("cannot close diagnostic output");
    }
    if(census.resim_raw && fclose(census.resim_raw))nativeSnapshotCensus_Abort("cannot close resimulation raw output");
    if(census.resim_parts && fclose(census.resim_parts))nativeSnapshotCensus_Abort("cannot close resimulation mismatch output");
    census.resim_raw=census.resim_parts=NULL;
    free(census.backing);free(census.comparison);free(census.rewind_backing);free(census.endpoint_backing);
    free(census.history_backing);census.history_backing=NULL;
    census.rewind_backing=census.endpoint_backing=NULL;
    census.backing=census.comparison=NULL;
    census.output = NULL;
    census.active = false;
}

bool nativeSnapshotCensus_HasFailed(void) { return census.failed; }

bool nativeSnapshotCensus_IsResimulating(void) {return census.active && census.resimulating;}

bool nativeSnapshotCensus_UsesReplayJournal(void) {return census.active && census.resimulate;}

bool nativeSnapshotCensus_BeforeInput(void) {return census.active && census.before_input;}
