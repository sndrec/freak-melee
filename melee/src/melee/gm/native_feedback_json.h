#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <yyjson.h>

typedef struct NativeFeedbackJsonBudget {
    size_t remaining;
    bool exhausted;
} NativeFeedbackJsonBudget;

// Charge the full request, including a resize that might briefly need both
// blocks. Keep the failure sticky: yyjson can omit a field after an allocation
// failure, so a caller must discard the whole document in that case.
static inline void* feedback_json_resize(void* context, void* pointer, size_t old_size, size_t size)
{
    NativeFeedbackJsonBudget* budget = (NativeFeedbackJsonBudget*)context;
    (void)old_size;
    if (!budget->exhausted && size <= budget->remaining) {
        void* bytes = realloc(pointer, size);
        if (bytes) {
            budget->remaining -= size;
            return bytes;
        }
    }
    budget->exhausted = true;
    return NULL;
}

static inline void* feedback_json_allocate(void* context, size_t size)
{
    return feedback_json_resize(context, NULL, 0, size);
}

static inline void feedback_json_free(void* context, void* pointer)
{
    (void)context;
    free(pointer);
}

static inline yyjson_alc nativeFeedbackJson_Allocator(NativeFeedbackJsonBudget* budget)
{
    yyjson_alc allocator = {feedback_json_allocate, feedback_json_resize, feedback_json_free, budget};
    return allocator;
}
