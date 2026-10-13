#include "native_source_item_visual.h"
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_source_resources.h>
#include <melee/ft/native_gx_image.h>
#include <melee/ef/native_script_effect_actor.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* text)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",text);return false;}
static bool idle(void)
{
    return nativeScriptRuntime_Idle() && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_804D7818 &&
        !HSD_GObj_DelayedProcInfo.flags;
}
static bool unselected(const NativeScriptCompileBank* bank)
{
    for(unsigned kind=0;kind<Ft_Kind_RuntimeCapacity;++kind) {
        const NativeScriptFighterProgram* program=nativeScriptFighter_Program((FighterKind)kind);
        if(program && program->bank==bank)return false;
    }
    return !nativeScriptFighter_ReferencesProgram(bank) &&
        !nativeScriptItem_ReferencesProgram(bank) && !nativeScriptEffect_ReferencesProgram(bank);
}
static bool vacant(const NativeSourceItemVisuals* owner)
{
    if(!owner)return false;
    const uint8_t* bytes=(const uint8_t*)owner;
    for(size_t i=0;i<sizeof(*owner);++i)if(bytes[i])return false;
    return true;
}
static void discard(NativeSourceItemVisuals* owner)
{
    for(unsigned i=0;i<owner->count;++i)nativeGxImage_Free(owner->records[i].pixels);
    memset(owner,0,sizeof(*owner));
}

bool nativeSourceItemVisuals_Prepare(NativeSourceItemVisuals* owner,
    NativeScriptItemPrograms* programs,NativeScriptDiagnostic* diagnostic)
{
    if(!vacant(owner) || !programs || !idle() || programs->count>NATIVE_SCRIPT_ENTITY_LIMIT ||
       programs->recipe_count>NATIVE_SCRIPT_ENTITY_LIMIT)
        return error(diagnostic,"Source visuals require vacant stable storage and an idle candidate load.");
    const NativeScriptCompileBank* bank=NULL;
    for(unsigned i=0;i<programs->count;++i) {
        const NativeScriptItemProgram* program=&programs->programs[i];
        if(!program->bank || !program->bank->valid || !program->identity || !program->owner || program->visual ||
           (bank && program->bank!=bank))
            return error(diagnostic,"Source visual programs need one unbound immutable bank and valid declaration identities.");
        bank=program->bank;
        for(unsigned j=0;j<i;++j)
            if(programs->programs[j].owner==program->owner)
                return error(diagnostic,"Source visual declarations have duplicate owners.");
        if(program->recipe && (program->kind!=It_Kind_NativeHoldable ||
           program->recipe_index>=programs->recipe_count ||
           program->recipe!=&programs->recipes[program->recipe_index]))
            return error(diagnostic,"Source visual recipe is outside its prepared resource owner.");
    }
    if(bank && !unselected(bank))return error(diagnostic,"An accepted or referenced source bank cannot be rebound to candidate visuals.");
    owner->address=owner;
    for(unsigned i=0;i<programs->count;++i) {
        const NativeScriptItemProgram* program=&programs->programs[i];
        const NativeItemDefinition* recipe=program->recipe;
        if(!recipe)continue;
        if(!isfinite(recipe->billboard_size) || recipe->billboard_size<=0 ||
           (!recipe->billboard_png && recipe->billboard_png_size)) {
            error(diagnostic,"Source billboard recipe has an invalid size or missing pinned PNG.");goto failure;
        }
        // An art-free recipe retains its prepared Article model. It does not
        // acquire another declaration's billboard during drawing.
        if(!recipe->billboard_png && !recipe->billboard_png_size)continue;
        uint16_t width=0,height=0;
        if(!nativeSourceResources_PNG(recipe->billboard_png,recipe->billboard_png_size,
            &width,&height,diagnostic?diagnostic->message:NULL,diagnostic?sizeof(diagnostic->message):0))goto failure;
        NativeSourceItemVisual* visual=&owner->records[owner->count++];
        *visual=(NativeSourceItemVisual){.bank=bank,.identity=program->identity,.owner=program->owner,
            .png=recipe->billboard_png,.png_bytes=recipe->billboard_png_size,.size=recipe->billboard_size};
        visual->pixels=nativeGxImage_Decode(visual->png,visual->png_bytes,&visual->width,&visual->height);
        if(!visual->pixels || visual->width!=width || visual->height!=height ||
           ((uintptr_t)visual->pixels&31)) {
            error(diagnostic,"Source billboard PNG pixel decoding or dimensions are invalid.");goto failure;
        }
    }
    owner->ready=true;
    for(unsigned i=0;i<owner->count;++i) {
        const NativeSourceItemVisual* visual=&owner->records[i];
        for(unsigned j=0;j<programs->count;++j)
            if(programs->programs[j].owner==visual->owner){programs->programs[j].visual=visual;break;}
    }
    return true;
failure:
    discard(owner);return false;
}

bool nativeSourceItemVisuals_Release(NativeSourceItemVisuals* owner)
{
    if(!owner || !idle())return false;
    if(vacant(owner))return true;
    if(owner->address!=owner || !owner->ready || owner->count>NATIVE_SOURCE_ITEM_VISUALS)return false;
    if(owner->count && !unselected(owner->records[0].bank))return false;
    discard(owner);return true;
}
