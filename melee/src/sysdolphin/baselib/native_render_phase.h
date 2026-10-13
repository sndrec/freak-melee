#ifndef HSD_NATIVE_RENDER_PHASE_H
#define HSD_NATIVE_RENDER_PHASE_H
#include <stdbool.h>
#include <stdint.h>
// Host render policy, never snapshot state. State preparation still runs when
// geometry is disabled: envelope joints, callbacks and RNG keep their order.
extern bool nativeRenderGeometryEnabled;
// False only during a GPU-free pass. Resource identities and CPU pose/state
// updates still run; graphics-only HSD commands can be omitted.
extern bool nativeRenderGraphicsEnabled;
extern uint64_t nativeRenderModelPrimitivesSkipped;
extern uint64_t nativeRenderParticleBatchesSkipped;
#endif
