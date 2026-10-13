#include "psdisp_state_internal.h"

// Actual storage, used directly by the display and snapshot owners.
u8 particle_display_frame = 0x7B;
u8 particle_sort_frames[16];
HSD_Particle* particle_non_edge_heads[17];
