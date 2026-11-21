#ifndef HSD_NATIVE_PARTICLE_LIMITS_H
#define HSD_NATIVE_PARTICLE_LIMITS_H

#include <dolphin/types.h>

enum {
    HSD_GENERATORS_PER_TICK = 256,
    HSD_GENERATORS_LIVE = 512,
    HSD_PARTICLES_PER_TICK = 1024,
    HSD_PARTICLES_LIVE = 4096,
};

typedef struct HSD_ParticleWork {
    u32 generators;
    u32 emissions;
    u32 particles;
} HSD_ParticleWork;

extern HSD_ParticleWork particle_work_this_tick;

#endif
