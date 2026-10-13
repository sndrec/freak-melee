#ifndef MELEE_LB_LBREFRACT_DATA_H
#define MELEE_LB_LBREFRACT_DATA_H

#include <Runtime/platform.h>

typedef struct lbRefractData {
    u8 count;
    f32* parameters;
} lbRefractData;

bool lbRefractData_Import(const void* bytes, size_t size,
                          lbRefractData* output, f32* parameters,
                          size_t parameter_capacity);

#endif
