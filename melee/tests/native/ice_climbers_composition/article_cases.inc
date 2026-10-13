static void article_cases(void) {
    const float rand_values[]={0,.001f,.25f,.5f,.999999f};
    for(unsigned kind=1;kind<=2;++kind)for(unsigned face=0;face<2;++face)
    for(unsigned allocation=0;allocation<2;++allocation)for(unsigned r=0;r<5;++r) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,kind);random_value=rand_values[r];allocation_success=allocation;
            Vec3 p={3.25f,-7.5f,0};float facing=face?-1:1;
            if(!authored){if(kind==1)it_802C1590(&source,&p,It_Kind_IceClimber_Ice,facing);else itClimbersBlizzard_Spawn(&source,&p,facing);}
            else {int32_t id=nativeArticle_Create(&source,item.kind,&p,&(Vec3){0},facing,true,0,0,true);CHECK(id==(allocation?(int32_t)item.x1C:-1));}
            if(!authored)save();else compare();
        }
    }
    HSD_GObjPredicate ice_anim[]={itClimbersice_UnkMotion0_Anim,itClimbersice_UnkMotion1_Anim,itClimbersice_UnkMotion2_Anim,itClimbersice_UnkMotion3_Anim};
    HSD_GObjEvent ice_phys[]={itClimbersice_UnkMotion0_Phys,itClimbersice_UnkMotion1_Phys,itClimbersice_UnkMotion2_Phys,itClimbersice_UnkMotion3_Phys};
    HSD_GObjPredicate ice_coll[]={itClimbersice_UnkMotion0_Coll,itClimbersice_UnkMotion1_Coll,itClimbersice_UnkMotion2_Coll,itClimbersice_UnkMotion3_Coll};
    const float lifetimes[]={-1,0,.5f,1,2,20};
    for(unsigned facing=0;facing<2;++facing)for(unsigned retained=0;retained<2;++retained)
    for(unsigned l=0;l<6;++l) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,1);item.facing_dir=facing?-1:1;item.xD44_lifeTimer=lifetimes[l];
            if(retained)retain_ice();
            if(authored)CHECK(signal(&source,(uint32_t)item.x1C,UINT32_C(4055382118),0));
            else it_802C16F8(&object);
            if(!authored)save();else compare();
        }
    }
    const float scales[]={.001f,.01f,.011f,.5f,1.5f};
    for(unsigned kind=1;kind<=2;++kind)for(unsigned state=0;state<(kind==1?4:1);++state)
    for(unsigned launched=0;launched<2;++launched)for(unsigned l=0;l<6;++l)for(unsigned scale=0;scale<5;++scale) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,kind);item.msid=state;item.xD44_lifeTimer=lifetimes[l];
            if(authored){ab("launched",launched);al(kind==1?"shrink_scale":"initial_scale",scales[scale]);callback_result=active_logic->logic->states[state].animated(&object);}
            else {if(kind==1){item.xDD4_itemVar.climbersice.x8_b0=launched;item.xDD4_itemVar.climbersice.x4=scales[scale];callback_result=ice_anim[state](&object);}
              else{item.xDD4_itemVar.climbersblizzard.flag0=launched;item.xDD4_itemVar.climbersblizzard.x0=scales[scale];callback_result=itClimbersBlizzard_UnkMotion0_Anim(&object);}}
            if(!authored)save();else compare();
        }
    }
    const float velocities[]={-8,-1,-.001f,0,.001f,1,8};
    const float normals[]={-.8f,0,.8f};
    for(unsigned kind=1;kind<=2;++kind)for(unsigned state=0;state<(kind==1?4:1);++state)
    for(unsigned v=0;v<7;++v)for(unsigned floor=0;floor<2;++floor)for(unsigned n=0;n<3;++n) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,kind);item.msid=state;item.x40_vel.x=velocities[v];item.x378_itemColl.env_flags=floor?Collide_FloorPush:0;item.x378_itemColl.floor.normal.x=normals[n];
            if(authored)active_logic->logic->states[state].physics_updated(&object);
            else if(kind==1)ice_phys[state](&object);else itClimbersBlizzard_UnkMotion0_Phys(&object);
            if(!authored)save();else compare();
        }
    }
    for(unsigned kind=1;kind<=2;++kind)for(unsigned state=0;state<(kind==1?4:1);++state)
    for(unsigned v=0;v<7;++v)for(unsigned masks=0;masks<16;++masks)for(unsigned transition=0;transition<2;++transition) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,kind);item.msid=state;item.x40_vel.x=velocities[v];stage_callback=transition;
            item.x378_itemColl.env_flags=(masks&1?Collide_LeftWallPush:0)|(masks&2?Collide_RightWallPush:0)|(masks&4?Collide_CeilingPush:0)|(masks&8?Collide_FloorPush:0);
            if(authored)callback_result=active_logic->logic->states[state].collided(&object);
            else if(kind==1)callback_result=ice_coll[state](&object);else callback_result=itClimbersBlizzard_UnkMotion0_Coll(&object);
            if(!authored)save();else compare();
        }
    }
    HSD_GObjPredicate original_callbacks[2][6]={
        {itClimbersIce_Logic90_DmgDealt,itClimbersIce_Logic90_Reflected,itClimbersIce_Logic90_Clanked,it_2725_Logic90_HitShield,itClimbersIce_Logic90_Absorbed,itClimbersIce_Logic90_ShieldBounced},
        {itClimbersBlizzard_DmgDealt,itClimbersBlizzard_Reflected,itClimbersBlizzard_Clanked,itClimbersBlizzard_HitShield,itClimbersBlizzard_Absorbed,itClimbersBlizzard_ShieldBounced}};
    for(unsigned kind=1;kind<=2;++kind)for(unsigned callback=0;callback<6;++callback)for(unsigned v=0;v<7;++v)for(unsigned l=0;l<6;++l) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,kind);item.x40_vel.x=velocities[v];item.xD44_lifeTimer=lifetimes[l];
            if(!authored)callback_result=original_callbacks[kind-1][callback](&object);
            else {ItemLogicTable*t=active_logic->logic;HSD_GObjPredicate cbs[]={t->dmg_dealt,t->reflected,t->clanked,t->hit_shield,t->absorbed,t->shield_bounced};callback_result=cbs[callback](&object);}
            if(!authored)save();else compare();
        }
    }
    for(unsigned retained=0;retained<2;++retained)for(unsigned matching=0;matching<2;++matching)for(unsigned changed_owner=0;changed_owner<2;++changed_owner) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,1);if(retained)retain_ice();if(!matching){if(authored)fi("ice_handle",123);else fighter.u.pp.x222C=&foreign;}
            if(changed_owner)item.owner=&foreign;
            if(authored)active_logic->logic->destroyed(&object);else it_2725_Logic90_Destroyed(&object);
            if(!authored)save();else compare();
        }
    }
    for(unsigned kind=1;kind<=2;++kind)for(unsigned retained=0;retained<2;++retained)for(unsigned recipient=0;recipient<3;++recipient) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,kind);if(kind==1&&retained)retain_ice();Item_GObj*other=recipient==0?&source:recipient==1?&foreign:NULL;
            if(authored)active_logic->logic->evt_unk(&object,other);else if(kind==1)itClimbersIce_Logic90_EvtUnk(&object,other);else itClimbersBlizzard_EvtUnk(&object,other);
            if(!authored)save();else compare();
        }
    }
    for(unsigned kind=1;kind<=2;++kind)for(unsigned mode=2;mode<=12;mode+=10) {
        initialize(true,kind);invalidation=mode;active_logic->on_created(&object);++safety;
        CHECK(trace_count>=1);CHECK(trace[trace_count-1].op==ItemChange);
        for(unsigned i=0;i<trace_count;++i)CHECK(trace[i].op!=Debug&&trace[i].op!=Refresh);
    }
    /* The original saved-pointer path is undefined after creator retirement;
     * these are separate authored safety contracts with the real registry. */
    for(unsigned mode=0;mode<3;++mode) {
        initialize(true,1);retain_ice();
        if(mode==0){source.user_data=NULL;heads[HSD_GOBJ_PLINK_FIGHTER]=&foreign;}
        else if(mode==1)fighter.x8_spawnNum++;
        else{source.user_data=NULL;heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;}
        active_logic->logic->destroyed(&object);++safety;
        CHECK(read_fi("ice_handle")==(int32_t)item.x1C);
        CHECK(read_int(item.xDD4_itemVar.native_article.state,af("saved_creator"))==0);
        CHECK(trace_count==0);
    }
}
