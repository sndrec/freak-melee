#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifndef FOLLOWER_MODULE
#define FOLLOWER_MODULE "follower_action.inc"
#endif
#include FOLLOWER_MODULE
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
static void* pages;static Fighter* fp;static HSD_GObj* object;static const NativeFighterDefinition* definition;
static uint32_t generation;static bool live,found;static unsigned checks,retire_at,retire_mode,invocations,log_count;static int log_values[40];
#define CHECK(x) do{++checks;assert(x);}while(0)
static unsigned offset(const char* name){for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(name,definition->state_fields[i].name))return definition->state_fields[i].offset;assert(0);return 0;}
static void word(const char* name,int32_t value){memcpy(fp->native_module_state+offset(name),&value,4);}
static int32_t readword(const char* name){int32_t value;memcpy(&value,fp->native_module_state+offset(name),4);return value;}
static void real(const char* name,float value){memcpy(fp->native_module_state+offset(name),&value,4);}
static float readreal(const char* name){float value;memcpy(&value,fp->native_module_state+offset(name),4);return value;}
static void flag(const char* name,bool value){fp->native_module_state[offset(name)]=value;}
static void protect(unsigned mode){unsigned long old;CHECK(VirtualProtect(pages,65536,mode,&old));}
static bool alive(HSD_GObj* source,uint32_t id,uint32_t type){CHECK(source==object&&id==generation&&type==NativeCallbackSource_Fighter);return live&&fp->x8_spawnNum==generation;}
static void boundary(int label){CHECK(log_count<40);log_values[log_count++]=label;++invocations;if(invocations==retire_at){if(retire_mode==1){live=false;protect(1);}else ++fp->x8_spawnNum;}}
static bool record(HSD_GObj* source,uint32_t id){CHECK(source==object&&id==0x80000007);boundary(1);return found;}
static bool pose(HSD_GObj* source,Vec3* point,float* facing){CHECK(source==object);*point=(Vec3){10,20,30};*facing=-1;boundary(2);return found;}
static bool delta(uint32_t id,Vec3* point){CHECK(id==0x80000007);*point=(Vec3){11,21,31};boundary(3);return found;}
static bool floor_query(float x0,float y0,float x1,float y1,float yoff,Vec3* point,int* line,u32* flags,Vec3* normal,int skipline,int skipjoint,int onlyjoint,bool(*callback)(HSD_GObj*,int),HSD_GObj* source){CHECK(x0==3&&y0==3&&x1==3&&y1==3&&yoff==0&&skipline==-1&&skipjoint==-1&&onlyjoint==-1&&!callback&&!source);*point=(Vec3){12,22,32};*line=7;*flags=3;*normal=(Vec3){0,1,0};boundary(4);return found;}
static bool island(int line,Vec3* left,Vec3* right){CHECK(line==(found?7:5));*left=(Vec3){-40,-50,0};*right=(Vec3){40,50,0};boundary(5);return found;}
static bool same(int first,int second){CHECK(first==second&&first==(found?7:5));boundary(6);return found;}
static bool excluded(int line){CHECK(line==(found?7:5));boundary(7);return found;}
static bool blocked(int line){CHECK(line==(found?7:5));boundary(8);return found;}
static void prepare(Fighter* f){CHECK(f==fp);boundary(9);}
static bool prepare_destination(Fighter* f){CHECK(f==fp);boundary(10);return found;}
static void recovery(Fighter* f){CHECK(f==fp);boundary(11);}
static bool attack(Fighter* f){CHECK(f==fp);boundary(12);return found;}
static void no_target(Fighter* f){CHECK(f==fp);boundary(13);}
static void destination(Fighter* f,float x,float y,float spacing){CHECK(f==fp&&x==3&&y==3&&spacing==3);boundary(14);}
static float damage(uint32_t id){CHECK(id==0x80000007);return 160;}
static bool apply(HSD_GObj* source){CHECK(source==object);boundary(15);return found;}
static float facing(uint32_t id){CHECK(id==0x80000007);return -1;}
static bool sleeping(uint32_t id){CHECK(id==0x80000007);return found;}
static int line(uint32_t id){CHECK(id==0x80000007);return 13;}
static float width(uint32_t id){CHECK(id==0x80000007);return 15;}
static void landing(HSD_GObj* source){CHECK(source==object);boundary(16);}
static void reset(unsigned at,unsigned mode,bool success){protect(4);memset(pages,0,65536);object=pages;fp=(Fighter*)((char*)pages+4096);object->user_data=fp;fp->gobj=object;fp->x8_spawnNum=generation=0x80000123;live=true;found=success;retire_at=at;retire_mode=mode;invocations=log_count=0;word("target",(int32_t)0x80000007);word("line",5);word("axis",-128);word("buttons",(int32_t)0xffffffff);word("trigger",255);flag("handled",true);flag("flag",true);real("x",101);real("y",102);real("z",103);real("facing",104);}
int main(void)
{
 pages=VirtualAlloc(NULL,65536,0x3000,4);CHECK(pages);MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
 NativeFighterHost host={.callback_source_alive=alive,.cpu_record_input=record,.cpu_read_history_pose=pose,.target_position_delta=delta,.stage_read_floor=floor_query,.stage_line_island_bounds=island,.stage_same_line_island=same,.stage_cpu_floor_excluded=excluded,.stage_cpu_island_blocked=blocked,.cpu_prepare_combat_plan=prepare,.cpu_prepare_destination_update=prepare_destination,.cpu_update_recovery_destination=recovery,.cpu_plan_attack=attack,.cpu_plan_without_target=no_target,.cpu_set_destination=destination,.target_damage=damage,.cpu_apply_recorded_input=apply,.target_facing=facing,.target_sleeping=sleeping,.target_floor_line=line,.target_cpu_body_half_width=width,.enter_landing_basic=landing};
 definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->on_cpu_mode_update&&definition->on_cpu_output);
 #define MISSING(field) do{NativeFighterHost missing=host;missing.field=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&missing));definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);}while(0)
 MISSING(callback_source_alive);MISSING(cpu_record_input);MISSING(cpu_read_history_pose);MISSING(target_position_delta);MISSING(stage_read_floor);MISSING(stage_line_island_bounds);MISSING(stage_same_line_island);MISSING(stage_cpu_floor_excluded);MISSING(stage_cpu_island_blocked);MISSING(cpu_prepare_combat_plan);MISSING(cpu_prepare_destination_update);MISSING(cpu_update_recovery_destination);MISSING(cpu_plan_attack);MISSING(cpu_plan_without_target);MISSING(cpu_set_destination);MISSING(cpu_apply_recorded_input);
 for(unsigned success=0;success<2;++success){reset(0,0,success);CHECK(definition->on_cpu_mode_update(object));CHECK(readword("after")==1&&readword("arg_order")==8&&fp->cpu.level==8&&fp->cpu.xF9_b2&&fp->cpu.xF9_b4);CHECK(readreal("x")== (success?-40:101)&&readreal("y")== (success?-50:102)&&readreal("z")== (success?40:103)&&readreal("facing")== (success?50:104));for(unsigned i=0;i<14;++i)CHECK(log_values[i]==(int)i+1);CHECK(definition->on_cpu_output(object));CHECK(readword("after")==2&&fp->cpu.lstick.x==-128&&fp->cpu.lstick.y==-128&&fp->cpu.buttons==0xffffffff&&fp->cpu.ltrigger==255&&fp->cpu.rtrigger==255&&readword("line")==13&&readreal("x")==15&&readreal("facing")==-1);}
 for(unsigned success=0;success<2;++success)for(unsigned mode=1;mode<=2;++mode)for(unsigned at=1;at<=14;++at){reset(at,mode,success);CHECK(!definition->on_cpu_mode_update(object));protect(4);CHECK(invocations==at&&readword("after")==0);}
 for(unsigned mode=1;mode<=2;++mode){reset(1,mode,true);CHECK(!definition->on_cpu_output(object));protect(4);CHECK(invocations==1&&readword("after")==0&&fp->cpu.buttons==0);}
 for(unsigned helper=1;helper<=2;++helper)for(unsigned mode=1;mode<=2;++mode){unsigned at=helper==1?1:4;reset(at,mode,true);word("invalidate",helper);CHECK(!definition->on_cpu_mode_update(object));protect(4);CHECK(log_values[at-1]==16&&invocations==at&&readword("after")==0&&readword("arg_order")==helper);}
 const int values[]={-129,-128,127,128,0,255,256,2147483647};
 for(unsigned i=0;i<8;++i){reset(0,0,true);word("axis",values[i]);word("trigger",values[i]);fp->cpu.lstick=(S8Vec2){9,10};fp->cpu.ltrigger=11;fp->cpu.rtrigger=12;CHECK(definition->on_cpu_output(object));CHECK(fp->cpu.lstick.x==(values[i]>=-128&&values[i]<=127?values[i]:9));CHECK(fp->cpu.ltrigger==(values[i]>=0&&values[i]<=255?values[i]:11));}
 printf("PASS %u generated follower checks\n",checks);return 0;
}
