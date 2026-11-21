#ifndef HSD_PARTICLE_STATE_INTERNAL_H
#define HSD_PARTICLE_STATE_INTERNAL_H
#include "particle.h"
#include "generator.h"
#include "native_particle_limits.h"

extern u32 particle_float_bits;
extern int (**particle_callbacks)(HSD_Particle*);
extern u16 particle_peak_count,particle_count,generator_id_cursor;
extern HSD_JObj* particle_attachment_joints[8];
extern HSD_Particle* hsd_804D0908[16];
extern u32* hsd_804D0948[65];
extern HSD_PSFormGroup** psFormGroupArray[65];
extern HSD_Generator* pending_generators;
extern u16 appsrt_configuration[4];
#endif
