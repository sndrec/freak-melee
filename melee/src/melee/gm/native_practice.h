#ifndef MELEE_GM_NATIVE_PRACTICE_H
#define MELEE_GM_NATIVE_PRACTICE_H

#include <melee/mn/forward.h>

enum { NATIVE_PRACTICE_OPPONENT_SLOT = 4 };

void nativePractice_Configure(StartMeleeData* start, int editor_port,
                              unsigned stage, unsigned opponent);
void nativePractice_InspectOpponent(void);

#endif
