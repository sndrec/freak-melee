#include "native_fighter_snapshot.h"
#ifdef MELEE_NATIVE
#include "native_fighter_authoring.h"
#include "native_fighter_parameters.h"
#include <cstdlib>
#include <cstring>
#include <memory>

namespace NativeFighterSnapshot {
namespace {
bool fail(std::string& error, const std::string& text) { error = text; return false; }
bool add(Capture& next, const std::string& prefix, const char* name,
         const void* bytes, size_t size, bool required, std::string& error)
{
    if ((size && !bytes) || size > NATIVE_CONTENT_MAX_PACKAGE_BYTES || (required && !size))
        return fail(error, std::string("Missing or oversized draft file: ") + name);
    next.files.push_back({prefix + name, std::string(size ? (const char*)bytes : "", size), !size && !required});
    return true;
}
}

bool capture(const NativeFighterFamilyProject& family,
             NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
             Capture& output, std::string& error)
{
    if (!family.member_count || family.member_count > NATIVE_FIGHTER_FAMILY_MEMBERS)
        return fail(error, "Capture needs an open fighter family.");
    Capture next;
    for (unsigned i = 0; i < family.member_count; ++i) {
        const auto& member = family.members[i];
        const std::string prefix = i ? "partner/" : "";
        char message[512]{};
        if (!nativeFighterProject_ValidateAssets(&member, message, sizeof(message)))
            return fail(error, std::string(family.keys[i]) + ": " + message);
        NativeSourceResources resources{};
        NativeFighterCpuProfile cpu{};
        if (!nativeSourceResources_Read(member.resources, member.resource_size, &resources, message, sizeof(message)))
            return fail(error, std::string(family.keys[i]) + ": " + message);
        if (!nativeFighterCpu_Parse(member.cpu_text ? (const char*)member.cpu_text : "", member.cpu_size, &cpu))
            return fail(error, std::string(family.keys[i]) + ": invalid CPU profile.");
        auto* source = sources ? sources[i] : nullptr;
        if (source && _stricmp(source->root.c_str(), family.paths[i]))
            return fail(error, "Editable sources belong to another family member.");
        const char* parameters = source ? source->parameters.data() : (const char*)member.parameter_text;
        size_t parameter_size = source ? source->parameters.size() : member.parameter_size;
        NativeFighterParameters parsed{}; unsigned line = 0;
        if (parameter_size > NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY ||
            !nativeFighterParameters_Parse(parameters ? parameters : "", parameter_size, &parsed, &line))
            return fail(error, std::string(family.keys[i]) + ": invalid parameter on line " + std::to_string(line) + '.');
        if (!add(next, prefix, "fighter.dat", member.fighter_dat, member.fighter_size, true, error) ||
            !add(next, prefix, "animation.dat", member.animation_dat, member.animation_size, true, error) ||
            !add(next, prefix, "costume.dat", member.costume_dat, member.costume_size, true, error) ||
            !add(next, prefix, "assets.fma", member.asset_record, member.asset_record_size, true, error) ||
            !add(next, prefix, "resources.fmr", member.resources, member.resource_size, false, error) ||
            !add(next, prefix, "behavior.fms", member.script_text, member.script_size, false, error) ||
            !add(next, prefix, "fighter_parameters.fmp", parameters, parameter_size, false, error) ||
            !add(next, prefix, "cpu_profile.fcp", member.cpu_text, member.cpu_size, true, error)) return false;
        uint8_t* names = nullptr; size_t names_size = 0;
        if (!nativeFighterAuthoring_Encode(&member, &names, &names_size))
            return fail(error, "Could not encode bone and mesh names.");
        bool names_ok = add(next, prefix, "workshop_names.txt", names, names_size, false, error);
        std::free(names);
        if (!names_ok) return false;
        // The draft manifest remains owned by the existing manifest operations.
        // Capture only files currently held and edited by the Workshop.
        if (source) {
            std::vector<NativeDraftAccess::FileEdit> edits;
            if (!NativeFighterSources::stage_save(*source, edits)) return fail(error, source->error);
            for (auto& edit : edits) { edit.file = prefix + edit.file; next.files.push_back(std::move(edit)); }
        }
    }
    output = std::move(next); error.clear(); return true;
}

bool save(NativeFighterFamilyProject& family,
          NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
          const std::string& expected, std::string& after, std::string& error)
{
    // Allocate the post-save parameter buffers before publishing disk changes.
    std::unique_ptr<uint8_t, decltype(&std::free)> parameters[NATIVE_FIGHTER_FAMILY_MEMBERS] = {
        {nullptr, &std::free}, {nullptr, &std::free}};
    if (!family.member_count || family.member_count > NATIVE_FIGHTER_FAMILY_MEMBERS)
        return fail(error, "Save needs an open fighter family.");
    std::string current;
    if (!NativeDraftAccess::revision(family.paths[0], current, error)) return false;
    if (current != expected) return fail(error, "Draft changed outside the Workshop. Reopen before saving; your edits are retained.");
    for (unsigned i = 0; i < family.member_count; ++i) if (sources && sources[i]) {
        const auto& text = sources[i]->parameters;
        if (text.size() > NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY)
            return fail(error, "Parameter text exceeds its capacity.");
        parameters[i].reset((uint8_t*)std::malloc(text.size() + 1));
        if (!parameters[i]) return fail(error, "Could not allocate saved parameter storage.");
        std::memcpy(parameters[i].get(), text.c_str(), text.size() + 1);
    }
    Capture snapshot;
    if (!capture(family, sources, snapshot, error) ||
        !NativeDraftAccess::apply(family.paths[0], expected, snapshot.files, after, error)) return false;
    for (unsigned i = 0; i < family.member_count; ++i) if (sources && sources[i]) {
        std::free(family.members[i].parameter_text);
        family.members[i].parameter_text = parameters[i].release();
        family.members[i].parameter_size = sources[i]->parameters.size();
        NativeFighterSources::mark_saved(*sources[i]);
    }
    return true;
}
}
#endif
