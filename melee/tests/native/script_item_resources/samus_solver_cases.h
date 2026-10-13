// Grapple solvers use real factory-created source Items and native HSD
// comparison graphs. Parent/capture state and the starting chain are staged.
#include <sysdolphin/baselib/random.h>
#include <melee/lb/lbaudio_ax.h>

static unsigned samus_capture_pairs,samus_capture_updates,samus_capture_completions;
static unsigned samus_retraction_pairs,samus_retraction_updates,samus_retraction_completions;
static unsigned samus_constraint_hitlag_updates,samus_returned_held_updates;
static unsigned samus_flight_pairs,samus_flight_updates,samus_flight_transitions;
static unsigned samus_slack_pairs,samus_wall_pairs,samus_wall_updates,samus_wall_transitions;
static unsigned samus_pull_pairs,samus_pull_updates,samus_pull_transitions,samus_pull_tail_clamps;

static bool samus_array_bool(Item_GObj* item,const char* name,unsigned index)
{
    NativeScriptReceiverView view;CHECK(nativeScriptItem_View(item,(uint32_t)((Item*)item->user_data)->x1C,&view));
    for(uint32_t n=0;n<view.program->value_count;++n)
        if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
            NativeScriptValue selector={.type=NativeScriptType_I32,.as.i64=index},value={0};
            CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,&selector,1,&value,&diagnostic));
            CHECK(value.type==NativeScriptType_Bool);return value.as.u64!=0;
        }
    CHECK(false);return false;
}

static bool samus_capture_hook(uint32_t hook)
{
    CHECK(nativeScriptFighter_HasHook(fighter_object,hook));
    NativeScriptValue result={0};
    CHECK(nativeScriptFighter_Run(fighter_object,hook,NULL,0,NULL,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_Bool);return result.as.u64!=0;
}

static void samus_deployed(Item_GObj* item,unsigned index,bool enabled)
{
    NativeScriptReceiverView view;CHECK(nativeScriptItem_View(item,(uint32_t)((Item*)item->user_data)->x1C,&view));
    for(uint32_t n=0;n<view.program->value_count;++n)
        if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,"deployed")){
            NativeScriptValue selector={.type=NativeScriptType_I32,.as.i64=index};
            NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=enabled};
            CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,&selector,1,&value,&diagnostic));
            CHECK(nativeArticleResource_SetDraw(item,index,enabled));return;
        }
    CHECK(false);
}

static void samus_retract_graph(HSD_JObj* actual,HSD_JObj* expected,unsigned depth)
{
    CHECK(depth<64);
    for(;actual&&expected;actual=actual->next,expected=expected->next){
        CHECK(actual->flags==expected->flags);
        samus_equal_vector(&actual->translate,&expected->translate);
        samus_equal_vector((Vec3*)&actual->rotate,(Vec3*)&expected->rotate);
        samus_equal_float(actual->rotate.w,expected->rotate.w);
        samus_equal_vector(&actual->scale,&expected->scale);
        same_animation_controllers(actual->aobj,expected->aobj);
        CHECK(!!actual->child==!!expected->child);
        if(actual->child&&!(actual->flags&JOBJ_INSTANCE))samus_retract_graph(actual->child,expected->child,depth+1);
    }
    CHECK(!actual&&!expected);
}

static void samus_solver_compare(Item_GObj* item,Item* reference,HSD_JObj* primary,unsigned mode,unsigned pattern,unsigned tick)
{
    ResourceCollection* set=nativeArticleResource_Collection(item);CHECK(set);
    unsigned count=set->count;ItemLink* expected=reference->xDD4_itemVar.samusgrapple.x4;
    for(unsigned n=0;n<count;++n){
        snprintf(diagnostic.message,sizeof(diagnostic.message),"solver mode %u pattern %u tick %u node %u",mode,pattern,tick,n);
        ResourceCapture actual;CHECK(expected&&nativeArticleResource_Capture(item,n,&actual));
        CHECK(actual.link->x2C_b0==expected->x2C_b0);
        CHECK(samus_array_bool(item,"deployed",n)==(bool)expected->x2C_b0);
        CHECK(samus_array_bool(item,"on_floor",n)==(bool)expected->x2C_b1);
        snprintf(diagnostic.message,sizeof(diagnostic.message),"solver mode %u tick %u node %u position",mode,tick,n);
        samus_equal_vector(&actual.link->pos,&expected->pos);samus_equal_vector(&actual.link->vel,&expected->vel);
        samus_equal_vector(&actual.link->coll_data.cur_pos,&expected->coll_data.cur_pos);
        samus_equal_vector(&actual.link->coll_data.last_pos,&expected->coll_data.last_pos);
        CHECK(actual.link->coll_data.env_flags==expected->coll_data.env_flags);
        CHECK(actual.link->coll_data.floor.index==expected->coll_data.floor.index);
        CHECK(actual.link->coll_data.left_facing_wall.index==expected->coll_data.left_facing_wall.index);
        CHECK(actual.link->coll_data.right_facing_wall.index==expected->coll_data.right_facing_wall.index);
        CHECK(actual.link->x1CC==expected->x1CC);
        samus_equal_vector(&actual.link->coll_data.floor.normal,&expected->coll_data.floor.normal);
        snprintf(diagnostic.message,sizeof(diagnostic.message),"solver mode %u tick %u node %u graph",mode,tick,n);
        samus_retract_graph(actual.model,expected->gobj->hsd_obj,0);
        HSD_JObj* native_model=expected->gobj->hsd_obj;
        if((actual.model->flags&0x03800000)==0x03800000){
            Mtx observed;CHECK(nativeArticleResource_ReadWorldMatrix(item,n,observed));
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
                samus_equal_float(observed[r][c],native_model->mtx[r][c]);
        }
        same_material_animation(actual.model,expected->gobj->hsd_obj,0);
        expected=expected->prev;
    }
    snprintf(diagnostic.message,sizeof(diagnostic.message),"solver mode %u tick %u primary graph",mode,tick);
    samus_retract_graph(item->hsd_obj,primary,0);
}

static void samus_solver_case(Item_GObj* item,unsigned pattern,unsigned mode)
{
    CHECK(mode>=1&&mode<=8);
    bool flight=mode<=2;
    bool constraint=mode==3||mode>=6;
    Item* payload=item->user_data;ResourceCollection* set=nativeArticleResource_Collection(item);CHECK(set);
    unsigned count=set->count;
    itSamusGrappleAttributes tuning=*(itSamusGrappleAttributes*)payload->xC4_article_data->x4_specialAttributes;
    Article article=*payload->xC4_article_data;article.x4_specialAttributes=&tuning;
    Item reference={.kind=It_Kind_Samus_GBeam,.xC4_article_data=&article,.xCC_item_attr=payload->xCC_item_attr,.scl=payload->scl,.msid=(int)mode};
    reference.xDD4_itemVar.samusgrapple.x16=(int)state(item,"modified_mode",NULL).as.i64;
    Fighter parent=*fighter_data;
    parent.dat_attrs=parent.ft_data->ext_attr;
    if(mode>=6){
        line_geometry_reset();
        parent.input.held_buttons[0]=pattern==2?0x40:0;
        fighter_data->input.held_buttons[0]=parent.input.held_buttons[0];
        parent.input.pressed_buttons=pattern==1?0x100:0;
        fighter_data->input.pressed_buttons=parent.input.pressed_buttons;
        parent.mv.ss.grapple.x4=240;
        samus_fighter_state("tether_hang_timer",(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=240});
    }
    Fighter_GObj* native_parent=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(native_parent);
    native_parent->user_data=&parent;parent.gobj=native_parent;
    Item_GObj* native_item=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(native_item);
    native_item->user_data=&reference;reference.entity=native_item;reference.owner=native_parent;
    CHECK(it_802B75FC(&reference,fighter_data->parts[51].joint,parent.motion_id==357,parent.x34_scale.y));
    CHECK((unsigned)tuning.x34==count);
    HSD_JObj* primary=HSD_JObjLoadJoint(article.x10_modelDesc->x0_joint);CHECK(primary&&primary->child);
    HSD_GObjObject_80390A70(native_item,HSD_GObj_JObjKind,primary);
    HSD_JObjAddAnimAll(primary->child,tuning.xA4?*tuning.xA4:NULL,tuning.xA8?*tuning.xA8:NULL,tuning.xAC?*tuning.xAC:NULL);
    HSD_JObjReqAnimAll(primary->child,0);
    itLinkHookshot_ApplyOwnerScale(native_item);
    fn_802B7E34(native_item);
    HSD_JObjReqAnimAll(primary->child,0);HSD_JObjReqAnimAll(((HSD_JObj*)item->hsd_obj)->child,0);
    HSD_JObjAnimAll(primary);HSD_JObjAnimAll(item->hsd_obj);
    Vec3 anchor;lb_8000B1CC(fighter_data->parts[51].joint,NULL,&anchor);
    unsigned first=pattern==0?0:pattern==1?count/2:count-1;
    // Pull completion enters full Fighter/ledge states. Keep this cohort in
    // the nonterminal solver, with at least half of the chain deployed.
    if(mode==7&&pattern==2)first=count/3;
    ItemLink* expected=reference.xDD4_itemVar.samusgrapple.x4;
    const uint8_t child[]={0};
    for(unsigned n=0;n<count;++n){
        CHECK(expected);
        float distance=(float)((int)n-(int)first+1)*tuning.x38*0.65f;
        // Exercise retail's tail-clamp branch, where the decomp used an
        // uninitialized pointer. The endpoint remains fixed to its wall.
        if(mode==7&&n==count-1)distance+=4*tuning.x38;
        Vec3 position={anchor.x+parent.facing_dir*distance,anchor.y+distance*0.15f,anchor.z+(float)(n%3)*0.125f};
        Vec3 velocity={0.2f,0.1f,-0.125f};
        expected->pos=position;expected->vel=velocity;
        expected->coll_data.cur_pos=position;expected->coll_data.last_pos=(Vec3){position.x+0.05f,position.y-0.1f,position.z+0.025f};
        expected->x2C_b0=n>=first;
        samus_deployed(item,n,n>=first);
        ResourceCapture actual;CHECK(nativeArticleResource_Capture(item,n,&actual));
        actual.link->pos=expected->pos;actual.link->vel=expected->vel;
        actual.link->coll_data.cur_pos=expected->coll_data.cur_pos;actual.link->coll_data.last_pos=expected->coll_data.last_pos;
        if(mode>=6&&n==count-1){
            actual.link->x1CC=expected->x1CC=pattern==0?-1:5;
        }
        CHECK(nativeArticleResource_RequestAnimation(item,n,child,1,0));
        HSD_JObjReqAnimAll(((HSD_JObj*)expected->gobj->hsd_obj)->child,0);
        CHECK(nativeArticleResource_AdvanceAnimation(item,n,NULL,0));
        HSD_JObjAnimAll(expected->gobj->hsd_obj);
        expected=expected->prev;
    }
    if(mode==5){
        fighter_data->motion_id=ftCo_MS_CatchPull;
        CHECK(samus_capture_hook(NativeScriptFighter_CatchPullAnchor));
    }else if(mode==4){
        NativeScriptValue result={0};
        CHECK(nativeScriptItem_RunSignal(item,signal_id(item,"retract"),0,&result)==NativeScriptVM_Ok);
    }else{
        Item_80268E5C(item,(int)mode,ITEM_ANIM_UPDATE);
        itLinkHookshot_ApplyOwnerScale(item);
    }
    // Apply the same shared state transition on the native comparison Item:
    // it updates the primary model position and removes its animation too.
    reference.pos=payload->pos;reference.facing_dir=payload->facing_dir;
    reference.xDC8_word.flags.x19=payload->xDC8_word.flags.x19;
    reference.xBC_itemStateContainer=it_803F73A8;
    reference.sfx_unk2=SFX_NONE;
    Item_80268E5C(native_item,(int)mode,ITEM_ANIM_UPDATE);
    itLinkHookshot_ApplyOwnerScale(native_item);
    CHECK(payload->msid==(int)mode);
    HSD_JObj* endpoint=set->nodes[count-1].model;
    if(mode==5){
        CHECK(state(item,"capture_bound",NULL).as.u64&&fighter_data->mv.co.capturedamage.x18==endpoint);
        CHECK(!samus_capture_hook(NativeScriptFighter_CatchPullReady));
        CHECK(!samus_capture_hook(NativeScriptFighter_CatchPullFollow));
    }
    unsigned completed_tick=0;
    parent.u.ss.x223C=native_item;parent.mv.ss.unk7.x0=1000;
    samus_fighter_state("tether_counter",(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1000});
    reference.xDD4_itemVar.samusgrapple.x8=native_parent;
    samus_flight_geometry=flight;
    samus_contact_flags=pattern==0?0:pattern==1?Collide_LeftWallMask:Collide_FloorMask;
    // The aerial wall branch performs full Fighter state/effect construction;
    // that is a separate cohort, not replaced by controlled handler stubs.
    if(mode==1&&parent.motion_id==357&&pattern==1)samus_contact_flags=Collide_FloorMask;
    for(unsigned step=0;step<260;++step){
        if(mode==3&&pattern==2&&step==4)fighter_data->motion_id=parent.motion_id=0;
        bool hitlag=(mode==4||mode==5)&&step<4;unsigned tick=hitlag?step:mode==4||mode==5?step-4:step;
        NativeScriptValue result={0};nativeScriptItem_BeginTick(item);
        if(!hitlag)CHECK(nativeScriptItem_RunPhase(item,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);
        int previous_mode=reference.msid;
        if(flight)CHECK(previous_mode>=1&&previous_mode<=3);
        if(mode==7&&step==0){
            ItemLink* tip=reference.xDD4_itemVar.samusgrapple.x0;CHECK(tip&&tip->next);
            Vec3 delta={tip->pos.x-tip->next->pos.x,tip->pos.y-tip->next->pos.y,tip->pos.z-tip->next->pos.z};
            CHECK(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z>tuning.x38*tuning.x38*1.01f);
        }
        u32 initial_seed=(u32)(0x712341u+step*137u+samus_capture_pairs*19u);*seed_ptr=initial_seed;
        grapple_segment_count=0;grapple_segment_reference=false;
        CHECK(nativeScriptFighter_RunCallback(fighter_object,hitlag?NativeScriptFighterCallback_AccessoryHitlag:NativeScriptFighterCallback_AccessoryBefore));
        unsigned source_segments=grapple_segment_count;
        grapple_segment_count=0;grapple_segment_reference=true;
        u32 source_seed=*seed_ptr;*seed_ptr=initial_seed;
        if(hitlag)it_802BACC4(native_parent);
        else if(flight&&reference.msid==1)fn_802B805C(native_item);
        else if(flight&&reference.msid==2)fn_802B8384(native_item);
        else if(mode==3||(flight&&reference.msid==3))fn_802B8524(native_item);
        else if(mode==6)fn_802B895C(native_item);
        else if(mode==7)fn_802B8B54(native_item);
        else if(mode==8)fn_802B8D38(native_item);
        else if(mode==5)fn_802B8814(native_item);else fn_802B8684(native_item);
        CHECK(grapple_segment_count==source_segments);
        CHECK(*seed_ptr==source_seed&&payload->msid==reference.msid);
        if(mode==5)CHECK(state(item,"completed",NULL).as.u64==(reference.xDD4_itemVar.samusgrapple.x14!=0));
        samus_solver_compare(item,&reference,primary,mode,pattern,step);
        if(mode==7&&step==0){
            ItemLink* tip=reference.xDD4_itemVar.samusgrapple.x0;
            Vec3 delta={tip->pos.x-tip->next->pos.x,tip->pos.y-tip->next->pos.y,tip->pos.z-tip->next->pos.z};
            samus_equal_float(sqrtf(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z),tuning.x38);
            ++samus_pull_tail_clamps;
        }
        if(mode>=6){
            samus_equal_vector(&fighter_data->cur_pos,&parent.cur_pos);
            samus_equal_vector(&fighter_data->self_vel,&parent.self_vel);
            samus_equal_vector(&fighter_data->pos_delta,&parent.pos_delta);
            samus_equal_float(samus_fighter_read("tether_hang_timer").as.f32,parent.mv.ss.grapple.x4);
        }
        if(hitlag)++samus_constraint_hitlag_updates;
        else{
            if(flight||constraint){
                if(flight)++samus_flight_updates;
                else if(mode==7)++samus_pull_updates;
                else ++samus_wall_updates;
                if(reference.msid!=previous_mode){
                    if(flight)++samus_flight_transitions;
                    else {completed_tick=tick+1;if(mode==7)++samus_pull_transitions;else ++samus_wall_transitions;break;}
                }
                if(tick==(mode==7?1u:7u)){completed_tick=tick+1;break;}
            }else{
                if(mode==5)++samus_capture_updates;else ++samus_retraction_updates;
                if(mode==5?reference.xDD4_itemVar.samusgrapple.x14:reference.msid==0){completed_tick=tick+1;break;}
            }
        }
    }
    CHECK(completed_tick);
    if(mode==5){
        CHECK(samus_capture_hook(NativeScriptFighter_CatchPullReady));
        CHECK(fighter_data->mv.co.capturedamage.x18==endpoint);
    }
    if(mode==4){
        for(unsigned tick=0;tick<2;++tick){
            NativeScriptValue result={0};nativeScriptItem_BeginTick(item);
            CHECK(nativeScriptItem_RunPhase(item,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);
            CHECK(nativeScriptFighter_RunCallback(fighter_object,NativeScriptFighterCallback_AccessoryBefore));
            fn_802B7E34(native_item);
            CHECK(payload->msid==0&&state(item,"solver",NULL).as.i64==0);
            samus_solver_compare(item,&reference,primary,0,pattern,tick);
            ++samus_returned_held_updates;
        }
    }
    // Capture completion retires on the next bound update; ordinary retraction
    // returns to held mode, then the authored remove helper retires it.
    NativeMatchPoolReference identity;CHECK(nativeMatchPool_Reference(&nativeGObjPool,item,&identity));
    if(mode==5){nativeScriptItem_BeginTick(item);CHECK(nativeScriptFighter_RunCallback(fighter_object,NativeScriptFighterCallback_AccessoryBefore));}
    else samus_fighter_helper("Tether_Remove");
    CHECK(!nativeMatchPool_Resolve(identity)&&samus_fighter_read("tether_handle").as.i64==-1);
    CHECK(fighter_data->mv.co.capturedamage.x18!=endpoint&&!fighter_data->parts[139].joint);
    if(mode==5)CHECK(samus_capture_hook(NativeScriptFighter_CatchPullReady));
    expected=reference.xDD4_itemVar.samusgrapple.x4;
    while(expected){ItemLink* next=expected->prev;HSD_GObjFree(expected->gobj);expected=next;}
    native_item->user_data=NULL;HSD_GObjFree(native_item);
    native_parent->user_data=NULL;HSD_GObjFree(native_parent);
    samus_flight_geometry=false;
    fighter_data->input.held_buttons[0]=fighter_data->input.pressed_buttons=0;
    if(flight)++samus_flight_pairs;
    else if(mode==7)++samus_pull_pairs;
    else if(mode==3)++samus_slack_pairs;
    else if(constraint)++samus_wall_pairs;
    else if(mode==5){++samus_capture_pairs;++samus_capture_completions;}
    else{++samus_retraction_pairs;++samus_retraction_completions;}
    diagnostic.message[0]=0;
}
