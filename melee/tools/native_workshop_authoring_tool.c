#include "native_workshop_authoring_tool.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/ft/native_fighter_animation_editor.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_material_animation.h>
static bool number(const char* text, unsigned* value)
{
    char* end;
    unsigned long n = strtoul(text, &end, 10);
    if (!*text || *end || n > UINT_MAX) {
        return false;
    }
    *value = (unsigned) n;
    return true;
}
bool nativeWorkshopAuthoring_Command(int count, char** args, int* result)
{
    if (count < 1) {
        return false;
    }
    bool create = !strcmp(args[0], "project-clip-create"),
         copy = !strcmp(args[0], "project-clip-copy"),
         labels = !strcmp(args[0], "project-label"),
         hidden = !strcmp(args[0], "project-hide-action"),
         sequences = !strcmp(args[0], "project-texture-sequences"),
         image = !strcmp(args[0], "project-texture-image"),
         meshes = !strcmp(args[0], "project-import-meshes");
    if (!create && !copy && !labels && !hidden && !sequences && !image &&
        !meshes)
    {
        return false;
    }
    *result = 1;
    if (count < 2) {
        return true;
    }
    NativeFighterProject p;
    if (!nativeFighterProject_Open(args[1], &p)) {
        return true;
    }
    bool ok = false;
    unsigned a = 0, b = 0, c = 0;
    char error[192] = {};
    if (create && count == 5 && number(args[2], &a) && number(args[4], &b)) {
        ok = nativeFighterAnimation_CreateRest(&p, a, args[3], b) &&
             nativeFighterProject_SaveAnimation(args[1], &p);
    }
    if (copy && count == 6 && number(args[3], &a) && number(args[4], &b) &&
        (!strcmp(args[2], "action") || !strcmp(args[2], "demo")))
    {
        NativeFighterAnimationClip source = {
            !strcmp(args[2], "demo") ? NativeFighterAnimation_Demo
                                     : NativeFighterAnimation_Action,
            a
        };
        ok = nativeFighterAnimation_Duplicate(&p, source, b, args[5]) &&
             nativeFighterProject_SaveAnimation(args[1], &p);
    }
    if (labels && count == 5 && number(args[3], &a)) {
        const char* categories[] = { "bone", "mesh", "material", "action" };
        unsigned category = 0;
        while (category < 4 && strcmp(categories[category], args[2])) {
            ++category;
        }
        ok = category < 4 &&
             nativeFighterAuthoring_SetName(&p, category, a, args[4]) &&
             nativeFighterAuthoring_Save(args[1], &p);
    }
    if (hidden && count == 4 && number(args[2], &a) && number(args[3], &b) &&
        b < 2)
    {
        ok = nativeFighterAuthoring_SetHidden(&p, a, b != 0) &&
             nativeFighterAuthoring_Save(args[1], &p);
    }
    if (sequences && count == 3 && number(args[2], &a)) {
        NativeFighterModelEditor* model = calloc(1, sizeof(*model));
        if (model && nativeFighterModel_Open(model, &p) &&
            a < model->material_count)
        {
            NativeMaterialTextureSequence entries[128];
            unsigned n =
                nativeFighterMaterial_Sequences(model, a, entries, 128);
            for (unsigned i = 0; i < n; ++i) {
                printf("sequence %u offset %u layer %u images %u\n", i,
                       entries[i].offset, entries[i].layer, entries[i].frames);
            }
            ok = true;
        }
        free(model);
    }
    if (image && count == 6 && number(args[2], &a) && number(args[3], &b) &&
        number(args[4], &c))
    {
        ok = nativeFighterGltf_SetTextureImage(&p, a, 0, b, c, args[5], error,
                                               sizeof(error)) &&
             nativeFighterProject_SaveCostume(args[1], &p);
    }
    if (meshes && count == 6 && number(args[3], &a)) {
        char* end;
        float scale = strtof(args[4], &end);
        NativeGltfScan scan;
        NativeGltfBindingMode binding = !strcmp(args[5], "auto")
                                            ? NATIVE_GLTF_BIND_AUTOMATIC
                                            : !strcmp(args[5], "source")
                                            ? NATIVE_GLTF_BIND_SOURCE : NATIVE_GLTF_BIND_RIGID;
        if (*args[4] && !*end && isfinite(scale) &&
            (!strcmp(args[5], "auto") || !strcmp(args[5], "rigid") || !strcmp(args[5], "source")) &&
            nativeFighterGltf_Scan(args[2], &scan, error, sizeof(error)))
        {
            NativeGltfImportRequest requests[NATIVE_GLTF_MAX_PRIMITIVES];
            for (unsigned i = 0; i < scan.count; ++i) {
                requests[i] =
                    (NativeGltfImportRequest){ i, a, UINT_MAX, binding };
            }
            ok =
                nativeFighterGltf_ApplyBatch(&p, args[2], requests, scan.count,
                                             scale, error, sizeof(error)) &&
                nativeFighterProject_SaveCostume(args[1], &p) &&
                nativeFighterAuthoring_Save(args[1], &p);
        }
    }
    if (!ok) {
        fprintf(stderr, "Authoring operation failed: %s\n",
                *error ? error : args[0]);
    }
    nativeFighterProject_Close(&p);
    *result = ok ? 0 : 1;
    return true;
}
