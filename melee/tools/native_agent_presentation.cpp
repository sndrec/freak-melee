#include "native_agent_internal.h"
#include <melee/ft/native_fighter_presentation.h>
#include <melee/ft/native_source_resources.h>
#include <cstring>

namespace NativeAgent {
namespace Presentation = NativeFighterPresentation;

void read_presentation(const Context&, yyjson_val* args, Reply& reply)
{
    const char* path = string(args, "project");
    Presentation::Snapshot data;
    std::string error;
    if (!Presentation::inspect(path, data, error)) {
        reply.fail("presentation_unavailable", error);
        return;
    }
    add_string(reply, reply.result, "project", path);
    add_string(reply, reply.result, "revision", data.revision);
    add_string(reply, reply.result, "title", data.title);
    add_string(reply, reply.result, "tile_name", data.tile_name);
    add_string(reply, reply.result, "displayed_tile_name", data.tile_name.empty() ? data.title : data.tile_name);
    for (unsigned i = 0; i < 2; ++i) {
        const auto& image = data.images[i];
        const auto& framing = data.framing[i];
        auto* target = yyjson_mut_obj(reply.document);
        yyjson_mut_obj_add_val(reply.document, reply.result, i ? "card" : "tile", target);
        yyjson_mut_obj_add_bool(reply.document, target, "has_image", image.bytes != 0);
        yyjson_mut_obj_add_uint(reply.document, target, "max_png_bytes", i ? NATIVE_SOURCE_RESOURCE_PNG_BYTES : 1048576);
        yyjson_mut_obj_add_uint(reply.document, target, "max_image_dimension", i ? NATIVE_SOURCE_RESOURCE_PNG_DIMENSION : 512);
        if (image.bytes) {
            yyjson_mut_obj_add_uint(reply.document, target, "bytes", image.bytes);
            yyjson_mut_obj_add_uint(reply.document, target, "width", image.width);
            yyjson_mut_obj_add_uint(reply.document, target, "height", image.height);
            add_string(reply,target,"sha256",image.sha256);
            add_string(reply,target,i ? "resource" : "file",i ? "ui/player-card.png" : "portrait.png");
        }
        if (framing.animation[0]) {
            auto* view = yyjson_mut_obj(reply.document);
            const auto& c = framing.camera;
            add_string(reply, view, "animation", framing.animation);
            yyjson_mut_obj_add_real(reply.document, view, "frame", c.frame);
            yyjson_mut_obj_add_real(reply.document, view, "yaw", c.yaw);
            yyjson_mut_obj_add_real(reply.document, view, "pitch", c.pitch);
            yyjson_mut_obj_add_real(reply.document, view, "zoom", c.zoom);
            yyjson_mut_obj_add_real(reply.document, view, "pan_x", c.pan_x);
            yyjson_mut_obj_add_real(reply.document, view, "pan_y", c.pan_y);
            yyjson_mut_obj_add_val(reply.document, target, "framing", view);
        }
    }
}

void edit_presentation(const Context&, yyjson_val* args, Reply& reply)
{
    const char* name = yyjson_get_str(yyjson_obj_get(args, "tile_name"));
    const char* image = yyjson_get_str(yyjson_obj_get(args, "image"));
    if ((!name && !image) || (image && !yyjson_obj_get(args, "target"))) {
        reply.fail("invalid_arguments", "Provide a tile name or an image with its target.");
        return;
    }
    const auto target = std::strcmp(string(args,"target"),"card") == 0 ?
        Presentation::Target::Card : Presentation::Target::Tile;
    std::string revision, error;
    switch (Presentation::save(string(args,"project"), string(args,"revision"), target, name, image,
                               nullptr, revision, error)) {
    case Presentation::Result::Saved:
        add_string(reply, reply.result, "revision", revision);
        yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", true);
        break;
    case Presentation::Result::Busy: reply.fail("draft_busy", error); break;
    case Presentation::Result::Conflict: reply.fail("revision_conflict", error); break;
    case Presentation::Result::Invalid: reply.fail("invalid_presentation", error); break;
    case Presentation::Result::Failed: reply.fail("save_failed", error); break;
    }
}
}
