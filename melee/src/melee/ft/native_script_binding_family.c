#include "native_script_binding_family.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <string.h>

typedef struct FamilyCaller {
    NativeFighterLifetime actor;
    const NativeSourcePackage* package;
    const NativeScriptMoveLayout* layout;
} FamilyCaller;

static bool error(NativeScriptDiagnostic* d, const char* message)
{
    if (d) snprintf(d->message, sizeof(d->message), "%s", message);
    return false;
}

static bool current(NativeScriptRuntimeContext* c, const FamilyCaller* saved)
{
    return c && c->source_class == NativeCallbackSource_Fighter &&
        c->source == saved->actor.object && c->generation == saved->actor.generation &&
        c->scene_epoch == saved->actor.reference.lifetime &&
        nativeFighterLifetime_Current(&saved->actor) &&
        nativeFighterRuntime_SourcePackage(saved->actor.kind) == saved->package &&
        nativeScriptBindings_SourceFighterLayout(c, NULL) == saved->layout;
}

static bool capture(NativeScriptRuntimeContext* c, FamilyCaller* saved,
                    NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference physical;
    if (!c || c->source_class != NativeCallbackSource_Fighter || !c->scene_epoch ||
        !nativeMatchPool_Reference(&nativeGObjPool, c->source, &physical) ||
        physical.lifetime != c->scene_epoch ||
        ((HSD_GObj*) c->source)->classifier != HSD_GOBJ_CLASS_FIGHTER ||
        !nativeScriptBindings_SourceAlive(c) ||
        !nativeFighterLifetime_Capture(((HSD_GObj*) c->source)->user_data, &saved->actor) ||
        !saved->actor.source_owned)
        return error(d, "Family operation requires its live owned Fighter scene");
    saved->package = nativeFighterRuntime_SourcePackage(saved->actor.kind);
    saved->layout = nativeScriptBindings_SourceFighterLayout(c, d);
    if (!saved->package || !saved->layout || !current(c, saved))
        return error(d, "Family operation selected package, program or storage changed");
    return true;
}

static bool member_index(const NativeScriptValue* v, unsigned* index, bool* present)
{
    uint64_t bits;
    *present = false;
    switch (v->type) {
    case NativeScriptType_Bool:
        if (v->as.u64 > 1) return false;
        bits = v->as.u64; break;
    case NativeScriptType_I32:
        if (v->as.i64 < INT32_MIN || v->as.i64 > INT32_MAX) return false;
        if (v->as.i64 < 0) return true;
        bits = (uint64_t) v->as.i64; break;
    case NativeScriptType_I64:
        if (v->as.i64 < 0) return true;
        bits = (uint64_t) v->as.i64; break;
    case NativeScriptType_U32:
        if (v->as.u64 > UINT32_MAX) return false;
        bits = v->as.u64; break;
    case NativeScriptType_U64: bits = v->as.u64; break;
    default: return false;
    }
    if (bits < NATIVE_FIGHTER_FAMILY_MEMBERS) {
        *index = (unsigned) bits;
        *present = true;
    }
    return true;
}

static bool retain_member(uint32_t id, uint64_t epoch, NativeFighterLifetime* saved)
{
    if (!HSD_GObjPLinkHead) return false;
    unsigned visited = 0;
    for (Fighter_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         object && visited++ < nativeGObjPool.capacity; object = object->next) {
        NativeMatchPoolReference physical;
        uint32_t generation;
        if (!nativeMatchPool_Reference(&nativeGObjPool, object, &physical) ||
            physical.lifetime != epoch || object->classifier != HSD_GOBJ_CLASS_FIGHTER ||
            !nativeCallbackSource_Snapshot(object, NativeCallbackSource_Fighter, &generation) ||
            generation != id) continue;
        return nativeFighterLifetime_Capture(object->user_data, saved);
    }
    return false;
}

bool nativeScriptBindingFamily_Invoke(NativeScriptRuntimeContext* c,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* d)
{
    bool lookup = semantic == NativeScriptBinding_FamilyMemberId;
    if ((!lookup && semantic != NativeScriptBinding_FamilyRetireCompanionOnSleep) ||
        !arguments || count != 1 || !output)
        return error(d, "Family operation argument shape is invalid");
    unsigned index = 0;
    bool present = false;
    if (lookup ? !member_index(arguments, &index, &present) :
        arguments->type != NativeScriptType_Bool || arguments->as.u64 > 1)
        return error(d, "Family operation requires an integral index or canonical Bool");
    FamilyCaller source = {0};
    if (!capture(c, &source, d)) return false;
    NativeScriptValue value = {0};
    NativeFighterLifetime member = {0};
    uint32_t id = 0;
    if (lookup) {
        if (present) id = nativeFighterRuntime_MemberId(source.actor.object, index);
        if (id == source.actor.generation) member = source.actor;
        else if (id && !retain_member(id, c->scene_epoch, &member))
            return error(d, "Resolved family member has no current physical Fighter identity");
        int32_t signed_id;
        memcpy(&signed_id, &id, sizeof(signed_id));
        value.type = NativeScriptType_I32;
        value.as.i64 = signed_id;
    } else {
        source.actor.data->x2222_b5 = arguments->as.u64;
        value.type = NativeScriptType_Void;
    }
    if (!current(c, &source) || (id && !nativeFighterLifetime_Current(&member)))
        return error(d, "Family caller or resolved member retired before publication");
    *output = value;
    return true;
}
