#include "native_script_binding_colors.h"
#include "native_script_bindings.h"
#include "native_script_fighter.h"
#include "native_source_package.h"
#include "fighter.h"
#include "ftmaterial.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/it/article_model_layers.h>
#include <melee/it/itmaterial.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct ColorSource {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    const void* data;
    const NativeScriptCompileBank* bank;
    const NativeScriptItemProgram* item;
    const NativeScriptFighterProgram* fighter;
    const NativeSourcePackage* package;
    NativeScriptReceiverView view;
    HSD_JObj* root;
    HSD_ClassInfo* root_class;
    uint64_t identity,layout,parameters;
    uint32_t owner;
} ColorSource;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool integral(const NativeScriptValue* value,uint32_t* bits)
{
    switch(value->type){
    case NativeScriptType_I32:case NativeScriptType_I64:*bits=(uint32_t)value->as.i64;return true;
    case NativeScriptType_U32:case NativeScriptType_U64:*bits=(uint32_t)value->as.u64;return true;
    case NativeScriptType_Bool:if(value->as.u64<=1){*bits=(uint32_t)value->as.u64;return true;}break;
    default:break;
    }
    return false;
}
static NativeScriptValue packed(GXColor color)
{
    uint32_t bits=(uint32_t)color.r<<24|(uint32_t)color.g<<16|(uint32_t)color.b<<8|color.a;
    int32_t signed_bits;memcpy(&signed_bits,&bits,sizeof(bits));
    return (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=signed_bits};
}
static GXColor unpacked(uint32_t bits)
{return (GXColor){(uint8_t)(bits>>24),(uint8_t)(bits>>16),(uint8_t)(bits>>8),(uint8_t)bits};}
static bool root_current(const ColorSource* s)
{
    if(((HSD_GObj*)s->context.source)->hsd_obj!=s->root)return false;
    if(!s->root)return true; // Native material traversals have a null-root no-op.
    return ((HSD_GObj*)s->context.source)->obj_kind==HSD_GObj_JObjKind && s->root_class &&
        HSD_CLASS_METHOD(s->root)==s->root_class && s->root_class->head.obj_size>=(int)sizeof(*s->root) &&
        hsdObjIsDescendantOf((HSD_Obj*)s->root,HSD_CLASS_INFO(&hsdJObj));
}
static bool current(const ColorSource* s,NativeScriptDiagnostic* d)
{
    NativeScriptRuntimeContext context=s->context;HSD_GObj* object=context.source;
    NativeScriptReceiverView view;
    if(nativeMatchPool_Resolve(s->reference)!=object || s->reference.lifetime!=context.scene_epoch ||
       object->user_data!=s->data || !nativeScriptBindings_SourceAlive(&context) || !s->bank->valid || !root_current(s))
        return error(d,"Color source or primary model retired");
    if(context.source_class==NativeCallbackSource_Fighter){
        const Fighter* fighter=s->data;
        if(object->classifier!=HSD_GOBJ_CLASS_FIGHTER || nativeScriptFighter_Program(fighter->kind)!=s->fighter ||
           s->fighter->identity!=s->identity || s->fighter->bank!=s->bank ||
           nativeFighterRuntime_SourcePackage(fighter->kind)!=s->package || !s->package ||
           !nativeScriptBindings_SourceFighterLayout(&context,d) ||
           (s->root && (!fighter->parts || fighter->parts[0].joint!=s->root)) ||
           !nativeScriptFighter_View(object,context.generation,&view))return error(d,"Fighter color program or owned rig retired");
    }else{
        const Item* item=s->data;
        if(object->classifier!=HSD_GOBJ_CLASS_ITEM || nativeScriptItem_Status(object)!=NativeScriptVM_Ok ||
           nativeScriptItem_Spec(object)!=s->item || s->item->identity!=s->identity || s->item->bank!=s->bank ||
           s->item->article!=item->xC4_article_data || s->item->owner!=item->native_script_owner ||
           s->item->kind!=item->kind || s->item->author_kind!=item->native_article_author_kind ||
           !s->item->article || !s->item->article->x10_modelDesc ||
           !nativeScriptItem_View(object,context.generation,&view))return error(d,"Item color owner or model schema retired");
    }
    if(view.program!=s->view.program || view.program!=&s->bank->declarations || view.storage!=s->view.storage ||
       !view.storage || !view.storage->live || view.storage->owner!=s->owner || view.storage->generation!=context.generation ||
       view.storage->layout!=&s->bank->storage_layout || view.storage->layout->identity!=s->layout ||
       view.storage->parameters!=&s->bank->storage_parameters || view.storage->parameters->identity!=s->parameters)
        return error(d,"Color declaration storage changed before publication");
    return true;
}
static bool capture(NativeScriptRuntimeContext* c,ColorSource* s,NativeScriptDiagnostic* d)
{
    if(!c || !c->scene_epoch || (c->source_class!=NativeCallbackSource_Fighter&&c->source_class!=NativeCallbackSource_Item) ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&s->reference) || s->reference.lifetime!=c->scene_epoch ||
       ((HSD_GObj*)c->source)->classifier!=(c->source_class==NativeCallbackSource_Fighter?HSD_GOBJ_CLASS_FIGHTER:HSD_GOBJ_CLASS_ITEM) ||
       !nativeScriptBindings_SourceAlive(c))return error(d,"Color operation requires a live source actor in its pinned scene");
    s->context=*c;s->data=((HSD_GObj*)c->source)->user_data;s->root=((HSD_GObj*)c->source)->hsd_obj;
    if(s->root && ((HSD_GObj*)c->source)->obj_kind!=HSD_GObj_JObjKind)return error(d,"Color primary object is not a native joint");
    s->root_class=s->root?HSD_CLASS_METHOD(s->root):NULL;
    if(c->source_class==NativeCallbackSource_Fighter){
        const Fighter* fighter=s->data;s->fighter=nativeScriptFighter_Program(fighter->kind);
        s->package=nativeFighterRuntime_SourcePackage(fighter->kind);s->bank=s->fighter?s->fighter->bank:NULL;s->owner=0;
        if(!nativeScriptFighter_View(c->source,c->generation,&s->view))return error(d,"Fighter color view is unavailable");
        s->identity=s->fighter?s->fighter->identity:0;
    }else{
        s->item=nativeScriptItem_Spec(c->source);s->bank=s->item?s->item->bank:NULL;s->owner=s->item?s->item->owner:0;
        if(!nativeScriptItem_View(c->source,c->generation,&s->view))return error(d,"Item color view is unavailable");
        s->identity=s->item?s->item->identity:0;
    }
    if(!s->bank || !s->identity || !s->view.storage || !s->view.storage->layout || !s->view.storage->parameters)
        return error(d,"Color operation has no immutable program/storage");
    s->layout=s->view.storage->layout->identity;s->parameters=s->view.storage->parameters->identity;
    return current(s,d);
}
bool nativeScriptBindingColors_Read(NativeScriptRuntimeContext* c,uint32_t semantic,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out)return error(d,"Color read output is absent");ColorSource source;if(!capture(c,&source,d))return false;
    NativeScriptValue value;
    if(c->source_class==NativeCallbackSource_Fighter){Fighter* fighter=(Fighter*)source.data;
        switch(semantic){
        case NativeScriptBinding_CostumeIndex:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=fighter->x619_costume_id};break;
        case NativeScriptBinding_VisualsOutlineColor:value=packed(fighter->x610_color_rgba[1]);break;
        case NativeScriptBinding_VisualsPreserveSecondaryMaterial:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=fighter->x2224_b0};break;
        case NativeScriptBinding_ModelInstanceScaleZ:value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=fighter->x34_scale.z};break;
        default:return error(d,"Unsupported Fighter color property");}
    }else{Item* item=(Item*)source.data;
        switch(semantic){
        case NativeScriptBinding_ItemOutlineColor:value=packed(item->xBC8.x0_unk);break;
        case NativeScriptBinding_ItemControllerTint:value=packed(item->xBC4);break;
        case NativeScriptBinding_ItemControllerTintEnabled:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=item->x5C8};break;
        case NativeScriptBinding_ItemMaterialOpacity:value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=item->x5C9};break;
        case NativeScriptBinding_ItemLayered:value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=item->xDCF_flag.b3};break;
        default:return error(d,"Unsupported Item color property");}
    }
    if(!current(&source,d))return false;*out=value;return true;
}
bool nativeScriptBindingColors_Write(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!value)return error(d,"Color write value is absent");
    bool boolean=semantic==NativeScriptBinding_VisualsPreserveSecondaryMaterial || semantic==NativeScriptBinding_ItemControllerTintEnabled || semantic==NativeScriptBinding_ItemLayered;
    bool scalar=semantic==NativeScriptBinding_ModelInstanceScaleZ;
    if(boolean?(value->type!=NativeScriptType_Bool||value->as.u64>1):scalar?value->type!=NativeScriptType_F32:value->type!=NativeScriptType_I32)
        return error(d,"Color property write has an invalid scalar type");
    ColorSource source;if(!capture(c,&source,d))return false;
    if(c->source_class==NativeCallbackSource_Fighter){Fighter* fighter=(Fighter*)source.data;
        switch(semantic){
        case NativeScriptBinding_VisualsOutlineColor:fighter->x610_color_rgba[1]=unpacked((uint32_t)value->as.i64);break;
        case NativeScriptBinding_VisualsPreserveSecondaryMaterial:fighter->x2224_b0=value->as.u64!=0;break;
        case NativeScriptBinding_ModelInstanceScaleZ:if(isfinite(value->as.f32)&&value->as.f32!=0)fighter->x34_scale.z=value->as.f32;break;
        default:return error(d,"Unsupported writable Fighter color property");}
    }else{Item* item=(Item*)source.data;
        switch(semantic){
        case NativeScriptBinding_ItemOutlineColor:item->xBC8.x0_unk=unpacked((uint32_t)value->as.i64);break;
        case NativeScriptBinding_ItemControllerTint:item->xBC4=unpacked((uint32_t)value->as.i64);break;
        case NativeScriptBinding_ItemControllerTintEnabled:item->x5C8=value->as.u64!=0;break;
        case NativeScriptBinding_ItemMaterialOpacity:if(value->as.i64>=0&&value->as.i64<=255)item->x5C9=(u8)value->as.i64;break;
        case NativeScriptBinding_ItemLayered:(void)articleModelLayers_Bind(c->source,value->as.u64!=0);break;
        default:return error(d,"Unsupported writable Item color property");}
    }
    return current(&source,d);
}
bool nativeScriptBindingColors_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out || !args)return error(d,"Color call arguments/output are absent");
    if(semantic==NativeScriptBinding_ColorRGBA){
        uint32_t channel[4];if(count!=4)return error(d,"RGBA requires four integral channels");
        for(unsigned i=0;i<4;++i)if(!integral(&args[i],&channel[i]))return error(d,"RGBA channels must retain their integral widths");
        NativeScriptValue result=packed((GXColor){(u8)channel[0],(u8)channel[1],(u8)channel[2],(u8)channel[3]});
        if(!nativeScriptBindings_SourceAlive(c))return error(d,"RGBA caller retired before publication");*out=result;return true;
    }
    uint32_t bits;if(count!=1 || !integral(&args[0],&bits))return error(d,"Material call needs one integral color or lookup slot");
    ColorSource source;if(!capture(c,&source,d))return false;
    NativeScriptValue result={.type=NativeScriptType_Void};GXColor diffuse=unpacked(bits);
    if(semantic==NativeScriptBinding_VisualsSetDiffuseColor && c->source_class==NativeCallbackSource_Fighter)ftMaterial_800BFB4C(c->source,&diffuse);
    else if(semantic==NativeScriptBinding_ItemSetDiffuseColor && c->source_class==NativeCallbackSource_Item)it_80278574(c->source,&diffuse);
    else if(semantic==NativeScriptBinding_VisualsInstallOutlineVisibility && c->source_class==NativeCallbackSource_Fighter){
        // Keep the original Bool admission result, rather than replacing a
        // missing owned lookup with another costume or fabricated success.
        bool slot=args[0].type==NativeScriptType_I32 || args[0].type==NativeScriptType_I64 ?
            args[0].as.i64>=0 && args[0].as.i64<FT_CHARACTER_ITEM_CAPACITY :
            args[0].as.u64<FT_CHARACTER_ITEM_CAPACITY;
        result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=slot && nativeFighterRuntime_InstallOutlineVisibility(c->source,(int32_t)bits)};
    }else return error(d,"Unsupported material call/source class");
    if(!current(&source,d))return false;*out=result;return true;
}
