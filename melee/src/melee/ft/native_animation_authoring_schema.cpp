#include <string>

#include "native_animation_authoring.h"

namespace NativeAnimationAuthoring {
    namespace {
        std::string schema(bool edit, bool live)
        {
            const std::string clip =
                R"({"type":"object","properties":{"bank":{"type":"string","enum":["action","demo"]},"index":{"type":"integer","minimum":0,"maximum":1023}},"required":["bank","index"],"additionalProperties":false})";
            const std::string channels =
                R"({"type":"string","enum":["rotation_x","rotation_y","rotation_z","position_x","position_y","position_z","scale_x","scale_y","scale_z"]})";
            const std::string scalar =
                R"({"type":"number","minimum":-1000000,"maximum":1000000})";
            const std::string frame =
                R"({"type":"integer","minimum":0,"maximum":65535})";
            const std::string real_frame =
                R"({"type":"number","minimum":0,"maximum":65535})";
            const std::string bones =
                R"({"type":"array","maxItems":512,"items":{"type":"integer","minimum":0,"maximum":511}})";
            std::string fields =
                live
                    ? R"("instance":{"type":"string","minLength":34,"maxLength":43})"
                    : R"("project":{"type":"string","minLength":1,"maxLength":500},"member":{"type":"string","maxLength":31})";
            std::string required = live ? "\"instance\"" : "\"project\"";
            fields += ",\"clip\":" + clip;
            if (!edit) {
                fields +=
                    R"(,"view":{"type":"string","enum":["clips","bones","info","tracks","keys","pose","samples"]},"query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200},"track":{"type":"integer","minimum":0,"maximum":4095},"frame":)" +
                    real_frame +
                    ",\"frames\":{\"type\":\"array\",\"minItems\":1,"
                    "\"maxItems\":2048,\"items\":" +
                    real_frame + "}";
                required += ",\"view\"";
            } else {
                fields +=
                    R"(,"revision":{"type":"string","minLength":64,"maxLength":64},"summary":{"type":"string","minLength":1,"maxLength":300},"operation":{"type":"string","enum":["create","duplicate","duration","insert","change","delete","transform","copy_pose"]},"target_action":{"type":"integer","minimum":0,"maximum":1023},"name":{"type":"string","minLength":1,"maxLength":63},"frames":)" +
                    real_frame + ",\"frame\":" + frame +
                    ",\"source_clip\":" + clip +
                    ",\"source_frame\":" + real_frame + ",\"bones\":" + bones +
                    R"(,"include_root_translation":{"type":"boolean"})";
                fields +=
                    R"(,"keys":{"type":"array","maxItems":32768,"items":{"type":"object","properties":{"track":{"type":"integer","minimum":0,"maximum":4095},"key":{"type":"integer","minimum":0,"maximum":65535},"frame":)" +
                    frame + ",\"value\":" + scalar + ",\"tangent\":" + scalar +
                    R"(},"required":["track","key"],"additionalProperties":false}})";
                fields +=
                    R"(,"insertions":{"type":"array","minItems":1,"maxItems":4096,"items":{"type":"object","properties":{"bone":{"type":"integer","minimum":0,"maximum":511},"channel":)" +
                    channels + ",\"frame\":" + frame + ",\"value\":" + scalar +
                    R"(},"required":["bone","channel","frame","value"],"additionalProperties":false}})";
                fields +=
                    R"(,"selection":{"type":"object","properties":{"bones":)" +
                    bones +
                    ",\"channels\":{\"type\":\"array\",\"maxItems\":9,"
                    "\"items\":" +
                    channels + "},\"start_frame\":" + real_frame +
                    ",\"end_frame\":" + real_frame +
                    R"(},"additionalProperties":false})";
                fields +=
                    R"(,"transform":{"type":"object","properties":{"pivot_frame":)" +
                    scalar + ",\"pivot_value\":" + scalar +
                    ",\"time_scale\":" + scalar +
                    ",\"value_scale\":" + scalar + ",\"rotation\":" + scalar +
                    ",\"frame_offset\":" + scalar +
                    ",\"value_offset\":" + scalar +
                    R"(},"additionalProperties":false})";
                required += ",\"revision\",\"operation\"";
                if (live) {
                    required += ",\"summary\"";
                }
            }
            return "{\"type\":\"object\",\"properties\":{" + fields +
                   "},\"required\":[" + required +
                   "],\"additionalProperties\":false}";
        }
    }
    const char* read_schema(bool live)
    {
        static const auto saved = schema(false, false),
                          running = schema(false, true);
        return (live ? running : saved).c_str();
    }
    const char* edit_schema(bool live)
    {
        static const auto saved = schema(true, false),
                          running = schema(true, true);
        return (live ? running : saved).c_str();
    }
}
