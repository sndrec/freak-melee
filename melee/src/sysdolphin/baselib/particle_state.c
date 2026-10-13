#include "particle_state_internal.h"

// Actual particle/generator/AppSRT storage, accessed directly by source code.
// Allocator descriptors have their own shared ObjAlloc snapshot owner.
u32 particle_float_bits;
int (**particle_callbacks)(HSD_Particle*);
u16 hsd_804D78D8=0,hsd_804D78DA=0,particle_peak_count;
u16 hsd_804D78DE=0,hsd_804D78E0=0,particle_count;
void (*hsd_804D78E8)(HSD_Generator*,Mtx)=NULL;
void (*hsd_804D78EC)(HSD_Generator*)=NULL;
HSD_CObj* psCamera=NULL;
HSD_SList* hsd_804D78F4=NULL;
HSD_JObj* particle_attachment_joints[8];
HSD_Particle* hsd_804D0908[16];
u32* hsd_804D0948[65];
HSD_PSFormGroup** psFormGroupArray[65];
HSD_PSTexGroup** psTexGroupArray[65];
HSD_PSFormGroup** psNumCmdList[65];
int psCmdListArray[65];
HSD_PSCmdList** ptclref_804D0E5C[65];
struct hsd_804D0F60_t hsd_804D0F60;
HSD_Generator* pending_generators;
HSD_Generator* hsd_804D78FC=NULL;
void (*hsd_804D7900)(HSD_Generator*)=NULL;
struct hsd_804D0F60_t hsd_804D0F90;
u16 generator_id_cursor=0x100;
u16 appsrt_configuration[4];
HSD_ObjAllocData HSD_PSAppSrt_804D10B0;
