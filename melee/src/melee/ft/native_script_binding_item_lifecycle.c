#include "native_script_binding_item_lifecycle.h"
#include "native_script_bindings.h"
#include <melee/it/native_item_lifetime.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_3F14.h>
#include <stdio.h>
static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
bool nativeScriptBindingItemLifecycle_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    (void)arguments;NativeItemLifetime source;
    if(!c||!output||count||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||
       (semantic!=NativeScriptBinding_ItemRefreshPhysics&&semantic!=NativeScriptBinding_ItemRemove&&
        semantic!=NativeScriptBinding_ItemBeginExpiry&&semantic!=NativeScriptBinding_ItemClearAnimationCommands&&semantic!=NativeScriptBinding_ItemReleaseCamera)||
       !nativeItemLifetime_Capture(c->source,&source)||!source.source_owned||
       source.reference.lifetime!=c->scene_epoch||source.generation!=c->generation||
       !nativeScriptBindings_SourceAlive(c))return error(d,"Item lifecycle requires its live owned source and zero arguments");
    if(semantic==NativeScriptBinding_ItemRemove){
        Item_8026A8EC(c->source);
        // Reentrant remove during Destroyed is already queued natively, while
        // callback storage is still writable. It must terminate that VM too.
        // Never retire a replacement that reused the old actor storage address.
        if(nativeItemLifetime_Current(&source))nativeScriptStorage_Destroy(source.view.storage);
        return error(d,"Item removal retired the calling source");
    }
    if(semantic==NativeScriptBinding_ItemClearAnimationCommands){
        source.data->x524_cmd.u=NULL;
    }else if(semantic==NativeScriptBinding_ItemReleaseCamera){
        Item_80267454(c->source);
    }else if(semantic==NativeScriptBinding_ItemBeginExpiry){
        if(!it_804D6D28)return error(d,"Item expiry requires common item data");
        it_8027518C(c->source);
    }else it_802750F8(c->source);
    if(!nativeItemLifetime_Current(&source)||!nativeScriptBindings_SourceAlive(c))
        return error(d,"Item lifecycle retired or faulted the calling source");
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
