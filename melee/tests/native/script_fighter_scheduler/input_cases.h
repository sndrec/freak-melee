static void input_hook_cases(HSD_GObj* object)
{
    Fighter* fighter=object->user_data;
    for(unsigned handled=0;handled<2;++handled) {
        NativeScriptValue choice={.type=NativeScriptType_Bool,.as.u64=handled};
        state(object,"handled",&choice);
        for(unsigned stop=0;stop<=20;++stop) {
            reset_trace(object);fighter_input_calls=0;fighter_input_stop=stop;
            ftCo_Wait_IASA(object);
            bool prior=stop&&stop<=14;
            CHECK(state(object,"trace",NULL).as.i64==(prior?1:18));
            unsigned expected=prior?stop:handled?14:stop?stop:20;
            CHECK(fighter_input_calls==expected);
        }
        const unsigned common[]={0,99,113,114,112,115,255};
        for(unsigned i=0;i<sizeof(common)/sizeof(common[0]);++i) {
            fighter->x2070.x2073=common[i];reset_trace(object);
            bool shortcut=common[i]==99||common[i]==113||common[i]==114;
            CHECK(ft_800898C0(object)==(shortcut||handled));
            CHECK(state(object,"trace",NULL).as.i64==(shortcut?1:19));
        }
    }
    fighter->x2070.x2073=0;fighter_input_stop=0;
    NativeScriptValue choice={.type=NativeScriptType_Bool};state(object,"handled",&choice);
    for(unsigned hook=0;hook<2;++hook) {
        HSD_GObj* fault=create();scheduler_fail_write=true;fighter_input_calls=0;
        if(hook)CHECK(!ft_800898C0(fault));else ftCo_Wait_IASA(fault);
        CHECK(nativeScriptFighter_Status(fault)==NativeScriptVM_Error);
        CHECK(fighter_input_calls==(hook?0:14));scheduler_fail_write=false;HSD_GObjFree(fault);
        HSD_GObj* retired=create();NativeMatchPoolReference reference;
        CHECK(nativeMatchPool_Reference(&nativeGObjPool,retired,&reference));
        scheduler_remove=true;fighter_input_calls=0;
        if(hook)CHECK(!ft_800898C0(retired));else ftCo_Wait_IASA(retired);
        CHECK(!nativeMatchPool_Resolve(reference)&&!scheduler_remove);
        CHECK(fighter_input_calls==(hook?0:14));
    }
    reset_trace(object);
}
static void absent_input_hooks(void)
{
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));
    program.hooks[NativeScriptFighter_WaitInput]=UINT32_MAX;
    program.hooks[NativeScriptFighter_ActionStatus]=UINT32_MAX;
    program.hooks[NativeScriptFighter_AuxiliaryItemAttach]=UINT32_MAX;
    program.hooks[NativeScriptFighter_AuxiliaryItemVisibilityReset]=UINT32_MAX;
    nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&program);
    HSD_GObj* object=create();fighter_input_calls=0;fighter_input_stop=0;
    ftCo_Wait_IASA(object);CHECK(fighter_input_calls==20&&state(object,"trace",NULL).as.i64==1);
    CHECK(!ft_800898C0(object)&&state(object,"trace",NULL).as.i64==1);
    unsigned aux_stock=fighter_stock_calls;
    ftCommon_8007F8E8(object);ftCommon_8007F948(object,NULL,9);
    CHECK(fighter_stock_calls==aux_stock&&state(object,"trace",NULL).as.i64==1);
    nativeScriptFighter_Detach(object);
    unsigned stock=fighter_stock_calls;fighter_input_calls=0;
    ftCo_Wait_IASA(object);CHECK(fighter_input_calls==14&&fighter_stock_calls==stock+1);
    CHECK(ft_800898C0(object)&&fighter_stock_calls==stock+2);
    HSD_GObjFree(object);
}
