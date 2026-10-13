#include "native_script_binding_forms.h"
#include "ftcommon.h"
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* text)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",text);return false;}

bool nativeScriptBindingForms_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)arguments;
    if(!result||semantic!=NativeScriptBinding_FormsActivatePartner||count)
        return error(diagnostic,"Invalid forms operation or argument count");
    if(!nativeScriptBindings_SourceFighterLayout(context,diagnostic))return false;
    // Running source scripts are expected. The shared transfer validates the
    // alternate pair and the dormant destination's CanActivateForm predicate.
    if(!ftCommon_ActivatePartner(context->source,NULL))
        return error(diagnostic,"Common partner activation rejected or retired the transfer");
    if(!nativeScriptBindings_SourceAlive(context)||
       !nativeScriptBindings_SourceFighterLayout(context,diagnostic))
        return error(diagnostic,"Form source retired or faulted before result publication");
    // Ordinary dormancy preserves this source generation and owned view.
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
