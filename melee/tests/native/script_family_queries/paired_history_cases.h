static HSD_GObj* create_member(unsigned member,uint32_t generation,bool attach)
{
 CHECK(allocated<95);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 NativeSourcePackage* p=member?&secondary_package:&package;Fighter* f=&fighters[allocated++];
 *f=(Fighter){.gobj=object,.kind=p->target,.behavior_kind=p->target,.ft_data=&p->assets.character.data,.x8_spawnNum=generation,.player_id=2};
 GObj_InitUserData(object,4,userdata_remove,f);if(attach)CHECK(nativeScriptFighter_Attach(object));return object;
}
static void select_pair(NativeFighterFamilyMode mode,HSD_GObj* first,HSD_GObj* second)
{
 families[0].metadata.mode=mode;families[0].metadata.member_count=2;slots[1].active=true;
 player_slots[2].player_character=ChKind_CustomFirst;player_slots[2].transformed[0]=0;player_slots[2].transformed[1]=1;
 player_slots[2].player_entity[0]=first;player_slots[2].player_entity[1]=second;
}
static void paired_queries(void)
{
 HSD_GObj* primary=create_member(0,UINT32_C(0x80000001),true),*secondary=create_member(1,UINT32_MAX,true);
 select_pair(NativeFighterFamily_AlternateForm,primary,secondary);HSD_GObj* objects[]={primary,secondary};
 for(unsigned swap=0;swap<2;++swap){if(swap)Player_SwapTransformedStates(2,0,1);
  for(unsigned dormant=0;dormant<2;++dormant){((Fighter*)secondary->user_data)->x221F_b3=dormant;
   ((Fighter*)primary->user_data)->x221F_b4=swap;((Fighter*)secondary->user_data)->x221F_b4=!swap;
   for(unsigned caller=0;caller<2;++caller)for(unsigned member=0;member<2;++member){NativeScriptRuntimeContext c=context(objects[caller]);NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=member},out;
    uint32_t expected=((Fighter*)objects[member]->user_data)->x8_spawnNum;
    CHECK(nativeFighterRuntime_MemberId(objects[caller],member)==expected);
    CHECK(invoke(&c,NativeScriptBinding_FamilyMemberId,&arg,&out));CHECK(out.type==NativeScriptType_I32&&(uint32_t)out.as.i64==expected);
    CHECK(helper(objects[caller],function_id(member?"Companion":"Primary"),&out));CHECK(out.type==NativeScriptType_I32&&(uint32_t)out.as.i64==expected);++pairs;++cases;
    CHECK(nativeFighterRuntime_CpuHistorySource(objects[caller])==0);
   }
  }
 }
 Fighter* target=secondary->user_data;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(secondary,target->x8_spawnNum,&view));
 NativeScriptStorageLayout* target_layout=(NativeScriptStorageLayout*)view.storage->layout;NativeScriptStorageParameters* target_parameters=(NativeScriptStorageParameters*)view.storage->parameters;
 NativeScriptStorageLayout changed_layout=*target_layout;NativeScriptStorageParameters changed_parameters=*target_parameters;
 for(unsigned mode=0;mode<9;++mode){
  if(mode==0)++target->x8_spawnNum;if(mode==1)target->player_id=3;if(mode==2)target->ft_data=&package.assets.character.data;
  if(mode==3)view.storage->layout=&changed_layout;if(mode==4)view.storage->parameters=&changed_parameters;
  if(mode==5)secondary_package.compiler.bank.valid=false;if(mode==6)slots[1].member_index=0;
  if(mode==7)slots[1].source=&package;if(mode==8)player_slots[2].player_character=CKind_Mario;
  CHECK(nativeFighterRuntime_MemberId(primary,1)==0);++negatives;++cases;
  target->x8_spawnNum=UINT32_MAX;target->player_id=2;target->ft_data=&secondary_package.assets.character.data;
  view.storage->layout=target_layout;view.storage->parameters=target_parameters;secondary_package.compiler.bank.valid=true;
  slots[1].member_index=1;slots[1].source=&secondary_package;player_slots[2].player_character=ChKind_CustomFirst;
 }
 player_slots[2].player_entity[1]=NULL;CHECK(nativeFighterRuntime_MemberId(secondary,0)==0);CHECK(nativeFighterRuntime_MemberId(primary,1)==0);++cases;
 HSD_GObjFree(secondary);HSD_GObj* recycled=create_member(1,73,true);CHECK(recycled==secondary);
 CHECK(nativeFighterRuntime_MemberId(primary,1)==0);player_slots[2].player_entity[1]=recycled;CHECK(nativeFighterRuntime_MemberId(primary,1)==73);++cases;
 player_slots[2].player_entity[0]=player_slots[2].player_entity[1]=NULL;HSD_GObjFree(primary);HSD_GObjFree(recycled);
}
static void constructor_history(void)
{
 const uint32_t ids[]={1,16777217,UINT32_C(0x80000001),UINT32_MAX};
 for(unsigned id=0;id<4;++id)for(unsigned donor_attached=0;donor_attached<2;++donor_attached){
  select_pair(NativeFighterFamily_SimultaneousCompanion,NULL,NULL);
  HSD_GObj* donor=create_member(0,ids[id],donor_attached),*constructing=create_member(1,64+id,true);Fighter* first=donor->user_data,*second=constructing->user_data;
  first->cur_pos=(Vec3){-12.5f,7.25f,-0.0f};first->facing_dir=-1;
  CHECK(nativeFighterRuntime_CpuHistorySource(donor)==0);CHECK(nativeFighterRuntime_CpuHistorySource(constructing)==0);
  player_slots[2].player_entity[0]=donor;
  CHECK(nativeFighterRuntime_MemberId(constructing,0)==0);CHECK(nativeFighterRuntime_CpuHistorySource(constructing)==ids[id]);++cases;
  // Native source Prepare is already attached, but Player has not published it.
  memset(&second->cpu,0xA5,sizeof(second->cpu));struct Fighter_x1A88_xFC_t expected[30];memcpy(expected,second->cpu.xFC,sizeof(expected));
  for(unsigned i=0;i<30;++i){expected[i].x0=0;expected[i].lstick=(S8Vec2){0,0};expected[i].x4=expected[i].x5=0;expected[i].cur_pos=first->cur_pos;expected[i].facing_dir=first->facing_dir;}
  second->co_attrs.gravity=2;second->co_attrs.jump_v_initial_velocity=4;second->co_attrs.air_jump_v_multiplier=1;
  ftCo_800A101C(second,7,9,3);++vm_calls;
  CHECK(!memcmp(expected,second->cpu.xFC,sizeof(expected)));CHECK(second->cpu.x444==&second->cpu.xFC[5]&&second->cpu.x448==&second->cpu.xFC[0]);
  CHECK(second->cpu.xC==7&&second->cpu.level==9&&second->cpu.x14==3&&second->cpu.x56C==3.5f);++pairs;
  NativeScriptReceiverView view;NativeScriptValue hint;CHECK(nativeScriptFighter_View(constructing,second->x8_spawnNum,&view));
  CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|1,NULL,0,&hint,&diagnostic));CHECK(hint.type==NativeScriptType_I32&&(uint32_t)hint.as.i64==ids[id]);
  player_slots[2].player_entity[1]=constructing;Player_SwapTransformedStates(2,0,1);CHECK(nativeFighterRuntime_CpuHistorySource(constructing)==ids[id]);++cases;
  player_slots[2].player_entity[0]=NULL;CHECK(nativeFighterRuntime_CpuHistorySource(constructing)==0);
  ftCo_800A101C(second,6,1,0);++vm_calls;
  for(unsigned i=0;i<30;++i){CHECK(second->cpu.xFC[i].cur_pos.x==0&&second->cpu.xFC[i].cur_pos.y==0&&second->cpu.xFC[i].cur_pos.z==0&&second->cpu.xFC[i].facing_dir==1);++pairs;}++cases;
  player_slots[2].player_entity[1]=NULL;HSD_GObjFree(donor);HSD_GObjFree(constructing);
 }
 select_pair(NativeFighterFamily_SimultaneousCompanion,NULL,NULL);
 HSD_GObj* first=create_member(0,412,true),*physical=create_member(1,413,false);player_slots[2].player_entity[0]=first;
 CHECK(nativeFighterRuntime_CpuHistorySource(physical)==412);CHECK(nativeFighterRuntime_MemberId(physical,0)==0);++cases;
 CHECK(nativeScriptFighter_Attach(physical));CHECK(nativeFighterRuntime_CpuHistorySource(physical)==412);++cases;
 player_slots[2].player_entity[0]=NULL;HSD_GObjFree(first);HSD_GObjFree(physical);
}
static void paired_history_cases(void)
{
 paired_queries();constructor_history();families[0].metadata.mode=NativeFighterFamily_Single;families[0].metadata.member_count=1;slots[1].active=false;
 player_slots[2].transformed[0]=0;player_slots[2].transformed[1]=1;
}
