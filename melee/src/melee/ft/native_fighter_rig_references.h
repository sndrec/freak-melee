#ifndef MELEE_FT_NATIVE_FIGHTER_RIG_REFERENCES_H
#define MELEE_FT_NATIVE_FIGHTER_RIG_REFERENCES_H
#include "native_fighter_project.h"
#include "ftparts.h"

typedef struct NativeFighterRigReferences {
    uint8_t* fighter;
    size_t fighter_size;
    unsigned metadata_fields, command_fields;
} NativeFighterRigReferences;
#ifdef __cplusplus
extern "C" {
#endif
// Authoring transition. old_to_new covers all physical slots, including article
// reservations; UINT16_MAX removes a slot. A referenced removed slot rejects.
// Physical metadata/subaction fields change; semantic roles and script IDs keep
// their values. Native dynamics chains must remain contiguous. Retail hitboxes
// whose following-command victim flag would change require interval conversion.
// No project state
// changes; output must own no buffer and receives a malloc buffer on success.
bool nativeFighterRig_RemapReferences(const NativeFighterProject*,
    const uint16_t old_to_new[MAX_FT_PARTS], NativeFighterRigReferences*,
    char* error,size_t error_size);
#ifdef __cplusplus
}
#endif
#endif
