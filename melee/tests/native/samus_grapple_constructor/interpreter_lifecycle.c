#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_storage.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftSamus/types.h>
#include <melee/it/it_26B1.h>
#include <melee/it/itarticle_data.h>
#include <melee/it/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned checks, cases, reloads, registrations, native_visual, script_visual, sequence_cases, hang_cases;
static NativeSourceBank source_bank;
static NativeScriptStorageInstance storage;
static NativeScriptStorageParameters parameters;
static NativeScriptDiagnostic diagnostic;
static NativeScriptParameterSupply supplies[128];
static NativeScriptValue registers[4096];
static NativeScriptVMFrame frames[32];
static uint8_t state[8192];
static _Alignas(16) uint8_t original_bytes[FT_CHARACTER_EXT_ATTR_CAPACITY];
static _Alignas(16) uint8_t native_scratch[FT_CHARACTER_EXT_ATTR_CAPACITY];
static _Alignas(16) uint8_t script_scratch[FT_CHARACTER_EXT_ATTR_CAPACITY];
static Fighter native_fighter, script_fighter;
static Item native_item;
static HSD_GObj native_object, script_object;
static ftData data;
static Article articles[4];
static void* item_list[4];
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"lifecycle line %u: %s (%s)\n",__LINE__,#x,diagnostic.message); exit(1); } } while(0)

void it_8026B3F8(Article* article,s32 kind)
{
    const int kinds[]={It_Kind_Samus_Bomb,It_Kind_Samus_Charge,It_Kind_Samus_Missile,It_Kind_Samus_GBeam};
    CHECK(registrations<4 && article==&articles[registrations] && kind==kinds[registrations]);++registrations;
}
void ftParts_80074A4C(HSD_GObj* object,int index,int value)
{CHECK(index==0 && value==0);if(object==&native_object)++native_visual;else{CHECK(object==&script_object);++script_visual;}}
// Whole stock load, attribute reload and primary death bodies, unchanged.
#include "native_lifecycle.inc"

static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    (void)unused;const NativeScriptSourceNode* node=&tree->nodes[at];
    if(node->kind==NativeScriptSource_Attribute && node->parent==0)return true;
    return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);
}
static bool import_parameters(void* unused,const NativeScriptProgramIndex* program,
    const NativeScriptParameterSupply** values,uint32_t* count,NativeScriptDiagnostic* d)
{
    (void)unused;(void)d;uint32_t used=0;
    for(uint32_t i=0;i<program->value_count;++i) {
        const NativeScriptValueDeclaration* v=&program->values[i];if(!v->imported)continue;
        CHECK(used<128 && v->import_offset+4<=sizeof(original_bytes));
        NativeScriptValue value={.type=v->type};
        if(v->type==NativeScriptType_I32){int32_t n;memcpy(&n,original_bytes+v->import_offset,4);value.as.i64=n;}
        else{CHECK(v->type==NativeScriptType_F32);memcpy(&value.as.f32,original_bytes+v->import_offset,4);}
        supplies[used++]=(NativeScriptParameterSupply){.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,.owner=v->owner,.value=value};
    }
    *values=supplies;*count=used;return true;
}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c)
{return c->user==&storage && storage.live;}
static bool read_value(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)c;
    if(semantic==NativeScriptBinding_ModelInstanceScaleY){CHECK(count==0);*out=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=script_fighter.x34_scale.y};return true;}
    if(semantic==NativeScriptBinding_InputUpHeld || semantic==NativeScriptBinding_InputDownHeld || semantic==NativeScriptBinding_InputAttackPressed) {
        CHECK(count==0);bool value=semantic==NativeScriptBinding_InputAttackPressed ? (script_fighter.input.pressed_buttons&0x100)!=0 :
            (script_fighter.input.held_buttons[0]&(semantic==NativeScriptBinding_InputUpHeld?8:4))!=0;
        *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};return true;
    }
    if(semantic==NativeScriptBinding_InputLHeld || semantic==NativeScriptBinding_InputRHeld) {
        CHECK(count==0);bool value=(script_fighter.input.held_buttons[0]&(semantic==NativeScriptBinding_InputLHeld?0x40:0x20))!=0;
        *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};return true;
    }
    return nativeScriptStorage_Read(&storage,semantic,selectors,count,out,d);
}
static bool write_value(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{(void)c;return nativeScriptStorage_Write(&storage,semantic,selectors,count,value,d);}
static bool validate(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptDiagnostic* d)
{NativeScriptValue value;return read_value(c,semantic,selectors,count,&value,d);}
static bool invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t selector_count,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)c;(void)selectors;(void)d;CHECK(selector_count==0);
    switch(semantic) {
    case NativeScriptBinding_FighterEnableWalljump:CHECK(count==0);script_fighter.can_walljump=true;break;
    case NativeScriptBinding_FighterCopyExtAttributes:
    case NativeScriptBinding_FighterReloadExtAttributes:
        CHECK(count==0);
        if(semantic==NativeScriptBinding_FighterReloadExtAttributes)CHECK(script_fighter.dat_attrs==script_scratch);
        // Controlled native setup boundary; use the generic binding's full
        // scratch copy contract, not a stock Samus handler.
        memcpy(script_scratch,original_bytes,sizeof(script_scratch));script_fighter.dat_attrs=script_scratch;break;
    case NativeScriptBinding_VisualsSetGroup:
        CHECK(count==2 && args[0].as.i64==0 && args[1].as.i64==0);ftParts_80074A4C(&script_object,0,0);break;
    default:CHECK(false);return false;
    }
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
static uint32_t field(const char* name)
{
    const NativeScriptCompileBank* b=&source_bank.bank;
    for(uint32_t i=0;i<b->declarations.value_count;++i)
        if(!b->declarations.values[i].parameter && nativeScriptSource_SpanEquals(&b->tree,b->declarations.values[i].name,name))return i;
    CHECK(false);return 0;
}
static NativeScriptValue get(const char* name)
{NativeScriptValue v;CHECK(nativeScriptStorage_Read(&storage,NATIVE_SCRIPT_STATE_SEMANTIC|field(name),NULL,0,&v,&diagnostic));return v;}
static void set(const char* name,NativeScriptValue v)
{CHECK(nativeScriptStorage_Write(&storage,NATIVE_SCRIPT_STATE_SEMANTIC|field(name),NULL,0,&v,&diagnostic));}
static void run(const char* event,const NativeScriptValue* args,uint32_t argument_count)
{
    const NativeScriptCompileBank* b=&source_bank.bank;uint32_t function=UINT32_MAX;
    for(uint32_t i=0;i<b->declarations.function_count;++i)
        if(nativeScriptSource_SpanEquals(&b->tree,b->declarations.functions[i].name,event))function=i;
    CHECK(function!=UINT32_MAX);
    NativeScriptVM vm={.registers=registers,.register_capacity=4096,.frames=frames,.frame_capacity=32};
    NativeScriptRuntimeContext context={.user=&storage,.source_class=NativeCallbackSource_Fighter};NativeScriptValue result;
    const NativeScriptVMHost host={.source_alive=nativeScriptBindings_SourceAlive,.read=read_value,.write=write_value,.validate=validate,.invoke=invoke};
    NativeScriptVMStatus status=nativeScriptVM_Run(&vm,&b->code,&host,&context,function,args,argument_count,&result);
    if(status!=NativeScriptVM_Ok)diagnostic=vm.diagnostic;
    CHECK(status==NativeScriptVM_Ok && result.type==NativeScriptType_Void);
}
static void compare_scaled(void)
{
    const char* names[]={"scaled_morph_jump_speed","scaled_bomb_spawn_y","scaled_bomb_hop_speed","scaled_bomb_air_start_speed",
        "scaled_morph_ecb_top","scaled_morph_ecb_bottom","scaled_morph_ecb_left_x","scaled_morph_ecb_left_y","scaled_morph_ecb_right_x","scaled_morph_ecb_right_y"};
    const unsigned offsets[]={8,120,84,88,132,136,140,144,148,152};
    for(unsigned i=0;i<10;++i){float expected;memcpy(&expected,native_scratch+offsets[i],4);NativeScriptValue v=get(names[i]);CHECK(v.type==NativeScriptType_F32 && v.as.f32==expected);}
}
int main(int argc,char** argv)
{
    CHECK(argc==2);FILE* f=fopen(argv[1],"rb");CHECK(f);CHECK(fseek(f,0,SEEK_END)==0);long bytes=ftell(f);CHECK(bytes>0);
    char* source=malloc((size_t)bytes+1);CHECK(source);rewind(f);CHECK(fread(source,1,(size_t)bytes,f)==(size_t)bytes);fclose(f);source[bytes]=0;
    CHECK(nativeSourceBank_Allocate(&source_bank,&diagnostic));const NativeScriptCompileBank* accepted=NULL;
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)bytes,.metadata=metadata,.parameter_provider=import_parameters};
    CHECK(nativeScriptCompile_Build(&source_bank.bank,&request,&accepted,&diagnostic));
    for(unsigned i=0;i<4;++i)item_list[i]=&articles[i];data=(ftData){.ext_attr=original_bytes,.x48_items=item_list};
    const float scales[]={.25f,.5f,.75f,1,1.25f,1.5f,2,3};
    for(unsigned s=0;s<8;++s)for(unsigned variant=0;variant<16;++variant) {
        for(unsigned offset=0;offset<sizeof(original_bytes);offset+=4){float value=((float)offset-80)*.03125f+variant*.125f;memcpy(original_bytes+offset,&value,4);}
        native_object=(HSD_GObj){.user_data=&native_fighter};script_object=(HSD_GObj){.user_data=&script_fighter};
        native_fighter=(Fighter){.gobj=&native_object,.ft_data=&data,.dat_attrs_backup=native_scratch,.x34_scale={1,scales[s],1}};
        script_fighter=(Fighter){.gobj=&script_object,.ft_data=&data,.dat_attrs_backup=script_scratch,.x34_scale={1,scales[s],1}};
        const NativeScriptParameterSupply* values;uint32_t count;
        CHECK(import_parameters(NULL,&accepted->declarations,&values,&count,&diagnostic));
        CHECK(nativeScriptStorage_PrepareParameters(&accepted->storage_layout,source_bank.bank.parameter_slots,source_bank.bank.parameter_slot_capacity,values,count,&parameters,&diagnostic));
        CHECK(nativeScriptStorage_Create(&accepted->storage_layout,&parameters,0,1,1,state,sizeof(state),&storage,&diagnostic));
        registrations=0;ftSs_Init_OnLoad(&native_object);CHECK(registrations==4);run("load",NULL,0);
        CHECK(native_fighter.can_walljump && script_fighter.can_walljump);
        CHECK(memcmp(native_scratch,script_scratch,sizeof(ftSs_DatAttrs))==0);
        CHECK(get("shot_handle").as.i64==-1 && get("tether_handle").as.i64==-1 && get("tether_code_sequence").as.i64==0);
        // Repeated reloads must start from immutable base parameters, never
        // multiply the previous scaled state again.
        for(unsigned repeat=0;repeat<3;++repeat) {
            if(repeat)native_fighter.x34_scale.y=script_fighter.x34_scale.y=scales[(s+repeat)%8];
            ftSs_Init_LoadSpecialAttrs(&native_object);
            if(repeat==0)compare_scaled(); // Also check the initial load helper.
            run("load_special_attrs",NULL,0);compare_scaled();++reloads;
        }
        native_fighter.u.ss.x222C=&native_object;native_fighter.u.ss.x2230=7;native_fighter.u.ss.x2238=13;
        native_fighter.u.ss.x2244=1;native_fighter.u.ss.x223C=&native_object;native_fighter.u.ss.x2240=4;
        set("shot_handle",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=42});set("shot_charge",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=7});
        set("missile_serial",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=13});set("spin_effect",(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1});
        set("tether_handle",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=43});set("tether_code_sequence",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=4});
        ftSs_Init_OnDeath(&native_object);run("death",NULL,0);
        CHECK(!native_fighter.u.ss.x222C && get("shot_handle").as.i64==-1);
        CHECK(get("shot_charge").as.i64==native_fighter.u.ss.x2230 && get("missile_serial").as.i64==native_fighter.u.ss.x2238);
        CHECK(get("spin_effect").as.u64==native_fighter.u.ss.x2244 && get("tether_code_sequence").as.i64==native_fighter.u.ss.x2240);
        CHECK(!native_fighter.u.ss.x223C && get("tether_handle").as.i64==-1 && native_visual==script_visual);++cases;
    }
    // One held/pressed sample must advance at most one switch branch. Compare
    // the unchanged stock helper over all 256 native byte states and every low
    // input-bit combination, then a sequence with both directions held at once.
    for(int initial=0;initial<256;++initial)for(unsigned held=0;held<16;++held)for(unsigned attack=0;attack<2;++attack) {
        native_fighter.u.ss.x2240=initial;
        set("tether_code_sequence",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=initial});
        native_fighter.input.held_buttons[0]=script_fighter.input.held_buttons[0]=held;
        native_fighter.input.pressed_buttons=script_fighter.input.pressed_buttons=attack?0x100:0;
        samus_grapple_state_sync(&native_fighter);run("Tether_AdvanceCodeSequence",NULL,0);
        CHECK(get("tether_code_sequence").as.i64==native_fighter.u.ss.x2240);++sequence_cases;
    }
    native_fighter.u.ss.x2240=0;
    set("tether_code_sequence",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=0});
    native_fighter.input.held_buttons[0]=script_fighter.input.held_buttons[0]=12;
    native_fighter.input.pressed_buttons=script_fighter.input.pressed_buttons=0x100;
    for(unsigned tick=0;tick<8;++tick) {
        samus_grapple_state_sync(&native_fighter);run("Tether_AdvanceCodeSequence",NULL,0);
        CHECK(get("tether_code_sequence").as.i64==native_fighter.u.ss.x2240);++sequence_cases;
    }
    const float timers[]={-1,0,.5f,1,60};
    for(unsigned mode=0;mode<3;++mode)for(unsigned buttons=0;buttons<16;++buttons)for(unsigned t=0;t<5;++t) {
        uint32_t held=(buttons&1?0x40:0)|(buttons&2?0x20:0)|(buttons&4?0x10:0)|(buttons&8?0x80000000u:0);
        native_item.xDD4_itemVar.samusgrapple.x16=(u8)mode;
        native_fighter.input.held_buttons[0]=script_fighter.input.held_buttons[0]=held;
        native_fighter.mv.ss.grapple.x4=timers[t];
        set("tether_hang_timer",(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=timers[t]});
        NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=mode};
        native_hold_hang_timer(&native_item,&native_fighter);run("Tether_HoldHangTimer",&argument,1);
        CHECK(get("tether_hang_timer").as.f32==native_fighter.mv.ss.grapple.x4);++hang_cases;
    }
    free(source);nativeSourceBank_Release(&source_bank);
    printf("{\"checks\":%u,\"lifecycle_cases\":%u,\"reloads\":%u,\"sequence_cases\":%u,\"hang_cases\":%u}\n",checks,cases,reloads,sequence_cases,hang_cases);return 0;
}
