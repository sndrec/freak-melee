#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_LABELS_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_LABELS_H

#ifdef MELEE_NATIVE
#include <string>
namespace WorkshopLabels {
const char* event(unsigned kind);
std::string command(unsigned opcode);
const char* channel(unsigned channel);
const char* action(const char* source);
unsigned group(const char* source);
}
#endif

#endif
