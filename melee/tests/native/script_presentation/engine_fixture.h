#ifndef SCRIPT_PRESENTATION_ENGINE_FIXTURE_H
#define SCRIPT_PRESENTATION_ENGINE_FIXTURE_H
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
extern int script_presentation_log[32][5];
extern unsigned script_presentation_log_count;
void scriptPresentationFixture_Reset(const NativeFighterDefinition*);
void scriptPresentationFixture_RetireOnMatrix(HSD_GObj*,bool replace);
struct NativeSourcePackage;
void scriptPresentationFixture_Source(struct NativeSourcePackage*);
#endif
