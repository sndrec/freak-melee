#include "native_render_phase.h"
#ifdef MELEE_NATIVE
#pragma clang section data = "" bss = ""
#endif
bool nativeRenderGeometryEnabled=true;
bool nativeRenderGraphicsEnabled=true;
uint64_t nativeRenderModelPrimitivesSkipped;
uint64_t nativeRenderParticleBatchesSkipped;
