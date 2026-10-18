#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_package.h>
#include <stdio.h>
#include <stdlib.h>

bool rig_test_prepare_source(const NativeFighterProject* p,char* error,size_t capacity)
{
    NativeSourcePackage* candidate=calloc(1,sizeof(*candidate));
    NativeScriptDiagnostic diagnostic={0};
    NativeSourcePackageInput input={.family=NativeSourcePackage_Single,
        .facts=p->asset_record,.facts_bytes=p->asset_record_size,
        .assets={p->fighter_dat,p->fighter_size,p->animation_dat,p->animation_size,p->costume_dat,p->costume_size},
        .source=(const char*)p->script_text,.source_bytes=p->script_size,
        .parameters=(const char*)p->parameter_text,.parameter_bytes=p->parameter_size,
        .resources=p->resources,.resource_bytes=p->resource_size};
    bool ok=candidate&&nativeSourcePackage_Prepare(candidate,&input,Ft_Kind_CustomFirst,NULL,&diagnostic);
    if(!ok&&error&&capacity)snprintf(error,capacity,"%s",diagnostic.message);
    if(candidate){nativeSourcePackage_Release(candidate);free(candidate);}
    return ok;
}
