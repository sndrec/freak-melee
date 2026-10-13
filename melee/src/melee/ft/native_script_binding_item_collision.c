#include "native_script_binding_item_collision.h"
#include "native_script_bindings.h"
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic, const char* message)
{
    if (diagnostic)
        snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    return false;
}

static bool current(NativeScriptRuntimeContext* context,
                    const NativeItemLifetime* source)
{
    return context && context->source == source->object &&
        context->source_class == NativeCallbackSource_Item &&
        context->generation == source->generation &&
        context->scene_epoch == source->reference.lifetime &&
        nativeItemLifetime_Current(source) &&
        nativeScriptBindings_SourceAlive(context) &&
        source->program->article == source->article &&
        source->program->owner == source->data->native_script_owner &&
        source->program->kind == source->kind &&
        source->program->author_kind == source->data->native_article_author_kind;
}

static bool line_operation(NativeScriptRuntimeContext* context, uint32_t semantic,
    NativeScriptValue* arguments, uint32_t count, NativeScriptValue* output,
    NativeScriptDiagnostic* diagnostic)
{
    if (!arguments || !output || count != 1)
        return error(diagnostic, "Item line operation argument shape is invalid");
    int line;
    switch (arguments[0].type) {
    case NativeScriptType_Bool:
        if (arguments[0].as.u64 > 1)
            return error(diagnostic, "Item line index contains a malformed Bool");
        line = (int) arguments[0].as.u64; break;
    case NativeScriptType_I32: case NativeScriptType_I64:
        if (arguments[0].as.i64 < INT32_MIN || arguments[0].as.i64 > INT32_MAX)
            return error(diagnostic, "Item line index exceeds native signed 32-bit width");
        line = (int32_t) arguments[0].as.i64; break;
    case NativeScriptType_U32: case NativeScriptType_U64:
        if (arguments[0].as.u64 > INT32_MAX)
            return error(diagnostic, "Item line index exceeds native signed 32-bit width");
        line = (int32_t) arguments[0].as.u64; break;
    default: return error(diagnostic, "Item line index must be integral");
    }
    NativeItemLifetime source;
    if (!context || context->source_class != NativeCallbackSource_Item ||
        !context->scene_epoch || !nativeItemLifetime_Capture(context->source, &source) ||
        !source.source_owned || !current(context, &source))
        return error(diagnostic, "Item line operation requires its exact live owned Article program/storage");
    MapCollData* map = mpLib_8004D164();
    CollLine* lines = mpGetGroundCollLine();
    CollVtx* vertices = mpGetGroundCollVtx();
    CollJoint* joints = mpGetGroundCollJoint();
    if (!map || !lines || line < 0 || line >= map->line_count ||
        !mpLib_80054ED8(line))
        return error(diagnostic, "Item line operation requires a present enabled collision line");
    MapLine* description = lines[line].x0;
    if (!description || !vertices || description->v0_idx >= map->vert_count ||
        description->v1_idx >= map->vert_count)
        return error(diagnostic, "Item collision line has no live vertex geometry");
    uint16_t v0 = description->v0_idx, v1 = description->v1_idx;
    int line_count = map->line_count, vertex_count = map->vert_count,
        joint_count = map->joint_count;
    NativeScriptValue result = {.type = NativeScriptType_Void};
    if (semantic == NativeScriptBinding_ItemBindStageLine) {
        CollData* collision = &source.data->x378_itemColl;
        if (collision->x0_gobj != source.object || collision->x34_flags.b1234 == 1)
            return error(diagnostic, "Item contact requires its actual Item collision actor/category");
        int kind = mpLineGetKind(line);
        if (kind == CollLine_Floor || kind == CollLine_Ceiling) {
            if (joint_count < 0 || (joint_count && !joints))
                return error(diagnostic, "Item contact has no live stage joint geometry");
            for (int i = 0; i < joint_count; ++i) {
                if (!joints[i].inner)
                    return error(diagnostic, "Item contact stage joint has no vertex range");
                // The native lookup stops at the first matching range. Later
                // joints are not part of this operation's accessed geometry.
                const MapJoint* range = joints[i].inner;
                if (range->vtx_start <= v0 && v0 < range->vtx_start + range->vtx_count)
                    break;
            }
            int joint = mpJointFromLine(line);
            if (joint != -1) {
                if (joint < 0 || joint >= joint_count)
                    return error(diagnostic, "Item contact joint index is outside live geometry");
                mpLib_JointCollisionCallback callback = NULL;
                void* data = NULL;
                if (kind == CollLine_Floor) mpJointGetCb1(joint, &callback, &data);
                else mpJointGetCb2(joint, &callback, &data);
                // mpColl_80043558 converts this float to the callback ABI's int
                // before entering the selected callback. Bound only that path.
                if (callback && (!isfinite(collision->x50) ||
                    (double) collision->x50 < INT32_MIN ||
                    (double) collision->x50 > INT32_MAX))
                    return error(diagnostic, "Item contact amount exceeds its native callback int ABI");
            }
        }
        mpColl_80043558(collision, line);
    } else {
        result.type = NativeScriptType_Bool;
        result.as.u64 = mpGetSpeed(line, &source.data->pos, &source.data->x40_vel);
    }
    if (!current(context, &source) || mpLib_8004D164() != map ||
        mpGetGroundCollLine() != lines || mpGetGroundCollVtx() != vertices ||
        mpGetGroundCollJoint() != joints || map->line_count != line_count ||
        map->vert_count != vertex_count || map->joint_count != joint_count ||
        lines[line].x0 != description || description->v0_idx != v0 || description->v1_idx != v1)
        return error(diagnostic, "Item line operation source or geometry changed before publication");
    *output = result;
    return true;
}

bool nativeScriptBindingItemCollision_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    if (semantic == NativeScriptBinding_ItemSetCollisionFacing ||
        semantic == NativeScriptBinding_ItemResetDefaultEcb) {
        bool facing = semantic == NativeScriptBinding_ItemSetCollisionFacing;
        if (!output || count != (facing ? 1u : 0u) ||
            (facing && (!arguments || arguments[0].type != NativeScriptType_I32 ||
             arguments[0].as.i64 < INT32_MIN || arguments[0].as.i64 > INT32_MAX)))
            return error(diagnostic, "Item collision operation differs from its typed signature");
        NativeItemLifetime source;
        if (!context || context->source_class != NativeCallbackSource_Item ||
            !context->scene_epoch || !nativeItemLifetime_Capture(context->source, &source) ||
            !source.source_owned || !current(context, &source))
            return error(diagnostic, "Item collision operation requires its exact live owned Article program/storage");
        CollData* collision = &source.data->x378_itemColl;
        if (collision->x0_gobj != source.object || collision->x34_flags.b1234 == 1)
            return error(diagnostic, "Item collision operation requires its actual Item collision actor/category");
        if (facing) mpCollSetFacingDir(collision, (int32_t) arguments[0].as.i64);
        else it_80275D5C(source.object, &source.data->xC0C);
        if (!current(context, &source))
            return error(diagnostic, "Item collision operation source changed before publication");
        *output = (NativeScriptValue){.type = NativeScriptType_Void};
        return true;
    }
    if (semantic == NativeScriptBinding_ItemBindStageLine ||
        semantic == NativeScriptBinding_ItemReadStageLineSpeed)
        return line_operation(context, semantic, arguments, count, output, diagnostic);
    if (semantic != NativeScriptBinding_ItemSweepSegment || !arguments ||
        !output || count != 9)
        return error(diagnostic, "Item sweep argument shape is invalid");
    for (unsigned i = 0; i < 6; ++i)
        if (arguments[i].type != NativeScriptType_F32)
            return error(diagnostic, "Item sweep position/normal values require exact F32 types");
    if (arguments[6].type != NativeScriptType_I32 ||
        arguments[7].type != NativeScriptType_I32 ||
        arguments[6].as.i64 < INT32_MIN || arguments[6].as.i64 > INT32_MAX ||
        arguments[7].as.i64 < INT32_MIN || arguments[7].as.i64 > INT32_MAX ||
        arguments[8].type != NativeScriptType_Bool)
        return error(diagnostic, "Item sweep line, flags and hit require exact Int/Bool types");
    NativeItemLifetime source;
    if (!context || context->source_class != NativeCallbackSource_Item ||
        !context->scene_epoch ||
        !nativeItemLifetime_Capture(context->source, &source) ||
        !source.source_owned || !current(context, &source))
        return error(diagnostic, "Item sweep requires its exact live owned Article program/storage");
    Vec3 previous = {arguments[0].as.f32, arguments[1].as.f32,
                     arguments[2].as.f32};
    if (!isfinite(previous.x) || !isfinite(previous.y) || !isfinite(previous.z) ||
        !isfinite(source.data->pos.x) || !isfinite(source.data->pos.y) ||
        !isfinite(source.data->pos.z))
        return error(diagnostic, "Item sweep geometry requires finite positions");
    Vec3 normal = {arguments[3].as.f32, arguments[4].as.f32,
                   arguments[5].as.f32};
    int line = (int32_t) arguments[6].as.i64;
    int32_t signed_flags = (int32_t) arguments[7].as.i64;
    u32 flags;
    memcpy(&flags, &signed_flags, sizeof(flags));
    // Preserve the actual Item kernel's in-place position snap and initialized
    // in/out values. Its map intersection path does not dispatch authored code.
    bool hit = it_8026EA20(source.object, &previous, &source.data->pos,
                          &normal, &line, &flags);
    if (!current(context, &source))
        return error(diagnostic, "Item sweep source retired/faulted or changed before publication");
    memcpy(&signed_flags, &flags, sizeof(flags));
    arguments[3] = (NativeScriptValue){.type = NativeScriptType_F32,
                                      .as.f32 = normal.x};
    arguments[4] = (NativeScriptValue){.type = NativeScriptType_F32,
                                      .as.f32 = normal.y};
    arguments[5] = (NativeScriptValue){.type = NativeScriptType_F32,
                                      .as.f32 = normal.z};
    arguments[6] = (NativeScriptValue){.type = NativeScriptType_I32,
                                      .as.i64 = line};
    arguments[7] = (NativeScriptValue){.type = NativeScriptType_I32,
                                      .as.i64 = signed_flags};
    arguments[8] = (NativeScriptValue){.type = NativeScriptType_Bool,
                                      .as.u64 = hit};
    *output = (NativeScriptValue){.type = NativeScriptType_Void};
    return true;
}
