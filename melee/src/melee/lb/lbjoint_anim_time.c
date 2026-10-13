#include "lb_00B0.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>

static HSD_JObj* lbFindJObjWithAObj(HSD_JObj* jobj)
{
    HSD_JObj* tmp;
    if (jobj == NULL) {
        return NULL;
    }
    if (jobj->aobj != NULL) {
        return jobj;
    }
    tmp = lbFindJObjWithAObj(jobj->child);
    if (tmp != NULL) {
        return tmp;
    }
    tmp = lbFindJObjWithAObj(jobj->next);
    if (tmp != NULL) {
        return tmp;
    }
    return NULL;
}

float lbGetJObjFramerate(HSD_JObj* jobj)
{
    jobj = lbFindJObjWithAObj(jobj);
    if (jobj != NULL) {
        return jobj->aobj->framerate;
    }
    return 0;
}

float lbGetJObjCurrFrame(HSD_JObj* jobj)
{
    jobj = lbFindJObjWithAObj(jobj);
    if (jobj != NULL) {
        return jobj->aobj->curr_frame;
    }
    return 0;
}

float lbGetJObjEndFrame(HSD_JObj* jobj)
{
    jobj = lbFindJObjWithAObj(jobj);
    if (jobj != NULL) {
        return jobj->aobj->end_frame;
    }
    return 0;
}

