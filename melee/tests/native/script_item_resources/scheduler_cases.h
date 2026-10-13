static unsigned scheduler_script_checks;
static size_t scheduler_component_bytes;
static uint8_t scheduler_state[2*1024*1024],scheduler_again[2*1024*1024];
static uint8_t fighter_script_state[128*1024],fighter_script_again[128*1024];
static uint8_t item_script_state[128*1024],item_script_again[128*1024];

static int64_t scheduler_fighter_ticks(void)
{
    NativeScriptReceiverView view;
    CHECK(nativeScriptFighter_View(fighter_object,(uint32_t)fighter_data->x8_spawnNum,&view));
    for(unsigned i=0;i<view.program->value_count;++i){
        const NativeScriptValueDeclaration* declaration=&view.program->values[i];
        if(!declaration->owner && nativeScriptSource_SpanEquals(view.program->tree,declaration->name,"scheduler_ticks")){
            NativeScriptValue value={0};
            CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));
            return value.as.i64;
        }
    }
    CHECK(false);return 0;
}
static void scheduler_fighter_callback(HSD_GObj* object)
{
    CHECK(object==fighter_object&&nativeScriptFighter_RunCallback(object,NativeScriptFighterCallback_PreHitlag));
}
static void scheduler_item_callback(HSD_GObj* object)
{
    nativeScriptItem_BeginTick(object);NativeScriptValue value={0};
    CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Animation,&value)==NativeScriptVM_Ok);
    CHECK(value.type==NativeScriptType_Bool&&!value.as.u64);
}
static void explicit_scheduler_scripts(void)
{
    unsigned start=checks;
    const NativeScriptProgramIndex* declarations=&candidate.program.bank->declarations;
    uint32_t pulse=UINT32_MAX;
    for(unsigned i=0;i<declarations->function_count;++i)
        if(!declarations->functions[i].owner && nativeScriptSource_SpanEquals(declarations->tree,declarations->functions[i].name,"scheduler_pulse"))pulse=i;
    CHECK(pulse!=UINT32_MAX);
    CHECK(nativeScriptFighter_BindCallback(fighter_object,NativeScriptFighterCallback_PreHitlag,pulse));
    CHECK(HSD_GObj_SetupProc(fighter_object,scheduler_fighter_callback,0));
    CHECK(HSD_GObj_SetupProc(objects[0],scheduler_item_callback,1));
    CHECK(HSD_GObj_SetupProc(objects[1],scheduler_item_callback,1));
    int64_t fighter_before=scheduler_fighter_ticks();
    int64_t item_before[2]={state(objects[0],"visits",NULL).as.i64,state(objects[1],"visits",NULL).as.i64};
    size_t gbytes=0,fbytes=0,ibytes=0;
    CHECK(nativeGObjPool_Save(scheduler_state,sizeof(scheduler_state),&gbytes));
    CHECK(nativeScriptFighter_Save(fighter_script_state,sizeof(fighter_script_state),&fbytes));
    CHECK(nativeScriptItem_Save(item_script_state,sizeof(item_script_state),&ibytes));
    // Actual scheduler dispatch enters the actual interpreter and mutates the
    // registered fighter/item state, callback counters and item tick state.
    HSD_GObj_80390CFC();
    CHECK(scheduler_fighter_ticks()==fighter_before+1);
    for(unsigned i=0;i<2;++i)CHECK(state(objects[i],"visits",NULL).as.i64==item_before[i]+1);
    HSD_GObj* future=GObj_Create(99,0,0);CHECK(future);
    NativeMatchPoolReference future_reference;
    CHECK(nativeMatchPool_Reference(&nativeGObjPool,future,&future_reference));
    CHECK(nativeScriptFighter_BindCallback(fighter_object,NativeScriptFighterCallback_PreHitlag,UINT32_MAX));
    HSD_GObj_80390C5C(objects[0]);
    CHECK(nativeGObjPool_ValidateSnapshot(scheduler_state,gbytes));
    CHECK(nativeScriptFighter_ValidateSnapshot(fighter_script_state,fbytes));
    CHECK(nativeScriptItem_ValidateSnapshot(item_script_state,ibytes));
    CHECK(nativeGObjPool_Load(scheduler_state,gbytes));
    // Restore native peers before VM actor storage/callback binding. Programs
    // stay resident. These loads execute no authored callback or destructor.
    CHECK(nativeScriptFighter_Load(fighter_script_state,fbytes));
    CHECK(nativeScriptItem_Load(item_script_state,ibytes));
    CHECK(nativeMatchPool_Resolve(future_reference)==NULL);
    CHECK(scheduler_fighter_ticks()==fighter_before);
    for(unsigned i=0;i<2;++i)CHECK(state(objects[i],"visits",NULL).as.i64==item_before[i]);
    // CallbackFunction queries the program's declared slot, not an instance's
    // mutable helper binding. Inspect the instance with the existing pure API.
    bool matches=false;
    CHECK(nativeScriptFighter_CallbackMatches(fighter_object,NativeScriptFighterCallback_PreHitlag,pulse,&matches)&&matches);
    CHECK(nativeScriptFighter_CallbackBound(fighter_object,NativeScriptFighterCallback_PreHitlag));
    size_t again=0;
    CHECK(nativeGObjPool_Save(scheduler_again,sizeof(scheduler_again),&again)&&again==gbytes&&!memcmp(scheduler_state,scheduler_again,again));
    CHECK(nativeScriptFighter_Save(fighter_script_again,sizeof(fighter_script_again),&again)&&again==fbytes&&!memcmp(fighter_script_state,fighter_script_again,again));
    CHECK(nativeScriptItem_Save(item_script_again,sizeof(item_script_again),&again)&&again==ibytes&&!memcmp(item_script_state,item_script_again,again));
    HSD_GObj_80390CFC();
    CHECK(scheduler_fighter_ticks()==fighter_before+1);
    for(unsigned i=0;i<2;++i)CHECK(state(objects[i],"visits",NULL).as.i64==item_before[i]+1);
    CHECK(GObj_Create(99,0,0)==future);HSD_GObjFree(future);
    HSD_GObjProc_RemoveAllProcs(fighter_object);
    HSD_GObjProc_RemoveAllProcs(objects[0]);HSD_GObjProc_RemoveAllProcs(objects[1]);
    CHECK(nativeScriptFighter_BindCallback(fighter_object,NativeScriptFighterCallback_PreHitlag,UINT32_MAX));
    scheduler_script_checks=checks-start;scheduler_component_bytes=gbytes;++cases;
}
