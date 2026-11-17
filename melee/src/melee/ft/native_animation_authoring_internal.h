#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

#include "native_animation_authoring.h"
#include "native_fighter_action_editor.h"
#include "native_fighter_authoring.h"
#include "native_fighter_model_editor.h"
#include "native_fighter_pose_editor.h"

namespace NativeAnimationAuthoring {
    using NativeAuthoring::add_string;
    using NativeAuthoring::Reply;
    inline const char* text(yyjson_val* v, const char* name)
    {
        const char* s = yyjson_get_str(yyjson_obj_get(v, name));
        return s ? s : "";
    }
    inline unsigned number(yyjson_val* v, const char* name)
    {
        return (unsigned) yyjson_get_uint(yyjson_obj_get(v, name));
    }
    inline bool has(yyjson_val* v, const char* name)
    {
        return yyjson_obj_get(v, name) != nullptr;
    }
    inline double numeric(yyjson_val* v, const char* name, double fallback = 0)
    {
        return has(v, name) ? yyjson_get_num(yyjson_obj_get(v, name))
                            : fallback;
    }
    inline NativeFighterAnimationClip reference(yyjson_val* v)
    {
        return { !std::strcmp(text(v, "bank"), "demo")
                     ? NativeFighterAnimation_Demo
                     : NativeFighterAnimation_Action,
                 number(v, "index") };
    }
    inline void reference(Reply& r, yyjson_mut_val* out, const char* name,
                          NativeFighterAnimationClip clip)
    {
        auto* value = yyjson_mut_obj(r.document);
        yyjson_mut_obj_add_val(r.document, out, name, value);
        add_string(r, value, "bank",
                   clip.bank == NativeFighterAnimation_Demo ? "demo"
                                                            : "action");
        yyjson_mut_obj_add_uint(r.document, value, "index", clip.index);
    }
    const char* channel(unsigned id);
    unsigned channel(const char* name);
    inline const char* unit(unsigned c)
    {
        return c <= 3 ? "radians" : c <= 7 ? "game_units" : "scale";
    }
    inline void page(Reply& r, size_t total, unsigned offset, unsigned limit)
    {
        yyjson_mut_obj_add_uint(r.document, r.result, "total", total);
        if (offset + limit < total) {
            yyjson_mut_obj_add_uint(r.document, r.result, "next_offset",
                                    offset + limit);
        } else {
            yyjson_mut_obj_add_null(r.document, r.result, "next_offset");
        }
    }
    struct Clip {
        NativeFighterAnimationClip ref{};
        NativeFighterAnimationInfo info{};
        std::vector<NativeFighterAnimationTrack> tracks;
        bool open(const NativeFighterProject&, yyjson_val*, Reply&);
    };
    struct Rig {
        std::unique_ptr<NativeFighterModelEditor> model;
        std::vector<int> node_to_bone, bone_to_node;
        bool bound = false;
        bool open(const NativeFighterProject&,
                  const NativeFighterAnimationInfo* info, Reply&);
    };
    void describe(Reply&, const Clip&);
    bool keys(const NativeFighterProject&, NativeFighterAnimationClip,
              unsigned, std::vector<NativeFighterAnimationKey>&, Reply&);
}
