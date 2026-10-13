typedef struct Observation {
    int motion, item_motion, command0, command3, air, result, countdown;
    bool retained, boost_used, effect, emitting, death_bound, damage_bound;
    bool launched, saved_creator, manual_lifetime, stop_on_surface, custom_floor;
    bool source_live, item_live;
    float height, scale, life, half_life, root_rotation, facing;
    Vec3 velocity, child_scale, position;
    int collision_line, collision_facing;
    uint32_t damage, generation, throw_flags;
} Observation;
static Observation expected;
static bool callback_result;
static int read_fi(const char*n) { return read_int(fighter.native_module_state,ff(n)); }
static bool read_fb(const char*n) { return fighter.native_module_state[ff(n)]!=0; }
static float read_fl(const char*n) { return read_float(fighter.native_module_state,ff(n)); }
static bool read_ab(const char*n) { return item.xDD4_itemVar.native_article.state[af(n)]!=0; }
static float read_al(const char*n) { return read_float(item.xDD4_itemVar.native_article.state,af(n)); }
static void initialize(bool authored,unsigned article_domain) {
    scripted=authored;domain=article_domain;active_logic=domain==2?blizzard_logic:ice_logic;
    memset(&source,0,sizeof(source));memset(&foreign,0,sizeof(foreign));memset(&object,0,sizeof(object));
    memset(&fighter,0,sizeof(fighter));memset(&foreign_fighter,0,sizeof(foreign_fighter));memset(&item,0,sizeof(item));
    memset(joints,0,sizeof(joints));memset(&article_root,0,sizeof(article_root));memset(&article_child,0,sizeof(article_child));
    memset(heads,0,sizeof(heads));memset(trace,0,sizeof(trace));trace_count=0;
    source.user_data=&fighter;foreign.user_data=&foreign_fighter;object.user_data=&item;
    source.hsd_obj=&joints[0];object.hsd_obj=&article_root;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&source;source.next=&foreign;heads[HSD_GOBJ_PLINK_ITEM]=&object;
    fighter.kind=authored?Ft_Kind_CustomFirst:(FORM_NANA?Ft_Kind_Nana:Ft_Kind_Popo);
    fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);foreign_fighter.x8_spawnNum=UINT32_C(0x87654321);
    fighter.parts=bones;fighter.dat_attrs=&fighter_attrs;fighter.x5AC.model_num=2;
    fighter.facing_dir=1;fighter.motion_id=341;fighter.self_vel=(Vec3){.7f,-.4f,.125f};
    fighter.throw_flags=UINT32_C(0xA5C3017F);
    fighter.cur_anim_frame=9.5f;fighter.coll_data.floor.normal=(Vec3){-.6f,.8f,0};
    for(unsigned i=0;i<49;++i){bones[i].joint=&joints[i];joints[i].scale=(Vec3){1,1,1};joints[i].translate=(Vec3){(float)i+.25f,5.5f,.75f};}
    article_root.child=&article_child;article_root.scale=(Vec3){1,1,1};article_child.scale=(Vec3){1,1,1};
    item.x1C=UINT32_C(0xDCBA9876);item.kind=domain==2?It_Kind_IceClimber_Blizzard:It_Kind_IceClimber_Ice;
    item.owner=&source;item.xCC_item_attr=&item_attrs;item.xC4_article_data=&article;
    item.facing_dir=1;item.x40_vel=(Vec3){1.25f,-.75f,.125f};item.pos=(Vec3){1,2,0};item.xC70=1.25f;
    item.xD44_lifeTimer=7;item.xD48_halfLifeTimer=3.5f;item.xDC8_word.flags.x15=true;
    item.x378_itemColl.floor.normal=(Vec3){-.6f,.8f,0};item.x378_itemColl.floor.index=3;
    item.x378_itemColl.left_facing_wall.normal=(Vec3){1,0,0};item.x378_itemColl.left_facing_wall.index=11;
    item.x378_itemColl.right_facing_wall.normal=(Vec3){-1,0,0};item.x378_itemColl.right_facing_wall.index=12;
    item.x378_itemColl.ceiling.index=13;item.xC30=-7;
    memset(&item_attrs,0,sizeof(item_attrs));item_attrs.x60_scale=1.25f;item_attrs.x58=.65f;
    item_attrs.x10_fall_speed=-.11f;item_attrs.x14_fall_speed_max=2.5f;item_common.x4C_float=.5f;item_common.x68_float=.2f;
    article.x4_specialAttributes=domain==2?(void*)&blizzard_attrs:(void*)&ice_attrs;
    if(authored) {
        item.native_article_author_kind=Ft_Kind_CustomFirst;item.xB8_itemLogicTable=active_logic->logic;
        item.xDD4_itemVar.native_article.attachment_owner=&source;
        item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;
        fi("ice_handle",-1);
    }
    remaining=true;ground=true;landed=false;allocation_success=true;stage_callback=false;
    invalidation=0;random_value=.25f;callback_result=false;active_definition=definition;
}
static Observation observe(void) {
    Observation o={0};o.motion=fighter.motion_id;o.item_motion=item.msid;o.command0=fighter.cmd_vars[0];o.command3=fighter.cmd_vars[3];
    o.air=fighter.ground_or_air;o.result=callback_result;o.death_bound=fighter.death2_cb!=NULL;o.damage_bound=fighter.take_dmg_cb!=NULL;
    o.source_live=source.user_data==&fighter;o.item_live=object.user_data==&item;o.root_rotation=joints[0].rotate.x;
    o.retained=scripted?read_fi("ice_handle")!=-1:fighter.u.pp.x222C!=NULL;
    o.boost_used=scripted?read_fb("ice_air_boost_used"):fighter.u.pp.x224C!=0;
    o.height=scripted?read_fl("ice_spawn_height"):fighter.u.pp.x2250;
    o.effect=scripted?read_fb("blizzard_effect_active"):fighter.u.pp.x2230_b0;
    o.countdown=scripted?read_fi("blizzard_countdown"):fighter.mv.pp.speciallw.x0;
    o.emitting=scripted?read_fb("blizzard_emitting"):fighter.mv.pp.speciallw.x4_b0;
    o.velocity=domain?item.x40_vel:fighter.self_vel;
    o.position=domain?item.pos:fighter.cur_pos;o.facing=domain?item.facing_dir:fighter.facing_dir;
    o.throw_flags=(uint32_t)fighter.throw_flags;
    if(domain){
        o.launched=scripted?read_ab("launched"):(domain==2?item.xDD4_itemVar.climbersblizzard.flag0:item.xDD4_itemVar.climbersice.x8_b0);
        o.scale=scripted?read_al(domain==2?"initial_scale":"shrink_scale"):(domain==2?item.xDD4_itemVar.climbersblizzard.x0:item.xDD4_itemVar.climbersice.x4);
        if(domain==1)o.saved_creator=scripted?read_int(item.xDD4_itemVar.native_article.state,af("saved_creator"))!=0:item.xDD4_itemVar.climbersice.x0!=NULL;
        o.life=item.xD44_lifeTimer;o.half_life=item.xD48_halfLifeTimer;o.child_scale=article_child.scale;
        o.manual_lifetime=item.xDC8_word.flags.x15;o.stop_on_surface=item.xDCD_flag.b4;o.custom_floor=item.xDCD_flag.b3;
        o.collision_line=item.xC30;o.collision_facing=item.x378_itemColl.facing_dir;o.damage=item.x5D4_hitboxes[0].hit.damage;o.generation=(uint32_t)item.x1C;
    }
    return o;
}
static void save(void) { expected=observe();expected_count=trace_count;memcpy(expected_trace,trace,sizeof(trace)); }
static void compare(void) {
    Observation actual=observe();++pairs;
    if(memcmp(&actual,&expected,sizeof(actual))||trace_count!=expected_count||memcmp(trace,expected_trace,trace_count*sizeof(*trace))) {
        fprintf(stderr,"domain%u mismatch motion %d/%d item %d/%d events %u/%u\n",domain,actual.motion,expected.motion,actual.item_motion,expected.item_motion,trace_count,expected_count);
        const unsigned char*a=(const unsigned char*)&actual,*b=(const unsigned char*)&expected;
        for(unsigned i=0;i<sizeof(actual);++i)if(a[i]!=b[i])fprintf(stderr,"observation byte%u %u/%u\n",i,a[i],b[i]);
        for(unsigned i=0;i<trace_count||i<expected_count;++i)fprintf(stderr,"event%u %d,%d,%d,%g,%g,%g / %d,%d,%d,%g,%g,%g\n",i,trace[i].op,trace[i].a,trace[i].b,trace[i].x,trace[i].y,trace[i].z,expected_trace[i].op,expected_trace[i].a,expected_trace[i].b,expected_trace[i].x,expected_trace[i].y,expected_trace[i].z);
        CHECK(false);
    }
    checks+=sizeof(actual)+trace_count*sizeof(*trace)+1;
}
static void retain_ice(void) {
    if(scripted) { fi("ice_handle",(int32_t)item.x1C);ai("saved_creator",(int32_t)fighter.x8_spawnNum); }
    else { fighter.u.pp.x222C=&object;item.xDD4_itemVar.climbersice.x0=&source; }
    fighter.death2_cb=ftPp_Init_8011F060;fighter.take_dmg_cb=ftPp_Init_8011F060;
}
static bool signal(HSD_GObj*caller,uint32_t handle,uint32_t hash,float value) {
    CHECK(caller==&source);if(object.user_data!=&item||handle!=(uint32_t)item.x1C)return false;
    const NativeFighterArticleLogic*logic=item.kind==It_Kind_IceClimber_Ice?ice_logic:blizzard_logic;
    for(unsigned i=0;i<logic->signal_count;++i)if(logic->signals[i].name_hash==hash){logic->signals[i].callback(&object,value);return true;}
    return false;
}
static void fighter_cases(void) {
    HSD_GObjEvent original_entry[]={ftPp_SpecialN_Enter,ftPp_SpecialAirN_Enter,ftPp_SpecialLw_Enter,ftPp_SpecialAirLw_Enter};
    HSD_GObjEvent generated_entry[]={definition->special_n,definition->special_air_n,definition->special_lw,definition->special_air_lw};
    unsigned state_offsets[]={0,1,16,17};
    HSD_GObjEvent original_phase[4][4]={
        {ftPp_SpecialN_Anim,ftPp_SpecialN_IASA,ftPp_SpecialN_Phys,ftPp_SpecialN_Coll},
        {ftPp_SpecialAirN_Anim,ftPp_SpecialAirN_IASA,ftPp_SpecialAirN_Phys,ftPp_SpecialAirN_Coll},
        {ftPp_SpecialLw_Anim,ftPp_SpecialLw_IASA,ftPp_SpecialLw_Phys,ftPp_SpecialLw_Coll},
        {ftPp_SpecialAirLw_Anim,ftPp_SpecialAirLw_IASA,ftPp_SpecialAirLw_Phys,ftPp_SpecialAirLw_Coll}};
    for(unsigned move=0;move<4;++move)for(unsigned boost=0;boost<2;++boost) {
        initialize(false,0);fighter.u.pp.x224C=boost;original_entry[move](&source);save();
        initialize(true,0);fb("ice_air_boost_used",boost);generated_entry[move](&source);compare();
    }
    for(unsigned move=0;move<4;++move)for(unsigned phase=0;phase<4;++phase)
    for(unsigned finished=0;finished<2;++finished)for(unsigned hit=0;hit<2;++hit)for(unsigned effects=0;effects<2;++effects) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,0);fighter.motion_id=341+state_offsets[move];remaining=!finished;ground=!hit;landed=hit;fighter.cmd_vars[3]=effects;
            if(authored)fb("blizzard_effect_active",effects);else fighter.u.pp.x2230_b0=effects;
            if(!authored)original_phase[move][phase](&source);
            else {
                const MotionState*s=&definition->motion_states[state_offsets[move]];
                HSD_GObjEvent callbacks[]={s->anim_cb,s->input_cb,s->phys_cb,s->coll_cb};CHECK(callbacks[phase]!=NULL);callbacks[phase](&source);
            }
            if(!authored)save();else compare();
        }
    }
    /* Ice cues execute complete constructors, including allocation failure. */
    for(unsigned cue=0;cue<4;++cue)for(unsigned retained=0;retained<2;++retained)
    for(unsigned success=0;success<2;++success)for(unsigned facing=0;facing<2;++facing) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,1);
            if(authored)generated_entry[0](&source);else original_entry[0](&source);
            trace_count=0;fighter.cmd_vars[0]=cue;allocation_success=success;fighter.facing_dir=facing?-1:1;
            if(retained)retain_ice();
            if(!authored)ftPp_SpecialN_8011F500(&source);
            else fighter.accessory4_cb(&source);
            if(!authored)save();else compare();
        }
    }
    const int timers[]={-1,0,1,3};
    for(unsigned cue=0;cue<4;++cue)for(unsigned emitting=0;emitting<2;++emitting)
    for(unsigned timer=0;timer<4;++timer)for(unsigned facing=0;facing<2;++facing) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,2);
            if(authored)generated_entry[2](&source);else original_entry[2](&source);
            trace_count=0;fighter.cmd_vars[0]=cue;fighter.facing_dir=facing?-1:1;
            if(!authored){fighter.mv.pp.speciallw.x0=timers[timer];fighter.mv.pp.speciallw.x4_b0=emitting;fn_80122D2C(&source);}
            else{fi("blizzard_countdown",timers[timer]);fb("blizzard_emitting",emitting);fighter.accessory4_cb(&source);}
            if(!authored)save();else compare();
        }
    }
    /* Retained Ice removal and identity-sensitive destruction link. */
    for(unsigned air=0;air<2;++air)for(unsigned retained=0;retained<2;++retained)for(unsigned hit=0;hit<2;++hit) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,0);fighter.motion_id=341+air;ground=!hit;landed=hit;if(retained)retain_ice();
            if(authored)definition->motion_states[air].coll_cb(&source);else original_phase[air][3](&source);
            if(!authored)save();else compare();
        }
    }
    /* Compare the actual component cleanup bodies, not the unported full
     * Squall/Belay callback to which the original binds them. */
    for(unsigned retained=0;retained<2;++retained)for(unsigned effects=0;effects<2;++effects) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,0);if(retained)retain_ice();
            joints[0].rotate.x=.75f;
            if(authored){fb("blizzard_effect_active",effects);script_fn_cleanup_ice(&source);script_fn_cleanup_blizzard(&source);}
            else{fighter.u.pp.x2230_b0=effects;ftPp_Init_8011F190(&source);ftPp_SpecialHi_80122898(&source);}
            if(!authored)save();else compare();
        }
    }
    for(unsigned retained=0;retained<2;++retained)for(unsigned effects=0;effects<2;++effects)
    for(unsigned boosted=0;boosted<2;++boosted)for(unsigned height=0;height<2;++height) {
        for(unsigned authored=0;authored<2;++authored) {
            initialize(authored,0);if(retained)retain_ice();
            if(authored){fb("blizzard_effect_active",effects);fb("ice_air_boost_used",boosted);fl("ice_spawn_height",height?-10:0);definition->on_death(&source);}
            else{fighter.u.pp.x2230_b0=effects;fighter.u.pp.x224C=boosted;fighter.u.pp.x2250=height?-10:0;ftPp_Init_OnDeath(&source);}
            if(!authored)save();else compare();
        }
    }
    for(unsigned mode=1;mode<=11;mode+=10) {
        initialize(true,0);invalidation=mode;definition->special_n(&source);++safety;
        CHECK(trace_count==1&&trace[0].op==Change);CHECK(fighter.accessory4_cb==NULL);
    }
}
