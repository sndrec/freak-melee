#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
static char* generate(const char* source,bool expected, const NativeFighterScriptConstruction* construction)
{
 FILE* file=tmpfile();assert(file);NativeFighterScriptError error={0};++checks;
 bool ok=nativeFighterScript_GenerateC(source,strlen(source),"follow",NULL,NULL, construction,file,&error);
 if(ok!=expected)fprintf(stderr,"Unexpected %s at %u: %s\n%s",ok?"accept":"reject",error.line,error.message,source);
 assert(ok==expected);if(!ok){assert(error.line);fclose(file);return NULL;}
 fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);char* code=malloc(size+1);assert(code);
 assert(fread(code,1,size,file)==(size_t)size);code[size]=0;fclose(file);return code;
}
static const char fields[]=
 "state int target\nstate int after\nstate int line\nstate int axis\nstate int buttons\nstate int trigger\nstate int arg_order\nstate int invalidate\n"
 "state float x\nstate float y\nstate float z\nstate float facing\nstate bool result\nstate bool handled\nstate bool flag\n";
static const char helpers[]=
 "func InputId() -> int:\n    state.arg_order = state.arg_order + 1\n    if state.invalidate == int(1):\n        state.basic_landing()\n    return state.target\n"
 "func Coordinate() -> float:\n    state.arg_order = state.arg_order + 1\n    if state.invalidate == int(2):\n        state.basic_landing()\n    return 3\n";
static const char handlers[]=
 "on cpu_mode_update:\n    state.result = cpu.record_input(InputId())\n    state.result = cpu.read_history_pose(state.x, state.y, state.z, state.facing)\n    state.result = targets.read_position_delta(state.target, state.x, state.y, state.z)\n    state.result = stage.read_floor(Coordinate(), Coordinate(), Coordinate(), Coordinate(), state.x, state.y, state.line)\n    state.result = stage.read_line_island_bounds(state.line, state.x, state.y, state.z, state.facing)\n    state.result = stage.same_line_island(state.line, state.line)\n    state.result = stage.cpu_floor_excluded(state.line)\n    state.result = stage.cpu_island_blocked(state.line)\n    cpu.prepare_combat_plan()\n    state.result = cpu.prepare_destination_update()\n    cpu.update_recovery_destination()\n    state.result = cpu.plan_attack()\n    cpu.plan_without_target()\n    cpu.set_destination(Coordinate(), Coordinate(), Coordinate())\n    cpu.level = int(targets.damage(state.target) / 20)\n    cpu.combat_candidates_enabled = state.flag\n    cpu.ranged_gate_enabled = state.flag\n    state.after = state.after + 1\n    return state.handled\n"
 "on cpu_output:\n    state.result = cpu.apply_recorded_input()\n    cpu.output.left_x = state.axis\n    cpu.output.left_y = state.axis\n    cpu.output.buttons = state.buttons\n    cpu.output.left_trigger = state.trigger\n    cpu.output.right_trigger = state.trigger\n    state.facing = targets.facing(state.target)\n    state.result = targets.sleeping(state.target)\n    state.line = targets.floor_line(state.target)\n    state.x = targets.cpu_body_half_width(state.target)\n    state.flag = motion.allows_controller_sync\n    state.after = state.after + 1\n    return state.handled\n";
static void program(char* output,size_t capacity,unsigned route,const char* vars,const char* helper,const char* body)
{
 int n=snprintf(output,capacity,"%s%s%s%s",route?"":"",vars,helper,route?"move Follow:\n":"action Follow:\n");assert(n>=0&&(size_t)n<capacity);size_t used=n;
 while(*body){const char* end=strchr(body,'\n');size_t length=end?(size_t)(end-body)+1:strlen(body);n=snprintf(output+used,capacity-used,"    %.*s",(int)length,body);assert(n>=0&&(size_t)n<capacity-used);used+=n;body+=length;}
 n=snprintf(output+used,capacity-used,"%s",route?"    on state_341_physics:\n        pass\n":"    on load:\n        pass\n");assert(n>=0&&(size_t)n<capacity-used);
}
int main(int argc,char** argv)
{
 assert(argc==1||argc==3);char source[20000];
 for(unsigned route=0;route<2;++route){
  program(source,sizeof(source),route,fields,helpers,handlers);char* code=generate(source,true, NULL);
  assert(strstr(code,".on_cpu_mode_update = authored_")&&strstr(code,".on_cpu_output = authored_"));
  assert(strstr(code,"host->cpu_read_history_pose == 0")&&strstr(code,"host->stage_read_floor == 0")&&strstr(code,"host->callback_source_alive == 0"));
  if(argc==3){FILE* out=fopen(argv[route+1],"wb");assert(out);assert(fwrite(code,1,strlen(code),out)==strlen(code));fclose(out);}free(code);
  const char* invalid[]={
   "on cpu_mode_update:\n    return int(1)\n", "on cpu_output:\n    return 1.5\n",
   "on cpu_output:\n    cpu.record_input(1.5)\n    return true\n", "on cpu_output:\n    cpu.record_input(true)\n    return true\n",
   "on cpu_output:\n    cpu.apply_recorded_input(int(1))\n    return true\n",
   "on cpu_output:\n    cpu.ranged_gate_enabled = int(1)\n    return true\n",
   "on cpu_output:\n    cpu.output.left_x = 1.5\n    return true\n",
   "on cpu_output:\n    cpu.output.buttons = true\n    return true\n",
   "on cpu_output:\n    cpu.set_destination(true, 1, 2)\n    return true\n",
   "on cpu_output:\n    stage.same_line_island(1.5, int(2))\n    return true\n",
   "on cpu_output:\n    stage.cpu_floor_excluded(false)\n    return true\n",
   "on cpu_output:\n    state.result = cpu.read_history_pose(state.x, state.x, state.z, state.facing)\n    return true\n",
   "on cpu_output:\n    state.result = cpu.read_history_pose(state.x, state.y, state.z, state.line)\n    return true\n",
   "on cpu_output:\n    state.result = stage.read_floor(0, 1, 2, 3, state.x, state.y, state.z)\n    return true\n",
   "on cpu_output:\n    state.result = stage.read_line_island_bounds(state.line, state.x, state.y, state.x, state.facing)\n    return true\n",
   "on cpu_output:\n    state.result = targets.read_position_delta(false, state.x, state.y, state.z)\n    return true\n",
  };
  for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){program(source,sizeof(source),route,fields,"",invalid[i]);free(generate(source,false, NULL));}
  program(source,sizeof(source),route,"","","on cpu_output:\n    let float x = 0\n    let float y = 0\n    let float z = 0\n    let float facing = 0\n    let int line = int(0)\n    cpu.read_history_pose(x, y, z, facing)\n    stage.read_floor(0, 10, 0, -1000, x, y, line)\n    stage.read_line_island_bounds(line, x, y, z, facing)\n    return true\n");free(generate(source,true, NULL));
 }
 const char* rejected[]={"cpu.record_input(int(1))","cpu.read_history_pose(item.state.x, item.state.y, item.state.z, item.state.x)","stage.cpu_island_blocked(int(1))","cpu.output.left_x = int(1)"};
 for(unsigned i=0;i<4;++i){snprintf(source,sizeof(source),"article It_Kind_Ness_PKFire states 1:\n    state float x\n    state float y\n    state float z\n    state 0 anim -1:\n        on physics:\n            %s\naction Follow:\n    on load:\n        pass\n",rejected[i]);free(generate(source,false, NULL));}
 snprintf(source,sizeof(source),"model_effect FollowEffect 0x47c:\n    on update:\n        cpu.plan_attack()\naction Follow:\n    on load:\n        pass\n");free(generate(source,false, NULL));
 printf("PASS %u follower compiler programs, both routes\n",checks);return 0;
}
