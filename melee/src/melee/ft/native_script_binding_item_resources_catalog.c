#include "native_script_binding_item_resources.h"
#include "native_script_program.h"
#include <melee/it/native_article_resource.h>
#define R NativeScriptFlag_Readable
#define W (NativeScriptFlag_Writable|NativeScriptFlag_Addressable)
#define P NativeScriptFlag_Pure
#define S NativeScriptFlag_SourceOnly
#define M NativeScriptFlag_MayInvalidateSource
#define A NativeScriptBindings_Article
#define IN(t) {NativeScriptType_##t,NativeScriptArgument_Input,false,false}
#define INT {NativeScriptType_Invalid,NativeScriptArgument_Input,true,false}
#define OUT {NativeScriptType_F32,NativeScriptArgument_Output,false,false}
#define NS(n,p) {.name=n,.parent=p,.kind=NativeScriptBinding_Namespace,.contexts=A,.result_type=NativeScriptType_Namespace,.flags=S}
#define PROP(n,p,t,f) {.name=n,.parent=p,.kind=NativeScriptBinding_Property,.contexts=A,.result_type=NativeScriptType_##t,.flags=R|P|S|f}
#define CALL(n,p,t,c,...) {.name=n,.parent=p,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_##t,.flags=R|NativeScriptFlag_Callable|S|M,.argument_count=c,.arguments={__VA_ARGS__}}
#define ID(n) NativeScriptItemResource_##n
static const NativeScriptBindingDescriptor descriptors[] = {
    NS("resources",NATIVE_SCRIPT_ITEM_NAMESPACE),
    CALL("begin",ID(Resources),Bool,1,INT),
    CALL("release",ID(Resources),Void,0,IN(Void)),
    PROP("count",ID(Resources),I32,0),
    {.name="resource",.parent=NATIVE_SCRIPT_ITEM_NAMESPACE,.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Namespace,.flags=R|NativeScriptFlag_Callable|S|P,.argument_count=1,.arguments={INT}},
    NS("",0), NS("position",ID(Node)), NS("velocity",ID(Node)),
    NS("collision",ID(Node)), NS("position",ID(Collision)),
    NS("previous",ID(Collision)), NS("floor_normal",ID(Collision)),
    NS("left_wall_normal",ID(Collision)), NS("right_wall_normal",ID(Collision)),
    NS("ceiling_normal",ID(Collision)),
    PROP("exists",ID(Node),Bool,0), PROP("surface_line",ID(Node),I32,W),
    PROP("environment",ID(Collision),I32,0), PROP("floor_line",ID(Collision),I32,0),
    PROP("left_wall_line",ID(Collision),I32,0), PROP("right_wall_line",ID(Collision),I32,0),
    PROP("ceiling_line",ID(Collision),I32,0),
    CALL("create",ID(Node),Bool,3,INT,IN(F32),INT),
    CALL("create",ID(Node),Bool,6,INT,IN(F32),INT,IN(F32),IN(F32),IN(F32)),
    CALL("load_model",ID(Node),Bool,1,IN(I32)),
    CALL("set_draw",ID(Node),Bool,1,IN(Bool)),
    CALL("bind_part",ID(Node),Bool,1,INT),
    CALL("bind_capture_anchor",ID(Node),Bool,0,IN(Void)),
    CALL("sweep_stage",ID(Node),Bool,0,IN(Void)),
    CALL("sweep_point",ID(Node),Bool,0,IN(Void)),
    CALL("follow_floor",ID(Node),Bool,0,IN(Void)),
    {.name="read_line_speed",.parent=ID(Node),.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.flags=R|NativeScriptFlag_Callable|S|M,.argument_count=4,.publication=NativeScriptOutputs_AlwaysAfterGuard,.arguments={INT,OUT,OUT,OUT}},
    {.name="read_attachment_point",.parent=ID(Node),.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Bool,.flags=R|NativeScriptFlag_Callable|S|M,.argument_count=6,.publication=NativeScriptOutputs_AlwaysAfterGuard,.arguments={IN(F32),IN(F32),IN(F32),OUT,OUT,OUT}},
    CALL("read_attachment_matrix",ID(Node),Bool,1,IN(I32)),
    CALL("read_world_matrix",ID(Node),Bool,1,IN(I32)),
    CALL("set_world_matrix",ID(Node),Bool,1,IN(I32)),
    CALL("spawn_effect_at_position",ID(Node),Void,4,INT,IN(F32),IN(F32),IN(F32)),
    CALL("spawn_effect_facing_at_position",ID(Node),Void,5,INT,IN(F32),IN(F32),IN(F32),IN(F32)),
    {.name="joint",.parent=ID(Node),.kind=NativeScriptBinding_Intrinsic,.contexts=A,.result_type=NativeScriptType_Namespace,.flags=R|NativeScriptFlag_Callable|S|P,.argument_count=1,.arguments={IN(Text)}},
    NS("",0), NS("translation",ID(JointNode)), NS("rotation",ID(JointNode)),
    NS("scale",ID(JointNode)), NS("quaternion",ID(JointNode)),
    CALL("add_animation",ID(JointNode),Bool,1,IN(I32)),
    CALL("request_animation",ID(JointNode),Bool,1,IN(F32)),
    CALL("advance_animation",ID(JointNode),Bool,0,IN(Void)),
    CALL("remove_animation",ID(JointNode),Bool,0,IN(Void)),
    CALL("set_animation_rate",ID(JointNode),Bool,1,IN(F32)),
};
#define XYZ(p,f) PROP("x",ID(p),F32,f),PROP("y",ID(p),F32,f),PROP("z",ID(p),F32,f)
static const NativeScriptBindingDescriptor floats[] = {
    XYZ(Position,W),XYZ(Velocity,W),XYZ(CollisionPosition,W),XYZ(Previous,W),
    XYZ(FloorNormal,0),XYZ(LeftWallNormal,0),XYZ(RightWallNormal,0),XYZ(CeilingNormal,0),
};
_Static_assert(sizeof(descriptors)/sizeof(*descriptors)==ID(StaticEnd)-ID(Resources),
    "Resource semantic/catalog order must remain exact");
_Static_assert(sizeof(floats)/sizeof(*floats)==NativeArticleResource_FloatCount,
    "Resource scalar channels must match the native collection");
const NativeScriptBindingDescriptor* nativeScriptBindingItemResources_Descriptor(uint32_t id)
{
    if(id>=ID(Resources)&&id<ID(StaticEnd))return &descriptors[id-ID(Resources)];
    if(id>=ID(FloatFirst)&&id<ID(FloatFirst)+NativeArticleResource_FloatCount)return &floats[id-ID(FloatFirst)];
    return NULL;
}
