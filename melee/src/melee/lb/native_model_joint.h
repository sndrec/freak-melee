#ifndef MELEE_LB_NATIVE_MODEL_JOINT_H
#define MELEE_LB_NATIVE_MODEL_JOINT_H

#include <sysdolphin/baselib/forward.h>

#ifdef MELEE_NATIVE
// Root-relative child ordinals separated by '/'; empty selects root.
// Each ordinal is 0..255 and a path may descend at most 12 levels.
HSD_JObj* nativeModelJoint_Resolve(HSD_JObj* root, const char* path);

// Channels 0..8: translation XYZ, Euler rotation XYZ, scale XYZ.
// 9: existence (read-only); 10: subtree visibility; 11..14: raw quaternion
// XYZW; 15: self visibility. Visibility reads the selected local hidden flag,
// not inherited visibility. Subtree writes preserve HSD instance boundaries.
// Euler rotation channels ignore quaternion joints. Writes require finite
// values and use the actual HSD setters, preserving matrix dirty propagation.
// Missing joints read zero except scale and quaternion W, which read one.
float nativeModelJoint_Read(HSD_JObj* joint, unsigned channel);
void nativeModelJoint_Write(HSD_JObj* joint, unsigned channel, float value);
// Place an effect at the requested Z angle, retaining its authored X/Y angles.
void nativeModelJoint_SetRoll(HSD_JObj* joint, float radians);
#endif

#endif
