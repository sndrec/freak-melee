#include "native_feedback_context.h"
#include "native_home.h"
#include "native_match_launch.h"
#include "native_replay.h"
#include "gmvs.h"
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/mn/types.h>
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/random.h>
#include <yyjson.h>
#include <SDL3/SDL_timer.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum { HISTORY_COUNT = 256, LOG_COUNT = 128, MAX_CONTEXT_BYTES = 8 * 1024 * 1024 };
static struct Boundary { int scene; int64_t scene_tick; uint64_t rollback_tick; bool resimulating, online; } boundary;
static struct Input { uint64_t tick, sample; int scene; PADStatus pads[4]; } inputs[HISTORY_COUNT];
static unsigned input_count;
static struct Log { uint64_t at_ns; unsigned level; char module[48], message[512]; } logs[LOG_COUNT];
static unsigned log_count;
static SRWLOCK log_lock = SRWLOCK_INIT;

void nativeFeedbackContext_Boundary(int scene, int64_t tick, uint64_t rollback, bool resimulating, bool online)
{
    boundary = (struct Boundary){ scene, tick, rollback, resimulating, online };
}
void nativeFeedbackContext_RecordPads(const PADStatus pads[4])
{
    if (boundary.resimulating) return;
    struct Input* sample = &inputs[input_count++ % HISTORY_COUNT];
    sample->tick = boundary.rollback_tick;
    sample->sample = nativeReplay_SampleCount();
    sample->scene = boundary.scene;
    memcpy(sample->pads, pads, sizeof(sample->pads));
}
void nativeFeedbackContext_Log(AuroraLogLevel level, const char* module, const char* message, unsigned length)
{
    static const char* names[] = { "debug", "info", "warning", "error", "fatal" };
    fprintf(stderr, "[%s] [%s] %.*s\n", names[level <= LOG_FATAL ? level : LOG_ERROR], module ? module : "",
        (int)length, message ? message : "");
    if (level < LOG_WARNING || !module || !message) return;
    // The diagnostic ring contains graphics/runtime warnings. Account and
    // authentication text, URLs and credential-shaped messages are excluded.
    if (!strstr(module, "aurora") && !strstr(module, "script") && !strstr(module, "screenshot")) return;
    char lower[512]; unsigned count = length < sizeof(lower)-1 ? length : sizeof(lower)-1;
    while (count && count < length && ((unsigned char)message[count] & 0xC0) == 0x80) --count;
    for (unsigned i=0;i<count;++i) lower[i] = message[i] >= 'A' && message[i] <= 'Z' ? message[i]+('a'-'A') : message[i];
    lower[count]=0;
    static const char* excluded[] = { "token", "secret", "password", "account", "authorization", "bearer", "recovery", "key", "http://", "https://" };
    for (unsigned i=0;i<sizeof(excluded)/sizeof(*excluded);++i) if (strstr(lower, excluded[i])) return;
    AcquireSRWLockExclusive(&log_lock);
    struct Log* entry = &logs[log_count++ % LOG_COUNT];
    entry->at_ns = SDL_GetTicksNS(); entry->level = level;
    snprintf(entry->module, sizeof(entry->module), "%s", module);
    memcpy(entry->message, message, count); entry->message[count]=0;
    ReleaseSRWLockExclusive(&log_lock);
}

static yyjson_mut_val* real(yyjson_mut_doc* doc, double value)
{
    return isfinite(value) ? yyjson_mut_real(doc,value) : yyjson_mut_str(doc,isnan(value)?"NaN":value<0?"-Infinity":"Infinity");
}
static void vector(yyjson_mut_doc* doc, yyjson_mut_val* object, const char* key, Vec3 value)
{
    yyjson_mut_val* array = yyjson_mut_arr(doc);
    yyjson_mut_arr_append(array,real(doc,value.x));yyjson_mut_arr_append(array,real(doc,value.y));yyjson_mut_arr_append(array,real(doc,value.z));
    yyjson_mut_obj_add_val(doc,object,key,array);
}
static void identity(yyjson_mut_doc* doc, yyjson_mut_val* object, const char* key, uint64_t value)
{
    char text[17];snprintf(text,sizeof(text),"%016llx",(unsigned long long)value);
    yyjson_mut_obj_add_strcpy(doc,object,key,text);
}
static yyjson_mut_val* value_json(yyjson_mut_doc* doc, NativeScriptValue value)
{
    switch (value.type) {
    case NativeScriptType_Bool:return yyjson_mut_bool(doc,value.as.u64!=0);
    case NativeScriptType_I32:case NativeScriptType_I64:return yyjson_mut_sint(doc,value.as.i64);
    case NativeScriptType_U32:case NativeScriptType_U64:return yyjson_mut_uint(doc,value.as.u64);
    case NativeScriptType_F32:return real(doc,value.as.f32);
    case NativeScriptType_F64:return real(doc,value.as.f64);
    default:return yyjson_mut_null(doc);
    }
}
static yyjson_mut_val* script(yyjson_mut_doc* doc, const NativeScriptReceiverView* view, uint64_t program_identity)
{
    yyjson_mut_val* result=yyjson_mut_obj(doc);
    identity(doc,result,"program",program_identity);
    const NativeScriptStorageInstance* storage=view->storage;
    identity(doc,result,"layout",storage->layout->identity);
    identity(doc,result,"parameters",storage->parameters?storage->parameters->identity:0);
    yyjson_mut_obj_add_uint(doc,result,"owner",storage->owner);
    yyjson_mut_obj_add_uint(doc,result,"generation",storage->generation);
    const NativeScriptStorageOwner* owner=nativeScriptStorage_Owner(storage->layout,storage->owner);
    if (owner && owner->state_bytes<=storage->capacity && owner->state_bytes<=8192) {
        char bytes[16385];static const char hex[]="0123456789abcdef";
        for (unsigned i=0;i<owner->state_bytes;++i) {bytes[i*2]=hex[storage->state[i]>>4];bytes[i*2+1]=hex[storage->state[i]&15];}
        bytes[owner->state_bytes*2]=0;
        yyjson_mut_obj_add_strcpy(doc,result,"state_hex",bytes);
    }
    yyjson_mut_val* fields=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,result,"fields",fields);
    yyjson_mut_obj_add_uint(doc,result,"named_values_limit",512);
    unsigned captured=0;
    for (unsigned i=0;i<storage->layout->field_count;++i) {
        const NativeScriptStorageField* field=&storage->layout->fields[i];
        const bool shared=field->parameter && field->owner==0 && storage->owner!=0;
        if (field->owner!=storage->owner && !shared) continue;
        if (captured>=512) { yyjson_mut_obj_add_bool(doc,result,"named_values_truncated",true);break; }
        const NativeScriptValueDeclaration* declaration=&view->program->values[i];
        const NativeScriptSourceTree* tree=view->program->tree;
        if (declaration->name.offset>tree->source_bytes || declaration->name.length>tree->source_bytes-declaration->name.offset) continue;
        yyjson_mut_val* row=yyjson_mut_obj(doc);yyjson_mut_arr_append(fields,row);
        yyjson_mut_obj_add_val(doc,row,"name",yyjson_mut_strncpy(doc,tree->source+declaration->name.offset,declaration->name.length));
        yyjson_mut_obj_add_uint(doc,row,"declaration",i);
        yyjson_mut_obj_add_uint(doc,row,"offset",field->offset);
        yyjson_mut_obj_add_uint(doc,row,"type",field->type);
        yyjson_mut_obj_add_uint(doc,row,"count",field->count);
        yyjson_mut_obj_add_bool(doc,row,"parameter",field->parameter);
        yyjson_mut_val* values=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,row,"values",values);
        for (unsigned j=0;j<field->count && captured<512;++j,++captured) {
            NativeScriptValue value, selector={.type=NativeScriptType_U32,.as.u64=j}; NativeScriptDiagnostic error;
            const uint32_t domain=shared?NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC:
                field->parameter?NATIVE_SCRIPT_PARAMETER_SEMANTIC:NATIVE_SCRIPT_STATE_SEMANTIC;
            if (nativeScriptStorage_Read(storage,domain|i,field->array?&selector:NULL,field->array?1:0,&value,&error))
                yyjson_mut_arr_append(values,value_json(doc,value));
            else {yyjson_mut_obj_add_strcpy(doc,row,"error",error.message);break;}
        }
    }
    return result;
}
static yyjson_mut_val* hitbox(yyjson_mut_doc* doc, const HitCapsule* hit)
{
    yyjson_mut_val* row=yyjson_mut_obj(doc);
    yyjson_mut_obj_add_sint(doc,row,"state",hit->state);
    yyjson_mut_obj_add_val(doc,row,"damage",real(doc,hit->damage));
    yyjson_mut_obj_add_sint(doc,row,"angle",hit->kb_angle);
    yyjson_mut_obj_add_uint(doc,row,"element",hit->element);
    yyjson_mut_obj_add_val(doc,row,"radius",real(doc,hit->scale));
    vector(doc,row,"offset",hit->b_offset);vector(doc,row,"position",hit->x4C);vector(doc,row,"previous_position",hit->x58);
    yyjson_mut_obj_add_uint(doc,row,"hit_victims",hit->x44);
    return row;
}
static void fighters(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
    yyjson_mut_val* rows=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,root,"fighters",rows);
    for (unsigned slot=0;slot<nativeFighterPool.initialized_count;++slot) {
        if (!nativeFighterPool.slots[slot].occupied) continue;
        Fighter* fighter=(Fighter*)(nativeFighterPool.payload+slot*nativeFighterPool.stride);
        yyjson_mut_val* row=yyjson_mut_obj(doc);yyjson_mut_arr_append(rows,row);
        yyjson_mut_obj_add_uint(doc,row,"slot",slot);yyjson_mut_obj_add_sint(doc,row,"kind",fighter->kind);
        yyjson_mut_obj_add_uint(doc,row,"player",fighter->player_id);yyjson_mut_obj_add_sint(doc,row,"action",fighter->motion_id);
        yyjson_mut_obj_add_sint(doc,row,"animation",fighter->anim_id);yyjson_mut_obj_add_val(doc,row,"animation_frame",real(doc,fighter->cur_anim_frame));
        const NativeContentId* package=nativeFighterRuntime_PackageId(fighter->kind);
        if (package) {char hex[65];nativeContentStore_IdHex(package,hex);yyjson_mut_obj_add_strcpy(doc,row,"package",hex);}
        yyjson_mut_obj_add_val(doc,row,"facing",real(doc,fighter->facing_dir));
        yyjson_mut_obj_add_val(doc,row,"damage",real(doc,fighter->dmg.x1830_percent));
        yyjson_mut_obj_add_val(doc,row,"shield",real(doc,fighter->shield_health));
        yyjson_mut_obj_add_val(doc,row,"hitlag",real(doc,fighter->dmg.x195c_hitlag_frames));
        yyjson_mut_obj_add_sint(doc,row,"ground_or_air",fighter->ground_or_air);
        yyjson_mut_obj_add_sint(doc,row,"collision_flags",fighter->coll_data.env_flags);
        yyjson_mut_obj_add_sint(doc,row,"floor",fighter->coll_data.floor.index);
        vector(doc,row,"position",fighter->cur_pos);vector(doc,row,"velocity",fighter->self_vel);vector(doc,row,"knockback_velocity",fighter->x8c_kb_vel);
        vector(doc,row,"collision_contact",fighter->coll_data.contact);
        yyjson_mut_val* hits=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,row,"hitboxes",hits);
        for (unsigned h=0;h<4;++h) yyjson_mut_arr_append(hits,hitbox(doc,&fighter->x914[h]));
        uint32_t generation=0;NativeScriptReceiverView view;
        if (nativeCallbackSource_Snapshot(fighter->gobj,NativeCallbackSource_Fighter,&generation) &&
            nativeScriptFighter_View(fighter->gobj,generation,&view)) {
            const NativeScriptFighterProgram* program=nativeScriptFighter_Program(fighter->kind);
            yyjson_mut_obj_add_val(doc,row,"script",script(doc,&view,program?program->identity:0));
            yyjson_mut_val* callbacks=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,row,"callbacks",callbacks);
            for (unsigned c=0;c<NativeScriptFighterCallback_Count;++c)
                yyjson_mut_arr_append(callbacks,yyjson_mut_uint(doc,nativeScriptFighter_CallbackFunction(fighter->gobj,c)));
            yyjson_mut_obj_add_uint(doc,row,"script_status",nativeScriptFighter_Status(fighter->gobj));
        }
    }
}
static void items(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
    yyjson_mut_val* rows=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,root,"items",rows);
    for (unsigned slot=0;slot<nativeItemPool.initialized_count;++slot) {
        if (!nativeItemPool.slots[slot].occupied) continue;
        Item* item=(Item*)(nativeItemPool.payload+slot*nativeItemPool.stride);
        yyjson_mut_val* row=yyjson_mut_obj(doc);yyjson_mut_arr_append(rows,row);
        yyjson_mut_obj_add_uint(doc,row,"slot",slot);yyjson_mut_obj_add_sint(doc,row,"kind",item->kind);
        yyjson_mut_obj_add_sint(doc,row,"action",item->msid);yyjson_mut_obj_add_sint(doc,row,"animation",item->anim_id);
        vector(doc,row,"position",item->pos);vector(doc,row,"velocity",item->x40_vel);
        yyjson_mut_val* hits=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,row,"hitboxes",hits);
        for (unsigned h=0;h<4;++h) yyjson_mut_arr_append(hits,hitbox(doc,&item->x5D4_hitboxes[h].hit));
        uint32_t generation;NativeScriptReceiverView view;
        if (nativeCallbackSource_Snapshot(item->entity,NativeCallbackSource_Item,&generation) && nativeScriptItem_View(item->entity,generation,&view)) {
            const NativeScriptItemProgram* program=nativeScriptItem_Spec(item->entity);
            yyjson_mut_obj_add_val(doc,row,"script",script(doc,&view,program?program->identity:0));
            yyjson_mut_obj_add_uint(doc,row,"script_status",nativeScriptItem_Status(item->entity));
        }
    }
}

bool nativeFeedbackContext_Write(const char* path, const char* id, const char* captured_at,
                                 const AuroraPresentedCapture* image, const char* workspace, const char* focused)
{
    yyjson_mut_doc* doc=yyjson_mut_doc_new(NULL);if (!doc) return false;
    yyjson_mut_val* root=yyjson_mut_obj(doc);yyjson_mut_doc_set_root(doc,root);
    yyjson_mut_obj_add_strcpy(doc,root,"id",id);yyjson_mut_obj_add_strcpy(doc,root,"captured_at",captured_at);
#ifdef NATIVE_NETPLAY_DIAGNOSTICS_BUILD
    yyjson_mut_obj_add_str(doc,root,"build",NATIVE_NETPLAY_DIAGNOSTICS_BUILD);
#endif
    yyjson_mut_obj_add_strcpy(doc,root,"workspace",workspace?workspace:"");
    yyjson_mut_obj_add_strcpy(doc,root,"focused_control",focused?focused:"");
    yyjson_mut_obj_add_uint(doc,root,"backend",aurora_get_backend());
    yyjson_mut_obj_add_str(doc,root,"platform","Windows PC");
    yyjson_mut_obj_add_uint(doc,root,"pointer_bytes",sizeof(void*));
    yyjson_mut_obj_add_uint(doc,root,"fighter_struct_bytes",sizeof(Fighter));
    yyjson_mut_obj_add_uint(doc,root,"item_struct_bytes",sizeof(Item));
    yyjson_mut_obj_add_uint(doc,root,"displayed_frame",image->displayed_frame);
    yyjson_mut_obj_add_uint(doc,root,"presented_at_ns",image->presented_at_ns);
    yyjson_mut_obj_add_uint(doc,root,"captured_at_ns",SDL_GetTicksNS());
    yyjson_mut_obj_add_uint(doc,root,"image_width",image->width);yyjson_mut_obj_add_uint(doc,root,"image_height",image->height);
    yyjson_mut_obj_add_sint(doc,root,"scene",boundary.scene);yyjson_mut_obj_add_sint(doc,root,"scene_tick",boundary.scene_tick);
    yyjson_mut_obj_add_uint(doc,root,"rollback_tick",boundary.rollback_tick);
    yyjson_mut_obj_add_uint(doc,root,"replay_sample",nativeReplay_SampleCount());
    yyjson_mut_obj_add_bool(doc,root,"resimulating",boundary.resimulating);yyjson_mut_obj_add_bool(doc,root,"online",boundary.online);
    identity(doc,root,"match_signature",nativeMatchLaunch_Signature());
    if (seed_ptr) yyjson_mut_obj_add_uint(doc,root,"rng_seed",*seed_ptr);
    if (nativeFighterPool.used) {
        yyjson_mut_obj_add_sint(doc,root,"stage",stage_info.grkind);
        const StartMeleeRules* rules=gm_GetStartMeleeRules();
        if (rules) {
            yyjson_mut_val* row=yyjson_mut_obj(doc);yyjson_mut_obj_add_val(doc,root,"rules",row);
            yyjson_mut_obj_add_bool(doc,row,"stock",rules->is_stock);yyjson_mut_obj_add_bool(doc,row,"teams",rules->is_teams!=0);
            yyjson_mut_obj_add_bool(doc,row,"friendly_fire",rules->friendly_fire);
            yyjson_mut_obj_add_uint(doc,row,"time_limit",rules->time_limit);yyjson_mut_obj_add_sint(doc,row,"item_frequency",rules->item_freq);
            identity(doc,row,"item_mask",rules->x20);
        }
    }
    const NativeNetplayLaunchConfig* network=nativeMatchLaunch_Netplay();
    if (network) {identity(doc,root,"match_id",network->session_id);yyjson_mut_obj_add_uint(doc,root,"local_player_mask",network->local_port_mask);}
    fighters(doc,root);items(doc,root);
    yyjson_mut_val* packages=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,root,"selected_packages",packages);
    for (unsigned port=0;port<4;++port) {char hex[65];const NativeContentId* package=nativeHome_FighterPackage(port);
        if (package) {nativeContentStore_IdHex(package,hex);yyjson_mut_arr_append(packages,yyjson_mut_strcpy(doc,hex));}
        else yyjson_mut_arr_append(packages,yyjson_mut_null(doc));}
    NativeScriptVM* vm=nativeScriptRuntime_VM();
    if (vm) {
        yyjson_mut_obj_add_strcpy(doc,root,"script_diagnostic",vm->diagnostic.message);
        yyjson_mut_obj_add_uint(doc,root,"script_error_function",vm->error_function);
        yyjson_mut_obj_add_uint(doc,root,"script_error_actor",vm->error_actor);
        yyjson_mut_obj_add_uint(doc,root,"script_error_frame",vm->error_frame);
    }
    yyjson_mut_val* history=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,root,"recent_inputs",history);
    const unsigned first=input_count>HISTORY_COUNT?input_count-HISTORY_COUNT:0;
    for (unsigned i=first;i<input_count;++i) {
        const struct Input* sample=&inputs[i%HISTORY_COUNT];yyjson_mut_val* row=yyjson_mut_obj(doc);yyjson_mut_arr_append(history,row);
        yyjson_mut_obj_add_uint(doc,row,"tick",sample->tick);yyjson_mut_obj_add_uint(doc,row,"sample",sample->sample);
        yyjson_mut_obj_add_sint(doc,row,"scene",sample->scene);
        yyjson_mut_val* pads=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,row,"pads",pads);
        for (unsigned p=0;p<4;++p) {const PADStatus* pad=&sample->pads[p];yyjson_mut_val* values=yyjson_mut_arr(doc);yyjson_mut_arr_append(pads,values);
            int64_t fields[]={pad->button,pad->stickX,pad->stickY,pad->substickX,pad->substickY,pad->triggerLeft,pad->triggerRight,pad->err,pad->extButton};
            for (unsigned f=0;f<sizeof(fields)/sizeof(*fields);++f) yyjson_mut_arr_append(values,yyjson_mut_sint(doc,fields[f]));}
    }
    yyjson_mut_obj_add_str(doc,root,"input_columns","buttons,stick_x,stick_y,cstick_x,cstick_y,L,R,error,extended");
    yyjson_mut_val* log=yyjson_mut_arr(doc);yyjson_mut_obj_add_val(doc,root,"recent_logs",log);
    AcquireSRWLockShared(&log_lock);
    const unsigned first_log=log_count>LOG_COUNT?log_count-LOG_COUNT:0;
    for (unsigned i=first_log;i<log_count;++i) {const struct Log* entry=&logs[i%LOG_COUNT];yyjson_mut_val* row=yyjson_mut_obj(doc);yyjson_mut_arr_append(log,row);
        yyjson_mut_obj_add_uint(doc,row,"at_ns",entry->at_ns);yyjson_mut_obj_add_uint(doc,row,"level",entry->level);
        yyjson_mut_obj_add_strcpy(doc,row,"module",entry->module);yyjson_mut_obj_add_strcpy(doc,row,"message",entry->message);}
    ReleaseSRWLockShared(&log_lock);
    size_t size=0;char* bytes=yyjson_mut_write(doc,0,&size);
    bool ok=bytes && size<=MAX_CONTEXT_BYTES;FILE* file=ok?fopen(path,"wb"):NULL;
    ok=file && fwrite(bytes,1,size,file)==size;
    if (file) ok=fclose(file)==0 && ok;
    free(bytes);yyjson_mut_doc_free(doc);return ok;
}
