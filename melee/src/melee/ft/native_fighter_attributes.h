#ifndef MELEE_FT_NATIVE_FIGHTER_ATTRIBUTES_H
#define MELEE_FT_NATIVE_FIGHTER_ATTRIBUTES_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT = 97 };
typedef enum NativeFighterAttributeType {
    NativeAttribute_Float, NativeAttribute_Int, NativeAttribute_Mask
} NativeFighterAttributeType;
typedef struct NativeFighterAttribute {
    const char* name;
    const char* label;
    const char* group;
    const char* description;
    unsigned offset;
    NativeFighterAttributeType type;
} NativeFighterAttribute;
extern const NativeFighterAttribute nativeFighterCommonAttributes[NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT];
#ifdef __cplusplus
}
#endif
#endif
