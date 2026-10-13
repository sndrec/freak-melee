// Actual production catalog base + generated imported member DLL. Raw motion
// pointers are failing fixture sentinels; no original gameplay is delegated.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/it/kinds/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, phases, callbacks, article_callbacks;
static HMODULE library;
#define CHECK(c) do { ++checks; if(!(c)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); exit(1); } } while(0)
static void unexpected(void) { CHECK(false); }
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
    CHECK(argc==2); library=LoadLibraryA(argv[1]); CHECK(library);
    NativeFighterHost host={0};
#include "required_host.inc"
    typedef const NativeFighterDefinition* (*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule"); CHECK(entry);
    typedef int (*Parameters)(const uint32_t*,unsigned,uint64_t);
    Parameters bind=(Parameters)GetProcAddress(library,"FreakMelee_SetFighterParameters"); CHECK(bind);
    CHECK(bind(parameter_words,sizeof(parameter_words)/sizeof(*parameter_words),PARAMETER_SCHEMA));
    const NativeFighterDefinition* base=&BASE_DEFINITION;
    const NativeFighterDefinition* d=entry(base,&host); CHECK(d);
    CHECK(base->source_kind==MEMBER_KIND && d->source_kind==Ft_Kind_None);
    CHECK(d->animation_source_kind==MEMBER_KIND);
    CHECK(d->joint_count==PART_COUNT && d->bone_count==BONE_COUNT && d->bones==base->bones);
    CHECK(d->action_count==ACTION_COUNT && d->demo_count==14 && d->motion_state_first==341 && d->motion_state_count==PHASE_STATES);
    CHECK(d->fighter_root==base->fighter_root && d->costume_joint==base->costume_joint && d->costume_matanim==base->costume_matanim);
    CHECK(d->article_assets==base->article_assets && d->article_asset_count==ARTICLE_COUNT);
    CHECK(d->demo_motion_names==base->demo_motion_names && d->sound_banks==(UINT64_C(1)<<27) && d->effect_bank==17);
    CHECK(!memcmp(&d->asset_layout,&base->asset_layout,sizeof(base->asset_layout)));
    CHECK(d->pose_graph_count==base->pose_graph_count);
    for(unsigned i=0;i<base->pose_graph_count;++i) {
        const NativeFighterPoseGraph* a=base->pose_graphs+i; const NativeFighterPoseGraph* b=d->pose_graphs+i;
        CHECK(a->name_hash==b->name_hash && a->slot==b->slot && !strcmp(a->path,b->path));
    }
    CHECK(!d->items && !d->item_count && !d->neutral_article && !d->on_user_data_remove && !d->on_absorb);
    CHECK(d->on_load && d->on_load_special_attrs && d->on_death && d->on_form_activated && d->on_cpu_pending_action);
    CHECK(d->on_item_pickup && d->on_item_pickup_ext && d->on_item_drop && d->on_item_drop_ext);
    CHECK(d->on_item_visible && d->on_item_invisible && d->on_knockback_enter && d->on_knockback_exit);
#if SHEIK
    CHECK(d->on_color_animation_reset && d->pose_graph_count==2);
#else
    CHECK(d->on_cpu_attack_decision && d->on_cpu_travel_decision && d->on_cpu_recovery);
#endif
#define VISIT_CALLBACK(field) do { if(d->field) { own((const void*)d->field); ++callbacks; } } while(0)
#include "callbacks.inc"
#undef VISIT_CALLBACK
    for(unsigned i=0;i<d->callback_export_count;++i) own((const void*)d->callback_exports[i].callback);
    for(unsigned i=0;i<base->motion_state_count;++i) {
        const MotionState* a=base->motion_states+i; const MotionState* b=d->motion_states+i;
        CHECK(a->anim_id==b->anim_id && a->x4_flags==b->x4_flags && a->_==b->_ && a->cam_cb==b->cam_cb);
        HSD_GObjEvent old[]={a->anim_cb,a->input_cb,a->phys_cb,a->coll_cb};
        HSD_GObjEvent now[]={b->anim_cb,b->input_cb,b->phys_cb,b->coll_cb};
        for(unsigned j=0;j<4;++j) { if(old[j]) { CHECK(now[j]); ++phases; } own((const void*)now[j]); }
    }
    CHECK(d->article_logic_count==ARTICLE_COUNT && phases==PHASE_STATES*4);
    for(unsigned i=0;i<ARTICLE_COUNT;++i) {
        const NativeFighterArticleLogic* a=d->article_logics+i; own(a->logic); own(a->logic->states);
        bool mapped=false; for(unsigned j=0;j<ARTICLE_COUNT;++j) if(a->kind==base->article_assets[j].kind) mapped=true;
        CHECK(mapped); ARTICLE_POINTER(a->on_created); ARTICLE_POINTER(a->on_out_of_bounds);
        ItemLogicTable* l=a->logic;
        ARTICLE_POINTER(l->spawned); ARTICLE_POINTER(l->destroyed); ARTICLE_POINTER(l->picked_up);
        ARTICLE_POINTER(l->dropped); ARTICLE_POINTER(l->thrown); ARTICLE_POINTER(l->dmg_dealt);
        ARTICLE_POINTER(l->dmg_received); ARTICLE_POINTER(l->entered_air); ARTICLE_POINTER(l->reflected);
        ARTICLE_POINTER(l->clanked); ARTICLE_POINTER(l->absorbed); ARTICLE_POINTER(l->shield_bounced);
        ARTICLE_POINTER(l->hit_shield); ARTICLE_POINTER(l->evt_unk);
        for(unsigned j=0;j<a->state_count;++j) { ARTICLE_POINTER(l->states[j].animated); ARTICLE_POINTER(l->states[j].physics_updated); ARTICLE_POINTER(l->states[j].collided); }
        for(unsigned j=0;j<a->signal_count;++j) ARTICLE_POINTER(a->signals[j].callback);
        if(a->projectile) { ARTICLE_POINTER(a->projectile->read_values); ARTICLE_POINTER(a->projectile->on_launched); }
    }
    FreeLibrary(library);
    printf("{\"checks\":%u,\"phases\":%u,\"definition_callbacks\":%u,\"article_callbacks\":%u}\n",checks,phases,callbacks,article_callbacks);
    return 0;
}
