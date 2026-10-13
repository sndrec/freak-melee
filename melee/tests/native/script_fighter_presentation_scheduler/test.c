#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ftdemo.h>
#include <melee/ft/ft_0BEC.h>
#include <melee/ft/ft_0BF0.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftwaitanim.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemScrew.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/pl/types.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=1024,F=32,V=32};
static char source_bytes[8192];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[2048];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
NativeScriptCompileBank presentation_bank;static NativeScriptFighterProgram program;
unsigned presentation_checks;
void presentation_check(bool value,const char* text,unsigned line){++presentation_checks;if(!value){fprintf(stderr,"presentation scheduler %u: %s\n",line,text);exit(1);}}
static void prepare(bool hooks){
 presentation_bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},
 .declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=2048,
 .function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* base="fighter:\n    behavior independent\nstate int trace\nstate int demo_mode\nfunc Trace(int code):\n    state.trace = state.trace * 10 + code\n    position.x = float(state.trace)\naction Life:\n    on load:\n        Trace(1)\n    on death:\n        Trace(2)\n";
 const char* extra="action Presentation:\n    on color_animation_reset:\n        Trace(3)\n    on presentation_item:\n        Trace(4)\n    on taunt:\n        Trace(5)\n    on demo_enter:\n        state.demo_mode = event.mode\n        Trace(6)\n";
 const char* policies="state bool idle_enabled\nstate bool jump_sound_enabled\nstate int selected_motion\naction Policies:\n    on idle_variants:\n        Trace(7)\n        return state.idle_enabled\n    on motion_is_angled:\n        state.selected_motion = event.motion\n        Trace(8)\n        return event.motion >= 51 and event.motion <= 61 and event.motion != 53\n    on worn_item_jump_sound:\n        Trace(9)\n        return state.jump_sound_enabled\n";
 static char source[8192];snprintf(source,sizeof(source),"%s%s%s",base,hooks?extra:"",hooks?policies:"");NativeScriptDiagnostic diagnostic;const NativeScriptCompileBank* accepted=NULL;
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source)};
 if(!nativeScriptCompile_Build(&presentation_bank,&request,&accepted,&diagnostic)||!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,NULL,NULL,&program,&diagnostic)){fprintf(stderr,"prepare %u:%u %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&program);
}
static void reset_trace(HSD_GObj* object){NativeScriptValue zero={.type=NativeScriptType_I32};presentation_state(object,"trace",&zero);presentation_reset();}
static unsigned trace(HSD_GObj* object){return(unsigned)presentation_state(object,"trace",NULL).as.i64;}
#include "common_policy_cases.h"
static void colors(HSD_GObj* object){Fighter* fp=object->user_data;
 reset_trace(object);ftCo_800C0134(fp);CHECK(trace(object)==3&&presentation_reset_calls==1&&presentation_overlay_calls==1&&presentation_stock_calls==0);
 reset_trace(object);ftCo_800C0200(fp,0x6A);CHECK(trace(object)==3&&presentation_reset_calls==1&&presentation_overlay_calls==1&&presentation_stock_calls==0);
 for(unsigned branch=0;branch<2;++branch){reset_trace(object);presentation_overlay_branch=(int)branch;fp->x408.x28_colanim.i=branch;ftCo_800C0408(object);CHECK(trace(object)==3&&presentation_reset_calls==1&&presentation_overlay_calls==1&&presentation_stock_calls==0);CHECK(presentation_loop_calls>=2);}
}
static void presentations(HSD_GObj* object){void(*entries[])(HSD_GObj*)={ftCo_800BECB0,ftCo_800BED88,ftCo_800BEF04,ftCo_800BF034,ftCo_800BF108};
 for(unsigned i=0;i<sizeof(entries)/sizeof(*entries);++i){reset_trace(object);entries[i](object);CHECK(trace(object)==4&&presentation_common_motion==1&&presentation_stock_calls==0);Fighter* fp=object->user_data;CHECK(fp->x2219_b1&&fp->x2219_b2);}
}
static void retirements(void){
 for(unsigned mode=1;mode<=6;++mode){HSD_GObj* object=presentation_create(Ft_Kind_Mario,true);reset_trace(object);presentation_retire_mode=mode;presentation_retire_at=1;ftCo_800C0134(object->user_data);CHECK(presentation_overlay_calls==0&&presentation_stock_calls==0);presentation_retire_mode=0;
  // Metadata is fixture-owned and is restored before the next independent case.
  presentation_restore();
  if(nativeMatchPool_Reference(&nativeGObjPool,object,&(NativeMatchPoolReference){0}))HSD_GObjFree(object);
 }
 for(unsigned entry=0;entry<7;++entry)for(unsigned mode=7;mode<=8;++mode){HSD_GObj* object=presentation_create(Ft_Kind_Mario,true);reset_trace(object);NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));presentation_retire_mode=mode;presentation_retire_at=1;
  switch(entry){case 0:ftCo_800C0134(object->user_data);break;case 1:ftCo_800BECB0(object);break;case 2:ftCo_800BED88(object);break;case 3:ftCo_800BEF04(object);break;case 4:ftCo_800BF034(object);break;case 5:ftCo_800BF108(object);break;case 6:ftCo_800DEA28(object);break;default:abort();}
  CHECK(presentation_overlay_calls==0&&presentation_bookkeeping==0&&presentation_stock_calls==0);presentation_retire_mode=0;
  if(nativeMatchPool_Resolve(ref)){CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error);HSD_GObjFree(object);}else CHECK(mode==8);
 }
 // Constructor stops before presentation/demo/tail after Load faults or retires.
 for(unsigned mode=7;mode<=8;++mode){presentation_reset();presentation_retire_mode=mode;presentation_retire_at=1;plAllocInfo2 input={.internal_id=Ft_Kind_Mario,.slot=0,.unk8=1};CHECK(ftDemo_CreateFighter(&input)==NULL);CHECK(presentation_common_motion==0&&presentation_demo_tail==0&&presentation_stock_calls==0);presentation_retire_mode=0;}
 // With Load and Death complete, a failed presentation event cannot deliver DemoEnter.
 presentation_reset();presentation_retire_mode=7;presentation_retire_at=3;plAllocInfo2 input={.internal_id=Ft_Kind_Mario,.slot=0,.unk8=1};CHECK(ftDemo_CreateFighter(&input)==NULL);CHECK(presentation_demo_tail==0&&presentation_stock_calls==0);presentation_retire_mode=0;
 for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];g;){HSD_GObj* next=g->next;HSD_GObjFree(g);g=next;}
}
int main(void){HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare(true);
 HSD_GObj* object=presentation_create(Ft_Kind_Mario,true);CHECK(trace(object)==1);common_policies(object);colors(object);presentations(object);
 reset_trace(object);ftCo_800DEA28(object);CHECK(trace(object)==5&&presentation_bookkeeping==1&&presentation_common_motion==0&&presentation_stock_calls==0);
 HSD_GObjFree(object);retirements();
 // Actual constructor must attach before presentation callbacks; reset changes
 // the native generation deliberately, then demo_enter uses that fresh life.
 presentation_reset();plAllocInfo2 input={.internal_id=Ft_Kind_Mario,.slot=0,.unk8=1};object=ftDemo_CreateFighter(&input);CHECK(object&&nativeScriptFighter_IsAttached(object));CHECK(trace(object)==1246&&presentation_demo_tail==1&&presentation_stock_calls==0);CHECK(presentation_state(object,"demo_mode",NULL).as.i64==1);HSD_GObjFree(object);
 prepare(false);object=presentation_create(Ft_Kind_Mario,true);((Fighter*)object->user_data)->behavior_kind=Ft_Kind_DrMario;reset_trace(object);ftCo_800DEA28(object);CHECK(trace(object)==0&&presentation_common_motion==1&&presentation_bookkeeping==1&&presentation_stock_calls==0);HSD_GObjFree(object);
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS Fighter presentation scheduler\",\"checks\":%u,\"color_sites\":4,\"presentation_entries\":5,\"common_policy_pairs\":%u,\"common_policy_retirements\":%u}\n",presentation_checks,common_policy_pairs,common_policy_retirements);return 0;
}
