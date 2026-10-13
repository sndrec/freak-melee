// Only dependency leaves are controlled. Four public planner kernels, their
// flag/latch phases, and authored grabbing dispatch remain extracted native C.
typedef struct PlannerPolicy {
    Fighter* target;Item* common_item;Item* special_item;
    int floor,excluded,game_count,command,grabbed,travel,candidate,defend,projectile,recovery,item_check;
    bool right,left;float random;unsigned retire_target;
} PlannerPolicy;
static PlannerPolicy policy;
static unsigned leaf_trace[128],leaf_count;
static void trace(unsigned value){CHECK(leaf_count<128);leaf_trace[leaf_count++]=value;}
bool mpCheckFloor(float ax,float ay,float bx,float by,float offset,Vec3* point,int* line,u32* flags,Vec3* normal,int skip,int joint,int only,bool(*filter)(Fighter_GObj*,int),Fighter_GObj* owner)
{
 (void)ax;(void)ay;(void)bx;(void)by;CHECK(offset==0&&skip==-1&&joint==-1&&only==-1&&!filter&&!owner);trace(1);
 *point=(Vec3){7,-11,0};*line=2;*flags=0;*normal=(Vec3){0,1,0};return policy.floor!=0;
}
static int ftCo_800A1B38_noinline(int line){CHECK(line==2);trace(2);return policy.excluded;}
int gm_8016C75C(HSD_GObj* o){(void)o;trace(3);return policy.game_count;}
float HSD_Randf(void){trace(4);return policy.random;}
void ftCo_800B4A78(Fighter* f){trace(5);f->cpu.x24++;}
static int ftCo_800A2C80(Fighter* f){(void)f;trace(6);return policy.grabbed;}
static int ftCo_800A5ACC(Fighter* f){(void)f;trace(7);return policy.travel;}
int ftCo_800BB9B4(Fighter* f){trace(8);f->cpu.xF8_b12=policy.travel;return policy.travel;}
bool ftCo_800B9CBC(Fighter* f)
{
 trace(9);if(policy.retire_target==1&&policy.target)policy.target->x8_spawnNum++;
 if(policy.retire_target==2&&policy.common_item)policy.common_item->x1C++;
 if(policy.retire_target==3){CHECK(nativeScriptFighter_View(f->gobj,f->x8_spawnNum,&(NativeScriptReceiverView){0}));CHECK(!helper(f->gobj,"Fault"));}
 return policy.candidate!=0;
}
bool ftCo_800B8A9C(Fighter* f){(void)f;trace(10);return policy.defend!=0;}
int ftCo_800B732C(Fighter* f){(void)f;trace(11);return policy.projectile;}
static bool ftCo_800A3710(Fighter* f){(void)f;trace(12);return policy.recovery!=0;}
static bool ftCo_ItemCheck(Item* item){(void)item;trace(13);return policy.item_check!=0;}
static bool ftCo_800A3554(Fighter* f,float value){(void)f;CHECK(value==0);trace(14);return policy.recovery!=0;}
static bool ftCo_800A5908(Item* item){(void)item;trace(15);return policy.item_check!=0;}
static Item* ftCo_800A5F4C(Fighter* f,ItemKind kind){(void)f;CHECK(kind==It_Kind_L_Gun_Ray);trace(16);return policy.common_item;}
static Item* ftCo_CpuFindSpecialItemTarget(Fighter* f){(void)f;trace(17);return policy.special_item;}
static Fighter* ftCo_800A4BEC(Fighter* f){trace(18);if(policy.target)f->cpu.x48=policy.target;f->cpu.xF9_b0=policy.target!=NULL;return policy.target;}
static void ftCo_800A20A0(Fighter* f){trace(19);f->cpu.xF8_b6=policy.target!=NULL;}
static void ftCo_800A80E4(Fighter* f){trace(20);f->cpu.x54=(Vec2){32,16};}
static bool ftCo_800A4038(Fighter* f,bool forward){trace(forward?21:22);if(policy.right){f->cpu.x54=(Vec2){50,10};return true;}return false;}
static bool ftCo_800A3908(Fighter* f,bool forward){trace(forward?23:24);if(policy.left){f->cpu.x54=(Vec2){-50,10};return true;}return false;}
static int ftCo_800A229C(Fighter* f,Vec3* point){(void)f;trace(25);*point=(Vec3){-17,19,0};return policy.command;}
static void ftCo_800AE7AC(Fighter* f,Vec3* point,int command){trace(26);f->cpu.x54=(Vec2){point->x,point->y};f->cpu.xA4=command;}
static Fighter* ftCo_800A5CE0(Fighter* f){(void)f;trace(27);return policy.target;}
static void ftCo_800A866C(Fighter* f){trace(28);f->cpu.x54=(Vec2){-7,6};}
static Fighter* ftCo_800A53DC(Fighter* f){(void)f;trace(29);return policy.target;}
static void ftCo_800A75DC(Fighter* f,Fighter* target){trace(30);if(target)f->cpu.x54=(Vec2){target->cur_pos.x,target->cur_pos.y};}
#include "planner.inc"
