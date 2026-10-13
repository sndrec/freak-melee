#include "native_script_binding_stage.h"
#include "native_fighter_lifetime.h"
#include <melee/it/native_item_lifetime.h>
#include <melee/db/db.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/gr/grbigblue.h>
#include <melee/gr/grcorneria.h>
#include <melee/gr/grinishie1.h>
#include <melee/gr/grvenom.h>
#include <melee/gr/stage_events.h>
#include <melee/it/it_26B1.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct StageCaller {
    NativeFighterLifetime fighter;
    NativeItemLifetime item;
    uint32_t kind;
} StageCaller;

static bool error(NativeScriptDiagnostic* d, const char* message)
{
    if (d) snprintf(d->message, sizeof(d->message), "%s", message);
    return false;
}

static bool current(NativeScriptRuntimeContext* c, const StageCaller* s)
{
    if (!c || c->source_class != s->kind)
        return false;
    if (s->kind == NativeCallbackSource_Fighter)
        return c->source == s->fighter.object &&
            c->generation == s->fighter.generation &&
            c->scene_epoch == s->fighter.reference.lifetime &&
            nativeFighterLifetime_Current(&s->fighter) &&
            nativeScriptBindings_SourceAlive(c) &&
            nativeScriptBindings_SourceFighterLayout(c, NULL) ==
                &s->fighter.program->bank->motion_layout;
    return c->source == s->item.object && c->generation == s->item.generation &&
        c->scene_epoch == s->item.reference.lifetime &&
        nativeItemLifetime_Current(&s->item) && nativeScriptBindings_SourceAlive(c);
}

static bool capture(NativeScriptRuntimeContext* c, StageCaller* s,
                    NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if (!c || !c->scene_epoch ||
        !nativeMatchPool_Reference(&nativeGObjPool, c->source, &reference) ||
        reference.lifetime != c->scene_epoch ||
        (c->source_class != NativeCallbackSource_Fighter &&
         c->source_class != NativeCallbackSource_Item) ||
        ((HSD_GObj*) c->source)->classifier !=
            (c->source_class == NativeCallbackSource_Fighter ?
                HSD_GOBJ_CLASS_FIGHTER : HSD_GOBJ_CLASS_ITEM) ||
        !nativeScriptBindings_SourceAlive(c))
        return error(d, "Stage query requires its live source scene");
    s->kind = c->source_class;
    if (s->kind == NativeCallbackSource_Fighter) {
        if (!nativeFighterLifetime_Capture(((HSD_GObj*) c->source)->user_data,
                                          &s->fighter) ||
            !s->fighter.source_owned)
            return error(d, "Stage query requires owned Fighter storage");
    } else if (s->kind == NativeCallbackSource_Item) {
        if (!nativeItemLifetime_Capture(c->source, &s->item) ||
            !s->item.source_owned)
            return error(d, "Stage query requires owned Article storage");
    } else return error(d, "Stage query source class is unsupported");
    return current(c, s) ? true : error(d, "Stage query source schema changed");
}

static bool category(const NativeScriptValue* v, uint32_t* result)
{
    uint64_t bits;
    switch (v->type) {
    case NativeScriptType_Bool:
        if (v->as.u64 > 1) return false;
        bits = v->as.u64; break;
    case NativeScriptType_I32:
        if (v->as.i64 < INT32_MIN || v->as.i64 > INT32_MAX) return false;
        bits = (uint64_t) v->as.i64; break;
    case NativeScriptType_I64: bits = (uint64_t) v->as.i64; break;
    case NativeScriptType_U32:
        if (v->as.u64 > UINT32_MAX) return false;
        bits = v->as.u64; break;
    case NativeScriptType_U64: bits = v->as.u64; break;
    default: return false;
    }
    *result = (uint32_t) (bits & 15);
    return true;
}

static bool sweep_arithmetic(const float* v)
{
    // The probe starts with a zero ECB. These are exactly the maximum terms
    // used by mpColl_80043754 before converting x/6 to s32 and adding one.
    float x = fabsf(v[3] - v[0]);
    const float terms[] = { fabsf(v[4] - v[1]), fabsf(v[8]),
        fabsf(v[10]), fabsf(v[6]), fabsf(v[11]) };
    for (unsigned i = 0; i < sizeof(terms) / sizeof(*terms); ++i)
        if (terms[i] > x) x = terms[i];
    float steps = x / 6.0F;
    if (!isfinite(x) || (x > 6.0F && (double) steps >= INT32_MAX))
        return false;
    // mpCollPrev/End assert this native debug domain, and their diagnostic
    // path assumes a GObj whereas an independent probe intentionally has none.
    return DbLevel < DbLKind_DebugRom ||
        (v[0] > -45000.0F && v[0] < 45000.0F &&
         v[1] > -45000.0F && v[1] < 45000.0F &&
         v[3] > -45000.0F && v[3] < 45000.0F &&
         v[4] > -45000.0F && v[4] < 45000.0F);
}

static bool line_index(const NativeScriptValue* argument, int* line,
                       NativeScriptDiagnostic* d)
{
    switch (argument->type) {
    case NativeScriptType_Bool:
        if (argument->as.u64 > 1) break;
        *line = (int) argument->as.u64; return true;
    case NativeScriptType_I32: case NativeScriptType_I64:
        if (argument->as.i64 < INT32_MIN || argument->as.i64 > INT32_MAX) break;
        *line = (int32_t) argument->as.i64; return true;
    case NativeScriptType_U32: case NativeScriptType_U64:
        if (argument->as.u64 > INT32_MAX) break;
        *line = (int32_t) argument->as.u64; return true;
    default: break;
    }
    return error(d, "Stage line requires an integral native signed 32-bit index");
}

static bool line_query(NativeScriptRuntimeContext* c, uint32_t semantic,
    NativeScriptValue* arguments, uint32_t count, NativeScriptValue* output,
    NativeScriptDiagnostic* d)
{
    bool normal = semantic == NativeScriptBinding_StageLineNormal;
    if (!arguments || !output || count != (normal ? 4u : 1u))
        return error(d, "Stage line query argument shape is invalid");
    if (normal)
        for (unsigned i = 1; i < 4; ++i)
            if (arguments[i].type != NativeScriptType_F32)
                return error(d, "Stage normal outputs require exact F32 destinations");
    int line;
    if (!line_index(arguments, &line, d)) return false;
    StageCaller source = {0};
    if (!capture(c, &source, d)) return false;
    MapCollData* map = mpLib_8004D164();
    CollLine* lines = mpGetGroundCollLine();
    bool present = map && lines && line >= 0 && line < map->line_count &&
        mpLib_80054ED8(line);
    NativeScriptValue result = {.type = normal ? NativeScriptType_Void : NativeScriptType_Bool};
    Vec3 value = {0};
    CollVtx* vertices = NULL;
    MapLine* description = NULL;
    int line_count = map ? map->line_count : 0, vertex_count = 0;
    uint16_t v0 = 0, v1 = 0;
    if (normal) {
        if (!present) return error(d, "Stage normal requires a present enabled collision line");
        vertices = mpGetGroundCollVtx(); description = lines[line].x0;
        vertex_count = map->vert_count;
        if (!vertices || !description || description->v0_idx >= vertex_count ||
            description->v1_idx >= vertex_count)
            return error(d, "Stage normal line has no live vertex geometry");
        v0 = description->v0_idx; v1 = description->v1_idx;
        mpLineGetNormal(line, &value);
    } else result.as.u64 = present;
    if (!current(c, &source) || mpLib_8004D164() != map ||
        mpGetGroundCollLine() != lines || (map && map->line_count != line_count) ||
        (normal && (mpGetGroundCollVtx() != vertices ||
         map->vert_count != vertex_count || lines[line].x0 != description ||
         description->v0_idx != v0 || description->v1_idx != v1)))
        return error(d, "Stage line query source or geometry changed before publication");
    if (normal) {
        arguments[1] = (NativeScriptValue){.type = NativeScriptType_F32, .as.f32 = value.x};
        arguments[2] = (NativeScriptValue){.type = NativeScriptType_F32, .as.f32 = value.y};
        arguments[3] = (NativeScriptValue){.type = NativeScriptType_F32, .as.f32 = value.z};
    }
    *output = result;
    return true;
}

typedef struct StageGeometry {
    MapCollData* map;
    CollLine* lines;
    CollVtx* vertices;
    CollJoint* joints;
    int line_count,vertex_count,joint_count;
} StageGeometry;

static StageGeometry geometry(void)
{
    StageGeometry g={.map=mpLib_8004D164(),.lines=mpGetGroundCollLine(),
        .vertices=mpGetGroundCollVtx(),.joints=mpGetGroundCollJoint()};
    if(g.map){g.line_count=g.map->line_count;g.vertex_count=g.map->vert_count;g.joint_count=g.map->joint_count;}
    return g;
}

static bool geometry_current(const StageGeometry* g)
{
    return mpLib_8004D164()==g->map && mpGetGroundCollLine()==g->lines &&
        mpGetGroundCollVtx()==g->vertices && mpGetGroundCollJoint()==g->joints &&
        (!g->map||(g->map->line_count==g->line_count&&g->map->vert_count==g->vertex_count&&g->map->joint_count==g->joint_count));
}

static bool bounded_line(const StageGeometry* g,int line)
{return g->map&&g->lines&&line>=0&&line<g->line_count;}

static bool island_items(const StageGeometry* g,uint64_t epoch,NativeScriptDiagnostic* d)
{
    if(!HSD_GObjPLinkHead)return error(d,"Stage island assessment requires its physical Item registry");
    unsigned visited=0;
    for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];object;object=object->next){
        NativeMatchPoolReference reference;uint32_t generation;
        if(visited++>=nativeGObjPool.capacity||!nativeMatchPool_Reference(&nativeGObjPool,object,&reference)||
           reference.lifetime!=epoch||object->classifier!=HSD_GOBJ_CLASS_ITEM||
           !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation))
            return error(d,"Stage island Item registry is stale, cyclic or has a foreign class");
        const Item* item=object->user_data;
        if(!item||item->entity!=object)return error(d,"Stage island Item payload changed physical identity");
        // These are exactly the native hazard kinds which reach mpIsland.
        // Its -1 sentinel is valid; other invalid indices would enter a native
        // diagnostic infinite loop rather than returning a missing island.
        if((item->kind==It_Kind_BombHei||item->kind==It_Kind_G_Shell||item->kind==It_Kind_R_Shell)&&
           it_8026C1B4(object)&&item->x378_itemColl.floor.index!=-1&&
           !bounded_line(g,item->x378_itemColl.floor.index))
            return error(d,"Stage island hazard has an invalid native floor index");
    }
    return true;
}

static bool terrain_query(NativeScriptRuntimeContext* c,uint32_t semantic,
    NativeScriptValue* args,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    bool floor=semantic==NativeScriptBinding_StageReadFloor;
    bool bounds=semantic==NativeScriptBinding_StageReadLineIslandBounds;
    bool bounds3=semantic==NativeScriptBinding_StageReadLineIslandBounds3D;
    bool same=semantic==NativeScriptBinding_StageSameLineIsland;
    if(!args||!output||count!=(floor||bounds3?7u:bounds?5u:same?2u:1u))
        return error(d,"Stage terrain query argument shape is invalid");
    int first=0,second=0;bool finite=true;
    if(floor){
        for(unsigned i=0;i<6;++i){
            if(args[i].type!=NativeScriptType_F32)return error(d,"Stage floor coordinates require exact F32 values and outputs");
            if(i<4&&!isfinite(args[i].as.f32))finite=false;
        }
        if(args[6].type!=NativeScriptType_I32)return error(d,"Stage floor line output requires an Int destination");
    }else{
        if(!line_index(&args[0],&first,d)||(same&&!line_index(&args[1],&second,d)))return false;
        if(bounds||bounds3)for(unsigned i=1;i<count;++i)
            if(args[i].type!=NativeScriptType_F32)return error(d,"Stage island bounds outputs require exact F32 destinations");
    }
    StageCaller source={0};if(!capture(c,&source,d))return false;
    StageGeometry g=geometry();Vec3 left={0},right={0};int line=-1;bool found=false;
    mp_UnkStruct0* island=NULL;
    if(floor){
        Vec3 normal;u32 flags;
        if(finite&&g.map&&g.lines&&g.vertices&&g.joints)
            found=mpCheckFloor(args[0].as.f32,args[1].as.f32,args[2].as.f32,args[3].as.f32,0.0f,
                &left,&line,&flags,&normal,-1,-1,-1,NULL,NULL);
    }else if(bounded_line(&g,first)&&(!same||bounded_line(&g,second))){
        if(semantic==NativeScriptBinding_StageCpuFloorExcluded)
            found=grBigBlue_801EF844(first)||grInishie1_801FCAAC(first)||grCorneria_801E2D90(first)||grVenom_80206D10(first);
        else{
            island=mpIsland_8005AB54(first);
            if(bounds||bounds3){if(island){left=island->x8;right=island->x14;found=true;}}
            else if(same)found=island&&island==mpIsland_8005AB54(second);
            else if(island){
                if(!island_items(&g,c->scene_epoch,d))return false;
                found=ftCo_CpuIslandBlocked(island);
            }
        }
    }
    if(!current(c,&source)||!geometry_current(&g)||
       (island&&mpIsland_8005AB54(first)!=island)||
       (floor&&found&&!bounded_line(&g,line)))
        return error(d,"Stage terrain source or geometry changed before publication");
    if(found&&(floor||bounds||bounds3)){
        float values[]={left.x,left.y,left.z,right.x,right.y,right.z};
        if(floor){
            args[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=left.x};
            args[5]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=left.y};
            args[6]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=line};
        }else for(unsigned i=1;i<count;++i){
            unsigned component=bounds3?i-1:i<3?i-1:i;
            args[i]=(NativeScriptValue){.type=NativeScriptType_F32};
            memcpy(&args[i].as.f32,&values[component],sizeof(float));
        }
    }
    *output=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=found};return true;
}

static bool island_bounds_admitted(const StageGeometry* g,const mp_UnkStruct0* island,
    NativeScriptDiagnostic* d)
{
    if((unsigned)island->x28>=(unsigned)g->joint_count||!g->joints[island->x28].inner)
        return error(d,"Floor island bounds have no admitted collision joint");
    const MapJoint* joint=g->joints[island->x28].inner;
    if(joint->floor_start<0||joint->floor_count<0||joint->floor_start>g->line_count||
       joint->floor_count>g->line_count-joint->floor_start)
        return error(d,"Floor island bounds exceed the collision-line table");
    for(unsigned i=joint->floor_start;i<(unsigned)(joint->floor_start+joint->floor_count);++i) {
        const MapLine* line=g->lines[i].x0;
        if(!line||line->v0_idx>=g->vertex_count||line->v1_idx>=g->vertex_count)
            return error(d,"Floor island bounds have missing endpoint geometry");
    }
    return true;
}

static bool island_bounds(NativeScriptRuntimeContext* c,NativeScriptValue* args,
    uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(!args||!output||count!=9||args[4].type!=NativeScriptType_Bool)
        return error(d,"Island bounds require four Float inputs and Bool/XY/XY outputs");
    for(unsigned i=0;i<9;++i)if(i!=4) {
        if(args[i].type!=NativeScriptType_F32||(i<4&&!isfinite(args[i].as.f32)))
            return error(d,"Island bounds coordinates/depth require finite Floats and exact Float outputs");
    }
    if(!isfinite(args[1].as.f32+args[3].as.f32))
        return error(d,"Island bounds probe endpoint overflowed");
    StageCaller source={0};if(!capture(c,&source,d))return false;
    StageGeometry g=geometry();Vec3 origin={args[0].as.f32,args[1].as.f32,args[2].as.f32};
    Vec3 left=origin,right=origin;mp_UnkStruct0* island=NULL;
    mp_UnkStruct0* islands=mpIsland_80458E88.next;
    if(g.map&&g.lines&&g.vertices&&g.joints) {
        island=mpIsland_8005AC14(&origin,args[3].as.f32);
        if(!current(c,&source)||!geometry_current(&g)||mpIsland_80458E88.next!=islands)
            return error(d,"Island bounds source/geometry changed during its floor probe");
        if(island) {
            if(!island_bounds_admitted(&g,island,d))return false;
            // This refreshes moving-terrain endpoints. Cached x8/x14 alone do
            // not implement the native query used by authored CPU policies.
            mpIsland_8005ACE8(island,&left,&right);
        }
    }
    if(!current(c,&source)||!geometry_current(&g)||mpIsland_80458E88.next!=islands)
        return error(d,"Island bounds source/geometry changed before publication");
    args[4]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=island!=NULL};
    args[5]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=left.x};
    args[6]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=left.y};
    args[7]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=right.x};
    args[8]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=right.y};
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

static bool compare_floor_islands(NativeScriptRuntimeContext* c,NativeScriptValue* args,
    uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(!args||!output||count!=12)return error(d,"Floor island comparison requires eight inputs and four outputs");
    for(unsigned i=0;i<7;++i)
        if(args[i].type!=NativeScriptType_F32||!isfinite(args[i].as.f32))
            return error(d,"Floor island comparison coordinates/depth require finite Floats");
    for(unsigned i=7;i<10;++i)
        if(args[i].type!=NativeScriptType_Bool||(i==7&&args[i].as.u64>1))
            return error(d,"Floor island comparison uses a Bool switch and two Bool outputs");
    if(args[10].type!=NativeScriptType_F32||args[11].type!=NativeScriptType_F32)
        return error(d,"Floor island comparison bounds require Float outputs");
    StageCaller source={0};if(!capture(c,&source,d))return false;
    StageGeometry g=geometry();Vec3 left={0},right={0};
    mp_UnkStruct0 *first=NULL,*second=NULL;
    mp_UnkStruct0* islands=mpIsland_80458E88.next;
    float depth=args[6].as.f32;
    if(!isfinite(args[1].as.f32+depth))return error(d,"Floor island comparison endpoint overflowed");
    if(g.map&&g.lines&&g.vertices&&g.joints){
        Vec3 origin={args[0].as.f32,args[1].as.f32,args[2].as.f32};
        first=mpIsland_8005AC14(&origin,depth);
        if(!current(c,&source)||!geometry_current(&g)||mpIsland_80458E88.next!=islands)
            return error(d,"Floor island comparison source/geometry changed after its first probe");
        if(first&&args[7].as.u64){
            if(!isfinite(args[4].as.f32+depth))return error(d,"Other floor island endpoint overflowed");
            Vec3 other={args[3].as.f32,args[4].as.f32,args[5].as.f32};
            second=mpIsland_8005AC14(&other,depth);
            if(!current(c,&source)||!geometry_current(&g)||mpIsland_80458E88.next!=islands)
                return error(d,"Floor island comparison source/geometry changed after its second probe");
        }
        if(first){
            if(!island_bounds_admitted(&g,first,d))return false;
            mpIsland_8005ACE8(first,&left,&right);
        }
    }
    if(!current(c,&source)||!geometry_current(&g)||mpIsland_80458E88.next!=islands)
        return error(d,"Floor island comparison source/geometry changed before publication");
    args[8]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=first!=NULL};
    args[9]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=first&&first==second};
    args[10]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=left.x};
    args[11]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=right.x};
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

static bool follow_floor(NativeScriptRuntimeContext* c,NativeScriptValue* args,
    uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(!args||!output||count!=10||args[6].type!=NativeScriptType_Bool)
        return error(d,"Floor following requires a line, five Float inputs and Bool/XYZ outputs");
    int line;if(!line_index(args,&line,d))return false;
    for(unsigned i=1;i<10;++i)if(i!=6) {
        if(args[i].type!=NativeScriptType_F32)
            return error(d,"Floor following coordinates and distances require exact Float values");
        if(i<6&&!isfinite(args[i].as.f32))
            return error(d,"Floor following inputs must be finite");
    }
    if(args[5].as.f32<0)return error(d,"Floor following gap allowance must be nonnegative");
    StageCaller source={0};if(!capture(c,&source,d))return false;
    StageGeometry g=geometry();
    Vec3 origin={args[1].as.f32,args[2].as.f32,args[3].as.f32},point=origin;
    bool found=false;
    if(bounded_line(&g,line)&&g.vertices&&g.joints) {
        MapLine* description=g.lines[line].x0;
        if(!description||description->v0_idx>=g.vertex_count||description->v1_idx>=g.vertex_count)
            return error(d,"Floor following line lacks its native endpoint geometry");
        // The native traversal/projection owns connected-line selection. Its
        // failure point is deliberately discarded, matching the script recipe.
        found=mpLib_80056C54(line,&origin,NULL,&point,NULL,NULL,args[4].as.f32,args[5].as.f32);
        if(!found)point=origin;
    }
    if(!current(c,&source)||!geometry_current(&g))
        return error(d,"Floor following source or geometry changed before publication");
    args[6]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=found};
    args[7]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=point.x};
    args[8]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=point.y};
    args[9]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=point.z};
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

bool nativeScriptBindingStage_Invoke(NativeScriptRuntimeContext* c,
    uint32_t semantic, NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_StageEvent) {
        if(!arguments || count!=2 || !output ||
           arguments[1].type!=NativeScriptType_F32 || !isfinite(arguments[1].as.f32))
            return error(d,"Stage event requires an integral channel and finite Float payload");
        const NativeScriptValue* channel=&arguments[0];
        uint64_t bits;
        if(channel->type==NativeScriptType_I32 || channel->type==NativeScriptType_I64) {
            if(channel->as.i64<0 || (channel->type==NativeScriptType_I32 && channel->as.i64>INT32_MAX))
                return error(d,"Stage channel is outside its declared integral width");
            bits=(uint64_t)channel->as.i64;
        } else if(channel->type==NativeScriptType_U32 || channel->type==NativeScriptType_U64)
            bits=channel->as.u64;
        else return error(d,"Stage channel requires an integral value");
        if(bits>UINT32_MAX)return error(d,"Stage channel exceeds UInt32");
        StageCaller caller={0};
        if(!capture(c,&caller,d))return false;
        bool accepted=Stage_Event((uint32_t)bits,arguments[1].as.f32);
        if(!current(c,&caller))return error(d,"Stage event retired or changed its source");
        *output=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=accepted};
        return true;
    }
    if(semantic==NativeScriptBinding_StageIslandBounds)
        return island_bounds(c,arguments,count,output,d);
    if(semantic==NativeScriptBinding_StageFollowFloor)
        return follow_floor(c,arguments,count,output,d);
    if(semantic==NativeScriptBinding_StageCompareFloorIslands)
        return compare_floor_islands(c,arguments,count,output,d);
    if(semantic==NativeScriptBinding_StageReadFloor||semantic==NativeScriptBinding_StageReadLineIslandBounds||
       semantic==NativeScriptBinding_StageReadLineIslandBounds3D||semantic==NativeScriptBinding_StageSameLineIsland||
       semantic==NativeScriptBinding_StageCpuFloorExcluded||semantic==NativeScriptBinding_StageCpuIslandBlocked)
        return terrain_query(c,semantic,arguments,count,output,d);
    if (semantic == NativeScriptBinding_StageLineExists ||
        semantic == NativeScriptBinding_StageLineNormal)
        return line_query(c, semantic, arguments, count, output, d);
    bool sweep = semantic == NativeScriptBinding_StageSweepBox;
    if ((!sweep && semantic != NativeScriptBinding_StageSegmentBlocked) ||
        !output || !arguments || count != (sweep ? 13u : 4u))
        return error(d, "Stage query argument shape is invalid");
    float values[12];
    for (unsigned i = 0; i < (sweep ? 12u : 4u); ++i) {
        if (arguments[i].type != NativeScriptType_F32 ||
            !isfinite(arguments[i].as.f32))
            return error(d, "Stage query geometry requires finite F32 values");
        values[i] = arguments[i].as.f32;
    }
    uint32_t mode = 0;
    if (sweep && (!category(&arguments[12], &mode) || !sweep_arithmetic(values)))
        return error(d, "Stage sweep category or native subdivision is invalid");
    // Native categories 1/2/3 identify real Fighter/Item contacts. Registered
    // Yorster callbacks dereference their GObj; this independent probe has no
    // actor. Preserve the native callbacks and reject that unsupported usage.
    if (sweep && mode >= 1 && mode <= 3)
        return error(d, "Stage sweep actor-contact categories require an actor and are unsupported");
    StageCaller source = {0};
    if (!capture(c, &source, d)) return false;
    if (sweep && source.kind != NativeCallbackSource_Fighter)
        return error(d, "Stage sweep requires a Fighter source");
    NativeScriptValue result = {0};
    if (sweep) {
        CollData probe = {0};
        ftCollisionBox box = {values[6], values[7],
            {values[8], values[9]}, {values[10], values[11]}};
        mpColl_80041EE4(&probe);
        probe.last_pos = probe.cur_pos;
        probe.cur_pos = (Vec3) {values[0], values[1], values[2]};
        probe.last_pos = probe.cur_pos;
        probe.cur_pos = (Vec3) {values[3], values[4], values[5]};
        probe.x34_flags.b1234 = mode;
        (void) mpColl_8004730C(&probe, &box);
        int32_t flags;
        memcpy(&flags, &probe.env_flags, sizeof(flags));
        result.type = NativeScriptType_I32;
        result.as.i64 = flags;
    } else {
        result.type = NativeScriptType_Bool;
        result.as.u64 = mpCheckAllRemap(NULL, NULL, NULL, NULL, -1, -1,
            values[0], values[1], values[2], values[3]);
    }
    if (!current(c, &source))
        return error(d, "Stage query source retired or changed during collision");
    *output = result;
    return true;
}
