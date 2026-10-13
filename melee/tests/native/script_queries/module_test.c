#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef QUERY_MODULE
#define QUERY_MODULE "script_queries_action.inc"
#endif
#include QUERY_MODULE

__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
__declspec(dllimport) int __stdcall VirtualFree(void*,size_t,unsigned long);
_Static_assert(sizeof(Fighter) < 65536-4096 && sizeof(Item) < 65536-4096,"Fixture actors must fit protected allocation");
static void *fighter_pages,*item_pages;
static HSD_GObj *fo,*io;
static HSD_GObj* query_source;
static Fighter* fp; static Item* ip;
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic* article;
static bool fighter_live,item_live,query_success;
static uint32_t fg,ig;
static unsigned checks,sequence,stop_at,mode,nearest_calls,translation_calls;
static unsigned position_calls;
static uint32_t expected_position_id;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static unsigned field(const NativeFighterStateField* fields,unsigned count,const char* name)
{for(unsigned i=0;i<count;++i)if(!strcmp(fields[i].name,name))return fields[i].offset;assert(0);return 0;}
static unsigned foff(const char* name){return field(definition->state_fields,definition->state_field_count,name);}
static unsigned ioff(const char* name){return field(article->state_fields,article->state_field_count,name);}
static void fword(const char* name,int32_t value){memcpy(fp->native_module_state+foff(name),&value,4);}
static int32_t fget(const char* name){int32_t value;memcpy(&value,fp->native_module_state+foff(name),4);return value;}
static void iword(const char* name,int32_t value){memcpy(ip->xDD4_itemVar.native_article.state+ioff(name),&value,4);}
static int32_t iget(const char* name){int32_t value;memcpy(&value,ip->xDD4_itemVar.native_article.state+ioff(name),4);return value;}
static void set_float(unsigned char* bytes,unsigned offset,float value){memcpy(bytes+offset,&value,4);}
static float get_float(const unsigned char* bytes,unsigned offset){float value;memcpy(&value,bytes+offset,4);return value;}
static void protect(void* pages,unsigned protection){unsigned long previous;CHECK(VirtualProtect(pages,65536,protection,&previous));}
static bool alive(HSD_GObj* source,uint32_t generation,uint32_t source_class)
{
    if(source_class==NativeCallbackSource_Fighter){CHECK(source==fo&&generation==fg);return fighter_live&&fp->x8_spawnNum==fg&&fo->user_data==fp;}
    CHECK(source_class==NativeCallbackSource_Item&&source==io&&generation==ig);
    return item_live&&ip->x1C==ig&&io->user_data==ip;
}
static void boundary(HSD_GObj* source)
{
    ++sequence;if(sequence!=stop_at)return;
    bool fighter=source==fo;CHECK(fighter||source==io);
    if(mode==2){if(fighter)++fp->x8_spawnNum;else ++ip->x1C;return;}
    if(fighter)fighter_live=false;else item_live=false;
    protect(fighter?fighter_pages:item_pages,1);
}
static void landing(HSD_GObj* source){CHECK(source==fo);boundary(source);}
static bool nearest(HSD_GObj* source,const Vec3* origin,uint32_t* target)
{
    CHECK(source==fo&&origin&&target);++nearest_calls;
    CHECK(origin->x==1.25f&&origin->y==-2.5f&&origin->z==3.75f);
    *target=UINT32_C(0x80000001);boundary(source);return query_success;
}
static bool translation(HSD_GObj* source,uint32_t handle,ItemKind kind,Vec3* result)
{
    CHECK(source==fo||source==io);CHECK(kind==It_Kind_Ness_PKFire&&result);++translation_calls;
    CHECK(handle==(query_success||source==io?UINT32_C(0x80000001):0));
    *result=(Vec3){10.25f,-20.5f,30.75f};boundary(source);return query_success;
}
static bool position(uint32_t handle,Vec3* result)
{
    CHECK(handle==expected_position_id&&result);++position_calls;
    *result=(Vec3){10.25f,-20.5f,30.75f};boundary(query_source);
    return query_success;
}
static void reset(bool success,unsigned stop,unsigned retirement)
{
    protect(fighter_pages,4);protect(item_pages,4);memset(fighter_pages,0,65536);memset(item_pages,0,65536);
    fo=fighter_pages;fp=(Fighter*)((unsigned char*)fighter_pages+4096);fo->user_data=fp;
    io=item_pages;ip=(Item*)((unsigned char*)item_pages+4096);io->user_data=ip;ip->entity=io;
    fg=UINT32_C(0x80000021);ig=UINT32_C(0x80000031);fp->x8_spawnNum=fg;ip->x1C=ig;
    fighter_live=item_live=true;query_success=success;stop_at=stop;mode=retirement;
    query_source=fo;
    expected_position_id=UINT32_C(0x80000001);
    sequence=nearest_calls=translation_calls=position_calls=0;iword("handle",(int32_t)UINT32_C(0x80000001));
    fword("handle",(int32_t)UINT32_C(0x80000001));
    const char* names[]={"x","y","z"};for(unsigned i=0;i<3;++i){set_float(fp->native_module_state,foff(names[i]),-11.f-i);set_float(ip->xDD4_itemVar.native_article.state,ioff(names[i]),-11.f-i);}
}
static void invoke(void)
{
#if QUERY_CALLBACKS
    definition->motion_states[0].phys_cb(fo);
#else
    definition->on_load(fo);
#endif
}
static void floats(bool item,bool updated)
{
    const char* names[]={"x","y","z"};const float expected[]={10.25f,-20.5f,30.75f};
    for(unsigned i=0;i<3;++i)CHECK(get_float(item?ip->xDD4_itemVar.native_article.state:fp->native_module_state,item?ioff(names[i]):foff(names[i]))==(updated?expected[i]:-11.f-i));
}
int main(void)
{
    fighter_pages=VirtualAlloc(NULL,65536,0x3000,4);item_pages=VirtualAlloc(NULL,65536,0x3000,4);CHECK(fighter_pages&&item_pages);
    MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.callback_source_alive=alive,.enter_landing_basic=landing,.cpu_nearest_enemy_at=nearest,.article_ref_primary_translation=translation,.target_position=position};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->article_logic_count==1);article=&definition->article_logics[0];
    NativeFighterHost missing=host;missing.cpu_nearest_enemy_at=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    missing=host;missing.article_ref_primary_translation=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    missing=host;missing.callback_source_alive=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    missing=host;missing.target_position=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);article=&definition->article_logics[0];
    for(unsigned success=0;success<2;++success){reset(success,0,0);invoke();CHECK(nearest_calls==1&&translation_calls==1&&sequence==6);CHECK(fget("handle")== (success?(int32_t)UINT32_C(0x80000001):0));CHECK(fget("order")==1234&&fget("after")==99);CHECK(fp->native_module_state[foff("found")]==success);floats(false,success);}
    for(unsigned retirement=1;retirement<=2;++retirement)for(unsigned stop=1;stop<=6;++stop)for(unsigned success=0;success<2;++success){
        reset(success,stop,retirement);invoke();protect(fighter_pages,4);CHECK(sequence==stop&&fget("after")==0);floats(false,false);
        const int order[]={0,1,12,123,123,1234,1234};CHECK(fget("order")==order[stop]);CHECK(nearest_calls==(stop>=4)&&translation_calls==(stop==6));
    }
    for(unsigned state_output=0;state_output<2;++state_output)for(unsigned success=0;success<2;++success)for(unsigned retirement=0;retirement<=2;++retirement){
        reset(success,retirement?1:0,retirement);
        if(state_output)article->logic->states[0].physics_updated(io);else article->on_created(io);
        protect(item_pages,4);CHECK(nearest_calls==0&&translation_calls==1);CHECK(iget("after")== (retirement?0:99));floats(true,!retirement&&success);
        CHECK(ip->xDD4_itemVar.native_article.state[ioff("found")]==(!retirement&&success));
    }
    CHECK(definition->callback_export_count==1);
    const uint32_t retained_ids[]={0,1,UINT32_C(16777217),UINT32_C(0x80000001),UINT32_MAX};
    for(unsigned id=0;id<5;++id)for(unsigned success=0;success<2;++success)for(unsigned retirement=0;retirement<=2;++retirement)for(unsigned stop=1;stop<=2;++stop){
        if(retained_ids[id]==0&&success)continue;
        reset(success,retirement?stop:0,retirement);expected_position_id=retained_ids[id];fword("handle",(int32_t)expected_position_id);
        definition->callback_exports[0].callback(fo);protect(fighter_pages,4);
        CHECK(sequence==(retirement?stop:2)&&position_calls==(!retirement||stop==2));
        CHECK(fget("order")==4&&fget("after")== (retirement?0:99));floats(false,!retirement&&success);
    }
    for(unsigned id=0;id<5;++id)for(unsigned state_output=0;state_output<2;++state_output)for(unsigned success=0;success<2;++success)for(unsigned retirement=0;retirement<=2;++retirement){
        if(retained_ids[id]==0&&success)continue;
        reset(success,retirement?1:0,retirement);query_source=io;expected_position_id=retained_ids[id];iword("handle",(int32_t)expected_position_id);
        if(state_output)article->logic->thrown(io);else article->logic->dropped(io);
        protect(item_pages,4);CHECK(position_calls==1&&nearest_calls==0&&translation_calls==0);
        CHECK(iget("after")== (retirement?0:99));floats(true,!retirement&&success);
        CHECK(ip->xDD4_itemVar.native_article.state[ioff("found")]==(!retirement&&success));
    }
    CHECK(VirtualFree(fighter_pages,0,0x8000));CHECK(VirtualFree(item_pages,0,0x8000));
    printf("PASS %u generated query checks: both query results/full-width ID/order/atomic outputs and protected/recycled fighter+article source guards\n",checks);return 0;
}
