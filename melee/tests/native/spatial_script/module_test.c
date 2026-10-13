#include "spatial.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned order, queries, winds;
static bool exists;
static s32 random_int(s32 bound)
{ assert(bound==100);return (s32)++order; }
static bool position(Item_GObj* object,unsigned model,const char* path,Vec3* output)
{
    assert(object && object->user_data);++queries;
    assert((model==0 && !strcmp(path,"0")) || (model==1 && !strcmp(path,"")));
    *output=exists?(Vec3){11+model,22+model,33+model}:(Vec3){0};return exists;
}
static void wind(Vec3* position,int frames,float strength,float decay,float frequency)
{
    if (!winds) {
        assert(order==7 && position->x==1 && position->y==2 && position->z==3);
        assert(frames==4 && strength==5 && decay==6 && frequency==7);
    } else {
        assert(queries==1 && position->x==(exists?11:0) && position->y==(exists?22:0) && position->z==(exists?33:0));
        assert(frames==120 && strength==3 && decay==.1f && frequency==1.0471976f);
    }
    ++winds;
}
int main(void)
{
    MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataGameWatch",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.spawn_wind=wind,.random_int=random_int,.item_joint_world_position=position};
    NativeFighterHost missing=host;missing.item_joint_world_position=NULL;assert(!FreakMelee_GetFighterModule(&base,&missing));
    missing=host;missing.spawn_wind=NULL;assert(!FreakMelee_GetFighterModule(&base,&missing));
    const NativeFighterDefinition* def=FreakMelee_GetFighterModule(&base,&host);assert(def && def->article_logic_count==1);
    Fighter* fighter=calloc(1,sizeof(*fighter));assert(fighter);HSD_GObj object={.user_data=fighter};def->on_load(&object);assert(winds==1 && order==7);free(fighter);
    Item* item=calloc(1,sizeof(*item));assert(item);object.user_data=item;
    const NativeFighterArticleLogic* logic=&def->article_logics[0];
    for(unsigned present=0;present<2;++present){
        queries=0;winds=1;exists=present;logic->logic->states[0].physics_updated(&object);assert(queries==2 && winds==2);
        for(unsigned axis=0;axis<3;++axis){float value;memcpy(&value,item->xDD4_itemVar.native_article.state+logic->state_fields[axis].offset,4);assert(value==(exists?12.0f+11.0f*axis:0));}
    }
    free(item);puts("PASS spatial query, output destinations and single ordered argument evaluation");
}
