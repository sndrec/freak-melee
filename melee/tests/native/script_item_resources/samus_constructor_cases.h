// Connected retail constructor cohort. The script runs through the real
// compiler, Item actor and resource kernels. The comparison side calls the
// unchanged native constructor and real HSD allocation/animation routines.
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/it/kinds/types.h>
#include <melee/it/it_26B1.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/objalloc.h>

static unsigned samus_constructor_pairs;

static void samus_animation_audit(void)
{
    for(unsigned p=0;p<candidate.program.items.count;++p){
        const NativeScriptItemProgram* program=&candidate.program.items.programs[p];
        if(!program->resource_capacity)continue;
        for(unsigned r=0;r<program->model_root_count;++r)
            for(unsigned b=0;b<program->model_animation_count;++b){
                const NativeScriptItemModelAnimationAsset* bundle=&program->model_animations[b];
                const HSD_Joint* root=program->model_roots[r].root;
                NativeArticleAnimationDemand demand={0};
                bool valid=nativeArticleAnimation_Census(root?root->child:NULL,
                    bundle->anim,bundle->matanim,bundle->shapeanim,&demand);
                fprintf(stderr,"resource animation audit: kind=%u root=%u bundle=%u valid=%u uncovered=0x%X clocks=%u tracks=%u\n",
                    (unsigned)program->kind,r,b,valid,demand.uncovered,demand.controllers,demand.tracks);
            }
    }
}

static void samus_fighter_state(const char* name, NativeScriptValue value)
{
    NativeScriptReceiverView view;
    CHECK(nativeScriptFighter_View(fighter_object,(uint32_t)fighter_data->x8_spawnNum,&view));
    for(uint32_t n=0;n<view.program->value_count;++n)
        if(view.program->values[n].owner==view.storage->owner&&
           nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
            CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,
                NULL,0,&value,&diagnostic));return;
        }
    CHECK(false);
}

static void samus_equal_float(float actual,float expected)
{
    if(!(fabsf(actual-expected)<=2e-6f+2e-5f*fmaxf(fabsf(actual),fabsf(expected))))
        fprintf(stderr,"Samus float mismatch %.9g / %.9g (%s)\n",actual,expected,diagnostic.message);
    CHECK(isfinite(actual)&&isfinite(expected));
    CHECK(fabsf(actual-expected)<=2e-6f+2e-5f*fmaxf(fabsf(actual),fabsf(expected)));
}

static void samus_equal_vector(const Vec3* actual,const Vec3* expected)
{
    samus_equal_float(actual->x,expected->x);
    samus_equal_float(actual->y,expected->y);
    samus_equal_float(actual->z,expected->z);
}

#include "samus_held_cases.h"

static void samus_constructor_pair(int motion,float scale,bool ground,int sequence)
{
    Item_GObj* owner=objects[0];
    const NativeScriptItemProgram* spec=nativeScriptItem_Spec(owner);CHECK(spec);
    // Each constructor starts with a fresh primary graph, just like spawning a
    // new Item. Held playback in a preceding case must not retain its pose.
    HSD_JObj* old_primary=HSD_GObjObject_80390ADC(owner);
    HSD_JObjRemoveAll(old_primary);
    HSD_JObj* fresh_primary=HSD_JObjLoadJoint(spec->article->x10_modelDesc->x0_joint);CHECK(fresh_primary);
    HSD_GObjObject_80390A70(owner,HSD_GObj_JObjKind,fresh_primary);
    itSamusGrappleAttributes tuning=*(itSamusGrappleAttributes*)spec->article->x4_specialAttributes;
    Article reference_article=*spec->article;reference_article.x4_specialAttributes=&tuning;
    Item reference={.xC4_article_data=&reference_article};
    int mode=sequence>=4?(ground?1:2):0;
    reference.xDD4_itemVar.samusgrapple.x16=mode;
    fighter_data->motion_id=motion;
    fighter_data->ground_or_air=ground?GA_Ground:GA_Air;
    fighter_data->x34_scale=(Vec3){scale,scale,scale};
    samus_fighter_state("tether_attachment_part",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=51});
    samus_fighter_state("tether_code_sequence",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=sequence});
    samus_fighter_state("tether_grounded",(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=ground});
    NativeScriptValue result={0};
    CHECK(nativeScriptItem_RunHook(owner,NativeScriptItemHook_Created,NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_Void);
    CHECK(state(owner,"modified_mode",NULL).as.i64==mode);
    CHECK(state(owner,"solver",NULL).as.i64==-1);
    HSD_JObj* endpoint=it_802B75FC(&reference,fighter_data->parts[51].joint,motion==357,scale);
    CHECK(endpoint);
    ResourceCollection* set=nativeArticleResource_Collection(owner);CHECK(set);
    CHECK(set->count==tuning.x34&&set->created==set->count);
    CHECK(state(owner,"count",NULL).as.i64==set->count);
    CHECK(state(owner,"endpoint",NULL).as.i64==set->count-1);
    const char* fields[]={"length","min_length","launch_speed","gravity","launch_auxiliary",
        "retract_speed","retract_auxiliary","pull_speed","friction"};
    const float values[]={tuning.x38,tuning.x3C,tuning.x40,tuning.x44,tuning.x48,
        tuning.x4C,tuning.x50,tuning.x54,tuning.x58};
    for(unsigned n=0;n<9;++n)samus_equal_float(state(owner,fields[n],NULL).as.f32,values[n]);
    ItemLink* expected=reference.xDD4_itemVar.samusgrapple.x4;
    for(unsigned n=0;n<set->count;++n){
        ResourceCapture actual;CHECK(nativeArticleResource_Capture(owner,n,&actual)&&expected);
        CHECK(actual.link->jobj==expected->jobj&&actual.link->jobj==fighter_data->parts[51].joint);
        CHECK(actual.link->x1CC==expected->x1CC);
        CHECK(actual.link->x2C_b0==expected->x2C_b0&&actual.link->x2C_b1==expected->x2C_b1&&actual.link->x2C_b2==expected->x2C_b2);
        samus_equal_vector(&actual.link->pos,&expected->pos);
        samus_equal_vector(&actual.link->vel,&expected->vel);
        samus_equal_vector(&actual.link->coll_data.cur_pos,&expected->coll_data.cur_pos);
        samus_equal_vector(&actual.link->coll_data.last_pos,&expected->coll_data.last_pos);
        CHECK(actual.link->next==(n?set->nodes[n-1].link:NULL));
        CHECK(actual.link->prev==(n+1<set->count?set->nodes[n+1].link:NULL));
        CHECK(actual.gobj->render_cb&&expected->gobj->render_cb);
        same_animation_graph(actual.model,expected->gobj->hsd_obj,0);
        same_material_animation(actual.model,expected->gobj->hsd_obj,0);
        const uint8_t child[]={0};
        const float samples[]={0,1,7,23,60};
        for(unsigned sample=0;sample<sizeof(samples)/sizeof(samples[0]);++sample){
            CHECK(nativeArticleResource_RequestAnimation(owner,n,child,1,samples[sample]));
            HSD_JObjReqAnimAll(((HSD_JObj*)expected->gobj->hsd_obj)->child,samples[sample]);
            CHECK(nativeArticleResource_AdvanceAnimation(owner,n,child,1));
            HSD_JObjAnimAll(((HSD_JObj*)expected->gobj->hsd_obj)->child);
            same_animation_graph(actual.model,expected->gobj->hsd_obj,0);
            same_material_animation(actual.model,expected->gobj->hsd_obj,0);
        }
        if(!samus_constructor_pairs){
            refuse_palette_mutations(owner,n,actual.model);
            // The refusal corpus ends by requesting frame zero on the source.
            // Reset both graphs before the later held-update comparison.
            CHECK(nativeArticleResource_RequestAnimation(owner,n,child,1,0));
            HSD_JObjReqAnimAll(((HSD_JObj*)expected->gobj->hsd_obj)->child,0);
        }
        expected=expected->prev;
    }
    CHECK(!expected);
    CHECK(fighter_data->parts[139].joint==set->nodes[set->count-1].model);
    // The complete created hook also installs/requests the primary animation.
    HSD_JObj* primary_model=owner->hsd_obj;CHECK(primary_model&&primary_model->child);
    HSD_JObj* primary_reference=HSD_JObjLoadJoint(spec->article->x10_modelDesc->x0_joint);CHECK(primary_reference&&primary_reference->child);
    HSD_JObjAddAnimAll(primary_reference->child,tuning.xA4?*tuning.xA4:NULL,
        tuning.xA8?*tuning.xA8:NULL,tuning.xAC?*tuning.xAC:NULL);
    HSD_JObjReqAnimAll(primary_reference->child,0);
    same_animation_graph(primary_model,primary_reference,0);
    samus_held_cases(owner,&reference,primary_reference);
    HSD_JObjRemoveAll(primary_reference);
    // Both implementations construct real nodes; every node and graph is freed.
    expected=reference.xDD4_itemVar.samusgrapple.x4;
    while(expected){ItemLink* next=expected->prev;HSD_GObjFree(expected->gobj);expected=next;}
    nativeArticleResource_Release(owner);
    CHECK(!nativeArticleResource_Collection(owner)&&!fighter_data->parts[139].joint);
    ++samus_constructor_pairs;
}

static void samus_constructor_cases(void)
{
    palette_census();
    CHECK(samus_graphs&&candidate.assets.parts.parts_num>51&&candidate.assets.parts.parts_num<=139);
    CHECK(nativeItemLinkPool_Ready());
    const int motions[]={212,214,357,358};
    const float scales[]={0.5f,1.0f,1.5f};
    for(unsigned m=0;m<4;++m)for(unsigned s=0;s<3;++s)
        for(unsigned g=0;g<2;++g)for(unsigned q=0;q<3;++q)
            samus_constructor_pair(motions[m],scales[s],g!=0,q==2?4:(int)q);
    printf("{\"status\":\"PASS Samus held updates\",\"held_updates\":%u,\"held_hitlag_updates\":%u,\"held_refusals\":%u}\n",samus_held_updates,samus_held_hitlag_updates,samus_held_refusals);
}
