#include "native_menu_rules.h"
#include "mnassets.h"
#include "mnmain.h"
#include "mnmainrule.h"
#include "mnruleplus.h"
#include "native_menu_paint.h"
#include "types.h"
#include <melee/gm/types.h>
#include "native_menu_scene.h"
#include <melee/lb/lbspdisplay.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <SDL3/SDL_timer.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern StaticModelDesc MenMainConRl_Top, MenMainCursorRl_Top, MenMainNmRl_Top;
extern StaticModelDesc MenMainCursorRl01_Top,MenMainCursorRl02_Top,MenMainCursorRl03_Top,MenMainCursorRl04_Top,MenMainCursorRl05_Top;
extern HSD_GObj* mn_804D6BD0;
extern HSD_GObj* mn_804D6BE0;
extern StaticModelDesc MenMainCursorTr01_Top,MenMainCursorTr02_Top,MenMainCursorTr03_Top,MenMainCursorTr04_Top;
static HSD_GObj* object;
static NativeMenuRulesPage current_page;
static Uint64 animated_tick;
static GameRules* edited_rules;
static MnMenuAssets* archive;

static HSD_JObj* joint_at(HSD_JObj* root,unsigned* index)
{
    for (;root;root=root->next) {
        if (!*index) return root;
        --*index; HSD_JObj* found=joint_at(root->child,index); if (found) return found;
    }
    return NULL;
}
static void forget(void* data)
{
    object=NULL; HSD_Free(data);
}
void nativeMenuRules_Unload(void)
{
    if (object) HSD_GObjFree(object);
    archive=NULL; edited_rules=NULL; animated_tick=0;
}
static void create(NativeMenuRulesPage page)
{
    if (object) HSD_GObjFree(object);
    MenuFlow saved=mn_804A04F0; HSD_GObj* saved_object=mn_804D6BD0;
    HSD_GObj* saved_extra=mn_804D6BE0;
    bool extra=page==NativeMenuRules_Additional;
    mn_804A04F0.cur_menu=extra?MENU_KIND_RULES_EXTRA:MENU_KIND_RULES; mn_804A04F0.hovered_selection=0;
    mn_804A04F0.confirmed_selection=edited_rules->mode;
    object=extra?mn_80233218(0,edited_rules,false,true):mn_80230E38(0,edited_rules,false,true);
    HSD_GObjProc_RemoveAllProcs(object); HSD_GObjGXLink_8039084C(object);
    object->user_data_remove_func=forget;
    mn_804A04F0=saved; mn_804D6BD0=saved_object; mn_804D6BE0=saved_extra;
    current_page=page; animated_tick=0;
}
void nativeMenuRules_Load(MnMenuAssets* assets,GameRules* rules)
{
    if (object && archive==assets && edited_rules==rules) return;
    nativeMenuRules_Unload(); archive=assets; edited_rules=rules;
    StaticModelDesc* models[]={&MenMainConRl_Top,&MenMainCursorRl_Top,&MenMainNmRl_Top,
        &MenMainCursorRl01_Top,&MenMainCursorRl02_Top,&MenMainCursorRl03_Top,&MenMainCursorRl04_Top,&MenMainCursorRl05_Top,&MenMainCursorTr01_Top,&MenMainCursorTr02_Top,&MenMainCursorTr03_Top,&MenMainCursorTr04_Top};
    const char* names[]={"MenMainConRl_Top","MenMainCursorRl_Top","MenMainNmRl_Top",
        "MenMainCursorRl01_Top","MenMainCursorRl02_Top","MenMainCursorRl03_Top","MenMainCursorRl04_Top","MenMainCursorRl05_Top","MenMainCursorTr01_Top","MenMainCursorTr02_Top","MenMainCursorTr03_Top","MenMainCursorTr04_Top"};
    for (unsigned i=0;i<12;++i) {
        char symbol[96];
        snprintf(symbol,sizeof(symbol),"%s_joint",names[i]); models[i]->joint=mnAssets_Get(assets,symbol);
        snprintf(symbol,sizeof(symbol),"%s_animjoint",names[i]); models[i]->animjoint=mnAssets_Get(assets,symbol);
        snprintf(symbol,sizeof(symbol),"%s_matanim_joint",names[i]); models[i]->matanim_joint=mnAssets_Get(assets,symbol);
        snprintf(symbol,sizeof(symbol),"%s_shapeanim_joint",names[i]); models[i]->shapeanim_joint=mnAssets_Get(assets,symbol);
    }
    create(NativeMenuRules_Match);
}
void nativeMenuRules_Draw(NativeMenuRulesPage page,unsigned selected,unsigned value)
{
    if (!object) return;
    if (current_page!=page) create(page);
    bool extra=page==NativeMenuRules_Additional;
    if (page==NativeMenuRules_Lobby) {
        struct mn_802307F8_t* data=object->user_data;
        const unsigned fields[]={data->x2,data->x2==1?data->x9:data->x3,data->x4,data->x5,data->x6,0,0};
        value=fields[selected<7?selected:0];
    }
    Uint64 tick=SDL_GetTicksNS()/16666667;
    if (tick!=animated_tick) {
        MenuFlow saved=mn_804A04F0; HSD_GObj* saved_object=mn_804D6BD0;
        HSD_GObj* saved_extra=mn_804D6BE0;
        mn_804A04F0.cur_menu=extra?MENU_KIND_RULES_EXTRA:MENU_KIND_RULES; mn_804A04F0.hovered_selection=selected<(extra?6:7)?selected:0;
        mn_804A04F0.confirmed_selection=value;
        mn_804D6BD0=object;
        mn_804D6BE0=object;
        if (extra) fn_80232F44(object); else fn_802309F0(object);
        mn_804A04F0=saved; mn_804D6BD0=saved_object; mn_804D6BE0=saved_extra;
        animated_tick=tick;
    }
    if (extra) {
        MenuRulesPlusData* data=object->user_data;
        unsigned label=7; HSD_JObjSetFlags(joint_at(data->cursors[5],&label),JOBJ_HIDDEN);
    }
    if (page==NativeMenuRules_Lobby) {
        struct mn_802307F8_t* data=object->user_data;
        for (unsigned i=0;i<7;++i) {
            unsigned label=7; HSD_JObjSetFlags(joint_at(data->cursors[i],&label),JOBJ_HIDDEN);
            if (i>=5) { unsigned arrow=8; HSD_JObjSetFlagsAll(joint_at(data->cursors[i],&arrow),JOBJ_HIDDEN); }
            if (i<5) HSD_JObjSetFlagsAll(data->x34[i].joints[0],JOBJ_HIDDEN);
        }
    }
    Mtx view; HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(),view);
    HSD_StateInvalidate(-1); HSD_StateInitTev(); HSD_ClearVtxDesc();
    HSD_JObjDispAll(object->hsd_obj,view,HSD_TRSP_ALL,0);
}
static bool mesh_bounds(HSD_JObj* body,NativeMenuRuleBounds* bounds)
{
    if (!body || !body->u.dobj || !body->u.dobj->pobj) return false;
    Vec3 low,high;
    if (!nativeModelView_MeshBounds(body,body->u.dobj,&low,&high)) return false;
    Mtx view; Mtx44 projection;
    HSD_CObj* camera=HSD_CObjGetCurrent(); HSD_CObjGetViewingMtx(camera,view); makeProjectionMtx(camera,projection);
    float min_x=FLT_MAX,min_y=FLT_MAX,max_x=-FLT_MAX,max_y=-FLT_MAX;
    for (unsigned corner=0;corner<8;++corner) {
        Vec3 world={corner&1?high.x:low.x,corner&2?high.y:low.y,corner&4?high.z:low.z},point;
        C_MTXMultVec(view,&world,&point);
        float w=projection[3][0]*point.x+projection[3][1]*point.y+projection[3][2]*point.z+projection[3][3];
        if (w<=0) continue;
        float x=projection[0][0]*point.x+projection[0][1]*point.y+projection[0][2]*point.z+projection[0][3];
        float y=projection[1][0]*point.x+projection[1][1]*point.y+projection[1][2]*point.z+projection[1][3];
        if (!nativeMenuScene_Unproject(.5f+x/w*.5f,.5f-y/w*.5f,&x,&y)) continue;
        min_x=fminf(min_x,x); max_x=fmaxf(max_x,x); min_y=fminf(min_y,y); max_y=fmaxf(max_y,y);
    }
    if (max_x<=min_x || max_y<=min_y) return false;
    *bounds=(NativeMenuRuleBounds){min_x,min_y,max_x-min_x,max_y-min_y}; return true;
}

static HSD_JObj* cursor(unsigned row)
{
    if (!object || row>=(current_page==NativeMenuRules_Additional?6:7)) return NULL;
    if (current_page==NativeMenuRules_Additional) return ((MenuRulesPlusData*)object->user_data)->cursors[row];
    return ((struct mn_802307F8_t*)object->user_data)->cursors[row];
}
bool nativeMenuRules_Bounds(unsigned row,NativeMenuRuleBounds* bounds)
{
    unsigned index=3; HSD_JObj* root=cursor(row);
    return root && bounds && mesh_bounds(joint_at(root,&index),bounds);
}
void nativeMenuRules_Label(unsigned row,const char* text,bool selected)
{
    unsigned index=7; HSD_JObj* root=cursor(row);
    NativeMenuRuleBounds b;
    if (!root || !mesh_bounds(joint_at(root,&index),&b)) return;
    nativeMenuPaint_Text(text,b.x,b.y,b.w,b.h,selected?0x101010FF:0x99949DFF,NativeMenuFont_MenuOption,false,false);
}

void nativeMenuRules_Value(unsigned row,const char* text)
{
    NativeMenuRuleBounds b;
    if (!nativeMenuRules_Bounds(row,&b)) return;
    nativeMenuPaint_Text(text,b.x+b.w*.54f,b.y+b.h*.18f,b.w*.4f,b.h*.62f,0xFFFFFFFF,NativeMenuFont_MenuOption,true,false);
}
