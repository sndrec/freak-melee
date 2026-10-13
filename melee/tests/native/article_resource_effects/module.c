#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include RESOURCE_EFFECT_MODULE
static HSD_GObj* object;
static Item* item;
static unsigned checks, operands, effects, retire_operand, retire_effect;
static bool dead, recycled;
static unsigned finished_offset;
#define CHECK(v) do{++checks;assert(v);}while(0)
static bool alive(HSD_GObj* source,uint32_t generation,uint32_t cls)
{ CHECK(source==object && cls==NativeCallbackSource_Item && generation==0xF1234567u); return !dead; }
static void retire(void)
{
    dead=true;
    if(recycled) { item->x1C++; } else {
        DWORD old; CHECK(VirtualProtect(item,sizeof(*item),PAGE_NOACCESS,&old));
        CHECK(VirtualProtect(object,sizeof(*object),PAGE_NOACCESS,&old));
    }
}
static void operand(Item_GObj* source)
{ CHECK(source==object); ++operands; if(operands==retire_operand)retire(); }
static bool position(Item_GObj* source,unsigned index,int32_t id,const Vec3* p)
{
    CHECK(source==object && index==0 && id==1011 && operands==5);
    CHECK(p->x==1 && p->y==2 && p->z==3);
    ++effects; if(effects==retire_effect)retire(); return true;
}
static bool facing(Item_GObj* source,unsigned index,int32_t id,const Vec3* p,float f)
{
    CHECK(source==object && index==1 && id==1009 && operands==11);
    CHECK(p->x==4 && p->y==5 && p->z==6 && f==-1);
    ++effects; if(effects==retire_effect)retire(); return false; // No allocation is not terminal.
}
int main(void)
{
    object=VirtualAlloc(NULL,sizeof(*object),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    item=VirtualAlloc(NULL,sizeof(*item),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE); CHECK(object && item);
    MotionState motion={0}; NativeFighterDefinition base={.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.callback_source_alive=alive,.item_advance_animation=operand,
        .article_resource_spawn_effect_at_position=position,
        .article_resource_spawn_effect_facing_at_position=facing};
    const NativeFighterDefinition* d=FreakMelee_GetFighterModule(&base,&host); CHECK(d && d->article_logic_count==1);
    const NativeFighterArticleLogic* article=d->article_logics;
    bool found=false;
    for(unsigned i=0;i<article->state_field_count;++i)if(!strcmp(article->state_fields[i].name,"finished")){finished_offset=article->state_fields[i].offset;found=true;}
    CHECK(found);
    NativeFighterHost missing=host; missing.article_resource_spawn_effect_at_position=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    missing=host; missing.article_resource_spawn_effect_facing_at_position=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    missing=host; missing.callback_source_alive=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    CHECK(FreakMelee_GetFighterModule(&base,&host));
    for(unsigned reuse=0;reuse<2;++reuse)for(unsigned scenario=0;scenario<14;++scenario){
        DWORD old; CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));CHECK(VirtualProtect(object,sizeof(*object),PAGE_READWRITE,&old));
        memset(object,0,sizeof(*object));memset(item,0,sizeof(*item));object->user_data=item;item->entity=object;item->x1C=0xF1234567u;
        operands=effects=0;dead=false;recycled=reuse!=0;
        retire_operand=scenario<=11?scenario:0;retire_effect=scenario>11?scenario-11:0;
        article->logic->states[0].physics_updated(object);
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));CHECK(VirtualProtect(object,sizeof(*object),PAGE_READWRITE,&old));
        int32_t result;memcpy(&result,item->xDD4_itemVar.native_article.state+finished_offset,4);
        CHECK(result==(scenario==0?7:0));
        CHECK(operands==(retire_operand?retire_operand:scenario==12?5:11));
        CHECK(effects==(scenario==0 || scenario==13?2:scenario==12 || scenario>=6?1:0));
        CHECK(dead==(scenario!=0));
    }
    VirtualFree(item,0,MEM_RELEASE);VirtualFree(object,0,MEM_RELEASE);
    printf("{\"generated_checks\":%u,\"lifetime_cases\":28}\n",checks);
}
