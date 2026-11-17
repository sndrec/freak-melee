#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_LABELS_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_LABELS_H

#ifdef MELEE_NATIVE
#include <string>
#include <melee/ft/native_fighter_action_labels.h>
namespace WorkshopLabels {
const char* channel(unsigned channel);
const char* action(const char* source);
unsigned group(const char* source);
}
#endif

#endif
