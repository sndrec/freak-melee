/* Constructor + scheduler healthy path, allocation safety and lifecycle. */
static void construction_reset(bool generated,unsigned bits)
{
    nativeArticleResource_Clear();
    for(unsigned i=0;i<40;++i){CHECK(!node_live[i]&&!link_live[i]&&!model_live[i]);}
    initialize(generated,bits&1?352:347,361);fighters[0].ground_or_air=(bits&1)!=0;
    fighters[0].facing_dir=bits&2?-1:1;fighters[0].cmd_vars[2]=0;
    fighters[0].mv.pp.unk_80123954.x0=7;
    if(generated)set_int(0,"rope_counter",7);
    memset(&rope_object,0,sizeof rope_object);memset(&rope_item,0,sizeof rope_item);
    memset(&rope_root,0,sizeof rope_root);memset(&rope_article,0,sizeof rope_article);
    memset(&rope_attributes,0,sizeof rope_attributes);
#include "rope_attributes.inc"
    rope_attributes.x24_joint=&segment_descriptor;rope_attributes.x28_joint=&tail_descriptor;
    rope_article.x4_specialAttributes=&rope_attributes;
    rope_article.native_model_root_count=2;
    rope_article.native_model_roots[0].name_hash=hash_name("Segment");rope_article.native_model_roots[0].root=&segment_descriptor;
    rope_article.native_model_roots[1].name_hash=hash_name("Tail");rope_article.native_model_roots[1].root=&tail_descriptor;
    stock_rope_logic=(ItemLogicTable){.picked_up=it_2725_Logic70_PickedUp};
    node_count=link_count=model_count=resource_frees=0;fail_node=fail_link=fail_model=-1;
    allocation_failure=false;memset(&last_spawn,0,sizeof last_spawn);
    Vec3 p={bits&4?-3.625f:1.125f,bits&8?7.875f:-2.25f,bits&16?.75f:-.125f};
    joints[0][29].translate=p;C_MTXIdentity(joints[0][29].mtx);
    joints[0][29].mtx[0][3]=p.x;joints[0][29].mtx[1][3]=p.y;joints[0][29].mtx[2][3]=p.z;
}
static void retire(void)
{
    if(scripted)nativeArticleResource_Clear();
    else for(unsigned i=node_count;i>0;--i)if(node_live[i-1])HSD_GObjFree(&rope_nodes[i-1]);
    heads[HSD_GOBJ_PLINK_ITEM]=NULL;
}
static void compare_spawn(void)
{
    CHECK(last_spawn.kind==expected_spawn.kind);CHECK(last_spawn.x0_parent_gobj==expected_spawn.x0_parent_gobj);
    CHECK(last_spawn.x4_parent_gobj2==expected_spawn.x4_parent_gobj2);CHECK(last_spawn.x40==expected_spawn.x40);
    CHECK(last_spawn.x3C_damage==expected_spawn.x3C_damage);CHECK(last_spawn.x44_flag.b0==expected_spawn.x44_flag.b0);
    for(unsigned axis=0;axis<3;++axis){same_float((&last_spawn.pos.x)[axis],(&expected_spawn.pos.x)[axis]);same_float((&last_spawn.prev_pos.x)[axis],(&expected_spawn.prev_pos.x)[axis]);same_float((&last_spawn.vel.x)[axis],(&expected_spawn.vel.x)[axis]);}
    same_float(last_spawn.facing_dir,expected_spawn.facing_dir);
}
static void compare_links(void)
{
    CHECK(node_count==40&&link_count==40&&model_count==40);
    for(unsigned i=0;i<40;++i){
        CHECK(node_live[i]&&link_live[i]&&model_live[i]);
        CHECK(rope_links[i].jobj==&joints[0][29]);CHECK(rope_links[i].gobj==&rope_nodes[i]);
        CHECK(rope_links[i].next==(i?&rope_links[i-1]:NULL));CHECK(rope_links[i].prev==(i==39?NULL:&rope_links[i+1]));
        for(unsigned axis=0;axis<3;++axis){same_float((&rope_links[i].pos.x)[axis],(&expected_links[i].pos.x)[axis]);same_float((&rope_links[i].vel.x)[axis],(&expected_links[i].vel.x)[axis]);}
        CHECK(rope_links[i].x2C_b0==expected_links[i].x2C_b0);
    }
}
static void lifecycle_cases(void);
static void accessory_cases(void);
static void interaction_cases(void);
static void retirement_cases(void);
static void full_cleanup_cases(void);
static void constructor_cases(NativeFighterHost*host)
{
    for(unsigned i=0;i<definitions[0]->article_logic_count;++i)if(definitions[0]->article_logics[i].kind==It_Kind_IceClimber_GumStrings)owned_rope=&definitions[0]->article_logics[i];
    CHECK(owned_rope&&owned_rope->resource_capacity==40);
    for(unsigned i=0;i<definitions[0]->article_logic_count;++i)if(definitions[0]->article_logics[i].kind==It_Kind_IceClimber_Ice)owned_ice=&definitions[0]->article_logics[i];
    CHECK(owned_ice);host->destroy_effects=efLib_DestroyAll;
    host->signal_article=signal_article;host->item_interact=it_8026B894;host->article_ref_exists=nativeArticleRef_Exists;host->create_article_at=nativeArticle_Create;host->bind_fighter_item=bind_auxiliary;host->clear_fighter_item=clear_auxiliary;
    host->item_creator=nativeArticle_Creator;host->item_current_owner=current_owner;host->item_remove=Item_8026A8EC;
    host->item_attach_to_owner=Item_8026AB54;host->item_change_state_flags=Item_80268E5C;host->item_set_joint_value=joint_value;
    host->article_resource_begin=nativeArticleResource_Begin;host->article_resource_create=nativeArticleResource_CreateNode;
    host->article_resource_load_model=nativeArticleResource_LoadModel;host->article_resource_set_draw=nativeArticleResource_SetDraw;
    host->article_resource_release=nativeArticleResource_Release;
    host->article_resource_read_float=nativeArticleResource_ReadFloat;host->article_resource_write_float=nativeArticleResource_WriteFloat;
    host->article_resource_read_attachment_point=nativeArticleResource_ReadAttachmentPoint;
    host->article_resource_read_attachment_matrix=nativeArticleResource_ReadAttachmentMatrix;
    host->article_resource_set_world_matrix=nativeArticleResource_SetWorldMatrix;host->article_resource_write_joint=nativeArticleResource_WriteJoint;
    for(unsigned bits=0;bits<32;++bits)for(unsigned failed=0;failed<2;++failed){
        for(unsigned mode=0;mode<2;++mode){construction_reset(mode,bits);allocation_failure=failed;
            if(mode)phase(0,bits&1?352:347,0)(&objects[0]);
            else (bits&1?ftPp_SpecialAirHiStart_0_Anim:ftPp_SpecialHiStart_0_Anim)(&objects[0]);
            if(mode){compare();compare_spawn();
                CHECK((fighters[0].x1984_heldItemSpec!=NULL)==(expected[0].x1984_heldItemSpec!=NULL));
                CHECK(get_int(0,"linked_rope_handle")== (failed?-1:(int32_t)UINT32_C(0xe123abcd)));
                CHECK((fighters[0].death3_cb!=NULL)==!failed);CHECK((fighters[0].take_dmg_cb!=NULL)==!failed);
                if(!failed){compare_links();CHECK(rope_item.on_accessory);CHECK(rope_root.flags&JOBJ_HIDDEN);}
            }else{save();expected_spawn=last_spawn;if(!failed)memcpy(expected_links,rope_links,sizeof rope_links);}
            retire();
        }
    }
    lifecycle_cases();accessory_cases();interaction_cases();retirement_cases();full_cleanup_cases();
    /* Original resource-prefix failure frees through stale/uninitialized data.
     * Check owned atomic cleanup independently, never execute that undefined path. */
    for(unsigned kind=0;kind<3;++kind)for(unsigned index=0;index<40;++index){
        construction_reset(true,0);if(kind==0)fail_node=index;else if(kind==1)fail_link=index;else fail_model=index;
        phase(0,347,0)(&objects[0]);++safety;
        CHECK(heads[HSD_GOBJ_PLINK_ITEM]==NULL);CHECK(nativeArticleResource_Count(&rope_object)==0);
        CHECK(get_int(0,"linked_rope_handle")==-1);CHECK(fighters[0].motion_id==14);CHECK(fighters[0].x1984_heldItemSpec==NULL);
        CHECK(fighters[0].death3_cb==NULL&&fighters[0].take_dmg_cb==NULL);
        for(unsigned i=0;i<40;++i)CHECK(!node_live[i]&&!link_live[i]&&!model_live[i]);
        retire();
    }
}

static bool article_state(void)
{
    return owned_rope->logic->states[rope_item.msid].animated(&rope_object);
}
static unsigned article_field(const char*name)
{
    for(unsigned i=0;i<owned_rope->state_field_count;++i)if(!strcmp(owned_rope->state_fields[i].name,name))return owned_rope->state_fields[i].offset;
    CHECK(false);return 0;
}
static void lifecycle_cases(void)
{
    /* Full original schedule body dispatches exact if/elif commands then83 removal.
     * The candidate uses the actual bounded package signal dispatcher. */
    for(unsigned bits=0;bits<8;++bits)for(unsigned command=9;command<=83;++command){
        Item old_item={0};u32 old_flags=0;bool old_live=false;
        for(unsigned mode=0;mode<2;++mode){construction_reset(mode,bits);
            if(mode)phase(0,bits&1?352:347,0)(&objects[0]);else (bits&1?ftPp_SpecialAirHiStart_0_Anim:ftPp_SpecialHiStart_0_Anim)(&objects[0]);
            fighters[0].motion_id=349;trace_count=0;memset(trace,0,sizeof trace);
            if(mode)set_int(0,"rope_counter",command-1);else fighters[0].mv.pp.unk_80123954.x0=command-1;
            if(mode)phase(0,349,0)(&objects[0]);else ftPp_SpecialHiThrow2_Anim(&objects[0]);
            if(mode){compare();CHECK((heads[HSD_GOBJ_PLINK_ITEM]!=NULL)==old_live);
                CHECK(get_int(0,"rope_counter")==expected[0].mv.pp.unk_80123954.x0);
                CHECK((get_int(0,"linked_rope_handle")!=-1)==(expected[0].u.pp.x2238!=NULL));
                if(old_live){CHECK(rope_item.msid==old_item.msid);CHECK(rope_root.flags==old_flags);
                    for(unsigned i=0;i<40;++i){
                        for(unsigned axis=0;axis<3;++axis){same_float((&rope_links[i].pos.x)[axis],(&expected_links[i].pos.x)[axis]);
                            if(i==39&&command==(unsigned)rope_attributes.x18){same_float((&rope_links[i].coll_data.cur_pos.x)[axis],(&expected_links[i].coll_data.cur_pos.x)[axis]);same_float((&rope_links[i].coll_data.last_pos.x)[axis],(&expected_links[i].coll_data.last_pos.x)[axis]);}}
                        CHECK((rope_item.xDD4_itemVar.native_article.state[article_field("active")+i]!=0)==expected_links[i].x2C_b0);
                    }
                }
            }else{save();old_live=heads[HSD_GOBJ_PLINK_ITEM]!=NULL;old_item=rope_item;memcpy(expected_links,rope_links,sizeof rope_links);old_flags=rope_root.flags;}
            retire();
        }
    }
    for(unsigned owner=0;owner<2;++owner)for(unsigned motion=340;motion<362;++motion){
        bool expected_result=false;
        for(unsigned mode=0;mode<2;++mode){construction_reset(mode,0);
            if(mode)phase(0,347,0)(&objects[0]);else ftPp_SpecialHiStart_0_Anim(&objects[0]);
            if(owner==0)rope_item.owner=NULL;else rope_item.owner=&objects[0];
            fighters[0].motion_id=motion;trace_count=0;memset(trace,0,sizeof trace);
            bool result=mode?article_state():itClimbersstring_UnkMotion3_Anim(&rope_object);
            if(mode){CHECK(result==expected_result);compare();CHECK(resource_frees==(result?40:0));CHECK((rope_item.owner!=NULL)==!result);}
            else{expected_result=result;save();}
            retire();
        }
    }
}

static void accessory_cases(void)
{
    for(unsigned state=0;state<4;++state)for(unsigned bits=0;bits<32;++bits)for(unsigned tick=0;tick<6;++tick){
        Item expected_item;HSD_JObj expected_models[40];u32 expected_flags=0;
        for(unsigned mode=0;mode<2;++mode){construction_reset(mode,bits);
            if(mode)phase(0,bits&1?352:347,0)(&objects[0]);else (bits&1?ftPp_SpecialAirHiStart_0_Anim:ftPp_SpecialHiStart_0_Anim)(&objects[0]);
            Vec3 endpoint={bits&4?15.5f:0,bits&8?-3.875f:0,bits&16?.625f:0};
            fighters[0].u.pp.x2240=endpoint;
            if(mode){set_float(0,"linked_rope_endpoint_x",endpoint.x);set_float(0,"linked_rope_endpoint_y",endpoint.y);set_float(0,"linked_rope_endpoint_z",endpoint.z);}
            if(state){unsigned command=state==1?rope_attributes.x18:state==2?rope_attributes.x1C:rope_attributes.x20;
                if(mode)CHECK(signal_article(&objects[0],rope_item.x1C,hash_name("schedule"),(float)command));
                else if(state==1)it_802C3950(&rope_object);else if(state==2)it_802C3810(&rope_object);else it_802C3864(&rope_object);
            }
            trace_count=0;memset(trace,0,sizeof trace);
            for(unsigned iteration=0;iteration<=tick;++iteration){CHECK(rope_item.on_accessory);rope_item.on_accessory(&rope_object);}
            if(mode){compare();CHECK(rope_item.msid==expected_item.msid);CHECK(rope_root.flags==expected_flags);
                for(unsigned i=0;i<40;++i){
                    CHECK((rope_item.xDD4_itemVar.native_article.state[article_field("active")+i]!=0)==expected_links[i].x2C_b0);
                    for(unsigned axis=0;axis<3;++axis){same_float((&rope_links[i].pos.x)[axis],(&expected_links[i].pos.x)[axis]);same_float((&rope_links[i].vel.x)[axis],(&expected_links[i].vel.x)[axis]);same_float((&rope_models[i].translate.x)[axis],(&expected_models[i].translate.x)[axis]);
                        for(unsigned column=0;column<4;++column)same_float(rope_models[i].mtx[axis][column],expected_models[i].mtx[axis][column]);}
                    CHECK(rope_models[i].flags==expected_models[i].flags);
                }
            }else{save();expected_item=rope_item;expected_flags=rope_root.flags;memcpy(expected_links,rope_links,sizeof rope_links);memcpy(expected_models,rope_models,sizeof rope_models);}
            retire();
        }
    }
}

static void interaction_cases(void)
{
    for(unsigned other=0;other<2;++other)for(unsigned state=0;state<4;++state){
        bool retained=false;
        for(unsigned mode=0;mode<2;++mode){construction_reset(mode,0);
            if(mode)phase(0,347,0)(&objects[0]);else ftPp_SpecialHiStart_0_Anim(&objects[0]);
            rope_item.msid=state;trace_count=0;memset(trace,0,sizeof trace);
            if(mode){owned_rope->logic->evt_unk(&rope_object,&objects[other]);compare();
                CHECK((nativeArticle_Creator(&rope_object)!=NULL)==retained);CHECK(rope_item.owner==&objects[0]);}
            else{it_2725_Logic70_EvtUnk(&rope_object,&objects[other]);retained=rope_item.xDD4_itemVar.climbersstring.xC!=NULL;save();}
            retire();
        }
    }
}
static void retirement_cases(void)
{
    const unsigned modes[]={4,14,5,15};
    for(unsigned test=0;test<4;++test){construction_reset(true,0);invalidation=modes[test];
        phase(0,347,0)(&objects[0]);++safety;
        CHECK(nativeArticleResource_Count(&rope_object)==0);CHECK(resource_frees==40);
        for(unsigned i=0;i<40;++i)CHECK(!node_live[i]&&!link_live[i]&&!model_live[i]);
        CHECK(fighters[0].death3_cb==NULL&&fighters[0].take_dmg_cb==NULL);
        CHECK(get_int(0,"linked_rope_handle")==-1);
        if(test<2){CHECK(fighters[0].motion_id==14);CHECK(fighters[0].x1984_heldItemSpec==NULL);}
        else CHECK(fighters[0].motion_id==347);
        retire();
    }
}

/* Compose the accepted Ice/Blizzard cleanup with actual rope teardown and
 * stamped hitlag release. Controlled Item removal invokes actual destroyed
 * callbacks; no complete Item removal or effect allocator claim is made. */
static void full_cleanup_cases(void)
{
    for(unsigned bits=0;bits<16;++bits){
        for(unsigned mode=0;mode<2;++mode){construction_reset(mode,0);
            if(mode)phase(0,347,0)(&objects[0]);else ftPp_SpecialHiStart_0_Anim(&objects[0]);
            memset(&ice_item,0,sizeof ice_item);memset(&ice_object,0,sizeof ice_object);
            ice_object.user_data=&ice_item;ice_object.classifier=HSD_GOBJ_CLASS_ITEM;
            ice_item.entity=&ice_object;ice_item.x1C=UINT32_C(0xfedc1234);ice_item.kind=It_Kind_IceClimber_Ice;ice_item.owner=&objects[0];
            if(bits&1){rope_object.next=&ice_object;
                if(mode){ice_item.native_article_author_kind=Ft_Kind_CustomFirst;ice_item.xB8_itemLogicTable=owned_ice->logic;
                    ice_item.xDD4_itemVar.native_article.creator_spawn_id=fighters[0].x8_spawnNum;
                    ice_item.xDD4_itemVar.native_article.attachment_owner=&objects[0];
                    for(unsigned i=0;i<owned_ice->state_field_count;++i)if(!strcmp(owned_ice->state_fields[i].name,"saved_creator")){
                        uint32_t id=fighters[0].x8_spawnNum;memcpy(ice_item.xDD4_itemVar.native_article.state+owned_ice->state_fields[i].offset,&id,4);}
                    set_int(0,"ice_handle",(int32_t)ice_item.x1C);
                }else{fighters[0].u.nn.x222C=&ice_object;ice_item.xDD4_itemVar.climbersice.x0=&objects[0];}
            }
            if(mode){bool active=(bits&2)!=0;memcpy(fighters[0].native_module_state+field(0,"blizzard_effect_active"),&active,1);}
            else fighters[0].u.pp.x2230_b0=(bits&2)!=0;
            fighters[0].x2219_b7=(bits&4)!=0;fighters[1].x2219_b7=(bits&4)!=0;
            if(bits&8){if(mode)CHECK(nativeFighterRelations_Link(&objects[0],fighters[1].x8_spawnNum));else{fighters[0].x1A5C=&objects[1];fighters[1].x1A5C=&objects[0];}}
            trace_count=0;memset(trace,0,sizeof trace);
            if(mode){CHECK(fighters[0].take_dmg_cb);fighters[0].take_dmg_cb(&objects[0]);compare();
                CHECK(get_int(0,"ice_handle")==-1);CHECK(get_int(0,"linked_rope_handle")==-1);
                CHECK(nativeArticleResource_Count(&rope_object)==0);
            }else{ftPp_Init_8011F060(&objects[0]);save();}
            CHECK(heads[HSD_GOBJ_PLINK_ITEM]==NULL);CHECK(resource_frees==40);
            CHECK(!fighters[0].x1A5C&&!fighters[1].x1A5C);CHECK(fighters[0].x221A_b1==((bits&4)!=0));
            CHECK(fighters[1].x221A_b1==((bits&4)!=0&&(bits&8)!=0));
            for(unsigned i=0;i<40;++i)CHECK(!node_live[i]&&!link_live[i]&&!model_live[i]);
            retire();
        }
    }
}
