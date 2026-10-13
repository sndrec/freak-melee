// Real HSD held-tether updates versus the unchanged native accessory body.
// The parent Fighter and input/counter samples are staged, not game-scheduled.
static unsigned samus_held_updates,samus_held_hitlag_updates,samus_held_refusals;

static void samus_compare_held_graphs(Item_GObj* owner,Item* reference,
                                      HSD_JObj* primary_reference)
{
    same_animation_graph(owner->hsd_obj,primary_reference,0);
    same_material_animation(owner->hsd_obj,primary_reference,0);
    ResourceCollection* set=nativeArticleResource_Collection(owner);CHECK(set);
    ItemLink* expected=reference->xDD4_itemVar.samusgrapple.x4;
    for(unsigned n=0;n<set->count;++n){
        CHECK(expected);
        HSD_JObj* model=expected->gobj->hsd_obj;
        if(set->nodes[n].model->child&&model->child&&memcmp(&set->nodes[n].model->child->rotate,&model->child->rotate,sizeof(model->rotate))){
            HSD_JObj* actual=set->nodes[n].model;
            fprintf(stderr,"held resource mismatch index=%u count=%u flags=0x%X clocks=%.9g/%.9g updates=%u hitlag=%u script_count=%lld solver=%lld\n",n,set->count,(unsigned)actual->flags,
                actual->child->aobj?actual->child->aobj->curr_frame:-1,model->child->aobj?model->child->aobj->curr_frame:-1,
                samus_held_updates,samus_held_hitlag_updates,(long long)state(owner,"count",NULL).as.i64,(long long)state(owner,"solver",NULL).as.i64);
        }
        same_animation_graph(set->nodes[n].model,model,0);
        same_material_animation(set->nodes[n].model,model,0);
        if(n==set->count-1){
            Mtx observed;CHECK(nativeArticleResource_ReadWorldMatrix(owner,n,observed));
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
                samus_equal_float(observed[r][c],model->mtx[r][c]);
        }
        expected=expected->prev;
    }
    CHECK(!expected);
}

static void samus_held_cases(Item_GObj* owner,Item* reference,
                             HSD_JObj* primary_reference)
{
    Item_GObj* native_object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(native_object);
    native_object->user_data=reference;native_object->hsd_obj=primary_reference;
    reference->entity=native_object;reference->owner=fighter_object;
    Item* actual=owner->user_data;actual->msid=0;
    CHECK(nativeScriptItem_SelectState(owner,0));
    NativeScriptValue result={0};nativeScriptItem_BeginTick(owner);
    CHECK(nativeScriptItem_RunPhase(owner,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);
    CHECK(state(owner,"solver",NULL).as.i64==0);
    fighter_data->u.ss.x2240=0;
    samus_fighter_state("tether_code_sequence",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=0});
    HSD_JObj* attachment=fighter_data->parts[51].joint;
    Vec3 original=attachment->translate;
    if(!samus_held_updates){
        ResourceCollection* set=nativeArticleResource_Collection(owner);CHECK(set);
        HSD_JObj* endpoint=set->nodes[set->count-1].model;CHECK(endpoint&&endpoint->child);
        u32 flags=endpoint->flags;endpoint->flags|=JOBJ_USER_DEF_MTX;
        CHECK(!nativeArticleResource_AdvanceAnimation(owner,set->count-1,NULL,0));
        endpoint->flags=flags;++samus_held_refusals;
        flags=endpoint->child->flags;endpoint->child->flags|=JOBJ_USER_DEF_MTX;
        CHECK(!nativeArticleResource_AdvanceAnimation(owner,set->count-1,NULL,0));
        endpoint->child->flags=flags;++samus_held_refusals;
    }
    const uint32_t buttons[]={0,8,8,4,8,0,0,0};
    for(unsigned tick=0;tick<8;++tick){
        Vec3 offset={original.x+(float)tick*0.125f,original.y-(float)tick*0.25f,original.z+(float)tick*0.5f};
        HSD_JObjSetTranslate(attachment,&offset);
        fighter_data->input.held_buttons[0]=buttons[tick];
        fighter_data->input.pressed_buttons=tick==6?0x100:0;
        nativeScriptItem_BeginTick(owner);
        CHECK(nativeScriptItem_RunSignal(owner,signal_id(owner,"normal_update"),0,&result)==NativeScriptVM_Ok);
        fn_802B7E34(native_object);
        samus_compare_held_graphs(owner,reference,primary_reference);
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(fighter_object,(uint32_t)fighter_data->x8_spawnNum,&view));
        bool found=false;
        for(uint32_t n=0;n<view.program->value_count;++n)
            if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,"tether_code_sequence")){
                NativeScriptValue value;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&value,&diagnostic));
                CHECK(value.as.i64==fighter_data->u.ss.x2240);found=true;break;
            }
        CHECK(found);++samus_held_updates;
        // Motion 358 has no pre-launch branch in the original hitlag body.
        // Flight/constraint hitlag is a separate solver acceptance boundary.
        if(fighter_data->motion_id!=358){
            offset.x+=0.0625f;HSD_JObjSetTranslate(attachment,&offset);
            fighter_data->mv.ss.unk7.x0=-1;
            samus_fighter_state("tether_before_launch",(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=true});
            nativeScriptItem_BeginTick(owner);
            CHECK(nativeScriptItem_RunSignal(owner,signal_id(owner,"hitlag_update"),0,&result)==NativeScriptVM_Ok);
            Item_GObj* saved=fighter_data->u.ss.x223C;fighter_data->u.ss.x223C=native_object;
            it_802BACC4(fighter_object);fighter_data->u.ss.x223C=saved;
            samus_compare_held_graphs(owner,reference,primary_reference);
            ++samus_held_hitlag_updates;
        }
    }
    HSD_JObjSetTranslate(attachment,&original);
    fighter_data->input.held_buttons[0]=0;fighter_data->input.pressed_buttons=0;
    actual->msid=-1;
    native_object->user_data=NULL;native_object->hsd_obj=NULL;HSD_GObjFree(native_object);
}
