#include "presentation_fixture.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static GXColor fighter_diffuse,item_diffuse;
static unsigned color_queries,layer_calls,disable_calls;
static bool source_alive(HSD_GObj* o,uint32_t generation,uint32_t kind) {
    return o && o->user_data && (kind==NativeCallbackSource_Item
        ? ((Item*)o->user_data)->x1C==generation : ((Fighter*)o->user_data)->x8_spawnNum==generation);
}
static void fighter_color(Fighter_GObj* o,GXColor* c) { assert(o->user_data);fighter_diffuse=*c; }
static void article_color(Item_GObj* o,GXColor* c) { assert(o->user_data);item_diffuse=*c; }
static bool controller(uint32_t id,GXColor* c) {
    assert(id==UINT32_C(0x80000001));++color_queries;*c=(GXColor){0xDE,0xAD,0xBE,0xEF};return true;
}
static bool visibility(Fighter_GObj* o,int32_t slot) { assert(o->user_data && slot==10);return true; }
static bool layered(Item_GObj* o,bool enabled) { ++layer_calls;((Item*)o->user_data)->xDCF_flag.b3=enabled;return true; }
static void disable(Item_GObj* o,s32 index) { assert(o->user_data && index==2);++disable_calls; }
static unsigned offset(const NativeFighterDefinition* d,const char* name) {
    for(unsigned i=0;i<d->state_field_count;++i)if(!strcmp(d->state_fields[i].name,name))return d->state_fields[i].offset;
    assert(0);return 0;
}
static uint32_t word(Fighter* f,const NativeFighterDefinition* d,const char* name) {
    uint32_t x;memcpy(&x,f->native_module_state+offset(d,name),4);return x;
}
int main(void) {
    MotionState motion={0}; NativeFighterDefinition base={.fighter_root="ftDataNess",
        .motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.set_fighter_diffuse_color=fighter_color,.set_item_diffuse_color=article_color,
        .target_controller_color=controller,.install_outline_visibility=visibility,
        .set_item_layered=layered,.item_disable_hitbox=disable,.callback_source_alive=source_alive};
    const NativeFighterDefinition* d=FreakMelee_GetFighterModule(&base,&host);assert(d);
    assert(d->asset_layout.visibility_mask==(1u<<10));
#define MISSING(field) do { NativeFighterHost incomplete=host; incomplete.field=NULL; assert(!FreakMelee_GetFighterModule(&base,&incomplete)); } while(0)
    MISSING(set_fighter_diffuse_color);MISSING(set_item_diffuse_color);MISSING(target_controller_color);
    MISSING(install_outline_visibility);MISSING(set_item_layered);MISSING(item_disable_hitbox);
    Fighter* f=calloc(1,sizeof(*f));Item* i=calloc(1,sizeof(*i));assert(f && i);
    HSD_GObj fg={.user_data=f},ig={.user_data=i};f->input.lstick[1].y=0.75f;i->entity=&ig;
#if PRESENTATION_STATE
    d->motion_states[0].phys_cb(&fg);
#else
    d->on_load(&fg);
#endif
    assert(word(f,d,"sequence")==4 && word(f,d,"packed")==UINT32_C(0xDEADBEEF));
    assert(f->native_module_state[offset(d,"available")] && f->native_module_state[offset(d,"installed")]);
    assert(!memcmp(&f->x610_color_rgba[1],&(GXColor){1,2,3,4},4));
    assert(!memcmp(&fighter_diffuse,&(GXColor){254,1,2,255},4));
    assert(f->x2224_b0 && f->x34_scale.z==-2 && f->self_vel.y==0.75f);
    const NativeFighterArticleLogic* a=&d->article_logics[0];a->on_created(&ig);
    assert(!memcmp(&i->xBC8.x0_unk,&(GXColor){254,255,0,255},4));
    assert(!memcmp(&i->xBC4,&(GXColor){222,173,190,239},4));
    assert(!memcmp(&item_diffuse,&i->xBC8.x0_unk,4));
    assert(i->x5C8 && i->x5C9==255 && i->xDCF_flag.b3 && i->xDCD_flag.b3);
    assert(color_queries==4 && layer_calls==1 && disable_calls==1);
    a->logic->states[0].physics_updated(&ig);assert(i->x5C9==255 && !i->xDCF_flag.b3 && layer_calls==2);
    free(i);free(f);puts("Generated presentation callbacks preserve RGBA, evaluation order, typed IDs and bounded writes.");return 0;
}
