#include "native_menu_rules.h"
#include "mnassets.h"
#include "mnmain.h"
#include "mnmainrule.h"
#include "mnruleplus.h"
#include "mnitemsw.h"
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
extern StaticModelDesc MenMainConIs_Top, MenMainCursorIs_Top;
extern StaticModelDesc MenMainCursorTr01_Top,MenMainCursorTr02_Top,MenMainCursorTr03_Top,MenMainCursorTr04_Top;
static HSD_GObj* object;
static NativeMenuRulesPage current_page;
static Uint64 animated_tick;
static unsigned adjustment_button;
static GameRules* edited_rules;
static struct GamePrefs* edited_prefs;
static MnMenuAssets* archive;
static HSD_JObj* cursor(unsigned row);
static struct LobbyValueMotion {
    Uint64 begin;
    unsigned value;
    int direction, next_direction;
    bool initialized;
} lobby_values[6];

static HSD_JObj* joint_at(HSD_JObj* root,unsigned* index)
{
    for (;root;root=root->next) {
        if (!*index) return root;
        --*index; HSD_JObj* found=joint_at(root->child,index); if (found) return found;
    }
    return NULL;
}

static void align_arrows(unsigned selected)
{
    unsigned index = 13;
    HSD_JObj* arrow_group = joint_at(cursor(selected), &index);
    HSD_JObj* left = arrow_group ? arrow_group->child : NULL;
    HSD_JObj* right = left ? left->next : NULL;
    if (!right || !left->u.dobj || !right->u.dobj ||
        arrow_group->flags & JOBJ_HIDDEN) return;

    Vec3 left_low, left_high, right_low, right_high;
    if (!nativeModelView_MeshBounds(left, left->u.dobj, &left_low, &left_high) ||
        !nativeModelView_MeshBounds(right, right->u.dobj, &right_low, &right_high))
        return;
    HSD_JObjSetupMatrix(arrow_group);
    const float scale_y = arrow_group->mtx[1][1];
    if (fabsf(scale_y) < .001f) return;
    const float correction = (right_low.y + right_high.y -
                              left_low.y - left_high.y) * .25f / scale_y;
    if (fabsf(correction) < .001f) return;
    HSD_JObjSetTranslateY(left, left->translate.y + correction);
    HSD_JObjSetTranslateY(right, right->translate.y - correction);
}
static void forget(void* data)
{
    object=NULL; HSD_Free(data);
}
void nativeMenuRules_Unload(void)
{
    if (object) HSD_GObjFree(object);
    archive=NULL; edited_rules=NULL; edited_prefs=NULL; animated_tick=0;
    adjustment_button=0;
}
static void create(NativeMenuRulesPage page)
{
    if (object) HSD_GObjFree(object);
    MenuFlow saved=mn_804A04F0; HSD_GObj* saved_object=mn_804D6BD0;
    HSD_GObj* saved_extra=mn_804D6BE0;
    HSD_GObj* saved_items = mnItemSw_804D6BE8;
    bool extra=page==NativeMenuRules_Additional;
    mn_804A04F0.cur_menu=extra?MENU_KIND_RULES_EXTRA:MENU_KIND_RULES; mn_804A04F0.hovered_selection=0;
    mn_804A04F0.confirmed_selection=edited_rules->mode;
    if (page == NativeMenuRules_Items) {
        mn_804A04F0.cur_menu = 0x10;
        object = mnItemSw_802351A0(0, edited_prefs);
        mnItemSw_80234104(object);
    } else {
        object=extra?mn_80233218(0,edited_rules,false,true):mn_80230E38(0,edited_rules,false,true);
    }
    HSD_GObjProc_RemoveAllProcs(object); HSD_GObjGXLink_8039084C(object);
    object->user_data_remove_func=forget;
    mn_804A04F0=saved; mn_804D6BD0=saved_object; mn_804D6BE0=saved_extra;
    mnItemSw_804D6BE8 = saved_items;
    if (page == NativeMenuRules_Lobby) memset(lobby_values, 0, sizeof(lobby_values));
    current_page=page; animated_tick=0;
}
void nativeMenuRules_Load(MnMenuAssets* assets,GameRules* rules,struct GamePrefs* prefs)
{
    if (object && archive==assets && edited_rules==rules) return;
    nativeMenuRules_Unload(); archive=assets; edited_rules=rules; edited_prefs=prefs;
    StaticModelDesc* models[]={&MenMainConRl_Top,&MenMainCursorRl_Top,&MenMainNmRl_Top,
        &MenMainCursorRl01_Top,&MenMainCursorRl02_Top,&MenMainCursorRl03_Top,&MenMainCursorRl04_Top,&MenMainCursorRl05_Top,&MenMainCursorTr01_Top,&MenMainCursorTr02_Top,&MenMainCursorTr03_Top,&MenMainCursorTr04_Top,
        &MenMainConIs_Top,&MenMainCursorIs_Top};
    const char* names[]={"MenMainConRl_Top","MenMainCursorRl_Top","MenMainNmRl_Top",
        "MenMainCursorRl01_Top","MenMainCursorRl02_Top","MenMainCursorRl03_Top","MenMainCursorRl04_Top","MenMainCursorRl05_Top","MenMainCursorTr01_Top","MenMainCursorTr02_Top","MenMainCursorTr03_Top","MenMainCursorTr04_Top",
        "MenMainConIs_Top","MenMainCursorIs_Top"};
    for (unsigned i=0;i<sizeof(models)/sizeof(models[0]);++i) {
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
    if (page == NativeMenuRules_Items) {
        mnItemSw_UpdateVisuals(object, selected, edited_prefs);
    } else if (tick!=animated_tick) {
        MenuFlow saved=mn_804A04F0; HSD_GObj* saved_object=mn_804D6BD0;
        HSD_GObj* saved_extra=mn_804D6BE0;
        mn_804A04F0.cur_menu=extra?MENU_KIND_RULES_EXTRA:MENU_KIND_RULES; mn_804A04F0.hovered_selection=selected<(extra?6:7)?selected:0;
        mn_804A04F0.confirmed_selection=value;
        mn_804A04F0.buttons=adjustment_button;
        adjustment_button=0;
        mn_804D6BD0=object;
        mn_804D6BE0=object;
        if (extra) fn_80232F44(object); else fn_802309F0(object);
        if (selected < 5) align_arrows(selected);
        mn_804A04F0=saved; mn_804D6BD0=saved_object; mn_804D6BE0=saved_extra;
        animated_tick=tick;
    }
    if (extra) {
        MenuRulesPlusData* data=object->user_data;
        HSD_JObjSetFlagsAll(data->cursors[5], JOBJ_HIDDEN);
    }
    if (page == NativeMenuRules_Match) {
        struct mn_802307F8_t* data = object->user_data;
        HSD_JObjSetFlagsAll(data->cursors[6], JOBJ_HIDDEN);
    }
    if (page==NativeMenuRules_Lobby) {
        struct mn_802307F8_t* data=object->user_data;
        HSD_JObjSetFlagsAll(data->cursors[6], JOBJ_HIDDEN);
        for (unsigned i=0;i<6;++i) {
            unsigned label=7; HSD_JObjSetFlags(joint_at(data->cursors[i],&label),JOBJ_HIDDEN);
            if (i>=5) { unsigned arrow=8; HSD_JObjSetFlagsAll(joint_at(data->cursors[i],&arrow),JOBJ_HIDDEN); }
            if (i<5) HSD_JObjSetFlagsAll(data->x34[i].joints[0],JOBJ_HIDDEN);
        }
    }
    Mtx view; HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(),view);
    HSD_StateInvalidate(-1); HSD_StateInitTev(); HSD_ClearVtxDesc();
    HSD_JObjDispAll(object->hsd_obj,view,HSD_TRSP_ALL,0);
}

void nativeMenuRules_AnimateAdjustment(int direction)
{
    // fn_802309F0 and fn_80232F44 select the stock left/right animation from
    // the same directional bits used by the original Rules input handlers.
    adjustment_button = direction < 0 ? 4 : 8;
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
    if (object && current_page == NativeMenuRules_Items && bounds && row < 32) {
        // The stock item's icon, label and toggle are distinct objects. Use
        // the complete row in the menu's neutral canvas for interaction.
        *bounds = row == 31 ? (NativeMenuRuleBounds){ 83, 114, 474, 26 }
                           : (NativeMenuRuleBounds){ 55 + 272 * (row / 16),
                                 153 + 17 * (row % 16), 252, 17 };
        return true;
    }
    unsigned index=3; HSD_JObj* root=cursor(row);
    return root && bounds && mesh_bounds(joint_at(root,&index),bounds);
}
void nativeMenuRules_Label(unsigned row,const char* text,bool selected)
{
    unsigned index=7; HSD_JObj* root=cursor(row);
    NativeMenuRuleBounds b;
    if (!root || !mesh_bounds(joint_at(root,&index),&b)) return;
    nativeMenuPaint_Text(text, b.x, b.y + b.h * .11f, b.w, b.h * .75f,
                         selected ? 0x070605FF : 0x696668FF,
                         NativeMenuFont_MenuOption, false, false);
}

void nativeMenuRules_AnimateLobbyValue(unsigned row, int direction)
{
    if (row < 6) lobby_values[row].next_direction = direction < 0 ? -1 : 1;
}

void nativeMenuRules_Value(unsigned row, const char* const* values,
                           unsigned count, unsigned value)
{
    NativeMenuRuleBounds b;
    if (row >= 6 || value >= count || !nativeMenuRules_Bounds(row, &b)) return;
    const float left = b.x + b.w * .532f;
    const float width = b.w * .401f;
    const float baseline = b.y + b.h * .18f;
    const float height = b.h * .62f;
    if (count == 1) {
        nativeMenuPaint_Text(values[value], left + 4, baseline, width - 8, height,
                             0xFFFFFFFF, NativeMenuFont_MenuOption, true, false);
        return;
    }

    struct LobbyValueMotion* motion = &lobby_values[row];
    const Uint64 now = SDL_GetTicksNS();
    if (!motion->initialized) {
        motion->initialized = true;
        motion->value = value;
    } else if (motion->value != value) {
        motion->begin = now;
        motion->direction = motion->next_direction ? motion->next_direction
            : (motion->value + 1) % count == value ? 1 : -1;
        motion->next_direction = 0;
        motion->value = value;
    }
    const float progress = motion->begin
        ? fminf(1, (now - motion->begin) / 133333333.0f) : 1;
    const float remainder = (1 - progress) * (1 - progress);
    const float shift = motion->direction * remainder;
    const float spacing = width * .48f;
    const float center = left + width * .5f;
    nativeMenuFont_Clip(left + 4, left + width - 4, width * .13f);
    for (int offset = -2; offset <= 2; ++offset) {
        const float lane = offset + shift;
        if (fabsf(lane) > 1.5f) continue;
        const unsigned option = (value + count * 2 + offset) % count;
        const unsigned luminosity = (unsigned)(255 - 185 * fminf(1, fabsf(lane)));
        const uint32_t color = (luminosity * 0x010101U << 8) | 0xFF;
        nativeMenuPaint_Text(values[option], center + lane * spacing - spacing * .46f,
                             baseline, spacing * .92f, height, color,
                             NativeMenuFont_MenuOption, true, false);
    }
    nativeMenuFont_Clip(0, 0, 0);
}
