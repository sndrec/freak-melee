#include "it_26B1.h"
#include "types.h"
#include <sysdolphin/baselib/gobj.h>

ItemKind itGetKind(HSD_GObj* gobj)
{
    Item* temp_item = gobj->user_data;
    return temp_item->kind;
}
