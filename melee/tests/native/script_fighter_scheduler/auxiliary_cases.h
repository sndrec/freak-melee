static void auxiliary_item_cases(HSD_GObj* object, HSD_GObj* held)
{
    Fighter* fighter=object->user_data;
    struct ftData_x8 model={0};data.x8=&model;
    unsigned stock=fighter_stock_calls;
    // The stock gate is OR: skip only when both auxiliary slots are occupied.
    for(unsigned occupied=0;occupied<4;++occupied) {
        fighter->x197C=(occupied&1)?held:NULL;
        fighter->x1980=(occupied&2)?held:NULL;
        reset_trace(object);ftCommon_8007F8E8(object);
        CHECK(state(object,"trace",NULL).as.i64==(occupied==3?1:13));
        reset_trace(object);ftCommon_8007F948(object,fighter->x197C,42);
        CHECK(state(object,"trace",NULL).as.i64==(occupied==3?1:12));
        if(occupied!=3) {
            CHECK(scheduler_aux_seen_first==fighter->x197C);
            CHECK(scheduler_aux_seen_second==fighter->x1980&&scheduler_aux_seen_timer==42);
        }
    }
    CHECK(fighter_stock_calls==stock);
    fighter->x197C=NULL;fighter->x1980=NULL;reset_trace(object);
    ftCommon_8007FE84(object,held,17,3.5f);
    CHECK(state(object,"trace",NULL).as.i64==12&&scheduler_aux_seen_second==held);
    CHECK(fighter->x2018==17&&fighter->x2024==3);
    fighter->x197C=NULL;fighter->x1980=NULL;reset_trace(object);scheduler_aux_tail=0;
    ftCommon_8007FA58(object,held);
    CHECK(state(object,"trace",NULL).as.i64==124&&scheduler_aux_tail==5);
    CHECK(scheduler_aux_seen_first==held&&scheduler_aux_seen_timer==27);
    fighter->x197C=NULL;fighter->x1980=NULL;reset_trace(object);
    scheduler_aux_spawn=held;scheduler_aux_tail=0;ftCommon_8007FC7C(object,2.0f);
    CHECK(state(object,"trace",NULL).as.i64==12&&scheduler_aux_tail==2);
    // A callback fault or actor retirement must not reach the native tail.
    for(unsigned route=0;route<3;++route)for(unsigned retire=0;retire<2;++retire) {
        HSD_GObj* target=create();NativeMatchPoolReference reference;
        CHECK(nativeMatchPool_Reference(&nativeGObjPool,target,&reference));
        scheduler_aux_tail=0;scheduler_remove=retire;scheduler_fail_write=!retire;
        if(route==2)CHECK(!ftCo_Attack11_EnterMotion(target,ftCo_MS_Attack11));
        else if(route)ftCommon_8007FC7C(target,2.0f);else ftCommon_8007FA58(target,held);
        scheduler_fail_write=false;
        CHECK(scheduler_aux_tail==0);
        if(retire)CHECK(!nativeMatchPool_Resolve(reference));
        else { CHECK(nativeScriptFighter_Status(target)==NativeScriptVM_Error);HSD_GObjFree(target); }
    }
    reset_trace(object);scheduler_aux_tail=0;
    CHECK(ftCo_Attack11_EnterMotion(object,ftCo_MS_Attack11));
    CHECK(state(object,"trace",NULL).as.i64==14&&scheduler_aux_tail==1);
    CHECK(scheduler_jab_flags==Ft_MF_SkipAttackCount);
    scheduler_aux_spawn=NULL;
    fighter->x197C=NULL;fighter->x1980=NULL;reset_trace(object);data.x8=NULL;
    // Unattached actors dispatch authored native hooks, then stock tables when absent.
    HSD_GObj* native=create();nativeScriptFighter_Detach(native);
    for(unsigned authored=0;authored<2;++authored) {
        fighter_scheduler_aux_native(authored);
        unsigned before=fighter_stock_calls;
        ftCommon_8007F8E8(native);ftCommon_8007F948(native,held,7);
        CHECK(fighter_stock_calls==before+2);
    }
    fighter_scheduler_aux_native(false);HSD_GObjFree(native);
    // Leave the existing source-only cases' stock-call sentinel unchanged.
    fighter_stock_calls=stock;
}
