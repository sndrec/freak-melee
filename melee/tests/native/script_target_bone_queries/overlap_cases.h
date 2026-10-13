// Connected capsule geometry: unchanged native Samus loop is the reference,
// while production targets.overlaps_hitbox has no Samus dependencies.
static bool overlap_call(HSD_GObj* caller,uint32_t id,unsigned slot,NativeScriptValue* result)
{
    NativeScriptRuntimeContext c=context(caller);
    NativeScriptValue args[2]={{.type=NativeScriptType_U32,.as.u64=id},integer(slot)};
    return nativeScriptBindings_Invoke(&c,NativeScriptBinding_TargetsOverlapsHitbox,NULL,0,args,2,result,&diagnostic);
}
static void overlap_cases(HSD_GObj* fighter_caller,HSD_GObj* article_caller,HSD_GObj* owned,HSD_GObj* stock)
{
    Item* ip=article_caller->user_data;
    HitCapsule old_hits[4];for(unsigned i=0;i<4;++i)old_hits[i]=ip->x5D4_hitboxes[i].hit;
    float old_scale=ip->scl;
    for(unsigned role=0;role<2;++role) {
        HSD_GObj* target=role?owned:stock;Fighter* fp=target->user_data;
        Vec3 old_fscale=fp->x34_scale,old_pos=fp->cur_pos;Mtx old_mtx;C_MTXCopy(fp->x44_mtx,old_mtx);
        FighterHurtCapsule old_capsules[15];memcpy(old_capsules,fp->hurt_capsules,sizeof(old_capsules));u8 old_len=fp->hurt_capsules_len;
        for(unsigned n=0;n<128;++n) {
            unsigned slot=n%4;ip->scl=.5f+(n%5)*.5f;
            fp->x34_scale=(Vec3){1,.5f+(n%3),n&1?2.0f:1.0f};fp->cur_pos.z=(int)(n%7)-3;
            C_MTXScale(fp->x44_mtx,1.0f,.75f,1.25f);
            fp->hurt_capsules_len=n%16;
            memset(fp->hurt_capsules,0,sizeof(fp->hurt_capsules));
            for(unsigned h=0;h<fp->hurt_capsules_len;++h) {
                HurtCapsule* c=&fp->hurt_capsules[h].capsule;
                c->bone=fp->parts[(h+n)%4].joint;c->scale=.5f+h;
                c->a_offset=(Vec3){.25f*(int)(n%5),-.5f*h,.75f};c->b_offset=(Vec3){1+h,2+h,-.25f};
                c->skip_update_pos=(n>>2)&1;
                c->a_pos=(Vec3){1+h,-2+h,3};c->b_pos=(Vec3){3+h,1+h,2};
            }
            HitCapsule* hit=&ip->x5D4_hitboxes[slot].hit;
            *hit=(HitCapsule){.scale=.25f+(n%6),.x43_b1=(n>>3)&1,
                .x58={(int)(n%13)-6,(int)(n%11)-5,(int)(n%9)-4},
                .x4C={(int)(n%7)-3,(int)(n%5)-2,(int)(n%3)-1}};
            // Disabled capsules still answer a geometry question, as in the
            // native bomb callback. Neither status nor immunity filters belong here.
            hit->state=n&1?HitCapsule_Disabled:HitCapsule_Enabled;
            HitCapsule initial_hit=*hit;FighterHurtCapsule initial_capsules[15];
            memcpy(initial_capsules,fp->hurt_capsules,sizeof(initial_capsules));
            for(unsigned j=0;j<4;++j)fp->parts[j].joint->flags|=JOBJ_MTX_DIRTY;
            bool expected=ftSs_Init_80128A1C(target,hit,ip->scl);
            HitCapsule expected_hit=*hit;FighterHurtCapsule expected_capsules[15];
            memcpy(expected_capsules,fp->hurt_capsules,sizeof(expected_capsules));
            for(unsigned mode=0;mode<2;++mode) {
                *hit=initial_hit;memcpy(fp->hurt_capsules,initial_capsules,sizeof(initial_capsules));
                for(unsigned j=0;j<4;++j)fp->parts[j].joint->flags|=JOBJ_MTX_DIRTY;
                if(!mode) {
                    NativeScriptValue result;CHECK(overlap_call(article_caller,fp->x8_spawnNum,slot,&result));
                    CHECK(result.type==NativeScriptType_Bool&&result.as.u64==expected);
                } else {
                    NativeScriptValue id=integer((int32_t)fp->x8_spawnNum),part=integer(slot);
                    field(article_caller,"handle",&id);field(article_caller,"part",&part);
                    geometry_run(article_caller,"QueryOverlap");
                    CHECK(field(article_caller,"found",NULL).as.u64==expected);
                }
                CHECK(!memcmp(hit,&expected_hit,sizeof(*hit)));
                CHECK(!memcmp(fp->hurt_capsules,expected_capsules,sizeof(expected_capsules)));
                ++comparisons;++cases;
            }
        }
        fp->x34_scale=old_fscale;fp->cur_pos=old_pos;C_MTXCopy(old_mtx,fp->x44_mtx);
        memcpy(fp->hurt_capsules,old_capsules,sizeof(old_capsules));fp->hurt_capsules_len=old_len;
    }
    NativeScriptValue result;CHECK(overlap_call(article_caller,0,0,&result)&&!result.as.u64);
    CHECK(overlap_call(article_caller,((Fighter*)owned->user_data)->x8_spawnNum,4,&result)&&!result.as.u64);
    CHECK(!overlap_call(fighter_caller,((Fighter*)owned->user_data)->x8_spawnNum,0,&result));++negative_count;
    Fighter* fp=owned->user_data;u8 old_len=fp->hurt_capsules_len;
    fp->hurt_capsules_len=16;CHECK(!overlap_call(article_caller,fp->x8_spawnNum,0,&result));++negative_count;
    fp->hurt_capsules_len=1;HurtCapsule original=fp->hurt_capsules[0].capsule;
    fp->hurt_capsules[0].capsule.bone=&replacement_joint;
    CHECK(!overlap_call(article_caller,fp->x8_spawnNum,0,&result));++negative_count;
    fp->hurt_capsules[0].capsule=original;fp->hurt_capsules_len=old_len;
    // Retained control ownership must survive a different damage owner and
    // disappear explicitly when control is cleared.
    CHECK(nativeScriptItem_SetAttachmentOwner(article_caller,owned));ip->owner=stock;
    CHECK(nativeScriptItem_AttachmentOwner(article_caller)==owned);
    CHECK(nativeScriptItem_ClearAttachmentOwner(article_caller));
    CHECK(nativeScriptItem_AttachmentOwner(article_caller)==NULL);ip->owner=NULL;
    ip->scl=old_scale;for(unsigned i=0;i<4;++i)ip->x5D4_hitboxes[i].hit=old_hits[i];
}
