#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>

static unsigned nearest_comparisons;
static bool nearest_host(HSD_GObj* source,NativeScriptValue* result)
{
    NativeScriptRuntimeContext c=context(source);
    return nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsNearestAvailableFighter,NULL,0,result,&diagnostic);
}
static void nearest_compare(HSD_GObj* source)
{
    Fighter* expected=ftCo_800A4A40(source->user_data);
    uint32_t id=expected?expected->x8_spawnNum:0;
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
    CHECK(nearest_host(source,&result) && result.type==NativeScriptType_I32 && (uint32_t)result.as.i64==id);
    geometry_run(source,"QueryNearest");CHECK(field(source,"nearest",NULL).as.i64==(int32_t)id);
    ++comparisons;++nearest_comparisons;++cases;
}
static void nearest_cases(HSD_GObj* caller,HSD_GObj* article_caller,HSD_GObj* owned,HSD_GObj* stock)
{
    Fighter *a=caller->user_data,*b=owned->user_data,*c=stock->user_data;
    Fighter saved[3]={*a,*b,*c};bool teams_saved=ahead_teams;
    for(unsigned i=0;i<128;++i) {
        *a=saved[0];*b=saved[1];*c=saved[2];
        a->cur_pos=(Vec3){(float)(i%3),-3,0};a->cpu.half_width=(float)(i%5);a->cpu.half_height=(float)(i%7);
        b->cur_pos=(Vec3){-90+(float)(i%11)*19,-90+(float)(i%9)*25,10000};
        c->cur_pos=(Vec3){-91+(float)(i%7)*32,-90+(float)(i%11)*19,-10000};
        a->team=b->team=1;c->team=2;ahead_teams=(i&1)!=0;
        b->x221F_b3=(i&8)!=0;b->x2224_b2=(i&16)!=0;b->motion_id=i&32?ftCo_MS_DeadDown:ftCo_MS_Wait;
        c->x221F_b3=(i&1)!=0;c->x2224_b2=(i&2)!=0;c->motion_id=i&4?ftCo_MS_DeadUpFallHitCameraIce:ftCo_MS_Wait;
        b->x2226_b4=b->x2226_b5=(i&64)!=0;
        b->smash_attrs.x2134_vibrateFrame=i%3;b->cpu.x7C=i*5;
        nearest_compare(caller);
    }
    *a=saved[0];*b=saved[1];*c=saved[2];
    a->cur_pos=(Vec3){0,0,0};a->cpu.half_width=a->cpu.half_height=5;
    b->cur_pos=(Vec3){1,0,10000};c->cur_pos=(Vec3){10,0,0};
    a->team=b->team=1;c->team=2;ahead_teams=true;
    b->motion_id=c->motion_id=ftCo_MS_Wait;
    nearest_compare(caller);
    CHECK(field(caller,"nearest",NULL).as.i64==b->x8_spawnNum); // Ally and full-width ID, despite far Z.
    b->cur_pos=(Vec3){-10,0,0};nearest_compare(caller); // Registry-order distance tie.
    c->x221F_b3=true;
    const Vec3 edges[]={{-95,0,0},{95,0,0},{0,-95,0},{0,95,0}};
    for(unsigned i=0;i<4;++i) {
        b->cur_pos=edges[i];nearest_compare(caller);CHECK(field(caller,"nearest",NULL).as.i64==b->x8_spawnNum);
        if(i<2)b->cur_pos.x+=i==0?-.25f:.25f;else b->cur_pos.y+=i==2?-.25f:.25f;
        nearest_compare(caller);CHECK(field(caller,"nearest",NULL).as.u64==0);
    }
    b->x2224_b2=true;nearest_compare(caller);CHECK(field(caller,"nearest",NULL).as.u64==0);
    *a=saved[0];*b=saved[1];*c=saved[2];ahead_teams=teams_saved;

    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!nearest_host(article_caller,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++negative_count;
    NativeScriptRuntimeContext ctx=context(caller);NativeScriptValue arg=integer(0);
    CHECK(!nativeScriptBindingTargets_Invoke(&ctx,NativeScriptBinding_TargetsNearestAvailableFighter,&arg,1,&result,&diagnostic));
    CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++negative_count;
    HSD_GObj* head=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];HSD_GObj* tail=head;
    while(tail->next)tail=tail->next;
    tail->next=head;CHECK(!nearest_host(caller,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);tail->next=NULL;++negative_count;
    void* poison=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);CHECK(poison);
    HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=poison;
    CHECK(!nearest_host(caller,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++negative_count;
    HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=head;CHECK(VirtualFree(poison,0,MEM_RELEASE));
}
