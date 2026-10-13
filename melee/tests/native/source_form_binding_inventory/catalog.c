#include <melee/ft/native_script_bindings.h>
#include <stdio.h>
int main(void)
{
    putchar('[');
    for (unsigned id=1;id<NativeScriptBinding_Count;++id) {
        const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(id);
        if (id>1) putchar(',');
        printf("{\"id\":%u,\"name\":\"%s\",\"parent\":%u,\"contexts\":%u,\"flags\":%u,\"kind\":%u,\"result\":%u,\"arguments\":[",id,d->name,d->parent,d->contexts,d->flags,d->kind,d->result_type);
        for (unsigned a=0;a<d->argument_count;++a) {
            if(a)putchar(',');
            printf("{\"type\":%u,\"mode\":%u,\"numeric_any\":%s}",d->arguments[a].type,d->arguments[a].mode,d->arguments[a].numeric_any?"true":"false");
        }
        printf("]}");
    }
    puts("]");return 0;
}
