#include "native_model_authoring_internal.h"

namespace NativeModelAuthoring {
namespace {
std::string schema(bool edit, bool live)
{
    // Keep initialization inside the builder: the tool catalogs call these
    // accessors while their static command tables are constructed.
    const std::string vec = R"({"type":"array","minItems":3,"maxItems":3,"items":{"type":"number","minimum":-10000,"maximum":10000}})";
    const std::string bone = R"({"type":"integer","minimum":0,"maximum":511})";
    const std::string mesh = R"({"type":"integer","minimum":0,"maximum":1023})";
    const std::string material = R"({"type":"integer","minimum":0,"maximum":511})";
    const std::string path = R"({"type":"string","minLength":1,"maxLength":500})";
    const std::string binding = R"({"type":"string","enum":["rigid","automatic","transfer","source","keep"]})";
    std::string fields = live ? R"("instance":{"type":"string","minLength":34,"maxLength":43})"
        : R"("project":{"type":"string","minLength":1,"maxLength":500},"member":{"type":"string","maxLength":31})";
    std::string required = live ? "\"instance\"" : "\"project\"";
    fields += ",\"mesh\":" + mesh + ",\"material\":" + material + ",\"path\":" + path + R"(,"whole_source":{"type":"boolean"})";
    if (!edit) {
        fields += R"(,"view":{"type":"string","enum":["info","bones","meshes","materials","textures","vertices","sources","texture_sequences","reimport_plan"]},"query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200})";
        required += ",\"view\"";
    } else {
        fields += R"(,"revision":{"type":"string","minLength":64,"maxLength":64},"summary":{"type":"string","minLength":1,"maxLength":300},"operation":{"type":"string","enum":["import","reimport","rest","paint","material","texture","mesh_material","label","texture_image","rig"]})";
        fields += ",\"binding\":" + binding + R"(,"replace_material":{"type":"boolean"})";
        fields += R"(,"imports":{"type":"array","minItems":1,"maxItems":128,"items":{"type":"object","properties":{"primitive":{"type":"integer","minimum":0,"maximum":127},"bone":)" + bone + ",\"replace_mesh\":" + mesh + ",\"binding\":" + binding + R"(,"scale":{"type":"number","minimum":0.000001,"maximum":10000},"replace_material":{"type":"boolean"}},"required":["primitive","bone"],"additionalProperties":false}})";
        fields += R"(,"changes":{"type":"array","minItems":1,"maxItems":128,"items":{"type":"object","properties":{"bone":)" + bone + ",\"position\":" + vec + ",\"rotation\":" + vec + ",\"scale\":" + vec + R"(},"required":["bone"],"additionalProperties":false}})";
        fields += ",\"properties\":" + std::string(material_schema());
        fields += ",\"texture\":" + std::string(texture_schema());
        fields += ",\"bone\":" + bone + R"(,"centers":{"type":"array","minItems":1,"maxItems":4096,"items":)" + vec + R"(},"radius":{"type":"number","minimum":0.000001,"maximum":1000},"strength":{"type":"number","minimum":0.000001,"maximum":1},"subtract":{"type":"boolean"})";
        fields += R"(,"category":{"type":"string","enum":["bone","mesh","material"]},"id":{"type":"integer","minimum":0,"maximum":1023},"name":{"type":"string","maxLength":63},"layer":{"type":"integer","minimum":0,"maximum":7},"image":{"type":"integer","minimum":0,"maximum":65535},"sequence":{"type":"integer","minimum":0,"maximum":32767},"rig_operation":{"type":"string","enum":["add","reparent","remove"]},"bone_id":{"type":"integer","minimum":0,"maximum":2147483647},"parent_id":{"type":"integer","minimum":0,"maximum":2147483647})";
        required += ",\"revision\",\"operation\""; if (live) required += ",\"summary\"";
    }
    return "{\"type\":\"object\",\"properties\":{" + fields + "},\"required\":[" + required + "],\"additionalProperties\":false}";
}
}
const char* read_schema(bool live) {static const auto saved = schema(false, false), running = schema(false, true); return (live ? running : saved).c_str();}
const char* edit_schema(bool live) {static const auto saved = schema(true, false), running = schema(true, true); return (live ? running : saved).c_str();}
const char* scan_schema() {return R"({"type":"object","properties":{"path":{"type":"string","minLength":1,"maxLength":500}},"required":["path"],"additionalProperties":false})";}
}
