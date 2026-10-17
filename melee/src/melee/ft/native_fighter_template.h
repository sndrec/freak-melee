#pragma once
#include <melee/ft/forward.h>
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// Read the bundled stock behavior template. The caller frees the returned
// source.
bool nativeFighterTemplate_Read(FighterKind kind, char** source, size_t* size);
// Copy a stock template and its owned parameters into a fresh draft. Existing
// behavior and parameter files are preserved. Package preparation validates
// it.
bool nativeFighterTemplate_Apply(const char* path);
#ifdef __cplusplus
}
#endif
