#ifndef ARTICLE_SIGNALS_FIXTURE_H
#define ARTICLE_SIGNALS_FIXTURE_H
#include <sysdolphin/baselib/gobj.h>
extern unsigned signal_mode;
extern HSD_GObj* signal_caller;
extern HSD_GObj* (*signal_reuse)(void);
#endif
