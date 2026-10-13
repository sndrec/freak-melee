#ifndef MELEE_IT_NATIVE_SOURCE_ITEM_VISUAL_H
#define MELEE_IT_NATIVE_SOURCE_ITEM_VISUAL_H

#include <melee/ft/native_script_ir.h>

struct NativeScriptCompileBank;
struct NativeScriptItemPrograms;
enum { NATIVE_SOURCE_ITEM_VISUALS=64 };

typedef struct NativeSourceItemVisual {
    const struct NativeScriptCompileBank* bank;
    uint64_t identity;
    uint32_t owner;
    const uint8_t* png;
    uint32_t png_bytes;
    uint8_t* pixels;
    uint16_t width,height;
    float size;
} NativeSourceItemVisual;

// The package embeds this resource owner at its final address. Prepared program
// copies retain its external visual pointers; no visual moves with a registry.
typedef struct NativeSourceItemVisuals {
    const struct NativeSourceItemVisuals* address;
    NativeSourceItemVisual records[NATIVE_SOURCE_ITEM_VISUALS];
    uint32_t count;
    bool ready;
} NativeSourceItemVisuals;

#ifdef __cplusplus
extern "C" {
#endif
// Vacant output and unbound candidate programs only. Decode at an idle load
// transition; a failed candidate leaves every program.visual unchanged.
bool nativeSourceItemVisuals_Prepare(NativeSourceItemVisuals*,
    struct NativeScriptItemPrograms*,NativeScriptDiagnostic*);
// Release only after this bank is unselected, exact actor references drain and
// no proc/VM/draw is running. Failure leaves every decoded resource untouched.
bool nativeSourceItemVisuals_Release(NativeSourceItemVisuals*);
#ifdef __cplusplus
}
#endif
#endif
