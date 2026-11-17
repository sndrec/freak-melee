#include "native_resource_authoring.h"
namespace NativeResourceAuthoring {
namespace {
std::string schema(unsigned operation,bool live)
{
    const std::string path=R"({"type":"string","minLength":1,"maxLength":500})";
    const std::string name=R"({"type":"string","minLength":1,"maxLength":127})";
    std::string fields=live?R"("instance":{"type":"string","minLength":34,"maxLength":43})":
        R"("project":{"type":"string","minLength":1,"maxLength":500},"member":{"type":"string","maxLength":31})";
    std::string required=live?"\"instance\"":"\"project\"";
    if(operation==0)fields+=R"(,"query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200})";
    if(operation==1) {
        fields+=R"(,"revision":{"type":"string","minLength":64,"maxLength":64},"summary":{"type":"string","minLength":1,"maxLength":300},"changes":{"type":"array","minItems":1,"maxItems":64,"items":{"type":"object","properties":{"operation":{"type":"string","enum":["import","remove"]},"name":)"+name+
            R"(,"kind":{"type":"string","enum":["png","blob"]},"path":)"+path+R"(},"required":["operation","name"],"additionalProperties":false}})";
        required+=",\"revision\",\"changes\"";if(live)required+=",\"summary\"";
    }
    if(operation==2) {fields+=",\"name\":"+name+",\"output\":"+path;required+=",\"name\",\"output\"";}
    return "{\"type\":\"object\",\"properties\":{"+fields+"},\"required\":["+required+"],\"additionalProperties\":false}";
}
}
const char* read_schema(bool live) {static const auto disk=schema(0,false),running=schema(0,true);return (live?running:disk).c_str();}
const char* edit_schema(bool live) {static const auto disk=schema(1,false),running=schema(1,true);return (live?running:disk).c_str();}
const char* export_schema(bool live) {static const auto disk=schema(2,false),running=schema(2,true);return (live?running:disk).c_str();}
const char* blueprint_schema()
{return R"({"type":"object","properties":{"iso":{"type":"string","minLength":1,"maxLength":500},"stock_kind":{"type":"integer","minimum":0,"maximum":42},"dat":{"type":"string","minLength":1,"maxLength":500},"article_offset":{"type":"integer","minimum":0,"maximum":4294967295},"output":{"type":"string","minLength":1,"maxLength":500}},"required":["output"],"additionalProperties":false})";}
}
