// Load the actual DLL. Every authored gameplay pointer must belong to that DLL;
// copied immutable metadata and the common camera callback may remain in base.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/it/kinds/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks, callback_count, article_callbacks, phases;
static HMODULE library;
#define CHECK(c) do { ++checks; if(!(c)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); exit(1); } } while(0)
static void stock_slot(HSD_GObj* object) { (void)object; CHECK(false); }
#include "base.h"
static void unexpected(void) { CHECK(false); }
static void own_pointer(const void* pointer)
{
    if(!pointer) return;
    MEMORY_BASIC_INFORMATION information;
    CHECK(VirtualQuery(pointer,&information,sizeof(information))==sizeof(information));
    CHECK(information.AllocationBase==library);
}
#define ARTICLE_POINTER(p) do { if(p) { own_pointer((const void*)(p)); ++article_callbacks; } } while(0)
int main(int argc,char** argv)
{
    CHECK(argc==2); library=LoadLibraryA(argv[1]); CHECK(library);
    NativeFighterHost host={0};
#include "required_host.h"
    typedef const NativeFighterDefinition* (*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule"); CHECK(entry);
    const NativeFighterDefinition* d=entry(&ftNessClone_Definition,&host); CHECK(d);
    CHECK(d->source_kind==Ft_Kind_None && ftNessClone_Definition.source_kind==Ft_Kind_Ness);
    CHECK(d->animation_source_kind==Ft_Kind_Ness);
    CHECK(d->bone_count==51 && d->joint_count==63 && d->bones==ftNessClone_Definition.bones);
    CHECK(d->action_count==326 && d->demo_count==14 && d->motion_state_count==36 && d->motion_state_first==341);
    CHECK(d->article_asset_count==11 && d->article_assets==ftNessClone_Definition.article_assets);
    CHECK(d->asset_layout.article_mask==0x7FF && d->asset_layout.item_format==FtCharacterItems_Yoyo);
    CHECK(d->sound_banks==(UINT64_C(1)<<21) && d->effect_bank==10);
    CHECK(d->demo_motion_names==ftNessClone_Definition.demo_motion_names);
    CHECK(!d->items && !d->item_count && !d->neutral_article && !d->on_death1 && !d->on_user_data_remove);
    CHECK(!d->model_effect_count && !d->model_effects && !d->owns_model_attachments);
    CHECK(d->on_load && d->on_load_special_attrs && d->on_death && d->on_absorb);
    CHECK(d->on_item_pickup_ext && d->on_item_drop_ext && d->on_item_pickup && d->on_item_drop);
    CHECK(d->on_item_invisible && d->on_item_visible && d->on_knockback_enter && d->on_knockback_exit);
    CHECK(d->on_aerial_jump && d->on_worn_item_jump_sound && d->on_presentation_item);
    CHECK(d->on_cpu_attack_decision && d->on_cpu_travel_decision && d->on_cpu_recovery &&
          d->on_cpu_special_candidate && d->on_cpu_projectile_hit);
    CHECK(d->on_forward_smash_enter && d->on_up_smash_enter && d->on_down_smash_enter);
#define VERIFY_CALLBACK(field) do { if(d->field) { own_pointer((const void*)d->field); ++callback_count; } } while(0)
#include "definition_callbacks.h"
#undef VERIFY_CALLBACK
    for(unsigned i=0;i<36;++i) {
        const MotionState* original=&ftNessClone_Definition.motion_states[i];
        const MotionState* owned=&d->motion_states[i];
        HSD_GObjEvent a[]={original->anim_cb,original->input_cb,original->phys_cb,original->coll_cb};
        HSD_GObjEvent b[]={owned->anim_cb,owned->input_cb,owned->phys_cb,owned->coll_cb};
        for(unsigned p=0;p<4;++p) { if(a[p]) CHECK(b[p]); own_pointer((const void*)b[p]); if(a[p]) ++phases; }
        CHECK(owned->cam_cb==original->cam_cb);
    }
    CHECK(phases==142 && d->article_logic_count==11);
    for(unsigned i=0;i<d->callback_export_count;++i) own_pointer((const void*)d->callback_exports[i].callback);
    for(unsigned i=0;i<11;++i) {
        const NativeFighterArticleLogic* article=&d->article_logics[i];
        CHECK(article->logic && article->state_count && article->state_count<=64);
        bool mapped=false;
        for(unsigned j=0;j<11;++j) if(d->article_assets[j].kind==article->kind) mapped=true;
        CHECK(mapped); own_pointer(article->logic); own_pointer(article->logic->states);
        ARTICLE_POINTER(article->on_created); ARTICLE_POINTER(article->on_out_of_bounds);
        ItemLogicTable* l=article->logic;
        ARTICLE_POINTER(l->spawned); ARTICLE_POINTER(l->destroyed); ARTICLE_POINTER(l->picked_up);
        ARTICLE_POINTER(l->dropped); ARTICLE_POINTER(l->thrown); ARTICLE_POINTER(l->dmg_dealt);
        ARTICLE_POINTER(l->dmg_received); ARTICLE_POINTER(l->entered_air); ARTICLE_POINTER(l->reflected);
        ARTICLE_POINTER(l->clanked); ARTICLE_POINTER(l->absorbed); ARTICLE_POINTER(l->shield_bounced);
        ARTICLE_POINTER(l->hit_shield); ARTICLE_POINTER(l->evt_unk);
        for(unsigned s=0;s<article->state_count;++s) {
            ARTICLE_POINTER(l->states[s].animated); ARTICLE_POINTER(l->states[s].physics_updated);
            ARTICLE_POINTER(l->states[s].collided);
        }
        for(unsigned s=0;s<article->signal_count;++s) ARTICLE_POINTER(article->signals[s].callback);
        if(article->projectile) {
            ARTICLE_POINTER(article->projectile->read_values); ARTICLE_POINTER(article->projectile->on_launched);
        }
    }
    CHECK(callback_count>=29 && article_callbacks>40);
    FreeLibrary(library);
    printf("Independent Ness ownership: %u checks, 0 failures, %u Definition callbacks, %u stock phases, %u article callbacks\n",
           checks,callback_count,phases,article_callbacks);
    return 0;
}
