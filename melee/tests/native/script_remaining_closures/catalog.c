#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/native_script_events.h>
#include <stdio.h>
#include <string.h>

static bool path(uint32_t id,char* output,size_t capacity,unsigned depth)
{
    if(depth>16)return false;
    if(id==NATIVE_SCRIPT_STATE_NAMESPACE){snprintf(output,capacity,"state");return true;}
    if(id==NATIVE_SCRIPT_ITEM_NAMESPACE){snprintf(output,capacity,"item");return true;}
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(id);
    if(!descriptor || !descriptor->name)return false;
    if(descriptor->parent) {
        if(!path(descriptor->parent,output,capacity,depth+1))return false;
        size_t used=strlen(output);int count=snprintf(output+used,capacity-used,".%s",descriptor->name);
        return count>0 && (size_t)count<capacity-used;
    }
    int count=snprintf(output,capacity,"%s",descriptor->name);return count>0 && (size_t)count<capacity;
}
int main(void)
{
    printf("{\"catalog\":[");bool comma=false;
    for(uint32_t id=1;id<NativeScriptBinding_Count;++id) {
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(id);char name[256];
        if(!descriptor || !descriptor->name || !path(id,name,sizeof(name),0))continue;
        if(comma)putchar(',');comma=true;
        printf("{\"semantic\":%u,\"path\":\"%s\",\"contexts\":%u,\"kind\":%u,\"flags\":%u,\"arguments\":%u,\"result_type\":%u}",
            id,name,descriptor->contexts,descriptor->kind,descriptor->flags,descriptor->argument_count,descriptor->result_type);
    }
    printf("],\"event_payloads\":[");comma=false;
    for(uint32_t id=0;id<nativeScriptEvents_Count();++id) {
        const NativeScriptEventDescriptor* event=nativeScriptEvents_Descriptor(id);if(!event)continue;
        for(unsigned argument=0;argument<event->argument_count;++argument) {
            if(comma)putchar(',');comma=true;
            printf("{\"event\":\"%s\",\"context\":%u,\"property\":\"%s\"}",event->name,event->context,event->arguments[argument].property);
        }
    }
    printf("]}\n");return 0;
}
