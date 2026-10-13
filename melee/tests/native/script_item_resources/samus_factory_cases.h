// Full shared Item factory and source fighter helper, with imported common Item
// data. The parent Fighter payload is staged; no game scheduler/draw is run.
#include <melee/it/itcommon_data.h>
#include <melee/it/kinds/itlinkhookshot.h>

static ItCommonAssets samus_common;
static unsigned samus_factory_pairs,samus_factory_refusals,samus_invalid_motions,samus_death_removals;
static unsigned samus_factory_updates,samus_factory_hitlag_updates;

static NativeScriptValue samus_fighter_read(const char* name)
{
    NativeScriptReceiverView view;
    CHECK(nativeScriptFighter_View(fighter_object,(uint32_t)fighter_data->x8_spawnNum,&view));
    for(uint32_t n=0;n<view.program->value_count;++n)
        if(view.program->values[n].owner==view.storage->owner&&
           nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
            NativeScriptValue value={0};
            CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&value,&diagnostic));return value;
        }
    CHECK(false);return(NativeScriptValue){0};
}

static void samus_fighter_helper(const char* name)
{
    for(uint32_t n=0;n<candidate.compiler.bank.declarations.function_count;++n)
        if(!candidate.compiler.bank.declarations.functions[n].owner&&
           nativeScriptSource_SpanEquals(&candidate.compiler.bank.tree,candidate.compiler.bank.declarations.functions[n].name,name)){
            NativeScriptValue result={0};
            CHECK(nativeScriptFighter_Helper(fighter_object,(uint32_t)fighter_data->x8_spawnNum,n,NULL,0,&result,&diagnostic)==NativeScriptVM_Ok);
            CHECK(result.type==NativeScriptType_Void);return;
        }
    CHECK(false);
}

static Item_GObj* samus_find_handle(int32_t handle)
{
    CHECK(handle>=0);
    for(Item_GObj* item=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];item;item=item->next)
        if(item->user_data&&(uint32_t)((Item*)item->user_data)->x1C==(uint32_t)handle)return item;
    CHECK(false);return NULL;
}

#include "samus_solver_cases.h"

static void samus_factory_cases(const char* common_path)
{
    FILE* file=fopen(common_path,"rb");CHECK(file&&fseek(file,0,SEEK_END)==0);
    long size=ftell(file);CHECK(size>0&&fseek(file,0,SEEK_SET)==0);
    void* bytes=malloc((size_t)size);void* arena=malloc(IT_COMMON_IMPORT_ARENA_SIZE);CHECK(bytes&&arena);
    CHECK(fread(bytes,1,(size_t)size,file)==(size_t)size&&fclose(file)==0);
    CHECK(itCommonAssets_Import(bytes,(size_t)size,arena,IT_COMMON_IMPORT_ARENA_SIZE,IT_COMMON_IMPORT_RECORD_CAPACITY,&samus_common));
    it_804D6D20=&samus_common.public_data;it_804D6D24=samus_common.common_articles;
    it_804D6D28=&samus_common.common_data;it_804D6D30=samus_common.pokemon_articles;
    it_804D6D38=samus_common.character_articles;it_804D6D40=&samus_common.zako_data;it_804D6D04=samus_common.color_entries;
    Item_80266FCC();
    // Earlier resource cases keep staged Items alive with explicit identities.
    // The real factory's reset counter must not collide with those test actors;
    // in gameplay every Item obtains its identity from that same factory.
    for(Item_GObj* item=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];item;item=item->next)
        if(item->classifier==HSD_GOBJ_CLASS_ITEM&&item->user_data){
            uint32_t id=(uint32_t)((Item*)item->user_data)->x1C;CHECK(id<UINT32_MAX);
            if(it_804D6D10<=id)it_804D6D10=id+1;
        }
    const int motions[]={212,214,357,358};const float scales[]={0.5f,1,1.5f};
    const int sequences[]={0,1,4};
    const unsigned modes[]={0,5,4,1,2,3,6,8,7};
    for(unsigned cohort=0;cohort<sizeof(modes)/sizeof(*modes);++cohort)
    for(unsigned m=0;m<4;++m)for(unsigned s=0;s<3;++s)for(unsigned f=0;f<2;++f)for(unsigned q=0;q<3;++q){
        fighter_data->motion_id=motions[m];fighter_data->facing_dir=f?-1:1;
        fighter_data->ground_or_air=m<2?GA_Ground:GA_Air;
        fighter_data->x34_scale=(Vec3){scales[s],scales[s],scales[s]};
        samus_fighter_state("tether_handle",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1});
        samus_fighter_state("tether_code_sequence",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=sequences[q]});
        Vec3 expected_position;lb_8000B1CC(fighter_data->parts[51].joint,NULL,&expected_position);
        unsigned objects_before=nativeGObjPool.used;
        samus_fighter_helper("Tether_Create");
        int32_t handle=(int32_t)samus_fighter_read("tether_handle").as.i64;
        Item_GObj* item=samus_find_handle(handle);Item* payload=item->user_data;
        CHECK(nativeScriptItem_Status(item)==NativeScriptVM_Ok&&nativeScriptItem_Creator(item)==fighter_object);
        CHECK(nativeScriptItem_AttachmentOwner(item)==fighter_object&&payload->owner==fighter_object);
        CHECK(payload->kind==It_Kind_Samus_GBeam&&payload->msid==0&&payload->xAC4_ignoreItemID==0);
        CHECK(payload->facing_dir==fighter_data->facing_dir);
        samus_equal_vector(&payload->xDD4_itemVar.native_article.launch_origin,&expected_position);
        samus_equal_float(payload->scl,scales[s]*payload->xCC_item_attr->x60_scale);
        CHECK(!fighter_data->item_gobj);
        CHECK(state(item,"modified_mode",NULL).as.i64==(q==2?(m<2?1:2):0));
        CHECK(samus_fighter_read("tether_code_sequence").as.i64==(q==2?4:0));
        // Exact native post-attach scale helper on a separate primary graph.
        Item_GObj* reference=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(reference);
        Item reference_data={.entity=reference,.owner=fighter_object,.xCC_item_attr=payload->xCC_item_attr};
        reference->user_data=&reference_data;
        HSD_JObj* reference_model=HSD_JObjLoadJoint(payload->xC4_article_data->x10_modelDesc->x0_joint);CHECK(reference_model);
        HSD_GObjObject_80390A70(reference,HSD_GObj_JObjKind,reference_model);
        itLinkHookshot_ApplyOwnerScale(reference);
        samus_equal_float(payload->scl,reference_data.scl);
        samus_equal_vector(&((HSD_JObj*)item->hsd_obj)->scale,&reference_model->scale);
        reference->user_data=NULL;HSD_GObjFree(reference);
        ResourceCollection* set=nativeArticleResource_Collection(item);CHECK(set&&set->count&&set->created==set->count);
        CHECK(fighter_data->parts[139].joint==set->nodes[set->count-1].model);
        CHECK(nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_AccessoryBefore));
        CHECK(nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_Death1));
        CHECK(nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_AccessoryHitlag));
        // Use the callbacks installed by Tether_Create, not direct helper calls.
        // Keep the counter before launch: the flight solver remains separate.
        samus_fighter_state("tether_counter",(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=-1});
        for(unsigned tick=0;tick<2;++tick){
            NativeScriptValue result={0};nativeScriptItem_BeginTick(item);
            CHECK(nativeScriptItem_RunPhase(item,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);
            CHECK(nativeScriptFighter_RunCallback(fighter_object,NativeScriptFighterCallback_AccessoryBefore));
            CHECK(state(item,"solver",NULL).as.i64==0&&nativeScriptItem_Status(item)==NativeScriptVM_Ok);
            Mtx observed;CHECK(nativeArticleResource_ReadWorldMatrix(item,set->count-1,observed));
            HSD_JObjSetupMatrix(fighter_data->parts[51].joint);
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
                samus_equal_float(observed[r][c],fighter_data->parts[51].joint->mtx[r][c]);
            ++samus_factory_updates;
            if(m<3){
                nativeScriptItem_BeginTick(item);
                CHECK(nativeScriptFighter_RunCallback(fighter_object,NativeScriptFighterCallback_AccessoryHitlag));
                CHECK(nativeScriptItem_Status(item)==NativeScriptVM_Ok&&samus_fighter_read("tether_before_launch").as.u64);
                ++samus_factory_hitlag_updates;
            }
        }
        NativeMatchPoolReference identity;CHECK(nativeMatchPool_Reference(&nativeGObjPool,item,&identity));
        if(cohort)samus_solver_case(item,q,modes[cohort]);
        else if(f){CHECK(nativeScriptFighter_RunCallback(fighter_object,NativeScriptFighterCallback_Death1));++samus_death_removals;}
        else samus_fighter_helper("Tether_Remove");
        CHECK(samus_fighter_read("tether_handle").as.i64==-1&&!nativeMatchPool_Resolve(identity));
        CHECK(!fighter_data->parts[139].joint&&nativeGObjPool.used==objects_before);
        CHECK(!nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_AccessoryBefore));
        CHECK(!nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_Death1));
        CHECK(!nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_AccessoryHitlag));
        CHECK(nativeScriptFighter_Status(fighter_object)==NativeScriptVM_Ok);
        ++samus_factory_pairs;
    }
    const int invalid_motions[]={0,213,356,359};
    for(unsigned n=0;n<4;++n){
        fighter_data->motion_id=invalid_motions[n];unsigned before=nativeGObjPool.used;
        samus_fighter_helper("Tether_Create");
        CHECK(samus_fighter_read("tether_handle").as.i64==-1&&nativeGObjPool.used==before);
        CHECK(!fighter_data->parts[139].joint);++samus_invalid_motions;
    }
    // Inject zero available GObj slots at the actual fixed-pool allocation
    // boundary. This is controlled capacity, not a maximum-volume game test.
    // The shared factory must return NULL without publishing an Item, and the
    // script must retain its incomplete code sequence.
    fighter_data->motion_id=212;
    samus_fighter_state("tether_code_sequence",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=3});
    unsigned saved_free=nativeGObjPool.free_count;nativeGObjPool.free_count=0;
    unsigned before=nativeGObjPool.used;samus_fighter_helper("Tether_Create");
    nativeGObjPool.free_count=saved_free;
    CHECK(samus_fighter_read("tether_handle").as.i64==-1&&samus_fighter_read("tether_code_sequence").as.i64==3);
    CHECK(nativeGObjPool.used==before&&!fighter_data->parts[139].joint);
    CHECK(!nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_Death1));
    ++samus_factory_refusals;
    free(arena);free(bytes);
    it_804D6D20=NULL;it_804D6D24=NULL;it_804D6D28=NULL;it_804D6D30=NULL;it_804D6D38=NULL;it_804D6D40=NULL;it_804D6D04=NULL;
    printf("{\"status\":\"PASS Samus factory lifecycle\",\"factory_pairs\":%u,\"factory_refusals\":%u,\"invalid_motions\":%u,\"death_removals\":%u,\"factory_updates\":%u,\"factory_hitlag_updates\":%u}\n",samus_factory_pairs,samus_factory_refusals,samus_invalid_motions,samus_death_removals,samus_factory_updates,samus_factory_hitlag_updates);
    printf("{\"status\":\"PASS Samus capture retraction\",\"capture_pairs\":%u,\"capture_updates\":%u,\"capture_completions\":%u}\n",samus_capture_pairs,samus_capture_updates,samus_capture_completions);
    printf("{\"status\":\"PASS Samus ordinary retraction\",\"retraction_pairs\":%u,\"retraction_updates\":%u,\"retraction_completions\":%u,\"returned_held_updates\":%u}\n",samus_retraction_pairs,samus_retraction_updates,samus_retraction_completions,samus_returned_held_updates);
    printf("{\"status\":\"PASS Samus post-launch hitlag constraints\",\"constraint_hitlag_updates\":%u}\n",samus_constraint_hitlag_updates);
    printf("{\"status\":\"PASS Samus controlled-contact flight\",\"flight_pairs\":%u,\"flight_updates\":%u,\"flight_transitions\":%u}\n",samus_flight_pairs,samus_flight_updates,samus_flight_transitions);
    printf("{\"status\":\"PASS Samus slack/wall constraints\",\"slack_pairs\":%u,\"wall_pairs\":%u,\"constraint_updates\":%u,\"constraint_transitions\":%u}\n",samus_slack_pairs,samus_wall_pairs,samus_wall_updates,samus_wall_transitions);
    printf("{\"status\":\"PASS Samus pull constraints\",\"pull_pairs\":%u,\"pull_updates\":%u,\"pull_transitions\":%u,\"pull_tail_clamps\":%u}\n",samus_pull_pairs,samus_pull_updates,samus_pull_transitions,samus_pull_tail_clamps);
}
