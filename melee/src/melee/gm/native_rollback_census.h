#ifndef MELEE_GM_NATIVE_ROLLBACK_CENSUS_H
#define MELEE_GM_NATIVE_ROLLBACK_CENSUS_H

#include <stdbool.h>

#ifdef MELEE_NATIVE

bool nativeRollbackCensus_Init(void);
void nativeRollbackCensus_Shutdown(void);
void nativeRollbackCensus_BeginSimulation(void);
void nativeRollbackCensus_EndSimulation(void);
void nativeRollbackCensus_BeginRender(void);
void nativeRollbackCensus_EndRender(void);

#endif

#endif
