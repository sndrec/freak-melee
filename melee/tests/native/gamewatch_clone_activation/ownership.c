// Actual generated DLL plus exact production copied metadata. Stock callback
// sentinels are never invoked. This gate proves ownership, not phase behavior.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftGameWatch/forward.h>
#include <melee/it/kinds/types.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned checks, phases, callbacks, article_callbacks;
static HMODULE library;
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr,"line%d: %s\n",__LINE__,#v); exit(1); } } while(0)
static void stock_slot(HSD_GObj* g) { (void)g; CHECK(false); }
static void common_slot(HSD_GObj* g) { (void)g; CHECK(false); }
static void unexpected(void) { CHECK(false); }
#include "base.h"
static void own(const void* p) {
    if (!p) return;
    MEMORY_BASIC_INFORMATION m;
    CHECK(VirtualQuery(p,&m,sizeof(m))==sizeof(m));
    CHECK(m.AllocationBase==library);
}
#define ARTICLE(p) do { if(p) { own((const void*)p); ++article_callbacks; } } while(0)
int main(int argc,char** argv) {
    CHECK(argc==2);
    library=LoadLibraryA(argv[1]); CHECK(library);
    NativeFighterHost host={0};
#include "required_host.h"
    MotionState common[341]={0};
    for(unsigned i=0;i<341;i++) {
        common[i].anim_cb=common[i].input_cb=common[i].phys_cb=common[i].coll_cb=common_slot;
    }
    host.common_motion_states=common;
    typedef const NativeFighterDefinition*(*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
    Entry open=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule"); CHECK(open);
    const NativeFighterDefinition* base=&ftGameWatchClone_Definition;
    const NativeFighterDefinition* d=open(base,&host); CHECK(d);
    CHECK(base->source_kind==Ft_Kind_GameWatch&&d->source_kind==Ft_Kind_None);
    CHECK(d->animation_source_kind==Ft_Kind_GameWatch);
    CHECK(d->motion_state_first==341&&d->motion_state_count==40);
    CHECK(d->action_count==323&&d->demo_count==14&&d->joint_count==53);
    CHECK(d->sound_banks==(UINT64_C(1)<<29)&&d->effect_bank==255);
    CHECK(base->sound_mute_channels==0&&d->sound_mute_channels==0);
    CHECK(d->bones==base->bones&&d->bone_count==38);
    CHECK(!d->omissions&&!d->omission_count);
    CHECK(d->costume_matanim&&d->costume_matanim[0]==0);
    CHECK(d->demo_motion_names==base->demo_motion_names);
    CHECK(d->asset_layout.article_mask==0x3FF&&d->asset_layout.visibility_mask==0x400);
    CHECK(d->asset_layout.item_format==FtCharacterItems_PaperArticles);
    CHECK(d->asset_layout.byte_range_count==1&&d->asset_layout.byte_ranges[0].offset==4&&d->asset_layout.byte_ranges[0].size==20);
    CHECK(d->on_load&&d->on_load_special_attrs&&d->on_death&&d->on_absorb);
    CHECK(d->on_jab_enter&&d->on_rapid_jab_enter&&d->on_down_tilt_enter&&d->on_forward_smash_enter);
    CHECK(d->on_aerial_attack_motion&&d->on_aerial_attack_enter&&d->on_landing_action);
#define VERIFY_CALLBACK(f) do { if(d->f) { own((const void*)d->f); ++callbacks; } } while(0)
#include "definition_callbacks.h"
#undef VERIFY_CALLBACK
    for(unsigned i=0;i<40;i++) {
        const MotionState* before=base->motion_states+i;
        const MotionState* after=d->motion_states+i;
        HSD_GObjEvent old[]={before->anim_cb,before->input_cb,before->phys_cb,before->coll_cb};
        HSD_GObjEvent next[]={after->anim_cb,after->input_cb,after->phys_cb,after->coll_cb};
        for(unsigned c=0;c<4;c++) {
            if(old[c]) CHECK(next[c]);
            bool shared=c==1&&(i==0||i==2||i==6||i==7||i==8);
            if(shared) CHECK(next[c]==common_slot);
            else own((const void*)next[c]);
            CHECK(next[c]!=stock_slot); ++phases;
        }
        CHECK(after->cam_cb==before->cam_cb);
    }
    CHECK(d->article_logic_count==10&&d->article_asset_count==10);
    CHECK(d->article_assets==base->article_assets);
    for(unsigned i=0;i<10;i++) {
        const NativeFighterArticleLogic* a=d->article_logics+i;
        bool mapped=false;
        for(unsigned j=0;j<10;j++) if(d->article_assets[j].kind==a->kind) {
            CHECK(d->article_assets[j].slot<10&&d->article_assets[j].kind_attribute_offset==-1);
            mapped=true;
        }
        CHECK(mapped&&a->on_created&&a->logic&&a->logic->states);
        own(a->logic); own(a->logic->states); ARTICLE(a->on_created);
        ItemLogicTable* l=a->logic;
        ARTICLE(l->spawned); ARTICLE(l->destroyed); ARTICLE(l->picked_up); ARTICLE(l->dropped); ARTICLE(l->thrown);
        ARTICLE(l->dmg_dealt); ARTICLE(l->dmg_received); ARTICLE(l->entered_air); ARTICLE(l->reflected); ARTICLE(l->clanked);
        ARTICLE(l->absorbed); ARTICLE(l->shield_bounced); ARTICLE(l->hit_shield); ARTICLE(l->evt_unk);
        for(unsigned s=0;s<a->state_count;s++) {
            ARTICLE(l->states[s].animated); ARTICLE(l->states[s].physics_updated); ARTICLE(l->states[s].collided);
        }
        for(unsigned s=0;s<a->signal_count;s++) ARTICLE(a->signals[s].callback);
    }
    FreeLibrary(library);
    printf("{\"checks\":%u,\"motion_slots\":%u,\"definition_callbacks\":%u,\"article_callbacks\":%u,\"failures\":0}\n",checks,phases,callbacks,article_callbacks);
    return 0;
}
