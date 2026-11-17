#include "native_fighter_build.h"
#include "native_fighter_authoring.h"
#include <cstdlib>
#include <cstring>

namespace NativeFighterBuild {
namespace {
bool fail(std::string& error, const std::string& text) { error = text; return false; }
bool same(const void* a, size_t an, const void* b, size_t bn)
{ return an == bn && (!an || (a && b && !std::memcmp(a, b, an))); }
bool piece(std::string& text, const void* data, size_t size)
{
    NativeContentId id{};
    if (!size) data = "";
    if (!nativeContent_Hash(data, size, &id)) return false;
    text.append((const char*)id.bytes, sizeof(id.bytes)); return true;
}
struct Packet {
    uint8_t* bytes = nullptr;
    size_t size = 0;
    NativeFighterPackage metadata{};
    NativeFighterPackageData data{};
    ~Packet() { std::free(bytes); }
    bool read(const char* root, const NativeContentId& expected, std::string& error) {
        NativeContentId actual{};
        return (root && nativeContentStore_Read(root, &expected, &bytes, &size) &&
            nativeContent_Hash(bytes, size, &actual) && !std::memcmp(&actual, &expected, sizeof(actual)) &&
            nativeContentStore_ParseFighter(bytes, size, &metadata, &data)) ||
            fail(error, "Could not read and verify the cached fighter package.");
    }
};
}

bool revision(const NativeFighterFamilyProject& family,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    std::string& result, std::string& error)
{
    if (!family.member_count || family.member_count > NATIVE_FIGHTER_FAMILY_MEMBERS)
        return fail(error, "A fighter revision needs an open family.");
    std::string fingerprint = std::to_string(family.mode) + ':' + std::to_string(family.member_count) + ';';
    bool ok = true;
    for (unsigned i = 0; ok && i < family.member_count; ++i) {
        const auto& member = family.members[i];
        auto* source = sources ? sources[i] : nullptr;
        std::string composed = source ? NativeFighterSources::compose(*source) : "";
        fingerprint += family.keys[i]; fingerprint += '\0';
        fingerprint += member.title; fingerprint += '\0'; fingerprint += member.tile_name; fingerprint += '\0';
        ok = piece(fingerprint, member.fighter_dat, member.fighter_size) &&
            piece(fingerprint, member.animation_dat, member.animation_size) &&
            piece(fingerprint, member.costume_dat, member.costume_size) &&
            piece(fingerprint, member.asset_record, member.asset_record_size) &&
            piece(fingerprint, member.resources, member.resource_size) &&
            piece(fingerprint, member.cpu_text, member.cpu_size) &&
            piece(fingerprint, source ? composed.data() : (const char*)member.script_text, source ? composed.size() : member.script_size) &&
            piece(fingerprint, source ? source->parameters.data() : (const char*)member.parameter_text,
                source ? source->parameters.size() : member.parameter_size);
        uint8_t* names = nullptr; size_t size = 0;
        ok = ok && nativeFighterAuthoring_Encode(&member, &names, &size) && piece(fingerprint, names, size);
        std::free(names);
    }
    NativeContentId id{}; char hex[65];
    if (!ok || !nativeContent_Hash(fingerprint.data(), fingerprint.size(), &id)) return fail(error, "Could not fingerprint the effective fighter data.");
    nativeContentStore_IdHex(&id, hex); result = hex; return true;
}

bool matches(const NativeFighterFamilyProject& family,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const NativeFighterPackage& metadata, const NativeFighterPackageData& data, std::string& error)
{
    if (!family.member_count || family.member_count > NATIVE_FIGHTER_FAMILY_MEMBERS)
        return fail(error, "Package matching needs an open fighter family.");
    if (metadata.member_count != family.member_count || metadata.mode != family.mode ||
        std::strcmp(metadata.title, family.members[0].title) || std::strcmp(metadata.tile_name, family.members[0].tile_name))
        return fail(error, "Package metadata differs from the current fighter family.");
    for (unsigned i = 0; i < family.member_count; ++i) {
        const auto& member = family.members[i]; const auto& built = data.members[i];
        auto* source = sources ? sources[i] : nullptr;
        std::string composed = source ? NativeFighterSources::compose(*source) : "";
        bool ok = !std::strcmp(metadata.members[i].key, family.keys[i]) &&
            same(member.fighter_dat, member.fighter_size, built.fighter_dat, built.fighter_size) &&
            same(member.animation_dat, member.animation_size, built.animation_dat, built.animation_size) &&
            same(member.costume_dat, member.costume_size, built.costume_dat, built.costume_size) &&
            same(member.asset_record, member.asset_record_size, built.asset_record, built.asset_record_size) &&
            same(member.resources, member.resource_size, built.resources, built.resource_size) &&
            same(member.cpu_text, member.cpu_size, built.cpu_text, built.cpu_size) &&
            same(source ? composed.data() : (const char*)member.script_text, source ? composed.size() : member.script_size,
                built.script_text, built.script_size) &&
            same(source ? source->parameters.data() : (const char*)member.parameter_text,
                source ? source->parameters.size() : member.parameter_size, built.parameter_text, built.parameter_size);
        if (!ok) return fail(error, std::string("Package differs from the current sources or owned assets of member ") + family.keys[i] + '.');
    }
    return true;
}

bool verify_cached(const NativeFighterFamilyProject& family,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const char* root, const NativeContentId& id, std::string& error)
{
    Packet packet;
    return packet.read(root, id, error) && matches(family, sources, packet.metadata, packet.data, error);
}

bool accept(NativeFighterFamilyProject& family,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const char* root, const NativeContentId& id, const std::string& expected,
    std::string& after, std::string& error)
{
    Packet packet;
    if (!packet.read(root, id, error) || !matches(family, sources, packet.metadata, packet.data, error)) return false;
    struct Candidate {
        NativeFighterFamilyProject family;
        ~Candidate() {
            for (unsigned i = 0; i < family.member_count; ++i) {
                std::free(family.members[i].script_text);
                std::free(family.members[i].parameter_text);
            }
        }
    } candidate{family};
    for (unsigned i = 0; i < family.member_count; ++i) {
        candidate.family.members[i].script_text = nullptr;
        candidate.family.members[i].parameter_text = nullptr;
        candidate.family.members[i].parameter_size = 0;
    }
    for (unsigned i = 0; i < family.member_count; ++i) {
        if (!sources || !sources[i]) return fail(error, "Load every member's editable sources before accepting behavior.");
        auto& part = candidate.family.members[i];
        const auto& built = packet.data.members[i];
        part.script_text = (uint8_t*)std::malloc(built.script_size + 1);
        if (!part.script_text) return fail(error, "Could not allocate accepted source storage.");
        if (built.script_size) std::memcpy(part.script_text, built.script_text, built.script_size);
        part.script_text[built.script_size] = 0;
        part.script_size = built.script_size;
    }
    if (!packet.data.portrait_png || !packet.data.portrait_size) return fail(error, "The accepted package needs a portrait PNG.");
    std::vector<NativeDraftAccess::FileEdit> additional{{"portrait.png", std::string((const char*)packet.data.portrait_png, packet.data.portrait_size)}};
    if (!NativeFighterSnapshot::save(candidate.family, sources, expected, after, error, additional)) return false;
    for (unsigned i = 0; i < family.member_count; ++i) {
        auto& current = family.members[i]; auto& built = candidate.family.members[i];
        std::free(current.script_text); std::free(current.parameter_text);
        current.script_text = built.script_text; current.script_size = built.script_size;
        current.parameter_text = built.parameter_text; current.parameter_size = built.parameter_size;
        built.script_text = nullptr; built.parameter_text = nullptr;
    }
    return true;
}
}
