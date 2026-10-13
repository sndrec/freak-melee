#ifndef HSD_PSDISP_STATE_INTERNAL_H
#define HSD_PSDISP_STATE_INTERNAL_H
#include "forward.h"

// Sorting mutates the source particle chains and caches their render split.
extern u8 particle_display_frame;
extern u8 particle_sort_frames[16];
extern HSD_Particle* particle_non_edge_heads[17];
#endif
