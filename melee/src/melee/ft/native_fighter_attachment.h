#ifndef MELEE_FT_NATIVE_FIGHTER_ATTACHMENT_H
#define MELEE_FT_NATIVE_FIGHTER_ATTACHMENT_H

#ifdef MELEE_NATIVE
#include <melee/ft/dobjlist.h>
#include <dolphin/mtx.h>
#include <stdint.h>

enum { NATIVE_FIGHTER_ATTACHMENTS = 4, NATIVE_ATTACHMENT_DOBJS = 32 };

// Fixed per-fighter storage. HSD joints/dynamics use the ordinary tracked
// game heap; creation and destruction belong to scene load/unload callbacks.
typedef struct NativeFighterAttachment {
    HSD_JObj* joint;
    DObjList objects;
    HSD_DObj* object_data[NATIVE_ATTACHMENT_DOBJS];
    FtPartsVis visibility;
    uint16_t dynamic_mask;
    uint8_t bone;
    bool visible;
    bool has_visibility;
} NativeFighterAttachment;

bool nativeFighterAttachment_HasRoot(Fighter_GObj*, const char*);
bool nativeFighterAttachment_Load(Fighter_GObj*, unsigned slot,
                                  const char* root, unsigned bone,
                                  int visibility_entry, unsigned costume);
bool nativeFighterAttachment_AddDynamics(Fighter_GObj*, unsigned slot,
                                         unsigned first, unsigned count);
void nativeFighterAttachment_Clear(Fighter_GObj*);
void nativeFighterAttachment_Draw(Fighter_GObj*, int pass, Mtx view);
void nativeFighterAttachment_Visibility(Fighter*, int group, bool enabled);

#endif
#endif
