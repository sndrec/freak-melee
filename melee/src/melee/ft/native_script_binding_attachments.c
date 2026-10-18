#include "native_script_binding_attachments.h"
#include "native_fighter_attachment.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include "native_costume_runtime.h"
#include "ftdata.h"
#include "ftparts.h"
#include <stdio.h>
#include <string.h>

typedef struct AttachmentSource {
    NativeFighterLifetime life;
    const NativeSourcePackage* package;
    const UnkCostumeStruct* costume;
    FtCostumeAssets* assets;
    HSD_DatImport import;
    FighterBone* parts;
    uint32_t part_count;
} AttachmentSource;

static bool error(NativeScriptDiagnostic* d, const char* message)
{ if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false; }

static const UnkCostumeStruct* selected_costume(const Fighter* fp)
{
    const UnkCostumeStruct* selected=nativeCostumeRuntime_ForFighter(fp);
    if(selected)return selected;
    const struct UnkCostumeList* list=&CostumeListsForeachCharacter[fp->kind];
    return list->costume_list && fp->x619_costume_id<list->numCostumes?
        &list->costume_list[fp->x619_costume_id]:NULL;
}

static bool current(const AttachmentSource* s, NativeScriptRuntimeContext* c)
{
    if(!c || c->source!=s->life.object || c->source_class!=NativeCallbackSource_Fighter ||
       c->generation!=s->life.generation || c->scene_epoch!=s->life.reference.lifetime ||
       !nativeFighterLifetime_Current(&s->life))return false;
    const Fighter* fp=s->life.data;
    const NativeSourcePackage* p=s->package;
    if(nativeFighterRuntime_SourcePackage(fp->kind)!=p || !p->ready || !p->assets.ready ||
       p->address!=p || p->target!=fp->kind || s->life.program->bank!=&p->compiler.bank ||
       s->life.program->identity!=p->program.identity ||
       fp->ft_data!=&p->assets.character.data || fp->parts!=s->parts ||
       !ftPartsTable || ftPartsTable[fp->kind]!=&p->assets.parts ||
       p->assets.parts.parts_num!=s->part_count ||
       nativeScriptBindings_SourceFighterLayout(c,NULL)!=&s->life.program->bank->motion_layout ||
       selected_costume(fp)!=s->costume || s->costume->native_assets!=s->assets)return false;
    const HSD_DatImport* i=&s->assets->import;
    // Lazy graph import appends to this arena during Load. Its allocation
    // identity is immutable; used bytes and record count may grow legitimately.
    return i->dat==&s->assets->dat && i->dat==s->import.dat &&
        i->arena==s->import.arena && i->arena_size==s->import.arena_size &&
        i->records==s->import.records && i->slots==s->import.slots &&
        i->slot_count==s->import.slot_count && i->record_capacity==s->import.record_capacity &&
        i->arena_used>=s->import.arena_used && i->arena_used<=i->arena_size &&
        i->record_count>=s->import.record_count && i->record_count<=i->record_capacity;
}

static bool capture(NativeScriptRuntimeContext* c, AttachmentSource* s, NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!c || c->source_class!=NativeCallbackSource_Fighter || !c->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference) ||
       reference.lifetime!=c->scene_epoch || !nativeScriptBindings_SourceAlive(c) ||
       !nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,&s->life) ||
       !s->life.source_owned || s->life.generation!=c->generation ||
       s->life.reference.lifetime!=c->scene_epoch)
        return error(d,"Attachments require their exact live owned fighter.");
    s->package=nativeFighterRuntime_SourcePackage(s->life.kind);
    if(!s->package || !s->package->assets.ready || !s->life.data->parts ||
       !s->package->assets.parts.parts_num || s->package->assets.parts.parts_num>MAX_FT_PARTS)
        return error(d,"Attachments require the fighter's loaded owned rig.");
    s->parts=s->life.data->parts;s->part_count=s->package->assets.parts.parts_num;
    s->costume=selected_costume(s->life.data);
    if(!s->costume || !s->costume->native_assets)
        return error(d,"Attachments require the selected costume's imported assets.");
    s->assets=s->costume->native_assets;s->import=s->assets->import;
    return current(s,c)?true:error(d,"Attachment costume, program or rig differs from its owned source.");
}

static bool root_name(const AttachmentSource* s, const NativeScriptValue* arg,
    char root[256], NativeScriptDiagnostic* d)
{
    const NativeScriptBytecodeProgram* code=&s->life.program->bank->code;
    const NativeScriptTextValue* text=&arg->as.text;
    if(arg->type!=NativeScriptType_Text || !code->source ||
       code->source!=s->life.program->bank->tree.source || !text->length || text->length>=256 ||
       text->offset>code->source_bytes || text->length>code->source_bytes-text->offset ||
       memchr(code->source+text->offset,0,text->length))
        return error(d,"Attachment root must be a nonempty asset symbol in the caller's source.");
    memcpy(root,code->source+text->offset,text->length);root[text->length]=0;return true;
}

static bool imported_span(const HSD_DatImport* i, const void* pointer, size_t bytes)
{
    uintptr_t start=(uintptr_t)i->arena,at=(uintptr_t)pointer;
    return pointer && i->arena && i->arena_used<=i->arena_size && at>=start &&
        at-start<=i->arena_used && bytes<=i->arena_used-(at-start);
}

static bool visibility(const AttachmentSource* s, int entry, NativeScriptDiagnostic* d)
{
    if(entry<0 || entry>=FT_CHARACTER_ITEM_CAPACITY)return true;
    const FtCharacterAssets* character=&s->package->assets.character;
    // The imported schema identifies parts descriptors, rather than guessing
    // the type of an arbitrary article/joint/visibility pointer in x48_items.
    if(s->package->assets.facts.layout.item_format!=FtCharacterItems_ModelParts || entry!=1 ||
       s->life.data->ft_data->x48_items!=character->fighter_items)
        return error(d,"Attachment visibility selector does not name an owned parts descriptor.");
    const FtPartsDesc* desc=character->fighter_items[entry];
    if(!desc)return true;
    const HSD_DatImport* i=&character->import;
    if(!imported_span(i,desc,sizeof(*desc)) || desc->model_num>11 ||
       !desc->costume_count || desc->costume_count>8 ||
       !imported_span(i,desc->vis_table,desc->costume_count*sizeof(*desc->vis_table)))
        return error(d,"Attachment visibility descriptor exceeds its owned imported arena.");
    for(unsigned costume=0;costume<desc->costume_count;++costume)
        for(unsigned group=0;group<4;++group) {
            const FtPartsVisLookup* rows=desc->vis_table[costume][group];
            if(!rows)continue;
            if(!imported_span(i,rows,desc->model_num*sizeof(*rows)))
                return error(d,"Attachment visibility groups exceed their owned imported arena.");
            for(unsigned model=0;model<desc->model_num;++model) {
                const FtPartsVisLookup* row=&rows[model];
                if(row->x0<0 || row->x0>256 || (row->x0 && !imported_span(i,row->x4,row->x0*sizeof(*row->x4))))
                    return error(d,"Attachment visibility variants exceed their owned imported arena.");
                for(int variant=0;variant<row->x0;++variant) {
                    const TempS* indices=&row->x4[variant];
                    if(indices->x0<0 || indices->x0>256 ||
                       (indices->x0 && !imported_span(i,indices->x4,indices->x0)))
                        return error(d,"Attachment visibility draw indices exceed their owned imported arena.");
                    for(int draw=0;draw<indices->x0;++draw)
                        if(indices->x4[draw]>=NATIVE_ATTACHMENT_DOBJS)
                            return error(d,"Attachment visibility draw index exceeds attachment capacity.");
                }
            }
        }
    return true;
}

bool nativeScriptBindingAttachments_Invoke(NativeScriptRuntimeContext* c, uint32_t semantic,
    const NativeScriptValue* args, uint32_t count, NativeScriptValue* out, NativeScriptDiagnostic* d)
{
    bool has=semantic==NativeScriptBinding_AttachmentsHasRoot;
    bool load=semantic==NativeScriptBinding_AttachmentsLoad;
    bool dynamics=semantic==NativeScriptBinding_AttachmentsAddDynamics;
    bool clear=semantic==NativeScriptBinding_AttachmentsClearAll;
    unsigned expected=has?1:load?5:dynamics?3:0;
    if((!has&&!load&&!dynamics&&!clear) || !out || count!=expected || (count&&!args))
        return error(d,"Attachment operation has an invalid signature.");
    for(unsigned j=0;j<count;++j) {
        if((has&&j==0)||(load&&j==1)) {
            if(args[j].type!=NativeScriptType_Text)return error(d,"Attachment asset root requires Text.");
        } else if(args[j].type!=NativeScriptType_I32 || args[j].as.i64<INT32_MIN || args[j].as.i64>INT32_MAX)
            return error(d,"Attachment selectors require native signed Int values.");
    }
    AttachmentSource s={0};if(!capture(c,&s,d))return false;
    if((load||dynamics)&&!nativeScriptFighter_IsLoading(s.life.object))
        return error(d,"Attachment creation and cloth setup belong to on load, not simulation ticks.");
    char root[256];if((has||load)&&!root_name(&s,&args[has?0:1],root,d))return false;
    if(load && !visibility(&s,(int)args[3].as.i64,d))return false;
    if(dynamics && (s.life.data->ft_data->x2C!=&s.package->assets.character.dynamics ||
       s.package->assets.character.dynamics.ftDynamicBones!=&s.package->assets.character.dynamic_bones ||
       s.package->assets.character.dynamics.descriptor_count>Ft_Dynamics_NumMax))
        return error(d,"Attachment cloth requires its owned dynamic-bone descriptors.");
    NativeScriptValue result={.type=has?NativeScriptType_Bool:NativeScriptType_Void};
    if(has)result.as.u64=nativeFighterAttachment_HasRoot(s.life.object,root);
    else if(load) {
        int part=nativeFighterRig_Resolve(&s.package->assets.rig,args[2].as.i64);
        (void)nativeFighterAttachment_Load(s.life.object,(unsigned)args[0].as.i64,root,
            (unsigned)part,(int)args[3].as.i64,(unsigned)args[4].as.i64);
    } else if(dynamics) {
        (void)nativeFighterAttachment_AddDynamics(s.life.object,(unsigned)args[0].as.i64,
            (unsigned)args[1].as.i64,(unsigned)args[2].as.i64);
    } else nativeFighterAttachment_Clear(s.life.object);
    if(!current(&s,c))return error(d,"Attachment source or costume retired before publication.");
    *out=result;return true;
}
