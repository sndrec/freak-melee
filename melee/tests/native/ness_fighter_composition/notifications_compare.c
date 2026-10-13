// Original load/notification bodies and actual shared presentation primitives.
// Immutable registration is compared with Definition assets; rendering is an
// ordered call boundary. Original NULL/pointer initialization is not an ID.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/it/types.h>
#include <melee/it/it_26B1.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,pairs,heavy,hold_kind,count,registration_count;
static Fighter fighter;
static HSD_GObj source,item;
static ftData data;
static uint8_t attributes[FT_CHARACTER_EXT_ATTR_CAPACITY], backup[FT_CHARACTER_EXT_ATTR_CAPACITY];
static Article* assets[11];
static const NativeFighterDefinition* definition;
typedef struct PresentationCall { unsigned op; int selector,value; float frame; } PresentationCall;
static PresentationCall calls[8];
static int registered[11];
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"%s:%d: %s\n", \
    __FILE__,__LINE__,#c); exit(1); } } while (0)
#undef GET_FIGHTER
#define GET_FIGHTER(o) ((Fighter*)(o)->user_data)
static void unexpected(void) { CHECK(false); }
static bool alive(HSD_GObj* o,uint32_t generation,uint32_t kind)
{ return o==&source && o->user_data==&fighter &&
    generation==(uint32_t)fighter.x8_spawnNum && kind==NativeCallbackSource_Fighter; }
static void record(unsigned op,int selector,int value,float frame)
{ CHECK(count<8); calls[count++]=(PresentationCall){op,selector,value,frame}; }
bool itIsHeavy(HSD_GObj* o) { CHECK(o==&item); return heavy!=0; }
Item_HoldKinds itGetHoldKind(HSD_GObj* o) { CHECK(o==&item); return hold_kind; }
void ftAnim_80070FB4(HSD_GObj* o,s32 selector,s32 value)
{ CHECK(o==&source); record(1,selector,value,0); }
void ftAnim_80070C48(HSD_GObj* o,s32 selector)
{ CHECK(o==&source); record(2,selector,0,0); }
void ftAnim_80070CC4(HSD_GObj* o,int selector)
{ CHECK(o==&source); record(3,selector,0,0); }
void ftAnim_800704F0(HSD_GObj* o,int selector,float frame)
{ CHECK(o==&source); record(4,selector,0,frame); }
void it_8026B3F8(Article* article,s32 kind)
{
    CHECK(registration_count<11); unsigned slot=11;
    for(unsigned i=0;i<11;++i) if(article==assets[i]) slot=i;
    CHECK(slot<11); registered[slot]=kind; ++registration_count;
}
#include "notification_primitives.h"
#include "notifications_original.h"
#include "attributes_current.h"
static void stock_slot(HSD_GObj* o) { (void)o; }
#include "base.h"
#include "behavior.generated.c"
static void reset(void)
{
    memset(&fighter,0,sizeof(fighter)); source=(HSD_GObj){.user_data=&fighter};
    fighter.gobj=&source; fighter.kind=Ft_Kind_Ness; fighter.x8_spawnNum=0x80000099u;
    fighter.item_gobj=&item; fighter.ft_data=&data; fighter.dat_attrs_backup=backup;
    data.ext_attr=attributes; data.x48_items=(void**)assets;
    for(unsigned i=0;i<sizeof(attributes);++i) attributes[i]=(uint8_t)(i*29+7);
    memset(backup,0xA5,sizeof(backup));
    for(unsigned i=0;i<11;++i) { assets[i]=(Article*)(uintptr_t)(0x1000+i*16); registered[i]=-1; }
    count=0; registration_count=0; memset(calls,0,sizeof(calls));
}
static void compare_load(void)
{
    reset(); ftNs_Init_OnLoad(&source);
    CHECK(fighter.dat_attrs==backup);
    CHECK(!memcmp(backup,attributes,sizeof(ftNessAttributes)));
    CHECK(registration_count==11);
    int registration[11]; memcpy(registration,registered,sizeof(registration));
    reset(); definition->on_load(&source);
    CHECK(fighter.dat_attrs==backup && !memcmp(backup,attributes,sizeof(ftNessAttributes)));
    // Registration is immutable package metadata, not mutable fighter OnLoad.
    CHECK(registration_count==0 && definition->article_asset_count==11);
    for(unsigned i=0;i<definition->article_asset_count;++i) {
        const NativeFighterArticleAsset* a=&definition->article_assets[i];
        CHECK(a->slot<11 && registration[a->slot]==a->kind);
    }
    for(unsigned i=0;i<definition->state_field_count;++i) {
        const NativeFighterStateField* field=&definition->state_fields[i];
        if(!strcmp(field->name,"flash_handle") || !strcmp(field->name,"thunder_ball_handle") ||
           !strcmp(field->name,"bat_handle") || !strcmp(field->name,"yoyo_handle")) {
            int32_t value; memcpy(&value,fighter.native_module_state+field->offset,4); CHECK(value==-1);
        }
        if(!strcmp(field->name,"thunder_effects_active")) CHECK(!fighter.native_module_state[field->offset]);
    }
    ++pairs;
    // Reload retains existing named handles; common extended-attribute copying
    // does not reinitialize live resource state.
    for(unsigned seed=0;seed<4;++seed) {
        reset(); fighter.dat_attrs=backup;
        for(unsigned i=0;i<sizeof(attributes);++i) attributes[i]^=(uint8_t)seed;
        ftNs_Init_LoadSpecialAttrs(&source); uint8_t expected[sizeof(ftNessAttributes)];
        memcpy(expected,backup,sizeof(expected)); memset(backup,0,sizeof(backup));
        memset(fighter.native_module_state,0x7D,sizeof(fighter.native_module_state));
        definition->on_load_special_attrs(&source);
        CHECK(!memcmp(expected,backup,sizeof(expected)));
        for(unsigned i=0;i<sizeof(fighter.native_module_state);++i) CHECK(fighter.native_module_state[i]==0x7D);
        ++pairs;
    }
}
static void compare_notifications(void)
{
    HSD_GObjEvent original[]={ftNs_Init_OnItemInvisible,ftNs_Init_OnItemVisible,
                             ftNs_Init_OnKnockbackEnter,ftNs_Init_OnKnockbackExit};
    HSD_GObjEvent owned[]={definition->on_item_invisible,definition->on_item_visible,
                          definition->on_knockback_enter,definition->on_knockback_exit};
    for(heavy=0;heavy<2;++heavy) for(hold_kind=0;hold_kind<=5;++hold_kind) {
        for(unsigned event=0;event<4;++event) {
            reset(); original[event](&source); PresentationCall expected[8];
            memcpy(expected,calls,sizeof(calls)); unsigned expected_count=count;
            count=0; memset(calls,0,sizeof(calls)); owned[event](&source);
            CHECK(count==expected_count && !memcmp(expected,calls,sizeof(calls))); ++pairs;
        }
        for(unsigned drop=0;drop<2;++drop) for(unsigned flag=0;flag<2;++flag)
          for(unsigned ext=0;ext<2;++ext) {
            reset();
            if(drop) ftNs_Init_OnItemDrop(&source,flag); else ftNs_Init_OnItemPickup(&source,flag);
            PresentationCall expected[8]; memcpy(expected,calls,sizeof(calls)); unsigned expected_count=count;
            count=0; memset(calls,0,sizeof(calls));
            Fighter_ItemEvent owned=drop?(ext?definition->on_item_drop_ext:definition->on_item_drop):
                                           (ext?definition->on_item_pickup_ext:definition->on_item_pickup);
            owned(&source,flag);
            CHECK(count==expected_count && !memcmp(expected,calls,sizeof(calls))); ++pairs;
        }
    }
}
int main(int argc,char** argv)
{
    CHECK(argc==2); NativeFighterHost host={0};
#include "required_host.h"
    host.callback_source_alive=alive; host.copy_ext_attributes=copy_ext_attributes;
    host.reload_ext_attributes=reload_ext_attributes; host.fighter_item_pickup=Fighter_OnItemPickup;
    host.fighter_item_drop=Fighter_OnItemDrop; host.fighter_item_invisible=Fighter_OnItemInvisible;
    host.fighter_item_visible=Fighter_OnItemVisible; host.fighter_knockback_enter=Fighter_OnKnockbackEnter;
    host.fighter_knockback_exit=Fighter_OnKnockbackExit;
    definition=FreakMelee_GetFighterModule(&ftNessClone_Definition,&host); CHECK(definition);
    FILE* file=fopen(argv[1],"rb"); CHECK(file); char text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY+1];
    size_t size=fread(text,1,sizeof(text)-1,file); CHECK(feof(file)&&!ferror(file)); fclose(file);
    NativeFighterParameters p; CHECK(nativeFighterParameters_Parse(text,size,&p,NULL));
    uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for(unsigned i=0;i<p.count;++i) values[i]=p.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(values,p.count,p.schema_hash));
    compare_load(); compare_notifications();
    printf("Ness composed notifications: %u checks, 0 failures, %u source pairs\n",checks,pairs);
    return 0;
}
