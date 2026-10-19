#include "native_fighter_authoring.h"
#ifdef MELEE_NATIVE
#include <stdlib.h>
#include <string.h>
static char* name_at(NativeFighterAuthoring* names, unsigned category,
                     unsigned key, bool create)
{
    if (!names || category >= NativeAuthoring_CategoryCount) {
        return NULL;
    }
    unsigned slot = (key * 2654435761u + category * 97u) & 4095;
    for (unsigned i = 0; i < 4096; ++i, slot = (slot + 1) & 4095) {
        if (!names->labels[slot].used) {
            if (!create) {
                return NULL;
            }
            names->labels[slot].used = true;
            names->labels[slot].key = key;
            names->labels[slot].category = category;
        }
        if (names->labels[slot].category == category &&
            names->labels[slot].key == key)
        {
            return names->labels[slot].name;
        }
    }
    return NULL;
}
const char* nativeFighterAuthoring_Name(const NativeFighterProject* p,
                                        unsigned category, unsigned index)
{
    return p ? name_at(p->authoring, category, index, false) : NULL;
}
bool nativeFighterAuthoring_SetName(NativeFighterProject* p, unsigned category,
                                    unsigned index, const char* value)
{
    if (!p || !value || strlen(value) >= 64 ||
        category >= NativeAuthoring_CategoryCount)
    {
        return false;
    }
    for (const unsigned char* c = (const unsigned char*) value; *c; ++c) {
        if (*c < 32 || *c == 127) {
            return false;
        }
    }
    if (!p->authoring && !(p->authoring = calloc(1, sizeof(*p->authoring)))) {
        return false;
    }
    char* target = name_at(p->authoring, category, index, true);
    if (!target) {
        return false;
    }
    strcpy(target, value);
    return true;
}
const char* nativeFighterAuthoring_BoneRole(const NativeFighterProject* p,
                                            unsigned joint)
{
    static const char* const roles[] = {
        "TopN",       "TransN",      "XRotN",      "YRotN",       "HipN",
        "WaistN",     "LLegJA",      "LLegJ",      "LKneeJ",      "LFootJA",
        "LFootJ",     "RLegJA",      "RLegJ",      "RKneeJ",      "RFootJA",
        "RFootJ",     "BustN",       "LShoulderN", "LShoulderJA", "LShoulderJ",
        "LArmJ",      "LHandN",      "L1stNa",     "L1stNb",      "L2ndNa",
        "L2ndNb",     "L3rdNa",      "L3rdNb",     "L4thNa",      "L4thNb",
        "LThumbNa",   "LThumbNb",    "LHandNb",    "NeckN",       "HeadN",
        "RShoulderN", "RShoulderJA", "RShoulderJ", "RArmJ",       "RHandN",
        "R1stNa",     "R1stNb",      "R2ndNa",     "R2ndNb",      "R3rdNa",
        "R3rdNb",     "R4thNa",      "R4thNb",     "RThumbNa",    "RThumbNb",
        "RHandNb",    "ThrowN",      "TransN2"
    };
    if (!p) {
        return NULL;
    }
    // Costume traversal omits reserved accessory parts. The role map uses
    // part indices, so account for those gaps before looking up a role.
    unsigned part = joint;
    for (unsigned i = 0; i < p->facts.omission_count; ++i) {
        if (p->facts.omissions[i].joint <= part) {
            ++part;
        }
    }
    for (unsigned i = 0; i < p->facts.bone_count; ++i) {
        if (p->facts.bones[i].joint == part &&
            (unsigned) p->facts.bones[i].part <
                sizeof(roles) / sizeof(roles[0]))
        {
            return roles[p->facts.bones[i].part];
        }
    }
    return NULL;
}
bool nativeFighterAuthoring_Hidden(const NativeFighterProject* p,
                                   unsigned action)
{
    return p && p->authoring && action < 1024 &&
           p->authoring->hidden_actions[action];
}
bool nativeFighterAuthoring_SetHidden(NativeFighterProject* p, unsigned action,
                                      bool hidden)
{
    if (!p || action >= 1024) {
        return false;
    }
    if (!p->authoring && !(p->authoring = calloc(1, sizeof(*p->authoring)))) {
        return false;
    }
    p->authoring->hidden_actions[action] = hidden;
    return true;
}
#endif
