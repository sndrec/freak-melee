// Included with actual lifecycle fixture leaves; all controls outside .mstate.
static uint8_t scheduler_saved[3*1024*1024],scheduler_after[3*1024*1024];
static uint8_t scheduler_bad[3*1024*1024];
static unsigned scheduler_checks,scheduler_refusals;
static size_t scheduler_bytes;
static uint64_t scheduler_mask;
#define SCHECK(x) do{++scheduler_checks;CHECK(x);}while(0)

static size_t scheduler_table_size(const NativeGObjSnapshotHeader* h)
{
    size_t links=h->config.p_link_max+1u,gx=h->config.gx_link_max+2u;
    size_t priorities=h->config.gproc_pri_max+1u;
    return (2*links+2*gx+priorities+links*priorities)*sizeof(void*);
}
static uint8_t* scheduler_object_record(uint8_t* data,unsigned index)
{
    NativeGObjSnapshotHeader h;memcpy(&h,data,sizeof(h));
    uint8_t* pool=data+sizeof(h)+scheduler_table_size(&h);
    NativeMatchPoolSnapshotHeader p;memcpy(&p,pool,sizeof(p));
    return pool+sizeof(p)+(p.capacity+p.free_count)*sizeof(uint32_t)+
        index*(sizeof(uint32_t)+sizeof(HSD_GObj))+sizeof(uint32_t);
}
static uint8_t* scheduler_proc_record(uint8_t* data,unsigned index)
{
    NativeGObjSnapshotHeader h;memcpy(&h,data,sizeof(h));
    uint8_t* pool=data+sizeof(h)+scheduler_table_size(&h)+h.object_bytes;
    NativeMatchPoolSnapshotHeader p;memcpy(&p,pool,sizeof(p));
    return pool+sizeof(p)+(p.capacity+p.free_count)*sizeof(uint32_t)+
        index*(sizeof(uint32_t)+sizeof(HSD_GObjProc))+sizeof(uint32_t);
}
static void scheduler_reject(size_t bytes)
{
    NativeRollbackStateRegion region;SCHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1);
    memcpy(oracle,region.memory,region.size);uint64_t mask=scheduler_mask;
    SCHECK(!nativeGObjPool_ValidateSnapshot(scheduler_bad,bytes));
    SCHECK(!nativeGObjPool_Load(scheduler_bad,bytes));
    SCHECK(!memcmp(oracle,region.memory,region.size)&&scheduler_mask==mask);
    ++scheduler_refusals;
}
static void snapshot_inside_callback(HSD_GObj* object)
{
    size_t bytes=777;
    SCHECK(!nativeGObjPool_SnapshotBytes());
    SCHECK(!nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),&bytes)&&bytes==777);
    record(object->classifier);
}
static void snapshot_inside_draw(HSD_GObj* object,int pass)
{
    SCHECK(HSD_GObj_804D7814==object&&pass==0);
    SCHECK(HSD_GObj_RenderCallbackDepth==1);
    SCHECK(!nativeGObjPool_SnapshotBytes());
    SCHECK(!nativeGObjPool_RetireScene());
    record(1000+object->classifier);
}
static void snapshot_nested_inner(HSD_GObj* object,int pass)
{
    SCHECK(pass==0 && HSD_GObj_804D7814==object && HSD_GObj_RenderCallbackDepth==2);
    size_t written=777;
    SCHECK(!nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),&written)&&written==777);
    SCHECK(!nativeGObjPool_Load(scheduler_saved,scheduler_bytes));
}
static void snapshot_nested_outer(HSD_GObj* object,int pass)
{
    SCHECK(pass==0 && HSD_GObj_804D7818==object && HSD_GObj_RenderCallbackDepth==1);
    HSD_GObj camera={.gxlink_prios=2};
    HSD_GObj_80390ED0(&camera,1);
    SCHECK(HSD_GObj_RenderCallbackDepth==1);
}
static void explicit_scheduler(void)
{
    initialize();HSD_GObjLibInitDataType config=HSD_GObjLibInitData;
    config.unk_2=&scheduler_mask;SCHECK(nativeGObjPool_RetireScene());HSD_GObj_80391304(&config);
    scheduler_mask=0;
    HSD_GObj* a=GObj_Create(11,0,0),*b=GObj_Create(22,1,0),*c=GObj_Create(33,2,0);
    HSD_GObjProc* ap=HSD_GObj_SetupProc(a,remove_self,0);
    HSD_GObjProc* bp=HSD_GObj_SetupProc(b,snapshot_inside_callback,0);
    HSD_GObj_SetupProc(b,tick,1);HSD_GObj_SetupProc(c,tick,2);
    GObj_InitUserData(a,6,cleanup,a);
    GObj_SetupGXLinkMax(b,draw,0);GObj_SetupGXLink(c,snapshot_inside_draw,1,0);
    NativeMatchPoolReference old_a,old_ap;
    SCHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&old_a));
    SCHECK(nativeMatchPool_Reference(&nativeGObjProcPool,ap,&old_ap));
    size_t bytes=0;SCHECK(nativeGObjPool_Save(scheduler_saved,sizeof(scheduler_saved),&bytes));
    scheduler_bytes=bytes;SCHECK(bytes==nativeGObjPool_SnapshotBytes());
    NativeGObjSnapshotHeader h;memcpy(&h,scheduler_saved,sizeof(h));
    SCHECK(bytes==sizeof(h)+scheduler_table_size(&h)+h.object_bytes+h.proc_bytes);
    SCHECK(nativeGObjPool_ValidateSnapshot(scheduler_saved,bytes));
    size_t unchanged=777;
    size_t prefix=sizeof(h)+scheduler_table_size(&h);
    const size_t cuts[]={0,sizeof(h)-1,sizeof(h),prefix-1,prefix,
        prefix+h.object_bytes-1,prefix+h.object_bytes,bytes-1};
    for(unsigned i=0;i<sizeof(cuts)/sizeof(*cuts);++i) {
        size_t unpublished=SIZE_MAX;
        SCHECK(!nativeGObjPool_Save(scheduler_after,cuts[i],&unpublished));
        SCHECK(unpublished==SIZE_MAX);
    }
    SCHECK(nativeGObjPool_Save(scheduler_after,bytes,&unchanged));
    SCHECK(unchanged==bytes && !memcmp(scheduler_saved,scheduler_after,bytes));
    // A corrupted second pool must not publish the otherwise valid first one.
    uint32_t proc_order=nativeGObjProcPool.free_order[1];
    nativeGObjProcPool.free_order[1]=nativeGObjProcPool.free_order[0];
    unchanged=777;
    SCHECK(!nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),&unchanged)&&unchanged==777);
    nativeGObjProcPool.free_order[1]=proc_order;
    SCHECK(nativeGObjPool_Save(scheduler_after,bytes,&unchanged));
    SCHECK(unchanged==bytes && !memcmp(scheduler_saved,scheduler_after,bytes));
    unchanged=777;
    SCHECK(!nativeGObjPool_Save(scheduler_after,bytes-1,&unchanged)&&unchanged==777);
    SCHECK(!nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),(size_t*)scheduler_after));
    SCHECK(!nativeGObjPool_Save(nativeGObjPool.payload,bytes,&unchanged));
    SCHECK(!nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),(size_t*)HSD_GObjPLinkHead));
    // Candidate refusal is atomic across both payload components and tables.
    for(unsigned n=0;n<16;++n){
        memcpy(scheduler_bad,scheduler_saved,bytes);size_t candidate_bytes=bytes;
        NativeGObjSnapshotHeader candidate=h;
        if(n==0)candidate_bytes=bytes-1;
        if(n==1)candidate_bytes=bytes+1;
        if(n==2)candidate.epoch++;
        if(n==3)candidate.invocation_phase=3;
        if(n==4)candidate.config.gproc_pri_max++;
        if(n==5)candidate.delayed.flags=1;
        if(n==13)candidate.render_context=(HSD_GObj*)(uintptr_t)1;
        if(n==14)candidate.render_context=(HSD_GObj*)((uint8_t*)b+1);
        if(n==15)candidate.render_context=(HSD_GObj*)(nativeGObjPool.payload+
            nativeGObjPool.free_order[0]*nativeGObjPool.stride);
        memcpy(scheduler_bad,&candidate,sizeof(candidate));
        if(n==6){const void* pointer=(void*)1;memcpy(scheduler_bad+sizeof(h),&pointer,sizeof(pointer));}
        if(n==7){HSD_GObj object;uint8_t* p=scheduler_object_record(scheduler_bad,0);memcpy(&object,p,sizeof(object));object.next=a;memcpy(p,&object,sizeof(object));}
        if(n==8){HSD_GObjProc proc;uint8_t* p=scheduler_proc_record(scheduler_bad,0);memcpy(&proc,p,sizeof(proc));proc.gobj=(void*)1;memcpy(p,&proc,sizeof(proc));}
        if(n==9){HSD_GObjProc proc;uint8_t* p=scheduler_proc_record(scheduler_bad,1);memcpy(&proc,p,sizeof(proc));proc.child=bp;memcpy(p,&proc,sizeof(proc));}
        if(n==10){const void* pointer=a;size_t offset=sizeof(h)+scheduler_table_size(&h)-sizeof(pointer);memcpy(scheduler_bad+offset,&pointer,sizeof(pointer));}
        if(n==11){HSD_GObj object;uint8_t* p=scheduler_object_record(scheduler_bad,1);memcpy(&object,p,sizeof(object));object.native_destroying=1;memcpy(p,&object,sizeof(object));}
        if(n==12){HSD_GObjProc proc;uint8_t* p=scheduler_proc_record(scheduler_bad,0);memcpy(&proc,p,sizeof(proc));proc.next=NULL;memcpy(p,&proc,sizeof(proc));}
        scheduler_reject(candidate_bytes);
    }
    event_count=0;HSD_GObj_80390CFC();HSD_GObj_80390FC0();
    HSD_GObj camera={.gxlink_prios=2};HSD_GObj_80390ED0(&camera,1);
    SCHECK(removed==1&&nativeMatchPool_Resolve(old_a)==NULL&&nativeMatchPool_Resolve(old_ap)==NULL);
    unsigned expected[64],expected_count=event_count;memcpy(expected,events,sizeof(expected));
    SCHECK(expected_count==6&&expected[0]==11&&expected[1]==22&&expected[2]==22&&expected[3]==33&&expected[4]==1022&&expected[5]==1033);
    HSD_GObj* reused=GObj_Create(44,3,0);SCHECK(reused==a);
    SCHECK(HSD_GObj_SetupProc(reused,tick,3)==ap);
    NativeMatchPoolReference future;SCHECK(nativeMatchPool_Reference(&nativeGObjPool,reused,&future));
    HSD_GObjPLink_ChangeGObjPri_Unk(0,b,2,2,NULL);
    HSD_GObj_80390C5C(c);scheduler_mask=UINT64_MAX;
    HSD_GObj_CameraKind=99;HSD_GObj_804085F0[1]=123;
    unsigned before_removal=removed;
    SCHECK(nativeGObjPool_Load(scheduler_saved,bytes));
    SCHECK(removed==before_removal&&scheduler_mask==0);
    SCHECK(nativeMatchPool_Resolve(old_a)==a&&nativeMatchPool_Resolve(old_ap)==ap&&nativeMatchPool_Resolve(future)==NULL);
    SCHECK(a->proc==ap&&b->proc->child==bp&&HSD_GObjPLinkHead[0]==a&&HSD_GObjPLinkHead[1]==b);
    size_t again=0;SCHECK(nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),&again)&&again==bytes&&!memcmp(scheduler_saved,scheduler_after,bytes));
    removed=0;event_count=0;HSD_GObj_80390CFC();HSD_GObj_80390FC0();HSD_GObj_80390ED0(&camera,1);
    SCHECK(removed==1&&event_count==expected_count&&!memcmp(expected,events,event_count*sizeof(*events)));
    SCHECK(GObj_Create(44,3,0)==reused&&HSD_GObj_SetupProc(reused,tick,3)==ap);
    SCHECK(nativeGObjPool_Load(scheduler_saved,bytes));
    // Live draw/VM/proc boundaries refuse; immutable scene config must match.
    HSD_GObj_RenderCallbackDepth=1;SCHECK(!nativeGObjPool_Load(scheduler_saved,bytes));HSD_GObj_RenderCallbackDepth=0;
    HSD_GObj_804D7814=b;SCHECK(nativeGObjPool_Load(scheduler_saved,bytes)&&HSD_GObj_804D7814==NULL);
    HSD_GObjLibInitData.gproc_pri_max++;SCHECK(!nativeGObjPool_Load(scheduler_saved,bytes));HSD_GObjLibInitData.gproc_pri_max--;
    SCHECK(nativeGObjPool_RetireScene());HSD_GObj_80391304(&config);
    SCHECK(!nativeGObjPool_Load(scheduler_saved,bytes));
    size_t empty=0;SCHECK(nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),&empty)&&empty<bytes);
    SCHECK(nativeGObjPool_Load(scheduler_after,empty));
    // Retained shadow context is state, not callback activity. Nested callbacks
    // reject capture; after dispatch the prior context survives unchanged.
    a=GObj_Create(11,0,0);b=GObj_Create(22,1,0);
    GObj_SetupGXLinkMax(a,snapshot_nested_outer,0);
    GObj_SetupGXLink(b,snapshot_nested_inner,1,0);
    HSD_GObj_804D7814=a;
    SCHECK(nativeGObjPool_Save(scheduler_saved,sizeof(scheduler_saved),&scheduler_bytes));
    HSD_GObj_80390FC0();
    SCHECK(HSD_GObj_RenderCallbackDepth==0 && HSD_GObj_804D7814==a);
    HSD_GObjFree(a);
    SCHECK(!nativeGObjPool_SnapshotBytes()); // stale current context cannot be captured
    SCHECK(nativeGObjPool_ValidateSnapshot(scheduler_saved,scheduler_bytes));
    SCHECK(nativeGObjPool_Load(scheduler_saved,scheduler_bytes) && HSD_GObj_804D7814==a);
    SCHECK(nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),&again) &&
        again==scheduler_bytes && !memcmp(scheduler_saved,scheduler_after,again));
    // Full configured table extents and real pool capacity; no callback loop
    // or shift-by-an-out-of-range plink is executed in this storage case.
    SCHECK(nativeGObjPool_RetireScene());
    config.p_link_max=255;config.gx_link_max=255;config.gproc_pri_max=255;
    HSD_GObj_80391304(&config);
    HSD_GObj* full=GObj_Create(77,255,0);SCHECK(full);
    for(unsigned i=0;i<NATIVE_GOBJ_PROC_CAPACITY;++i)SCHECK(HSD_GObj_SetupProc(full,tick,255));
    for(unsigned i=1;i<NATIVE_GOBJ_CAPACITY;++i)SCHECK(GObj_Create(77,255,0));
    size_t full_bytes=0;SCHECK(nativeGObjPool_Save(scheduler_after,sizeof(scheduler_after),&full_bytes)&&full_bytes>bytes);
    SCHECK(nativeGObjPool_Load(scheduler_after,full_bytes));
    SCHECK(nativeGObjPool_Save(scheduler_bad,sizeof(scheduler_bad),&again)&&again==full_bytes&&!memcmp(scheduler_after,scheduler_bad,full_bytes));
    ++cases;
}
#undef SCHECK
