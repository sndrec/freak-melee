// Actual floor/island/hazard orchestration below uses controlled geometry,
// segment intersections, bounding state and dynamic stage response leaves.
#include <melee/gr/types.h>
#include <melee/gr/ground.h>
#include <melee/mp/mpisland.h>
#include <melee/it/ithitbox.h>
#include <melee/it/it_26B1.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
static MapCollData terrain_map;
static MapCollData replacement_terrain_map;
static MapCollData* mpLib_804D64B4=&terrain_map;
static MapLine terrain_descriptions[4];
static CollLine terrain_lines[4];
static CollVtx terrain_vertices[8];
static MapJoint terrain_joint_description;
static CollJoint terrain_joints[1];
static CollLine* groundCollLine=terrain_lines;
static CollVtx* groundCollVtx=terrain_vertices;
static CollJoint* groundCollJoint=terrain_joints;
static CollJoint* jointListStart=terrain_joints;
struct mpIsland_80458E88_t mpIsland_80458E88;
static mp_UnkStruct0 terrain_islands[2];
StageInfo stage_info;
static int terrain_joint_id=3;
static bool terrain_intersection[4],terrain_checked,terrain_traffic;
static float terrain_x[4],terrain_y[4],terrain_slope,terrain_intercept;
static unsigned terrain_last_line,terrain_floor_calls,terrain_mutation;
static HSD_GObj* terrain_source;
static bool island_compare_testing,island_first_exists,island_bounds_dynamic;
static unsigned island_second_mode,island_probes,island_left_calls,island_right_calls,island_bounds_mutation;
MapCollData* mpLib_8004D164(void){return mpLib_804D64B4;}
CollLine* mpGetGroundCollLine(void){return groundCollLine;}
CollVtx* mpGetGroundCollVtx(void){return terrain_vertices;}
CollJoint* mpGetGroundCollJoint(void){return groundCollJoint;}
Vec3* mpLineGetNormal(int line,Vec3* value){(void)line;(void)value;abort();}
bool mpCheckedBounding(void){return terrain_checked;}
void mpBoundingCheck2(float x0,float y0,float x1,float y1)
{bounds[0]=x0;bounds[1]=y0;bounds[2]=x1;bounds[3]=y1;++terrain_floor_calls;if(island_compare_testing)++island_probes;}
void mpFloorGetLeft(int line,Vec3* point)
{CHECK(island_compare_testing&&line==0);++island_left_calls;*point=island_bounds_dynamic?(Vec3){-19,7,1}:terrain_islands[0].x8;}
void mpFloorGetRight(int line,Vec3* point)
{CHECK(island_compare_testing&&line==1);++island_right_calls;*point=island_bounds_dynamic?(Vec3){31,9,2}:terrain_islands[0].x14;
 if(island_bounds_mutation){unsigned action=island_bounds_mutation;island_bounds_mutation=0;if(action==1)mpLib_804D64B4=NULL;if(action==2)mpIsland_80458E88.next=NULL;}}
void mpLib_8004ED5C(int line,float* x0,float* y0,float* x1,float* y1)
{
 CHECK(line>=0&&line<4);terrain_last_line=(unsigned)line;*x0=0;*x1=10;*y0=(float)line;*y1=line&1?*y0+1:*y0;
 unsigned action=terrain_mutation;terrain_mutation=0;
 if(action==1){replacement_terrain_map=terrain_map;mpLib_804D64B4=island_compare_testing?&replacement_terrain_map:NULL;}
 if(action==2)nativeScriptFighter_Detach(terrain_source);
 if(action==3)((Fighter*)terrain_source->user_data)->x8_spawnNum++;
 if(action==4)bank.storage_layout.identity++;
}
bool mpLineIntersection(float a0x,float a0y,float a1x,float a1y,float b0x,float b0y,float b1x,float b1y,float* x,float* y)
{(void)a0x;(void)a0y;(void)a1x;(void)a1y;(void)b0x;(void)b0y;(void)b1x;(void)b1y;*x=terrain_x[terrain_last_line];*y=terrain_y[terrain_last_line];
 if(island_compare_testing){if(island_probes==1)return island_first_exists&&terrain_last_line==0;return island_second_mode<2&&terrain_last_line==(island_second_mode?2u:0u);}
 return terrain_intersection[terrain_last_line];}
bool mpLineIntersectionH(float* x,float* y,float a0x,float a0y,float a1x,float b0x,float b0y,float b1x,float b1y)
{return mpLineIntersection(a0x,a0y,a1x,a0y,b0x,b0y,b1x,b1y,x,y);}
void PSVECNormalize(Vec3* input,Vec3* output)
{float length=sqrtf(input->x*input->x+input->y*input->y+input->z*input->z);*output=(Vec3){input->x/length,input->y/length,input->z/length};}
s32 Ground_801C5794(void){return terrain_traffic;}
void Ground_801C4368(float* slope,float* intercept){*slope=terrain_slope;*intercept=terrain_intercept;}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#define LINEID_CHECK(location,line) CHECK((line)>=0&&(line)<terrain_map.line_count)
#include "native_terrain.inc"
#undef LINEID_CHECK
#include "island_reference.inc"
#pragma clang diagnostic pop
static void terrain_seed(void)
{
 island_compare_testing=false;island_probes=island_left_calls=island_right_calls=island_bounds_mutation=0;
 mpLib_804D64B4=&terrain_map;groundCollLine=terrain_lines;groundCollJoint=terrain_joints;jointListStart=terrain_joints;
 terrain_map=(MapCollData){.line_count=4,.vert_count=8,.joint_count=1};
 terrain_joint_description=(MapJoint){.floor_start=0,.floor_count=3,.dynamic_start=3,.dynamic_count=1};
 terrain_joints[0]=(CollJoint){.inner=&terrain_joint_description};terrain_checked=false;
 for(unsigned i=0;i<4;++i){terrain_descriptions[i]=(MapLine){.next_id0=-1,.next_id1=-1,.lo_flags=i+5};terrain_lines[i]=(CollLine){.x0=&terrain_descriptions[i],.flags=CollLine_Floor|LINE_FLAG_ENABLED};terrain_intersection[i]=true;terrain_x[i]=i+1;terrain_y[i]=i+2;}
 terrain_descriptions[0].next_id0=1;
 terrain_islands[0]=(mp_UnkStruct0){.next=&terrain_islands[1],.x8={-7,4,-0.0f},.x14={13,6,8},.x24=0,.x26=1,.x28=0};
 terrain_islands[1]=(mp_UnkStruct0){.x8={-7,4,-0.0f},.x14={13,6,8},.x24=2,.x26=2,.x28=0};
 mpIsland_80458E88.next=&terrain_islands[0];stage_info.grkind=Gr_Kind_Battle;
 terrain_joint_id=3;terrain_traffic=false;terrain_slope=0;terrain_intercept=0;
}
