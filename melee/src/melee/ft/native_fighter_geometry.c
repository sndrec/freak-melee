#include "native_fighter_geometry.h"

#ifdef MELEE_NATIVE
#include "ftparts.h"
#include "fighter.h"
#include "types.h"
#include "inlines.h"
#include <melee/mp/mpcoll.h>
#include <melee/ef/efsync.h>
#include <melee/it/types.h>
#include <melee/it/native_article.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <math.h>

bool nativeFighterGeometry_ProbeLedge(Fighter_GObj* object, float snap_y,
                                      float snap_height)
{
    Fighter* fighter = object != NULL ? object->user_data : NULL;
    if (fighter == NULL || !isfinite(snap_y) || !isfinite(snap_height))
        return false;
    CollData collision = *getFtColl(fighter);
    collision.ledge_snap_y += snap_y;
    collision.ledge_snap_height += snap_height;
    if (fighter->facing_dir > 0) {
        if (!mpColl_80044164(&collision, &fighter->coll_data.ledge_id_left))
            return false;
        fighter->coll_data.env_flags |= Collide_LeftLedgeGrab;
    } else {
        if (!mpColl_800443C4(&collision, &fighter->coll_data.ledge_id_right))
            return false;
        fighter->coll_data.env_flags |= Collide_RightLedgeGrab;
    }
    return true;
}

float nativeFighterGeometry_BoneSegmentAngle(Fighter_GObj* object,
                                            Fighter_Part part, Vec3* endpoint,
                                            Vec3* start)
{
    Fighter* fighter = object != NULL ? object->user_data : NULL;
    if (fighter == NULL || endpoint == NULL || start == NULL) return 0;
    FighterPartsTable* table = ftPartsTable[fighter->kind];
    Fighter_Part bone = ftParts_GetBoneIndex(fighter, part);
    if (table == NULL || (u32)bone >= table->parts_num ||
        fighter->parts[bone].joint == NULL) return 0;
    Vec3 world_endpoint, world_start;
    lb_8000B1CC(fighter->parts[bone].joint, endpoint, &world_endpoint);
    lb_8000B1CC(fighter->parts[bone].joint, start, &world_start);
    return atan2f(world_endpoint.y - world_start.y,
                  world_endpoint.x - world_start.x);
}

void nativeFighterGeometry_ItemEffect(Item_GObj* object, s32 effect,
                                     Fighter_Part part, Vec3* offset,
                                     float angle)
{
    Item* item = object != NULL ? object->user_data : NULL;
    HSD_GObj* owner = nativeArticle_Creator(object);
    if (owner == NULL || item->owner != owner ||
        owner->classifier != HSD_GOBJ_CLASS_FIGHTER ||
        owner->user_data == NULL || offset == NULL || !isfinite(angle) ||
        !isfinite(offset->x) || !isfinite(offset->y) || !isfinite(offset->z))
        return;
    Fighter* fighter = owner->user_data;
    FighterPartsTable* table = ftPartsTable[fighter->kind];
    Fighter_Part bone = ftParts_GetBoneIndex(fighter, part);
    if (table == NULL || (u32)bone >= table->parts_num ||
        fighter->parts[bone].joint == NULL) return;
    Vec3 position;
    lb_8000B1CC(fighter->parts[bone].joint, offset, &position);
    // Directional effects copy the position and angle during creation.
    efSync_Spawn(effect, object, &position, &angle);
}
#endif
