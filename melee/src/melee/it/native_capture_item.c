#include "native_capture_item.h"

#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_source_package.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_279C.h>
#include <melee/it/item.h>
#include <melee/it/ithitbox.h>
#include <melee/it/itmaplib.h>
#include <melee/it/itzako.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/jobj.h>

static bool inert_animation(Item_GObj* object) { return false; }
static void inert_physics(Item_GObj* object) {}
static bool inert_collision(Item_GObj* object) { return false; }

static ItemStateTable captured_states[] = {
    { -1, inert_animation, inert_physics, inert_collision },
};

bool nativeCaptureItem_Current(const NativeCaptureItemPair* pair,bool attached)
{
    if(!pair || !nativeFighterLifetime_Current(&pair->holder))return false;
    if(!pair->item.object)return !pair->holder.data->target_item_gobj;
    if(!nativeItemLifetime_Current(&pair->item) || pair->item.removing ||
       pair->item.reference.lifetime!=pair->holder.reference.lifetime ||
       pair->holder.data->target_item_gobj!=pair->item.object)return false;
    return !attached || (pair->item.data->grab_victim==pair->holder.object &&
        pair->item.data->atk_victim==pair->holder.object && pair->item.data->native_capture_inert);
}

bool nativeCaptureItem_Retain(Fighter_GObj* holder,NativeCaptureItemPair* output)
{
    NativeMatchPoolReference reference;NativeCaptureItemPair pair={0};
    if(!output || !nativeMatchPool_Reference(&nativeGObjPool,holder,&reference) ||
       holder->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !nativeFighterLifetime_Capture(holder->user_data,&pair.holder))return false;
    Item_GObj* target=pair.holder.data->target_item_gobj;
    if(target && !nativeItemLifetime_Capture(target,&pair.item))return false;
    if(!nativeCaptureItem_Current(&pair,false))return false;
    *output=pair;return true;
}

typedef struct CaptureRig {
    FighterPartsTable* parts_table;
    FighterBone* parts;
    uint32_t part_count,part;
    HSD_JObj* holder_joint;
    const ItemModelDesc* model;
    DynamicBoneTable* bones;
    const HSD_JObj* item_joint;
} CaptureRig;

static bool rig_current(const NativeCaptureItemPair* pair,const CaptureRig* rig,bool attached)
{
    if(!nativeCaptureItem_Current(pair,attached) || !ftPartsTable ||
       ftPartsTable[pair->holder.kind]!=rig->parts_table || rig->parts_table->parts_num!=rig->part_count ||
       pair->holder.data->parts!=rig->parts || rig->parts[rig->part].joint!=rig->holder_joint ||
       pair->item.data->xC4_article_data->x10_modelDesc!=rig->model ||
       pair->item.data->xBBC_dynamicBoneTable!=rig->bones)return false;
    int bone=rig->model->x8_bone_attach_id;const HSD_JObj* joint=pair->item.model;
    if(bone<0 || bone>=100 || rig->model->x4_bone_count>100)return false;
    if(rig->model->x4_bone_count){
        if((unsigned)bone>=rig->model->x4_bone_count || !rig->bones)return false;
        joint=rig->bones->bones[bone];
    }else for(int n=0;n<bone&&joint;++n)joint=joint->child;
    return joint && joint==rig->item_joint;
}

static bool retain_rig(const NativeCaptureItemPair* pair,unsigned bone,CaptureRig* output)
{
    Fighter* holder=pair->holder.data;Item* item=pair->item.data;
    if(!pair->item.object || !ftPartsTable || (unsigned)pair->holder.kind>=Ft_Kind_RuntimeCapacity ||
       !holder->parts || !ftPartsTable[pair->holder.kind] || !item->xC4_article_data ||
       !item->xC4_article_data->x10_modelDesc)return false;
    CaptureRig rig={.parts_table=ftPartsTable[pair->holder.kind],.parts=holder->parts,.part=bone,
        .model=item->xC4_article_data->x10_modelDesc,.bones=item->xBBC_dynamicBoneTable};
    rig.part_count=rig.parts_table->parts_num;
    if(!rig.part_count || rig.part_count>MAX_FT_PARTS || bone>=rig.part_count || !rig.parts[bone].joint)return false;
    if(pair->holder.source_owned){
        const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(pair->holder.kind);
        if(!package || rig.parts_table!=&package->assets.parts ||
           rig.part_count!=package->assets.facts.joints+package->assets.facts.omission_count)return false;
    }
    rig.holder_joint=rig.parts[bone].joint;int item_bone=rig.model->x8_bone_attach_id;
    if(item_bone<0 || item_bone>=100 || rig.model->x4_bone_count>100)return false;
    rig.item_joint=pair->item.model;
    if(rig.model->x4_bone_count){
        if((unsigned)item_bone>=rig.model->x4_bone_count || !rig.bones)return false;
        rig.item_joint=rig.bones->bones[item_bone];
    }else for(int n=0;n<item_bone&&rig.item_joint;++n)rig.item_joint=rig.item_joint->child;
    if(!rig.item_joint || !rig_current(pair,&rig,false))return false;
    *output=rig;return true;
}

bool nativeCaptureItem_Attach(Item_GObj* object, Fighter_GObj* holder,
                              unsigned bone)
{
    NativeCaptureItemPair pair;CaptureRig rig;
    if(!nativeCaptureItem_Retain(holder,&pair) || pair.item.object!=object ||
       !retain_rig(&pair,bone,&rig))return false;
    Item* item=pair.item.data;
    // Match native capture: selection has already admitted this Item, and
    // capture replaces its previous attack/grab recipient relationships.
    item->atk_victim = holder;
    item->grab_victim = holder;
    item->native_capture_inert = true;
    it_8026C220(object, holder);
    if(!rig_current(&pair,&rig,true))return false;
    item->xDD0_flag.b1 = false;
    item->xBC_itemStateContainer = captured_states;
    it_80274F48(object, item->xC4_article_data->x10_modelDesc->x8_bone_attach_id,
                holder, bone);
    if(!rig_current(&pair,&rig,true))return false;
    Item_80268E5C(object, 0, 0);
    if(!rig_current(&pair,&rig,true))return false;
    it_802762BC(item);
    it_802756D0(object);
    if(!rig_current(&pair,&rig,true))return false;
    it_80274ECC(object, true);
    it_80274C88(object);
    if(!rig_current(&pair,&rig,true))return false;
    it_8026BD54(object);
    if(!rig_current(&pair,&rig,true))return false;
    it_80273408(object);
    return rig_current(&pair,&rig,true);
}

bool nativeCaptureItem_Release(Item_GObj* object, unsigned bone,
                               bool clear_destroy_type)
{
    NativeItemLifetime target;NativeCaptureItemPair pair;CaptureRig rig;
    if(!nativeItemLifetime_Capture(object,&target) || !target.data->grab_victim ||
       !nativeCaptureItem_Retain(target.data->grab_victim,&pair) || pair.item.object!=object ||
       !nativeCaptureItem_Current(&pair,true) || !retain_rig(&pair,bone,&rig))return false;
    Item* item=pair.item.data;
    it_80275070(object,
                item->xC4_article_data->x10_modelDesc->x8_bone_attach_id);
    if(!rig_current(&pair,&rig,true))return false;
    lb_8000B1CC(rig.holder_joint, NULL, &item->pos);
    if(!rig_current(&pair,&rig,true))return false;
    it_8027B4A4(item->grab_victim, object);
    if(!rig_current(&pair,&rig,true))return false;
    it_8027B378(item->grab_victim, object, 0);
    if(!rig_current(&pair,&rig,true))return false;
    if (clear_destroy_type) item->destroy_type = 0;
    item->atk_victim = NULL;
    item->grab_victim = NULL;
    it_8027CE44(object);
    if(!rig_current(&pair,&rig,false))return false;
    // Destruction can run authored callbacks. Unlink this captured Item first;
    // a callback which selects another target must keep its replacement link.
    pair.holder.data->target_item_gobj=NULL;
    if(pair.holder.data->x1A64==object)pair.holder.data->x1A64=NULL;
    Item_8026A8EC(object);
    return nativeFighterLifetime_Current(&pair.holder) &&
        nativeMatchPool_Resolve(pair.item.reference)==NULL;
}
