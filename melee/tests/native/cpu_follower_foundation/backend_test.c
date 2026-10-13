#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#include <melee/mp/types.h>
#include <sysdolphin/baselib/gobj.h>
static unsigned checks,lookups,exclusions;static bool map_live,story,traffic,item_live;static float slope,intercept;static unsigned excluded_mask;static MapCollData collision;static mp_UnkStruct0 islands[2];static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;StageInfo stage_info;
#define CHECK(x) do{++checks;assert(x);}while(0)
MapCollData* mpLib_8004D164(void){return map_live?&collision:NULL;}
mp_UnkStruct0* mpIsland_8005AB54(int line){++lookups;CHECK(line>=0&&line<collision.line_count);return line==3?NULL:&islands[line<2?0:1];}
bool mpIsland_8005AC8C(mp_UnkStruct0* island){return story;}
s32 Ground_801C5794(void){return traffic;}
void Ground_801C4368(float* a,float* b){*a=slope;*b=intercept;}
bool it_8026C1B4(HSD_GObj* object){return item_live;}
bool grBigBlue_801EF844(int line){++exclusions;return excluded_mask&1;}
bool grInishie1_801FCAAC(int line){++exclusions;return excluded_mask&2;}
bool grCorneria_801E2D90(int line){++exclusions;return excluded_mask&4;}
bool grVenom_80206D10(int line){++exclusions;return excluded_mask&8;}
static Fighter target;static HSD_GObj object;static bool target_live;
HSD_GObj* ftLib_8008741C(u32 id){return target_live&&object.user_data&&id==(u32)target.x8_spawnNum?&object:NULL;}
#include "backend_bodies.h"
int main(void)
{
 collision.line_count=4;map_live=true;
 for(unsigned live=0;live<2;++live)for(int a=-4;a<10;++a)for(int b=-4;b<10;++b){map_live=live;lookups=0;bool valid=live&&a>=0&&a<4&&b>=0&&b<4;CHECK(stage_same_line_island(a,b)==(valid&&a!=3&&b!=3&&(a<2)==(b<2)));CHECK(valid||lookups==0);}
 for(unsigned live=0;live<2;++live)for(int line=-4;line<10;++line)for(unsigned mask=0;mask<16;++mask){map_live=live;excluded_mask=mask;lookups=exclusions=0;bool valid=live&&line>=0&&line<4;CHECK(stage_cpu_floor_excluded(line)==(valid&&mask));CHECK(valid||exclusions==0);CHECK(!stage_cpu_island_blocked(line));CHECK(valid||lookups==0);}
 map_live=true;Item item={0};HSD_GObj item_object={.user_data=&item};
 for(unsigned kind=0;kind<10;++kind)for(unsigned active=0;active<2;++active)for(unsigned has_item=0;has_item<2;++has_item)for(int stage=0;stage<40;++stage)for(unsigned flag=0;flag<4;++flag){item.kind=kind==0?It_Kind_BombHei:kind==1?It_Kind_G_Shell:kind==2?It_Kind_R_Shell:It_Kind_Heart;item.x378_itemColl.floor.index=0;heads[HSD_GOBJ_PLINK_ITEM]=has_item?&item_object:NULL;item_live=active;stage_info.grkind=stage;story=flag&1;traffic=flag&2;islands[0].x8.y=flag&1?4:6;islands[0].x14.y=flag&2?-2:8;slope=0;intercept=-.25f;CHECK(stock_island_blocked(&islands[0])==ftCo_CpuIslandBlocked(&islands[0]));CHECK(stage_cpu_island_blocked(0)==ftCo_CpuIslandBlocked(&islands[0]));}
 CHECK(!ftCo_CpuIslandBlocked(NULL));
 const u32 ids[]={0,1,0x1000001,0x80000001,0xffffffff};
 for(unsigned i=0;i<5;++i)for(unsigned live=0;live<2;++live)for(unsigned userdata=0;userdata<2;++userdata){target=(Fighter){0};target.x8_spawnNum=(s32)ids[i];target.pos_delta=(Vec3){3,-4,5};target.dmg.x1830_percent=160;target.facing_dir=-1;target.x221F_b3=true;target.coll_data.floor.index=123;target.cpu.x564=15;object.user_data=userdata?&target:NULL;target_live=live;bool found=ids[i]!=0&&live&&userdata;Vec3 output={101,102,103};CHECK(nativeFighterTarget_PositionDelta(ids[i],&output)==found);CHECK(output.x==(found?3:101)&&output.y==(found?-4:102)&&output.z==(found?5:103));CHECK(!nativeFighterTarget_PositionDelta(ids[i],NULL));CHECK(nativeFighterTarget_Damage(ids[i])==(found?160:0));CHECK(nativeFighterTarget_Facing(ids[i])==(found?-1:0));CHECK(nativeFighterTarget_Sleeping(ids[i])==found);CHECK(nativeFighterTarget_FloorLine(ids[i])==(found?123:-1));CHECK(nativeFighterTarget_CpuBodyHalfWidth(ids[i])==(found?15:0));}
 printf("PASS %u actual stage/hazard/target checks; stage dynamic responses and live registry controlled\n",checks);return 0;
}
