#include "native_fighter_state.h"
#include <string.h>

static bool valid_name(const char* name)
{
    if (name == NULL) return false;
    for (unsigned i = 0; i < 64; ++i) {
        unsigned char c = (unsigned char) name[i];
        if (c == 0) return i != 0;
        bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        if (!letter && c != '_' && (i == 0 || c < '0' || c > '9'))
            return false;
    }
    return false;
}

bool nativeFighterState_Validate(const NativeFighterStateField* fields,
                                unsigned count, unsigned declared_bytes,
                                unsigned capacity)
{
    if (declared_bytes > capacity || count > 64 ||
        ((count == 0) != (fields == NULL))) return false;
    for (unsigned i = 0; i < count; ++i) {
        const NativeFighterStateField* field = &fields[i];
        unsigned stride = field->type == NativeFighterState_Bool ? 1 : 4;
        unsigned end = field->offset + field->count * stride;
        if (!valid_name(field->name) || field->type > NativeFighterState_Int ||
            field->stride != stride || field->count == 0 ||
            field->count > 128 || (!field->array && field->count != 1) ||
            field->offset % stride != 0 || end > declared_bytes)
            return false;
        for (unsigned j = 0; j < i; ++j) {
            const NativeFighterStateField* prior = &fields[j];
            unsigned prior_end = prior->offset + prior->count * prior->stride;
            if (!strcmp(field->name, prior->name) ||
                (field->offset < prior_end && prior->offset < end))
                return false;
        }
    }
    return true;
}
