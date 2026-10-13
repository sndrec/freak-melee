// Actual production catalog base + generated imported member DLL. Raw motion
// pointers are failing fixture sentinels; no original gameplay is delegated.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/it/kinds/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, phases, callbacks, article_callbacks;
static HMODULE library;
#define CHECK(c) do { ++checks; if(!(c)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); exit(1); } } while(0)
static void unexpected(void) { CHECK(false); }
static void* read_file(const char* path,size_t* size)
{
    FILE* f=fopen(path,"rb"); CHECK(f && !fseek(f,0,SEEK_END)); long n=ftell(f);
    CHECK(n>0 && !fseek(f,0,SEEK_SET)); void* data=malloc(n); CHECK(data);
    CHECK(fread(data,1,n,f)==(size_t)n && !fclose(f)); *size=n;return data;
}
#include "stock_table.inc"
#include "catalog.c"
#include "parameters.inc"
static void own(const void* pointer)
{
    if(!pointer) return;
    MEMORY_BASIC_INFORMATION m;
    CHECK(VirtualQuery(pointer,&m,sizeof(m))==sizeof(m));
    CHECK(m.AllocationBase==library);
}
#define ARTICLE_POINTER(p) do { if(p) { own((const void*)(p)); ++article_callbacks; } } while(0)
int main(int argc,char** argv)
{
    CHECK(argc==4); library=LoadLibraryA(argv[1]); CHECK(library);
    NativeFighterHost host={0};
#include "required_host.inc"
    typedef const NativeFighterDefinition* (*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule"); CHECK(entry);
    typedef int (*Parameters)(const uint32_t*,unsigned,uint64_t);
    Parameters bind=(Parameters)GetProcAddress(library,"FreakMelee_SetFighterParameters"); CHECK(bind);
    CHECK(bind(parameter_words,sizeof(parameter_words)/sizeof(*parameter_words),PARAMETER_SCHEMA));
    const NativeFighterDefinition* base=&BASE_DEFINITION;
    const NativeFighterDefinition* d=entry(base,&host); CHECK(d);
    CHECK(base->source_kind==MEMBER_KIND && d->source_kind==EXPECTED_SOURCE_KIND);
    CHECK(d->animation_source_kind==MEMBER_KIND);
    CHECK(d->joint_count==PART_COUNT && d->bone_count==BONE_COUNT && d->bones==base->bones);
    CHECK(d->action_count==ACTION_COUNT && d->demo_count==14 && d->motion_state_first==341 && d->motion_state_count==PHASE_STATES);
    CHECK(d->fighter_root==base->fighter_root && d->costume_joint==base->costume_joint && d->costume_matanim==base->costume_matanim);
    CHECK(d->article_assets==base->article_assets && d->article_asset_count==ARTICLE_COUNT);
    CHECK(d->demo_motion_names==base->demo_motion_names && d->sound_banks==(UINT64_C(1)<<13) && d->effect_bank==14);
    CHECK(!memcmp(&d->asset_layout,&base->asset_layout,sizeof(base->asset_layout)));
    CHECK(d->asset_layout.article_mask==7 && d->asset_layout.item_format==FtCharacterItems_ClimberString);
    CHECK(!memcmp(d->special_ground_state,base->special_ground_state,sizeof(base->special_ground_state)));
    CHECK(!memcmp(d->special_air_state,base->special_air_state,sizeof(base->special_air_state)));
    CHECK(d->omission_count==0);
#include "expected_bones.inc"
    CHECK(sizeof(expected_bones)/sizeof(*expected_bones)==d->bone_count);
    for(unsigned i=0;i<d->bone_count;++i) {
        bool found=false;
        for(unsigned j=0;j<d->bone_count;++j) if(d->bones[j].part==expected_bones[i].part) {
            CHECK(d->bones[j].joint==expected_bones[i].joint);found=true;
        }
        CHECK(found);
    }
    CHECK(d->pose_graph_count==base->pose_graph_count);
    for(unsigned i=0;i<base->pose_graph_count;++i) {
        const NativeFighterPoseGraph* a=base->pose_graphs+i; const NativeFighterPoseGraph* b=d->pose_graphs+i;
        CHECK(a->name_hash==b->name_hash && a->slot==b->slot && !strcmp(a->path,b->path));
    }
    CHECK(!d->items && !d->item_count && !d->neutral_article && !d->on_user_data_remove && !d->on_absorb);
    CHECK(d->on_load && d->on_load_special_attrs && d->on_death);
    CHECK(d->on_item_pickup && d->on_item_pickup_ext && d->on_item_drop && d->on_item_drop_ext);
    CHECK(d->on_item_visible && d->on_item_invisible && d->on_knockback_enter && d->on_knockback_exit);
    CHECK(d->special_n && d->special_air_n && d->special_lw && d->special_air_lw);
#if NANA
    CHECK(!d->special_s && !d->special_air_s && !d->special_hi && !d->special_air_hi);
    CHECK(!d->special_ground_state[1] && !d->special_ground_state[2]);
    CHECK(!d->special_air_state[1] && !d->special_air_state[2]);
    CHECK(d->on_cpu_prepare && d->on_cpu_initialize && d->on_cpu_spacing_update);
    CHECK(d->on_cpu_stick_step && d->on_cpu_stick_limit && d->on_cpu_drop_command);
    CHECK(d->on_cpu_mode_update && d->on_cpu_output);
#else
    CHECK(d->special_s && d->special_air_s && d->special_hi && d->special_air_hi);
    CHECK(d->on_cpu_capture_decision && d->on_cpu_pummel_special);
#endif
    CHECK(d->ignore_surface_friction && d->sound_mute_channels==0x44);
    const NativeFighterSoundPolicy* sound=d->sound_policy; CHECK(sound); own(sound);
    CHECK(sound->rule_count==2 && sound->ko_cue_count==1); own(sound->rules); own(sound->ko_cues);
    CHECK(sound->rules[0].costume_mask==UINT32_C(0xfffffffc) && sound->rules[0].bank==13);
    CHECK(sound->rules[0].first==0x1fbfd && sound->rules[0].last==0x1fc62 && sound->rules[0].offset==0x66);
    CHECK(sound->rules[1].costume_mask==3 && sound->rules[1].bank==13);
    CHECK(sound->rules[1].first==0x1fc63 && sound->rules[1].last==0x1fcc8 && sound->rules[1].offset==-0x66);
    CHECK(sound->ko_cues[0].costume_mask==UINT32_C(0xfffffffc) && sound->ko_cues[0].sound_id==0x1fbd1);
#include "expected_state.inc"
    CHECK(d->state_field_count==sizeof(expected_state)/sizeof(*expected_state)); own(d->state_fields);
    unsigned state_end=0;
    for(unsigned i=0;i<d->state_field_count;++i) {
        const NativeFighterStateField* f=d->state_fields+i; const ExpectedState* e=expected_state+i;
        own(f->name); CHECK(!strcmp(f->name,e->name) && f->type==e->type && f->count==e->count && f->array==e->array);
        CHECK(f->stride==(f->type==NativeFighterState_Bool?1:4));
        CHECK(f->offset>=state_end && f->offset+f->stride*f->count<=d->state_bytes);
        state_end=f->offset+f->stride*f->count;
    }
    CHECK(d->state_bytes>0);
#include "expected_exports.inc"
    CHECK(d->callback_export_count==sizeof(expected_exports)/sizeof(*expected_exports)); own(d->callback_exports);
    for(unsigned i=0;i<d->callback_export_count;++i) {
        CHECK(d->callback_exports[i].name_hash==expected_exports[i].hash);
        CHECK(d->callback_exports[i].result_type==expected_exports[i].type);
        own((const void*)d->callback_exports[i].callback);
    }
#define VISIT_CALLBACK(field) do { if(d->field) { own((const void*)d->field); ++callbacks; } } while(0)
#include "callbacks.inc"
#undef VISIT_CALLBACK
    for(unsigned i=0;i<base->motion_state_count;++i) {
        const MotionState* a=base->motion_states+i; const MotionState* b=d->motion_states+i;
        CHECK(a->anim_id==b->anim_id && a->x4_flags==b->x4_flags && a->_==b->_ && a->cam_cb==b->cam_cb);
        HSD_GObjEvent old[]={a->anim_cb,a->input_cb,a->phys_cb,a->coll_cb};
        HSD_GObjEvent now[]={b->anim_cb,b->input_cb,b->phys_cb,b->coll_cb};
        for(unsigned j=0;j<4;++j) { if(old[j]) { CHECK(now[j]); ++phases; } own((const void*)now[j]); }
    }
    own(d->motion_states);
    CHECK(d->article_logic_count==ARTICLE_COUNT && phases==96); own(d->article_logics);
    bool article_seen[ARTICLE_COUNT]={false};
#include "expected_articles.inc"
    CHECK(sizeof(expected_articles)/sizeof(*expected_articles)==ARTICLE_COUNT);
    for(unsigned i=0;i<ARTICLE_COUNT;++i) {
        const NativeFighterArticleLogic* a=d->article_logics+i; own(a->logic); own(a->logic->states);
        bool mapped=false; for(unsigned j=0;j<ARTICLE_COUNT;++j) if(a->kind==base->article_assets[j].kind) { CHECK(!article_seen[j]); article_seen[j]=true; mapped=true; }
        CHECK(mapped); const ExpectedArticle* expected=NULL;
        for(unsigned j=0;j<ARTICLE_COUNT;++j) if(a->kind==expected_articles[j].kind) expected=expected_articles+j;
        CHECK(expected && a->state_count==expected->states && a->state_field_count==expected->field_count);
        unsigned end=0;
        for(unsigned j=0;j<a->state_field_count;++j) {
            const NativeFighterStateField* f=a->state_fields+j;const ExpectedState* e=expected->fields+j;
            own(f->name);CHECK(!strcmp(f->name,e->name) && f->type==e->type && f->count==e->count && f->array==e->array);
            CHECK(f->stride==(f->type==NativeFighterState_Bool?1:4));
            CHECK(f->offset>=end && f->offset+f->stride*f->count<=a->local_state_bytes);
            end=f->offset+f->stride*f->count;
        }
        ARTICLE_POINTER(a->on_created); ARTICLE_POINTER(a->on_out_of_bounds);
        ARTICLE_POINTER(a->on_held_status); ARTICLE_POINTER(a->on_held_request); ARTICLE_POINTER(a->on_holder_neutral);
        if(a->state_fields) own(a->state_fields); if(a->signals) own(a->signals); if(a->projectile) own(a->projectile);
        if(a->attachment) own(a->attachment); if(a->model_roots) own(a->model_roots); if(a->model_animations) own(a->model_animations);
        ItemLogicTable* l=a->logic;
        ARTICLE_POINTER(l->spawned); ARTICLE_POINTER(l->destroyed); ARTICLE_POINTER(l->picked_up);
        ARTICLE_POINTER(l->dropped); ARTICLE_POINTER(l->thrown); ARTICLE_POINTER(l->dmg_dealt);
        ARTICLE_POINTER(l->dmg_received); ARTICLE_POINTER(l->entered_air); ARTICLE_POINTER(l->reflected);
        ARTICLE_POINTER(l->clanked); ARTICLE_POINTER(l->absorbed); ARTICLE_POINTER(l->shield_bounced);
        ARTICLE_POINTER(l->hit_shield); ARTICLE_POINTER(l->evt_unk);
        for(unsigned j=0;j<a->state_count;++j) { ARTICLE_POINTER(l->states[j].animated); ARTICLE_POINTER(l->states[j].physics_updated); ARTICLE_POINTER(l->states[j].collided); }
        for(unsigned j=0;j<a->signal_count;++j) { ARTICLE_POINTER(a->signals[j].callback); for(unsigned k=0;k<j;++k) CHECK(a->signals[k].name_hash!=a->signals[j].name_hash); }
        if(a->projectile) { ARTICLE_POINTER(a->projectile->read_values); ARTICLE_POINTER(a->projectile->on_launched); }
    }
    // Parse the real project CPU profile and compare all fields to the actual
    // stock importer operating on the real PlCo bytes; no mirrored table.
    size_t common_size, profile_size; void* common=read_file(argv[2],&common_size);
    char* profile=read_file(argv[3],&profile_size); NativeFighterCpuProfile stock_cpu, owned_cpu;
    CHECK(nativeFighterCpu_ImportStock(common,common_size,MEMBER_KIND,&stock_cpu));
    CHECK(nativeFighterCpu_Parse(profile,profile_size,&owned_cpu));
    CHECK(!memcmp(&stock_cpu,&owned_cpu,sizeof(stock_cpu))); free(common);free(profile);
    FreeLibrary(library);
    printf("{\"checks\":%u,\"phases\":%u,\"definition_callbacks\":%u,\"article_callbacks\":%u}\n",checks,phases,callbacks,article_callbacks);
    return 0;
}
