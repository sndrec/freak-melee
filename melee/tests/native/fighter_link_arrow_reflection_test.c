#include "arrow_reflection_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

static Item items[2];
static Fighter_GObj* fixture_creator(Item_GObj* gobj) { return gobj ? ((Item*)gobj->user_data)->xDD4_itemVar.native_article.attachment_owner : NULL; }
static HSD_GObj objects[2];
static HSD_JObj roots[2], trails[2][2];
static unsigned removed[2], removal_order[2], active, comparisons, failures;
static unsigned random_calls[2], effects[2], hides[2];
static unsigned damage_calls[2], damage_values[2];
static unsigned line_calls[2], speed_calls[2], normal_calls[2];
static bool line_present;
static Vec3 line_normal;
static HSD_GObj shield_objects[2];
static unsigned target_kind, lifetime_calls[2], motion_calls[2];
static bool target_guard;
static float target_scale, target_radius;
static Vec3 target_center;
static unsigned sweep_calls[2], sounds[2];
static bool sweep_hit;
static int random_integer;
static Fighter creators[2];
static HSD_GObj creator_objects[2], alternate_objects[2];
static bool callback_source_alive(HSD_GObj* object,uint32_t generation,uint32_t source_class)
{
    if(!object||!object->user_data)return false;
    for(unsigned i=0;i<2;++i) {
        if(source_class==NativeCallbackSource_Item && object==&objects[i])
            return object->user_data==&items[i] && items[i].x1C==generation;
        if(source_class==NativeCallbackSource_Fighter && object==&creator_objects[i])
            return object->user_data==&creators[i] &&
                (uint32_t)creators[i].x8_spawnNum==generation;
    }
    return false;
}
static unsigned interactions[2];
static HSD_Joint* trail_descriptors[2];
static unsigned loads[2], releases[2];
static unsigned joint_effects[2];
static float random_value;
void __assert(char* file, u32 line, char* message)
{ fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,message);abort(); }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void (HSD_JObjSetMtxDirty)(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void HSD_JObjSetFlagsAll(HSD_JObj* joint,u32 flags) { joint->flags|=flags; }
void HSD_JObjClearFlagsAll(HSD_JObj* joint,u32 flags) { joint->flags&=~flags; }
void it_80272460(HitCapsule* hit,u32 damage,Item_GObj* object)
{
    if(hit!=&((Item*)object->user_data)->x5D4_hitboxes[0].hit)abort();
    ++damage_calls[active];damage_values[active]=damage;hit->damage=damage;
}
bool mpLib_80054ED8(int line) { return line>=0&&line_present; }
void mpColl_80043558(CollData* data,int line) { (void)data;if(line!=7)abort();++line_calls[active]; }
bool mpGetSpeed(int line,Vec3* position,Vec3* velocity)
{ if(line!=7)abort();++speed_calls[active];position->x+=.25f;position->y-=.5f;*velocity=(Vec3){.125f,-.25f,0};return true; }
Vec3* mpLineGetNormal(int line,Vec3* out)
{ if(line!=7)abort();++normal_calls[active];*out=line_normal;return out; }
static uint32_t target_contact(Item_GObj* object)
{ return ((Item*)object->user_data)->xCF4_fighterGObjUnk!=NULL&&target_kind==1?99:0; }
static bool target_is_fighter(uint32_t target) { return target==99; }
static bool target_guard_active(uint32_t target) { return target==99&&target_guard; }
static float target_model_scale(uint32_t target) { if(target!=99)abort();return target_scale; }
static float target_shield(uint32_t target,Vec3* out)
{ if(target!=99)abort();*out=target_center;return target_radius; }
s32 it_80272D40(Item_GObj* object) { if(object!=&shield_objects[active])abort();return target_kind==1?0:1; }
float ftLib_800869D4(HSD_GObj* object)
{ if(object==&creator_objects[active])return creators[active].x34_scale.y;if(object!=&shield_objects[active])abort();return target_scale; }
bool ftLib_80086A18(HSD_GObj* object) { if(object!=&shield_objects[active])abort();return target_guard; }
float ftCo_80094098(HSD_GObj* object,float* center)
{ if(object!=&shield_objects[active])abort();memcpy(center,&target_center,12);return target_radius; }
void it_80275158(Item_GObj* object,float lifetime)
{ ++lifetime_calls[active];((Item*)object->user_data)->xD44_lifeTimer=lifetime; }
void Item_80268E5C(Item_GObj* object,enum_t state,Item_StateChangeFlags flags)
{ if(flags!=ITEM_ANIM_UPDATE)abort();++motion_calls[active];((Item*)object->user_data)->msid=state; }
s32 HSD_Randi(s32 upper) { if(upper!=4)abort();++random_calls[active];return random_integer; }
bool it_8026EA20(HSD_GObj* object,Vec3* previous,Vec3* position,Vec3* normal,int* line,u32* flags)
{ if(previous->x!=-.5f||position!=&((Item*)object->user_data)->pos)abort();++sweep_calls[active];*normal=line_normal;*line=7;*flags=0x42;return sweep_hit; }
void Item_8026AE84(Item* item,enum_t sound,u8 pan,u8 volume)
{ if(sound!=(item->kind==It_Kind_CLink_Arrow?70082:160082)||pan!=127||volume!=64)abort();++sounds[active]; }
static void play_sound(Item_GObj* object,s32 sound,u8 pan,u8 volume)
{ Item_8026AE84(object->user_data,sound,pan,volume); }
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj* object,...)
{
    va_list args;va_start(args,object);HSD_JObj* joint=va_arg(args,HSD_JObj*);Vec3* offset=va_arg(args,Vec3*);va_end(args);
    if(effect!=1096||joint!=object->hsd_obj||offset->x||offset->y||offset->z!=5)abort();
    ++joint_effects[active];return (EF_SpawnResult){ 0 };
}
bool ftLk_Init_BoomerangExists(Fighter_GObj* object) { (void)object;return false; }
bool ftKb_SpecialNLk800FB470(Fighter_GObj* object) { (void)object;abort(); }
void ftKb_SpecialNLk800FB418(Fighter_GObj* object) { (void)object;abort(); }
bool it_8026B894(Item_GObj* object,HSD_GObj* other)
{ (void)object;(void)other;++interactions[active];return false; }
Item_GObj* Item_80268B18(SpawnItem* spawn)
{
    Item* item=&items[active];item->kind=spawn->kind;item->pos=spawn->pos;
    item->owner=spawn->x0_parent_gobj;item->facing_dir=spawn->facing_dir;item->x40_vel=spawn->vel;
    if(spawn->prev_pos.z!=0||spawn->pos.z!=0||spawn->x4_parent_gobj2!=spawn->x0_parent_gobj||!spawn->x44_flag.b0)abort();
    return &objects[active];
}
void Item_8026AB54(Item_GObj* object,HSD_GObj* owner,Fighter_Part part)
{ if(object!=&objects[active]||owner!=&creator_objects[active]||part!=4)abort(); }
void db_80225DD8(Item_GObj* item,Fighter_GObj* owner)
{ if(item!=&objects[active]||owner!=&creator_objects[active])abort(); }
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* descriptor)
{
    if(descriptor!=trail_descriptors[0]&&descriptor!=trail_descriptors[1])abort();
    ++loads[active];return &trails[active][descriptor==trail_descriptors[1]];
}
HSD_JObj* ftLib_80086630(Fighter_GObj* owner,Fighter_Part part)
{ if(owner!=&creator_objects[active]||part!=4)abort();return &roots[active]; }
float ftLib_800865C0(HSD_GObj* owner)
{ if(owner!=&creator_objects[active])abort();return creators[active].facing_dir; }
void HSD_MtxGetRotation(Mtx matrix,Vec3* rotation) { (void)matrix;*rotation=(Vec3){0,0,0}; }
void it_8027429C(Item_GObj* object,Vec3* offset)
{ if(offset->x||offset->y||offset->z)abort();++releases[active];((Item*)object->user_data)->xDC8_word.flags.x13=false; }
void it_8026B3A8(Item_GObj* object) { ((Item*)object->user_data)->xDC8_word.flags.x15=false; }
void HSD_JObjRemoveAll(HSD_JObj* joint)
{
    if(joint!=&trails[active][0]&&joint!=&trails[active][1])abort();
    removal_order[active]=(removal_order[active]<<2)|(joint==&trails[active][0]?1:2);
    ++removed[active];
}
float HSD_Randf(void) { ++random_calls[active];return random_value; }
void it_80278800(Item_GObj* object,s32 effect,s32 bone,Vec3* offset,
                 Vec3* variance,s32 synchronous,float argument)
{
    (void)object;
    if(effect!=1057||bone!=0||synchronous!=1||argument!=0||
       offset->x||offset->y||offset->z||variance->x||variance->y||variance->z)abort();
    ++effects[active];
    /* The shared effect backend consumes three random samples at zero variance. */
    HSD_Randf();HSD_Randf();HSD_Randf();
}
void it_802756D0(Item_GObj* object) { ((Item*)object->user_data)->xD0C=2; }
void it_8026BB44(Item_GObj* object)
{ ++hides[active];((HSD_JObj*)object->hsd_obj)->flags|=JOBJ_HIDDEN; }
static uint32_t bits(float value) { uint32_t result;memcpy(&result,&value,4);return result; }
static void run(ItemKind kind,float angle,float facing,unsigned trail_mask,unsigned flags)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));memset(removed,0,sizeof(removed));
    memset(removal_order,0,sizeof(removal_order));
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        items[i].kind=kind;items[i].facing_dir=facing;
        items[i].pos=(Vec3){1.25f,-2.5f,9};items[i].x40_vel=(Vec3){.35f,-.75f,2};
        roots[i].flags=flags;roots[i].rotate=(Quaternion){.2f,.3f,.4f,.5f};
        for(unsigned slot=0;slot<2;++slot) {
            HSD_JObj* trail=(trail_mask&(1u<<slot))?&trails[i][slot]:NULL;
            if(i==0)items[i].xDD4_itemVar.linkarrow.xB4[slot]=trail;
            else items[i].native_models[slot]=trail;
        }
    }
    items[0].xDD4_itemVar.linkarrow.x94=angle;
    memcpy(items[1].xDD4_itemVar.native_article.state+20,&angle,4);
    active=0;bool stock=itLinkArrow_Logic98_Reflected(&objects[0]);
    active=1;bool script=scripted_article_1_logic_8(&objects[1]);
    float script_angle=scripted_article_state_float(&items[1],20);
    bool equal=stock==script&&removed[0]==removed[1]&&removal_order[0]==removal_order[1]&&
        items[0].xDD4_itemVar.linkarrow.xB4[0]==NULL&&
        items[0].xDD4_itemVar.linkarrow.xB4[1]==NULL&&
        items[1].native_models[0]==NULL&&items[1].native_models[1]==NULL&&
        bits(items[0].xDD4_itemVar.linkarrow.x94)==bits(script_angle)&&
        bits(items[0].facing_dir)==bits(items[1].facing_dir)&&
        memcmp(&items[0].pos,&items[1].pos,sizeof(Vec3))==0&&
        memcmp(&items[0].x40_vel,&items[1].x40_vel,sizeof(Vec3))==0&&
        memcmp(&roots[0].translate,&roots[1].translate,sizeof(Vec3))==0&&
        memcmp(&roots[0].rotate,&roots[1].rotate,sizeof(Quaternion))==0&&
        roots[0].flags==roots[1].flags;
    ++comparisons;
    if(!equal) {
        ++failures;
        if(failures<=8)fprintf(stderr,"Arrow reflection mismatch input=%08x result=%08x/%08x rootZ=%08x/%08x\n",
            bits(angle),bits(items[0].xDD4_itemVar.linkarrow.x94),bits(script_angle),
            bits(roots[0].rotate.z),bits(roots[1].rotate.z));
    }
}
static void vibration(int index,float random,int tail,float lifetime,unsigned mask)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));
    memset(removed,0,sizeof(removed));memset(removal_order,0,sizeof(removal_order));
    memset(random_calls,0,sizeof(random_calls));memset(effects,0,sizeof(effects));
    memset(hides,0,sizeof(hides));random_value=random;
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        items[i].xD44_lifeTimer=lifetime;
        for(unsigned slot=0;slot<2;++slot) {
            HSD_JObj* trail=(mask&(1u<<slot))?&trails[i][slot]:NULL;
            if(i==0)items[i].xDD4_itemVar.linkarrow.xB4[slot]=trail;
            else items[i].native_models[slot]=trail;
        }
    }
    items[0].xDD4_itemVar.linkarrow.x94=.75f;
    items[0].xDD4_itemVar.linkarrow.x9C=index;items[0].xDD4_itemVar.linkarrow.xF0=tail;
    float angle=.75f;memcpy(items[1].xDD4_itemVar.native_article.state+20,&angle,4);
    memcpy(items[1].xDD4_itemVar.native_article.state+36,&index,4);
    memcpy(items[1].xDD4_itemVar.native_article.state+44,&tail,4);
    active=0;bool stock=itLinkarrow_UnkMotion4_Anim(&objects[0]);
    active=1;bool script=scripted_article_1_state_4_0(&objects[1]);
    bool equal=stock==script&&bits(roots[0].rotate.z)==bits(roots[1].rotate.z)&&
        roots[0].flags==roots[1].flags&&
        items[0].xDD4_itemVar.linkarrow.x9C==(uint32_t)scripted_article_state_int(&items[1],36)&&
        items[0].xDD4_itemVar.linkarrow.xF0==scripted_article_state_int(&items[1],44)&&
        items[0].xD44_lifeTimer==items[1].xD44_lifeTimer&&
        items[0].xDCF_flag.b2==items[1].xDCF_flag.b2&&items[0].xD0C==items[1].xD0C&&
        random_calls[0]==random_calls[1]&&effects[0]==effects[1]&&hides[0]==hides[1]&&
        removed[0]==removed[1]&&removal_order[0]==removal_order[1];
    ++comparisons;
    if(!equal) {
        ++failures;
        if(failures<=8)fprintf(stderr,"Arrow vibration mismatch index=%d tail=%d life=%g random=%g angle=%08x/%08x result=%d/%d\n",
            index,tail,lifetime,random,bits(roots[0].rotate.z),bits(roots[1].rotate.z),stock,script);
    }
}
static void flight_physics(float gravity,float vertical)
{
    memset(items,0,sizeof(items));
    itLinkArrowAttributes attributes={.x1C=gravity};
    Article article={.x4_specialAttributes=&attributes};
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&items[i]};items[i].entity=&objects[i];items[i].pos=(Vec3){1,2,3};
        items[i].x40_vel=(Vec3){4,vertical,6};items[i].xC4_article_data=&article;
    }
    script_parameters[32].f=gravity;
    itLinkarrow_UnkMotion1_Phys(&objects[0]);scripted_article_1_state_1_1(&objects[1]);
    bool equal=memcmp(&items[0].x40_vel,&items[1].x40_vel,sizeof(Vec3))==0&&
        bits(items[0].xDD4_itemVar.linkarrow.x18.x)==bits(scripted_article_state_float(&items[1],8))&&
        bits(items[0].xDD4_itemVar.linkarrow.x18.y)==bits(scripted_article_state_float(&items[1],12))&&
        bits(items[0].xDD4_itemVar.linkarrow.x18.z)==bits(scripted_article_state_float(&items[1],16));
    ++comparisons;
    if(!equal) {
        ++failures;
        if(failures<=8)fprintf(stderr,"Arrow physics mismatch gravity=%08x incomingY=%08x result=%08x/%08x\n",
            bits(gravity),bits(vertical),bits(items[0].x40_vel.y),bits(items[1].x40_vel.y));
    }
}
static void flight_animation(int age,int command,float scale,float lifetime,unsigned mask,unsigned flags)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));memset(trails,0,sizeof(trails));
    memset(removed,0,sizeof(removed));memset(removal_order,0,sizeof(removal_order));
    memset(damage_calls,0,sizeof(damage_calls));memset(damage_values,0,sizeof(damage_values));
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        items[i].xD44_lifeTimer=lifetime;items[i].xDB4_itcmd_var2=command;
        items[i].xDAC_itcmd_var0=command;items[i].xDB0_itcmd_var1=command;
        roots[i].flags=flags;items[i].native_model_draw_disabled_mask=3;
        for(unsigned slot=0;slot<2;++slot) {
            trails[i][slot].flags=flags|JOBJ_HIDDEN;
            HSD_JObj* trail=(mask&(1u<<slot))?&trails[i][slot]:NULL;
            if(i==0)items[i].xDD4_itemVar.linkarrow.xB4[slot]=trail;
            else items[i].native_models[slot]=trail;
        }
    }
    items[0].xDD4_itemVar.linkarrow.xC0=scale;items[0].xDD4_itemVar.linkarrow.xA4=17;
    items[0].xDD4_itemVar.linkarrow.xB0=age;
    memcpy(items[1].xDD4_itemVar.native_article.state+4,&scale,4);
    int damage=17;memcpy(items[1].xDD4_itemVar.native_article.state+32,&damage,4);
    memcpy(items[1].xDD4_itemVar.native_article.state+40,&age,4);
    for(unsigned slot=0;slot<2;++slot) {
        Vec3 position={1.25f+slot,-2.5f-slot,4.75f};Quaternion rotation={.1f,.2f,.3f,.4f};
        (&items[0].xDD4_itemVar.linkarrow.x30)[slot]=position;
        (&items[0].xDD4_itemVar.linkarrow.x64)[slot]=rotation;
        memcpy(items[1].xDD4_itemVar.native_article.state+136+28*slot,&position,12);
        memcpy(items[1].xDD4_itemVar.native_article.state+148+28*slot,&rotation,16);
    }
    active=0;bool stock=itLinkarrow_UnkMotion1_Anim(&objects[0]);
    active=1;bool script=scripted_article_1_state_1_0(&objects[1]);
    bool equal=stock==script&&items[0].xD44_lifeTimer==items[1].xD44_lifeTimer&&
        damage_calls[0]==damage_calls[1]&&damage_values[0]==damage_values[1]&&
        items[0].xDAC_itcmd_var0==items[1].xDAC_itcmd_var0&&
        items[0].xDB0_itcmd_var1==items[1].xDB0_itcmd_var1&&
        items[0].xDB4_itcmd_var2==items[1].xDB4_itcmd_var2&&
        items[0].xDD4_itemVar.linkarrow.xB0==scripted_article_state_int(&items[1],40)&&
        removed[0]==removed[1]&&removal_order[0]==removal_order[1]&&
        memcmp(&roots[0].scale,&roots[1].scale,sizeof(Vec3))==0&&roots[0].flags==roots[1].flags;
    for(unsigned slot=0;slot<2;++slot) {
        equal=equal&&memcmp(&trails[0][slot].translate,&trails[1][slot].translate,sizeof(Vec3))==0&&
            memcmp(&trails[0][slot].rotate,&trails[1][slot].rotate,sizeof(Quaternion))==0&&
            memcmp(&trails[0][slot].scale,&trails[1][slot].scale,sizeof(Vec3))==0&&
            trails[0][slot].flags==trails[1][slot].flags;
        if((mask&(1u<<slot))&&((slot?items[1].xDB0_itcmd_var1:items[1].xDAC_itcmd_var0)==1))
            equal=equal&&!(items[1].native_model_draw_disabled_mask&(1u<<slot));
    }
    ++comparisons;
    if(!equal) {++failures;if(failures<=8)fprintf(stderr,"Arrow flight animation mismatch age=%d command=%d scale=%g life=%g mask=%u flags=%u\n",age,command,scale,lifetime,mask,flags);}
}
static void history(unsigned mask,unsigned flags)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));memset(trails,0,sizeof(trails));
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        roots[i].flags=flags;roots[i].scale=(Vec3){.5f,1.25f,2};
        roots[i].rotate=(Quaternion){-.125f,.25f,-.5f,1};
        for(unsigned slot=0;slot<2;++slot) {
            trails[i][slot].flags=flags;
            HSD_JObj* trail=(mask&(1u<<slot))?&trails[i][slot]:NULL;
            if(i==0)items[i].xDD4_itemVar.linkarrow.xB4[slot]=trail;
            else items[i].native_models[slot]=trail;
        }
    }
    Vec3 previous={.125f,-.5f,1};items[0].xDD4_itemVar.linkarrow.x18=previous;
    memcpy(items[1].xDD4_itemVar.native_article.state+8,&previous,12);
    for(unsigned index=0;index<4;++index) {
        Vec3 position={2+index,3+index,4+index};Quaternion rotation={5+index,6+index,7+index,8+index};
        (&items[0].xDD4_itemVar.linkarrow.x24)[index]=position;
        (&items[0].xDD4_itemVar.linkarrow.x54)[index]=rotation;
        memcpy(items[1].xDD4_itemVar.native_article.state+108+28*index,&position,12);
        memcpy(items[1].xDD4_itemVar.native_article.state+120+28*index,&rotation,16);
    }
    active=0;it_802A7E40(&objects[0]);active=1;scripted_article_1_signal_1(&objects[1],0);
    bool equal=true;
    for(unsigned index=0;index<4;++index) {
        equal=equal&&memcmp(&(&items[0].xDD4_itemVar.linkarrow.x24)[index],items[1].xDD4_itemVar.native_article.state+108+28*index,12)==0&&
            memcmp(&(&items[0].xDD4_itemVar.linkarrow.x54)[index],items[1].xDD4_itemVar.native_article.state+120+28*index,16)==0;
    }
    for(unsigned slot=0;slot<2;++slot)
        equal=equal&&memcmp(&trails[0][slot].translate,&trails[1][slot].translate,12)==0&&
            memcmp(&trails[0][slot].rotate,&trails[1][slot].rotate,16)==0&&
            memcmp(&trails[0][slot].scale,&trails[1][slot].scale,12)==0&&trails[0][slot].flags==trails[1][slot].flags;
    ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow history mismatch mask=%u flags=%u\n",mask,flags);}
}
static void embedded_collision(int line,bool present,Vec3 normal,float previous,unsigned mask,unsigned flags)
{
    history(mask,flags);
    memset(line_calls,0,sizeof(line_calls));memset(speed_calls,0,sizeof(speed_calls));memset(normal_calls,0,sizeof(normal_calls));
    line_present=present;line_normal=normal;
    for(unsigned i=0;i<2;++i) { items[i].pos=(Vec3){3,4,5};items[i].x40_vel=(Vec3){6,7,8}; }
    items[0].xDD4_itemVar.linkarrow.xE4=line;items[0].xDD4_itemVar.linkarrow.xE8=previous;
    items[0].xDD4_itemVar.linkarrow.x94=.5f;
    float angle=.5f;
    memcpy(items[1].xDD4_itemVar.native_article.state+48,&line,4);
    memcpy(items[1].xDD4_itemVar.native_article.state+72,&previous,4);
    memcpy(items[1].xDD4_itemVar.native_article.state+20,&angle,4);
    active=0;bool stock=itLinkarrow_UnkMotion4_Coll(&objects[0]);
    active=1;bool script=scripted_article_1_state_4_2(&objects[1]);
    bool equal=stock==script&&memcmp(&items[0].pos,&items[1].pos,12)==0&&memcmp(&items[0].x40_vel,&items[1].x40_vel,12)==0&&
        bits(items[0].xDD4_itemVar.linkarrow.x94)==bits(scripted_article_state_float(&items[1],20))&&
        bits(items[0].xDD4_itemVar.linkarrow.xE8)==bits(scripted_article_state_float(&items[1],72))&&
        bits(items[0].xDD4_itemVar.linkarrow.xEC)==bits(scripted_article_state_float(&items[1],76))&&
        memcmp(&roots[0].rotate,&roots[1].rotate,16)==0&&roots[0].flags==roots[1].flags&&
        line_calls[0]==line_calls[1]&&speed_calls[0]==speed_calls[1]&&normal_calls[0]==normal_calls[1];
    ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow embedded collision mismatch line=%d present=%d previous=%g mask=%u\n",line,present,previous,mask);}
}
static void contact(bool clank,float multiplier,float velocity,unsigned mask)
{
    memset(items,0,sizeof(items));memset(removed,0,sizeof(removed));memset(removal_order,0,sizeof(removal_order));
    itLinkArrowAttributes attributes={.x14=multiplier};Article article={.x4_specialAttributes=&attributes};
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&items[i]};items[i].entity=&objects[i];items[i].xC4_article_data=&article;
        items[i].x40_vel=(Vec3){velocity,-.5f,1};
        for(unsigned slot=0;slot<2;++slot) {
            HSD_JObj* trail=(mask&(1u<<slot))?&trails[i][slot]:NULL;
            if(i==0)items[i].xDD4_itemVar.linkarrow.xB4[slot]=trail;
            else items[i].native_models[slot]=trail;
        }
    }
    script_parameters[30].f=multiplier;
    active=0;bool stock=clank?itLinkArrow_Logic98_Clanked(&objects[0]):itLinkArrow_Logic98_DmgDealt(&objects[0]);
    active=1;bool script=clank?scripted_article_1_logic_9(&objects[1]):scripted_article_1_logic_5(&objects[1]);
    bool equal=stock==script&&memcmp(&items[0].x40_vel,&items[1].x40_vel,12)==0&&removed[0]==removed[1]&&removal_order[0]==removal_order[1];
    ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow contact mismatch\n");}
}
static void flight_collision(bool hit,bool exists,float facing,Vec3 position,float limit,unsigned mask,unsigned flags)
{
    history(mask,flags);memset(removed,0,sizeof(removed));memset(removal_order,0,sizeof(removal_order));
    memset(line_calls,0,sizeof(line_calls));memset(speed_calls,0,sizeof(speed_calls));memset(sweep_calls,0,sizeof(sweep_calls));
    memset(random_calls,0,sizeof(random_calls));memset(sounds,0,sizeof(sounds));memset(motion_calls,0,sizeof(motion_calls));
    memset(lifetime_calls,0,sizeof(lifetime_calls));sweep_hit=hit;line_present=exists;line_normal=(Vec3){-.25f,.75f,0};random_integer=2;
    itLinkArrowAttributes attributes={.x18=9.5f,.x20=limit};Article article={.x4_specialAttributes=&attributes};
    Vec3 previous={-.5f,.75f,0};float creator_scale=1.25f;
    for(unsigned i=0;i<2;++i) {
        items[i].kind=It_Kind_Link_Arrow;items[i].xC4_article_data=&article;items[i].facing_dir=facing;
        items[i].pos=position;items[i].x40_vel=(Vec3){6,7,8};
    }
    items[0].xDD4_itemVar.linkarrow.xC0=creator_scale;items[0].xDD4_itemVar.linkarrow.x18=previous;
    memcpy(items[1].xDD4_itemVar.native_article.state+4,&creator_scale,4);
    memcpy(items[1].xDD4_itemVar.native_article.state+8,&previous,12);
    script_parameters[31].f=9.5f;script_parameters[33].f=limit;
    active=0;bool stock=itLinkarrow_UnkMotion1_Coll(&objects[0]);
    active=1;bool script=scripted_article_1_state_1_2(&objects[1]);
    bool equal=stock==script&&memcmp(&items[0].pos,&items[1].pos,12)==0&&memcmp(&items[0].x40_vel,&items[1].x40_vel,12)==0&&
        bits(items[0].xDD4_itemVar.linkarrow.x94)==bits(scripted_article_state_float(&items[1],20))&&
        roots[0].flags==roots[1].flags&&memcmp(&roots[0].rotate,&roots[1].rotate,16)==0&&
        removed[0]==removed[1]&&removal_order[0]==removal_order[1]&&random_calls[0]==random_calls[1]&&
        sounds[0]==sounds[1]&&motion_calls[0]==motion_calls[1]&&lifetime_calls[0]==lifetime_calls[1]&&
        line_calls[0]==line_calls[1]&&speed_calls[0]==speed_calls[1]&&sweep_calls[0]==sweep_calls[1];
    for(unsigned index=0;index<4;++index)
        equal=equal&&memcmp(&(&items[0].xDD4_itemVar.linkarrow.x24)[index],items[1].xDD4_itemVar.native_article.state+108+28*index,12)==0&&
            memcmp(&(&items[0].xDD4_itemVar.linkarrow.x54)[index],items[1].xDD4_itemVar.native_article.state+120+28*index,16)==0;
    if(hit&&exists)equal=equal&&items[0].xDD4_itemVar.linkarrow.x9C==(u32)scripted_article_state_int(&items[1],36)&&
        items[0].xDD4_itemVar.linkarrow.xF0==scripted_article_state_int(&items[1],44)&&items[0].xD44_lifeTimer==items[1].xD44_lifeTimer;
    ++comparisons;if(!equal){++failures;if(failures<9)fprintf(stderr,"Arrow flight collision mismatch hit=%d exists=%d facing=%g limit=%g\n",hit,exists,facing,limit);}
}
static void shield(unsigned kind,bool guard,float scale,float radius,float lifetime,unsigned mask)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));
    memset(removed,0,sizeof(removed));memset(removal_order,0,sizeof(removal_order));
    memset(lifetime_calls,0,sizeof(lifetime_calls));memset(motion_calls,0,sizeof(motion_calls));
    target_kind=kind;target_guard=guard;target_scale=scale;target_radius=radius;
    target_center=(Vec3){.75f,-.5f,2};
    itLinkArrowAttributes attributes={.x18=lifetime};Article article={.x4_specialAttributes=&attributes};
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        items[i].xC4_article_data=&article;items[i].pos=(Vec3){1.25f,-2.5f,3};
        items[i].x40_vel=(Vec3){4,5,6};items[i].xCF4_fighterGObjUnk=kind?&shield_objects[i]:NULL;
        for(unsigned slot=0;slot<2;++slot) {
            HSD_JObj* trail=(mask&(1u<<slot))?&trails[i][slot]:NULL;
            if(i==0)items[i].xDD4_itemVar.linkarrow.xB4[slot]=trail;
            else items[i].native_models[slot]=trail;
        }
    }
    float creator_scale=1.5f;Vec3 previous={-.5f,.75f,1};
    items[0].xDD4_itemVar.linkarrow.xC0=creator_scale;items[0].xDD4_itemVar.linkarrow.x18=previous;
    memcpy(items[1].xDD4_itemVar.native_article.state+4,&creator_scale,4);
    memcpy(items[1].xDD4_itemVar.native_article.state+8,&previous,12);script_parameters[31].f=lifetime;
    active=0;bool stock=itLinkArrow_Logic98_HitShield(&objects[0]);
    active=1;bool script=scripted_article_1_logic_12(&objects[1]);
    bool equal=stock==script&&memcmp(&items[0].pos,&items[1].pos,12)==0&&memcmp(&items[0].x40_vel,&items[1].x40_vel,12)==0&&
        lifetime_calls[0]==lifetime_calls[1]&&motion_calls[0]==motion_calls[1]&&items[0].msid==items[1].msid&&
        items[0].xD44_lifeTimer==items[1].xD44_lifeTimer&&roots[0].flags==roots[1].flags&&
        memcmp(&roots[0].scale,&roots[1].scale,12)==0&&removed[0]==removed[1]&&removal_order[0]==removal_order[1];
    if(kind==1)equal=equal&&bits(items[0].xDD4_itemVar.linkarrow.xD4)==bits(scripted_article_state_float(&items[1],96))&&
        bits(items[0].xDD4_itemVar.linkarrow.xD8)==bits(scripted_article_state_float(&items[1],100));
    ++comparisons;if(!equal){++failures;if(failures<9)fprintf(stderr,"Arrow shield impact mismatch type=%u scale=%g radius=%g\n",kind,scale,radius);}
    if(kind==1) {
        active=0;stock=itLinkarrow_UnkMotion2_Anim(&objects[0]);
        active=1;script=scripted_article_1_state_2_0(&objects[1]);
        equal=stock==script&&items[0].xD44_lifeTimer==items[1].xD44_lifeTimer&&removed[0]==removed[1]&&removal_order[0]==removal_order[1];
        ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow shield animation mismatch\n");}
        target_center.x+=.25f;target_center.y-=.5f;target_scale*=2;
        active=0;itLinkarrow_UnkMotion2_Phys(&objects[0]);
        active=1;scripted_article_1_state_2_1(&objects[1]);
        equal=memcmp(&items[0].pos,&items[1].pos,12)==0&&
            bits(items[0].xDD4_itemVar.linkarrow.xD4)==bits(scripted_article_state_float(&items[1],96));
        ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow shield physics mismatch\n");}
    }
}
static void lifecycle(ItemKind kind,int motion,unsigned ownership,bool detaches,bool launched,unsigned mask)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));memset(creators,0,sizeof(creators));
    memset(removed,0,sizeof(removed));memset(removal_order,0,sizeof(removal_order));
    for(unsigned i=0;i<2;++i) {
        creator_objects[i]=(HSD_GObj){.user_data=&creators[i]};items[i].entity=&objects[i];
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        creators[i].motion_id=motion;creators[i].x2070.x2071_b6=detaches;
        creators[i].u.lk.arrow_gobj=&objects[i];int handle=123;memcpy(creators[i].native_module_state+36,&handle,4);
        items[i].kind=kind;items[i].owner=ownership==2?&alternate_objects[i]:ownership==3?NULL:&creator_objects[i];
        items[i].xDC8_word.flags.x13=true;
        for(unsigned slot=0;slot<2;++slot) {
            HSD_JObj* trail=(mask&(1u<<slot))?&trails[i][slot]:NULL;
            if(i==0)items[i].xDD4_itemVar.linkarrow.xB4[slot]=trail;
            else items[i].native_models[slot]=trail;
        }
    }
    float scale=1.25f;
    items[0].xDD4_itemVar.linkarrow.xC0=scale;items[0].xDD4_itemVar.linkarrow.xA0=launched;
    items[0].xDD4_itemVar.linkarrow.xE0=ownership==1?NULL:&creator_objects[0];
    items[1].xDD4_itemVar.native_article.attachment_owner=ownership==1?NULL:&creator_objects[1];
    scripted_article_set_state_float(&items[1],4,scale);scripted_article_set_state_bool(&items[1],0,launched);
    active=0;bool stock=itLinkarrow_UnkMotion0_Anim(&objects[0]);
    active=1;bool script=scripted_article_1_state_0_0(&objects[1]);
    bool equal=stock==script&&memcmp(&roots[0].scale,&roots[1].scale,12)==0&&roots[0].flags==roots[1].flags&&
        (items[0].xDD4_itemVar.linkarrow.xE0!=NULL)==(items[1].xDD4_itemVar.native_article.attachment_owner!=NULL);
    ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow held mismatch\n");}
    active=0;itLinkArrow_Logic98_Destroyed(&objects[0]);active=1;scripted_article_1_logic_1(&objects[1]);
    int handle;memcpy(&handle,creators[1].native_module_state+36,4);
    equal=items[0].owner==NULL&&items[1].owner==NULL&&items[0].xDD4_itemVar.linkarrow.xE0==NULL&&
        items[1].xDD4_itemVar.native_article.attachment_owner==NULL&&
        items[0].xDC8_word.flags.x13==items[1].xDC8_word.flags.x13&&
        (creators[0].u.lk.arrow_gobj!=NULL)==(handle>=0)&&removed[0]==removed[1]&&removal_order[0]==removal_order[1];
    ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow destroyed mismatch\n");}
    memset(interactions,0,sizeof(interactions));
    active=0;itLinkArrow_Logic98_EvtUnk(&objects[0],&creator_objects[0]);
    active=1;scripted_article_1_logic_13(&objects[1],&creator_objects[1]);
    ++comparisons;if(interactions[0]!=interactions[1])++failures;
    memset(motion_calls,0,sizeof(motion_calls));
    active=0;itLinkArrow_Logic98_PickedUp(&objects[0]);active=1;scripted_article_1_logic_2(&objects[1]);
    ++comparisons;if(items[0].msid!=items[1].msid||motion_calls[0]!=motion_calls[1])++failures;
}
static void created(ItemKind kind,float scale,float lifetime)
{
    memset(items,0,sizeof(items));memset(creators,0,sizeof(creators));
    memset(lifetime_calls,0,sizeof(lifetime_calls));
    itLinkArrowAttributes attributes={.x0=lifetime};Article article={.x4_specialAttributes=&attributes};
    for(unsigned i=0;i<2;++i) {
        creator_objects[i]=(HSD_GObj){.user_data=&creators[i]};items[i].entity=&objects[i];objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        creators[i].x34_scale.y=scale;items[i].xC4_article_data=&article;
        items[i].xDAC_itcmd_var0=3;items[i].xDB0_itcmd_var1=3;items[i].xDB4_itcmd_var2=3;items[i].xDB8_itcmd_var3=3;
    }
    active=0;Vec3 position={1.25f,2.5f,7};HSD_GObj* stock=it_802A83E0(-1,&creator_objects[0],&position,4,kind);
    items[1].owner=&creator_objects[1];items[1].xDD4_itemVar.native_article.attachment_owner=&creator_objects[1];
    active=1;script_parameters[25].f=lifetime;scripted_article_1_logic_16(&objects[1]);
    bool equal=stock==&objects[0]&&items[0].xDAC_itcmd_var0==items[1].xDAC_itcmd_var0&&
        items[0].xDB0_itcmd_var1==items[1].xDB0_itcmd_var1&&items[0].xDB4_itcmd_var2==items[1].xDB4_itcmd_var2&&
        items[0].xDB8_itcmd_var3==items[1].xDB8_itcmd_var3&&items[0].xD44_lifeTimer==items[1].xD44_lifeTimer&&
        lifetime_calls[0]==lifetime_calls[1]&&bits(items[0].xDD4_itemVar.linkarrow.xC0)==bits(scripted_article_state_float(&items[1],4))&&
        items[0].xDD4_itemVar.linkarrow.xE4==scripted_article_state_int(&items[1],48)&&
        items[0].xDD4_itemVar.linkarrow.xB0==scripted_article_state_int(&items[1],40)&&
        items[0].xDD4_itemVar.linkarrow.xA0==scripted_article_state_bool(&items[1],0);
    ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow created mismatch\n");}
}
static void launch(float facing,float charge,float maximum,unsigned flags)
{
    memset(items,0,sizeof(items));memset(creators,0,sizeof(creators));memset(roots,0,sizeof(roots));memset(trails,0,sizeof(trails));
    memset(loads,0,sizeof(loads));memset(releases,0,sizeof(releases));memset(sweep_calls,0,sizeof(sweep_calls));
    memset(lifetime_calls,0,sizeof(lifetime_calls));memset(motion_calls,0,sizeof(motion_calls));
    /* Immutable descriptors are only identity tokens to this external loader. */
    trail_descriptors[0]=(HSD_Joint*)&trails[0][0];trail_descriptors[1]=(HSD_Joint*)&trails[0][1];
    itLinkArrowAttributes attributes={.x0=60,.x4=.75f,.x8=3.5f,.xC=2,.x10=17,.x18=20,
        .x24=trail_descriptors[0],.x28=trail_descriptors[1]};
    Article article={.x4_specialAttributes=&attributes,.native_model_root_count=2};
    article.native_model_roots[0].name_hash=336011530;article.native_model_roots[0].root=trail_descriptors[0];
    article.native_model_roots[1].name_hash=3991491936;article.native_model_roots[1].root=trail_descriptors[1];
    Vec3 position={1.25f,-2.5f,0},previous={-.5f,.75f,0};float angle=MTXDegToRad(5);
    for(unsigned i=0;i<2;++i) {
        creator_objects[i]=(HSD_GObj){.user_data=&creators[i]};items[i].entity=&objects[i];objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
        creators[i].facing_dir=facing;items[i].owner=&creator_objects[i];items[i].xC4_article_data=&article;
        roots[i].flags=flags;roots[i].rotate=(Quaternion){.125f,.25f,.5f,1};
        items[i].xDC4=4;items[i].xDC8_word.flags.x13=true;items[i].xDC8_word.flags.x14=true;items[i].xDC8_word.flags.x15=true;
    }
    items[0].xDD4_itemVar.linkarrow.xE0=&creator_objects[0];items[1].xDD4_itemVar.native_article.attachment_owner=&creator_objects[1];
    memcpy(creators[1].native_module_state+24,&charge,4);memcpy(creators[1].native_module_state+40,&previous,12);
    memcpy(creators[1].native_module_state+52,&position,12);
    script_parameters[25].f=60;script_parameters[26].f=.75f;script_parameters[27].f=3.5f;
    script_parameters[28].f=2;script_parameters[29].f=17;script_parameters[31].f=20;
    sweep_hit=true;line_present=true;line_normal=(Vec3){0,1,0};
    active=0;bool stock=itLinkArrow_802A850C(&objects[0],&position,&previous,angle,charge,maximum);
    active=1;scripted_article_1_signal_5(&objects[1],maximum);
    bool equal=stock==scripted_article_state_bool(&items[1],56)&&
        memcmp(&items[0].pos,&items[1].pos,12)==0&&memcmp(&items[0].x40_vel,&items[1].x40_vel,12)==0&&
        memcmp(&roots[0].rotate,&roots[1].rotate,16)==0&&roots[0].flags==roots[1].flags&&
        bits(items[0].facing_dir)==bits(items[1].facing_dir)&&
        bits(items[0].xDD4_itemVar.linkarrow.xA8)==bits(scripted_article_state_float(&items[1],24))&&
        items[0].xDD4_itemVar.linkarrow.xA4==(u32)scripted_article_state_int(&items[1],32)&&
        bits(items[0].xDD4_itemVar.linkarrow.xAC)==bits(scripted_article_state_float(&items[1],28))&&
        items[0].xDD4_itemVar.linkarrow.xA0==scripted_article_state_bool(&items[1],0)&&
        loads[0]==loads[1]&&releases[0]==releases[1]&&sweep_calls[0]==sweep_calls[1]&&
        lifetime_calls[0]==lifetime_calls[1]&&motion_calls[0]==motion_calls[1]&&items[0].msid==items[1].msid&&
        items[0].xDC8_word.flags.x14==items[1].xDC8_word.flags.x14&&items[0].xDC8_word.flags.x15==items[1].xDC8_word.flags.x15;
    for(unsigned index=0;index<4;++index)
        equal=equal&&memcmp(&(&items[0].xDD4_itemVar.linkarrow.x24)[index],items[1].xDD4_itemVar.native_article.state+108+28*index,12)==0&&
            memcmp(&(&items[0].xDD4_itemVar.linkarrow.x54)[index],items[1].xDD4_itemVar.native_article.state+120+28*index,16)==0;
    ++comparisons;if(!equal){++failures;if(failures<9)fprintf(stderr,"Arrow launch mismatch charge=%g maximum=%g facing=%g\n",charge,maximum,facing);}
}
static void embed(ItemKind kind,int random,unsigned mask,unsigned flags)
{
    history(mask,flags);memset(removed,0,sizeof(removed));memset(removal_order,0,sizeof(removal_order));
    memset(joint_effects,0,sizeof(joint_effects));memset(random_calls,0,sizeof(random_calls));
    memset(sounds,0,sizeof(sounds));memset(lifetime_calls,0,sizeof(lifetime_calls));memset(motion_calls,0,sizeof(motion_calls));
    itLinkArrowAttributes attributes={.x18=20};Article article={.x4_specialAttributes=&attributes};
    float scale=1.5f,angle=.125f;random_integer=random;script_parameters[31].f=20;
    for(unsigned i=0;i<2;++i){items[i].kind=kind;items[i].xC4_article_data=&article;}
    items[0].xDD4_itemVar.linkarrow.xC0=scale;items[0].xDD4_itemVar.linkarrow.x94=angle;
    scripted_article_set_state_float(&items[1],4,scale);scripted_article_set_state_float(&items[1],20,angle);
    active=0;it_802A9458(&objects[0]);active=1;
    if(kind==It_Kind_CLink_Arrow)scripted_clink_embed(&objects[1],0);else scripted_article_1_signal_6(&objects[1],0);
    bool equal=memcmp(&roots[0].scale,&roots[1].scale,12)==0&&memcmp(&roots[0].rotate,&roots[1].rotate,16)==0&&roots[0].flags==roots[1].flags&&
        items[0].xDD4_itemVar.linkarrow.x9C==(u32)scripted_article_state_int(&items[1],36)&&
        items[0].xDD4_itemVar.linkarrow.xF0==scripted_article_state_int(&items[1],44)&&items[0].msid==items[1].msid&&
        items[0].xD44_lifeTimer==items[1].xD44_lifeTimer&&removed[0]==removed[1]&&removal_order[0]==removal_order[1]&&
        joint_effects[0]==joint_effects[1]&&random_calls[0]==random_calls[1]&&sounds[0]==sounds[1]&&
        lifetime_calls[0]==lifetime_calls[1]&&motion_calls[0]==motion_calls[1];
    ++comparisons;if(!equal){++failures;fprintf(stderr,"Arrow embed mismatch kind=%d rng=%d\n",kind,random);}
}
int main(void)
{
    NativeFighterHost host={.callback_source_alive=callback_source_alive,.item_creator=fixture_creator,.item_set_joint_value=nativeArticleModel_Write,
        .item_destroy_model=nativeArticleModel_Destroy,.random_float=HSD_Randf,
        .item_expire_tick=it_802751D8,.item_spawn_effect=it_80278800,
        .item_hide_model=it_8026BB44,.abs_float=fabsf,
        .item_joint_value=nativeArticleModel_Read,.item_set_joint_transform=nativeArticleModel_SetTransform,
        .item_set_uniform_scale=item_set_uniform_scale,.item_set_hitbox_damage=it_80272460};engine=&host;
    host.stage_line_exists=mpLib_80054ED8;host.set_collision_line=mpColl_80043558;
    host.stage_line_speed=mpGetSpeed;host.stage_line_normal=mpLineGetNormal;host.angle_atan2_float=atan2f;
    host.item_contact_fighter=target_contact;host.target_is_fighter=target_is_fighter;
    host.target_guard_active=target_guard_active;host.target_model_scale=target_model_scale;
    host.target_shield=target_shield;host.item_set_lifetime=it_80275158;
    host.item_change_state_flags=Item_80268E5C;host.cos_float=cosf;host.sin_float=sinf;
    host.item_segment_sweep=it_8026EA20;host.random_int=HSD_Randi;host.item_play_sound=play_sound;
    host.item_interact=it_8026B894;host.fighter_model_scale=ftLib_800869D4;
    host.item_load_model=nativeArticleModel_Load;host.item_release_attachment=it_8027429C;host.item_use_manual_lifetime=it_8026B3A8;
    host.item_spawn_joint_effect_at=nativeArticleModel_SpawnEffectAt;
    const float boundary[]={0,.1f,-.1f,.6981317f,-.6981317f,
        (float)M_PI,-(float)M_PI,18,-18};
    const unsigned flags[]={0,JOBJ_MTX_DIRTY,JOBJ_MTX_INDEP_SRT};
    for(unsigned family=0;family<2;++family)
    for(unsigned i=0;i<sizeof(boundary)/sizeof(boundary[0]);++i)
    for(unsigned face=0;face<2;++face)
    for(unsigned mask=0;mask<4;++mask)
    for(unsigned f=0;f<3;++f)run(family?It_Kind_CLink_Arrow:It_Kind_Link_Arrow,
        boundary[i],face?-1:1,mask,flags[f]);
    const float randoms[]={0,.25f,.75f,1};const int tails[]={0,1,299,300};
    const float lifetimes[]={0,1,1.25f,2,60};
    for(int index=-1;index<=7;++index)for(unsigned r=0;r<4;++r)
    for(unsigned tail=0;tail<4;++tail)for(unsigned life=0;life<5;++life)
    for(unsigned mask=0;mask<4;++mask)vibration(index,randoms[r],tails[tail],lifetimes[life],mask);
    const float gravity[]={.053f,-.053f,0,-0.0f,1,-1};
    const float vertical[]={0,-0.0f,1,-1,.03125f};
    for(unsigned g=0;g<6;++g)for(unsigned v=0;v<5;++v)flight_physics(gravity[g],vertical[v]);
    const int ages[]={-1,0,1,2,99};const float scales[]={.5f,1,2};
    for(unsigned a=0;a<5;++a)for(int command=0;command<3;++command)
    for(unsigned s=0;s<3;++s)for(unsigned life=0;life<5;++life)
    for(unsigned mask=0;mask<4;++mask)for(unsigned f=0;f<3;++f)
        flight_animation(ages[a],command,scales[s],lifetimes[life],mask,flags[f]);
    const unsigned history_flags[]={0,JOBJ_MTX_DIRTY,JOBJ_MTX_INDEP_SRT,JOBJ_USE_QUATERNION};
    for(unsigned mask=0;mask<4;++mask)for(unsigned f=0;f<4;++f)history(mask,history_flags[f]);
    for(unsigned mode=0;mode<2;++mode)for(unsigned g=0;g<6;++g)
    for(unsigned v=0;v<5;++v)for(unsigned mask=0;mask<4;++mask)contact(mode,gravity[g],vertical[v],mask);
    const Vec3 normals[]={{1,0,0},{0,1,0},{0,-1,0},{-.25f,.75f,0}};
    for(unsigned l=0;l<2;++l)for(unsigned present=0;present<2;++present)
    for(unsigned n=0;n<4;++n)for(unsigned p=0;p<4;++p)
    for(unsigned mask=0;mask<4;++mask)for(unsigned f=0;f<3;++f)
        embedded_collision(l?7:-1,present,normals[n],atan2f(normals[p].y,normals[p].x),mask,history_flags[f]);
    const float radii[]={.125f,1.5f,8};
    for(unsigned kind=0;kind<3;++kind)for(unsigned guard=0;guard<2;++guard)
    for(unsigned s=0;s<3;++s)for(unsigned r=0;r<3;++r)
    for(unsigned life=0;life<5;++life)for(unsigned mask=0;mask<4;++mask)
        shield(kind,guard,scales[s],radii[r],lifetimes[life],mask);
    const Vec3 positions[]={{1,2,0},{-2,3,0},{-.5f,.75f,0},{0,-1,0}};
    const float limits[]={0,.125f,.6981317f,3.5f};
    for(unsigned hit=0;hit<2;++hit)for(unsigned exists=0;exists<2;++exists)
    for(unsigned face=0;face<2;++face)for(unsigned p=0;p<4;++p)for(unsigned limit=0;limit<4;++limit)
    for(unsigned mask=0;mask<4;++mask)for(unsigned f=0;f<3;++f)
        flight_collision(hit,exists,face?-1:1,positions[p],limits[limit],mask,flags[f]);
    for(unsigned family=0;family<2;++family)for(int motion=343;motion<=350;++motion)
    for(unsigned ownership=0;ownership<4;++ownership)for(unsigned detach=0;detach<2;++detach)
    for(unsigned launch=0;launch<2;++launch)for(unsigned mask=0;mask<4;++mask)
        lifecycle(family?It_Kind_CLink_Arrow:It_Kind_Link_Arrow,motion,ownership,detach,launch,mask);
    for(unsigned family=0;family<2;++family)for(unsigned s=0;s<3;++s)for(unsigned life=0;life<5;++life)
        created(family?It_Kind_CLink_Arrow:It_Kind_Link_Arrow,scales[s],lifetimes[life]);
    const float maxima[]={1,7,60};const float charges[]={0,.125f,.5f,1,2};
    for(unsigned face=0;face<2;++face)for(unsigned c=0;c<5;++c)for(unsigned m=0;m<3;++m)
    for(unsigned f=0;f<3;++f)launch(face?-1:1,charges[c]*maxima[m],maxima[m],flags[f]);
    for(unsigned family=0;family<2;++family)for(int random=0;random<4;++random)
    for(unsigned mask=0;mask<4;++mask)for(unsigned f=0;f<3;++f)
        embed(family?It_Kind_CLink_Arrow:It_Kind_Link_Arrow,random,mask,flags[f]);
    printf("Arrow original-C/generated lifecycle: %u comparisons, %u failures\n",comparisons,failures);
    return failures?1:0;
}
