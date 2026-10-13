#ifndef MELEE_GM_NATIVE_ROLLBACK_BENCHMARK_H
#define MELEE_GM_NATIVE_ROLLBACK_BENCHMARK_H

#ifdef MELEE_NATIVE
#include <stdbool.h>
#include <stdint.h>

bool nativeRollbackBenchmark_Init(void);
void nativeRollbackBenchmark_Shutdown(void);
bool nativeRollbackBenchmark_BeginFrame(uint64_t frame);
// Returns false on failure or successful completion. next_frame is always set.
bool nativeRollbackBenchmark_EndFrame(uint64_t frame, uint64_t* next_frame);
void nativeRollbackBenchmark_BeginInput(void);
void nativeRollbackBenchmark_InputPoll(bool waited);
void nativeRollbackBenchmark_EndInput(void);
void nativeRollbackBenchmark_BeginSimulation(void);
void nativeRollbackBenchmark_EndSimulation(void);
void nativeRollbackBenchmark_BeginRender(void);
void nativeRollbackBenchmark_EndRender(void);
void nativeRollbackBenchmark_Abort(const char* reason);
bool nativeRollbackBenchmark_IsResimulating(void);
bool nativeRollbackBenchmark_HasFailed(void);
#endif
#endif
