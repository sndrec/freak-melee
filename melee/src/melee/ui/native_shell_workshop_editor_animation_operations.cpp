#include <memory>

#include "native_shell_workshop_editor_internal.h"
#include <melee/ft/native_animation_authoring.h>
#include <melee/ft/native_fighter_model_editor.h>

void nativeShellWorkshopEditor_Animations(const char* method, yyjson_val* args,
                                          NativeAuthoring::Reply& reply)
{
    using namespace WorkshopEditor;
    if (!session->project) {
        reply.fail("no_draft", "Open a fighter draft first.");
        return;
    }
    if (!std::strcmp(method, "read")) {
        NativeAnimationAuthoring::read(*session->project, args, reply);
        return;
    }
    if (!std::strcmp(method, "preview")) {
        NativeFighterAnimationClip clip;
        float frame;
        if (!nativeHome_WorkshopPoseStatus(session->project, &clip, &frame)) {
            reply.fail("preview_pending", "The frozen draft pose has not "
                                          "reached the preview actor yet.");
            return;
        }
        auto* reference = yyjson_mut_obj(reply.document);
        yyjson_mut_obj_add_val(reply.document, reply.result, "clip",
                               reference);
        NativeAuthoring::add_string(
            reply, reference, "bank",
            clip.bank == NativeFighterAnimation_Demo ? "demo" : "action");
        yyjson_mut_obj_add_uint(reply.document, reference, "index",
                                clip.index);
        yyjson_mut_obj_add_real(reply.document, reply.result, "frame", frame);
        auto model = std::make_unique<NativeFighterModelEditor>();
        if (!nativeFighterModel_Open(model.get(), session->project)) {
            reply.fail("rig_unavailable",
                       "Could not read the draft's skeleton.");
            return;
        }
        const unsigned count = model->joint_count;
        std::vector<unsigned> physical(count);
        for (unsigned bone = 0, part = 0; bone < count; ++bone, ++part) {
            bool skipped;
            do {
                skipped = false;
                for (unsigned o = 0;
                     o < session->project->facts.omission_count; ++o)
                {
                    if (session->project->facts.omissions[o].joint == part) {
                        ++part;
                        skipped = true;
                        break;
                    }
                }
            } while (skipped);
            physical[bone] = part;
        }
        auto* selected = yyjson_obj_get(args, "bones");
        for (size_t i = 0; i < yyjson_arr_size(selected); ++i) {
            if (yyjson_get_uint(yyjson_arr_get(selected, i)) >= count) {
                reply.fail(
                    "unknown_bone",
                    "Choose a bone present in the frozen preview actor.");
                return;
            }
        }
        auto* rows = yyjson_mut_arr(reply.document);
        yyjson_mut_obj_add_val(reply.document, reply.result, "bones", rows);
        for (unsigned i = 0; i < count; ++i) {
            if (selected) {
                bool found = false;
                for (size_t n = 0; n < yyjson_arr_size(selected); ++n) {
                    if (yyjson_get_uint(yyjson_arr_get(selected, n)) == i) {
                        found = true;
                    }
                }
                if (!found) {
                    continue;
                }
            }
            NativeHomeWorkshopBoneLocal local;
            float matrix[3][4];
            if (!nativeHome_WorkshopBoneLocal(physical[i], &local) ||
                !nativeHome_WorkshopBoneMatrix(physical[i], matrix))
            {
                reply.fail(
                    "preview_pending",
                    "A preview bone is unavailable at this editor boundary.");
                return;
            }
            auto* row = yyjson_mut_obj(reply.document);
            yyjson_mut_arr_append(rows, row);
            yyjson_mut_obj_add_uint(reply.document, row, "id", i);
            const char* names[] = { "position", "rotation", "scale" };
            const float* vectors[] = { local.position, local.rotation,
                                       local.scale };
            for (unsigned v = 0; v < 3; ++v) {
                auto* values = yyjson_mut_arr(reply.document);
                yyjson_mut_obj_add_val(reply.document, row, names[v], values);
                for (unsigned axis = 0; axis < 3; ++axis) {
                    yyjson_mut_arr_add_real(reply.document, values,
                                            vectors[v][axis]);
                }
            }
            auto* world = yyjson_mut_arr(reply.document);
            yyjson_mut_obj_add_val(reply.document, row, "world_matrix", world);
            for (unsigned axis = 0; axis < 3; ++axis) {
                for (unsigned c = 0; c < 4; ++c) {
                    yyjson_mut_arr_add_real(reply.document, world,
                                            matrix[axis][c]);
                }
            }
        }
        return;
    }
    NativeFighterAnimationClip clip{ NativeFighterAnimation_Action, 0 };
    if (!std::strcmp(method, "edit")) {
        const size_t animation_size = session->project->animation_size,
                     fighter_size = session->project->fighter_size;
        std::vector<uint8_t> animation(session->project->animation_dat,
                                       session->project->animation_dat +
                                           animation_size);
        std::vector<uint8_t> fighter(session->project->fighter_dat,
                                     session->project->fighter_dat +
                                         fighter_size);
        if (!NativeAnimationAuthoring::edit(*session->project, args, reply)) {
            return;
        }
        auto* value = yyjson_mut_obj_get(reply.result, "clip");
        clip.bank =
            !std::strcmp(yyjson_mut_get_str(yyjson_mut_obj_get(value, "bank")),
                         "demo")
                ? NativeFighterAnimation_Demo
                : NativeFighterAnimation_Action;
        clip.index =
            (unsigned) yyjson_mut_get_uint(yyjson_mut_obj_get(value, "index"));
        if (yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "changed"))) {
            record_structural_edit(true, animation.data(), animation_size,
                                   fighter.data(), fighter_size);
            session->animation_dirty = session->action_dirty = true;
            const char* summary =
                yyjson_get_str(yyjson_obj_get(args, "summary"));
            if (summary && !session->edit_history.empty()) {
                session->edit_history.back().summary = summary;
            }
            update_history_buttons();
        }
    } else {
        auto* value = yyjson_obj_get(args, "clip");
        if (!value) {
            reply.fail("missing_clip",
                       "Choose a clip returned by the clips view.");
            return;
        }
        const char* bank = yyjson_get_str(yyjson_obj_get(value, "bank"));
        clip.bank = bank && !std::strcmp(bank, "demo")
                        ? NativeFighterAnimation_Demo
                        : NativeFighterAnimation_Action;
        clip.index =
            (unsigned) yyjson_get_uint(yyjson_obj_get(value, "index"));
    }
    NativeFighterAnimationInfo info;
    if (!nativeFighterAnimation_Info(session->project, clip, &info)) {
        reply.fail("clip_unavailable",
                   "This clip is unavailable for preview.");
        return;
    }
    const int previous_frame =
        session->animation_clip.bank == clip.bank &&
                session->animation_clip.index == clip.index
            ? session->animation_frame
            : 0;
    nativeFighterAction_List(session->project, session->actions, 1024,
                             &session->action_count);
    for (unsigned i = 0; i < session->action_count; ++i) {
        NativeFighterAnimationInfo action{};
        session->action_has_clip[i] = nativeFighterAnimation_Info(
            session->project, { NativeFighterAnimation_Action, i }, &action);
        session->action_clip_frames[i] = action.frames;
    }
    select_mode(WorkshopMode::Animation);
    if (clip.bank == NativeFighterAnimation_Action) {
        select_action(clip.index);
    } else {
        read_demo_clips();
        select_demo_clip(clip.index);
    }
    session->animation_frame =
        std::clamp(yyjson_obj_get(args, "frame")
                       ? (int) yyjson_get_uint(yyjson_obj_get(args, "frame"))
                       : previous_frame,
                   0, (int) std::ceil(info.frames));
    session->displayed_animation_clip.index = UINT_MAX;
    session->graph_selection.clear();
    show_animation();
    show_animation_graph();
    show_animation_dopesheet();
    nativeShellWorkshopEditor_FilterClips();
    nativeHome_WorkshopPoseInvalidate();
    set_text("workshop-editor-status", "Animation selected. Unsaved changes "
                                       "are visible in the frozen preview.");
}
