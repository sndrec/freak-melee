#include "native_fighter_validation.h"
#include "native_source_package.h"
#include <stdlib.h>
#include <string.h>

bool nativeFighterProject_CheckSource(const NativeFighterProject* project,
    const char* source, size_t source_size, const char* parameters,
    size_t parameter_size, unsigned member, NativeFighterValidation* output)
{
    if (!project || !source || (!parameters && parameter_size) || !output ||
        (output->capacity && !output->entries) || member >= NATIVE_FIGHTER_FAMILY_MEMBERS)
        return false;
    NativeSourcePackage* candidate = calloc(1, sizeof(*candidate));
    if (!candidate) return false;
    NativeFighterValidation result = {.entries=output->entries, .capacity=output->capacity};
    NativeScriptCompileDiagnostics diagnostics = {.entries=result.entries, .capacity=result.capacity};
    NativeSourcePackageInput input = {
        .family=NativeSourcePackage_Single,
        .facts=project->asset_record, .facts_bytes=project->asset_record_size,
        .assets={project->fighter_dat, project->fighter_size, project->animation_dat,
            project->animation_size, project->costume_dat, project->costume_size},
        .source=source, .source_bytes=source_size,
        .parameters=parameters ? parameters : "", .parameter_bytes=parameter_size,
        .resources=project->resources, .resource_bytes=project->resource_size,
        .diagnostics=&diagnostics,
    };
    NativeScriptDiagnostic first = {0};
    result.accepted = nativeSourcePackage_Prepare(candidate, &input,
        (FighterKind)(Ft_Kind_CustomFirst + member), NULL, &first);
    result.count = diagnostics.count; result.total = diagnostics.total;
    result.metadata_complete = diagnostics.metadata_complete;
    result.bodies_complete = diagnostics.bodies_complete;
    result.functions_checked = diagnostics.functions_checked;
    if (!result.accepted && !result.total) {
        result.package_diagnostic = true;
        if (!*first.message) strcpy(first.message, "Project assets or source could not be prepared.");
        result.total = 1;
        if (result.capacity) { result.entries[0] = first; result.count = 1; }
    }
    if (result.accepted) {
        result.functions = candidate->stats.functions;
        result.instructions = candidate->stats.instructions;
        result.parameters = candidate->stats.parameters;
        result.imports = candidate->parameters.root_imports + candidate->parameters.article_imports;
    }
    bool released = nativeSourcePackage_Release(candidate);
    if (released) free(candidate);
    *output = result;
    return released;
}
