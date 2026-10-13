#include "native_script_binding_model_graph.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "native_fighter_model_graph.h"
#include "native_source_package.h"
#include "ftparts.h"
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <string.h>

typedef struct ModelGraphSource {
    NativeScriptRuntimeContext context;
    NativeFighterLifetime life;
    const NativeSourcePackage* package;
    FighterBone* parts;
    HSD_JObj* root;
    HSD_JObj* alternate;
    HSD_DObj** draws;
    void** graphs;
    const void* arena;
    size_t arena_used,arena_size;
    uint32_t part_count,omission_count,graph_mask;
    int32_t slot,omission;
    HSD_Joint* descriptor;
    struct Fighter_804D6540_x0_t placement;
    HSD_JObj* parent;
    HSD_JObj* alternate_parent;
    bool selected;
} ModelGraphSource;

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool current(const ModelGraphSource* s,NativeScriptRuntimeContext* c)
{
    if(c->source!=s->context.source || c->source_class!=s->context.source_class ||
       c->generation!=s->context.generation || c->scene_epoch!=s->context.scene_epoch ||
       !nativeFighterLifetime_Current(&s->life))return false;
    const NativeSourcePackage* p=s->package;
    const Fighter* fp=s->life.data;
    if(nativeFighterRuntime_SourcePackage(fp->kind)!=p || !p->ready || !p->assets.ready ||
       p->address!=p || p->target!=fp->kind || s->life.program->identity!=p->program.identity ||
       s->life.program->bank!=p->program.bank || s->life.program->bank!=&p->compiler.bank ||
       fp->ft_data!=&p->assets.character.data ||
       nativeScriptBindings_SourceFighterLayout(c,NULL)!=&s->life.program->bank->motion_layout ||
       !ftPartsTable || ftPartsTable[fp->kind]!=&p->assets.parts ||
       !Fighter_804D6540 || Fighter_804D6540[fp->kind]!=&p->assets.omissions ||
       p->assets.parts.parts_num!=s->part_count ||
       s->part_count!=(uint32_t)p->assets.facts.joints+p->assets.facts.omission_count ||
       p->assets.parts.joint_to_part!=p->assets.joint_to_part ||
       p->assets.parts.part_to_joint!=p->assets.part_to_joint ||
       p->assets.omissions.x0!=p->assets.omission_rows ||
       p->assets.omissions.x4!=(int)s->omission_count ||
       p->assets.facts.omission_count!=s->omission_count ||
       p->assets.facts.layout.joint_graph_mask!=s->graph_mask ||
       fp->parts!=s->parts || s->life.object->hsd_obj!=s->root ||
       fp->x8AC_animSkeleton!=s->alternate || fp->parts[0].joint!=s->root ||
       fp->parts[0].x4_jobj2!=s->alternate || fp->dobj_list.data!=s->draws ||
       fp->ft_data->x48_items!=s->graphs || s->graphs!=p->assets.character.fighter_items ||
       p->assets.character.import.arena!=s->arena ||
       p->assets.character.import.arena_used!=s->arena_used ||
       p->assets.character.import.arena_size!=s->arena_size)return false;
    if(s->slot>=0 && (unsigned)s->slot<FT_CHARACTER_ITEM_CAPACITY &&
       (s->graph_mask&(1u<<s->slot)) &&
       (s->graphs[s->slot]!=s->descriptor || p->assets.character.joint_graphs[s->slot]!=s->descriptor))return false;
    if(s->selected) {
        const struct Fighter_804D6540_x0_t* row=&p->assets.omission_rows[s->omission];
        if(memcmp(row,&s->placement,sizeof(*row)))return false;
        if(row->x1<s->part_count &&
           (s->parts[row->x1].joint!=s->parent || s->parts[row->x1].x4_jobj2!=s->alternate_parent))return false;
    }
    return true;
}

static bool capture(NativeScriptRuntimeContext* c,int32_t slot,int32_t omission,
    ModelGraphSource* s,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!c || c->source_class!=NativeCallbackSource_Fighter || !c->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference) || reference.lifetime!=c->scene_epoch ||
       !nativeScriptBindings_SourceFighterLayout(c,d) ||
       !nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,&s->life) ||
       !s->life.source_owned || s->life.generation!=c->generation ||
       s->life.reference.lifetime!=c->scene_epoch)
        return error(d,"Model graph installation requires its exact live source Fighter");
    const NativeSourcePackage* p=nativeFighterRuntime_SourcePackage(s->life.kind);
    Fighter* fp=s->life.data;
    if(!p || !fp->parts || !s->life.object->hsd_obj || !fp->x8AC_animSkeleton ||
       !fp->ft_data || !fp->ft_data->x48_items || !p->assets.parts.parts_num ||
       p->assets.parts.parts_num>MAX_FT_PARTS || p->assets.facts.omission_count>NATIVE_SOURCE_OMISSIONS)
        return error(d,"Model graph installation requires its owned model poses and tables");
    s->context=*c;s->package=p;s->parts=fp->parts;s->root=s->life.object->hsd_obj;
    s->alternate=fp->x8AC_animSkeleton;s->draws=fp->dobj_list.data;s->graphs=fp->ft_data->x48_items;
    s->arena=p->assets.character.import.arena;s->arena_used=p->assets.character.import.arena_used;
    s->arena_size=p->assets.character.import.arena_size;s->part_count=p->assets.parts.parts_num;
    s->omission_count=p->assets.facts.omission_count;s->graph_mask=p->assets.facts.layout.joint_graph_mask;
    s->slot=slot;s->omission=omission;
    if(slot>=0 && (unsigned)slot<FT_CHARACTER_ITEM_CAPACITY && (s->graph_mask&(1u<<slot)))
        s->descriptor=s->graphs[slot];
    if(omission>=0 && (unsigned)omission<s->omission_count) {
        s->selected=true;s->placement=p->assets.omission_rows[omission];
        if(s->placement.x1<s->part_count) {
            s->parent=s->parts[s->placement.x1].joint;
            s->alternate_parent=s->parts[s->placement.x1].x4_jobj2;
        }
    }
    return current(s,c)?true:error(d,"Model graph installation assets differ from the retained owned rig");
}

bool nativeScriptBindingModelGraph_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic!=NativeScriptBinding_ModelGraphsInstall || !out || !args || count!=2 ||
       args[0].type!=NativeScriptType_I32 || args[1].type!=NativeScriptType_I32 ||
       args[0].as.i64<INT32_MIN || args[0].as.i64>INT32_MAX ||
       args[1].as.i64<INT32_MIN || args[1].as.i64>INT32_MAX)
        return error(d,"Model graph installation requires Int slot and omission selectors");
    ModelGraphSource source={0};
    if(!capture(c,(int32_t)args[0].as.i64,(int32_t)args[1].as.i64,&source,d))return false;
    bool installed=nativeFighterModelGraph_Install(c->source,source.slot,source.omission);
    if(!current(&source,c))return error(d,"Model graph source, program, storage or owned rig changed before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=installed};return true;
}
