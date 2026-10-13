#include "native_match_classes.h"
#include <melee/ft/ftparts.h>
#include <melee/ft/ftmaterial.h>
#include <melee/gr/grmaterial.h>
#include <melee/it/itmaterial.h>
#include <melee/lb/lbrefract.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <sysdolphin/baselib/fog.h>

void nativeMatchClasses_Initialize(void)
{
    // Complete compiled class inventory, including materials/refraction classes
    // first used by later-spawned items/stage interactions. ClassInfoInit guards
    // initialized definitions, preserving live counters and existing methods.
#define TYPE(value) (HSD_ClassInfo*)&(value)
    HSD_ClassInfo* types[]={TYPE(hsdClass),TYPE(hsdObj),TYPE(hsdJObj),
        TYPE(hsdDObj),TYPE(hsdPObj),TYPE(hsdMObj),TYPE(hsdTObj),TYPE(hsdCObj),
        TYPE(hsdLObj),TYPE(hsdWObj),TYPE(hsdFog),TYPE(hsdFogAdj),
        TYPE(ftJObj),TYPE(ftIntpJObj),TYPE(ftPObj),TYPE(ftMObj),TYPE(it_mobj),
        TYPE(grMaterialMObj),TYPE(lbRefractDObjInfo),TYPE(lbRefractPObjInfo)};
#undef TYPE
    for (unsigned i=0;i<sizeof(types)/sizeof(*types);++i) ClassInfoInit(types[i]);
}
