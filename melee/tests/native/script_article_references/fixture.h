#ifndef ARTICLE_REF_FIXTURE_H
#define ARTICLE_REF_FIXTURE_H
#include <sysdolphin/baselib/gobj.h>
extern unsigned reference_mode;
extern HSD_GObj* reference_caller;
extern HSD_GObj* reference_victim;
extern HSD_GObj* (*reference_reuse)(void);
#endif
