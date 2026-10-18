#include "psdisp_state_internal.h"

// Actual storage, used directly by the display and snapshot owners.
u8 particle_display_frame = PS_DISPLAY_FRAME_INITIAL;
u8 particle_sort_frames[16];
HSD_Particle* particle_non_edge_heads[17];
