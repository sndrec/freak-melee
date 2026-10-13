static void terrain_hazard_remove(void* data){(void)data;}
static uint32_t terrain_key(const char* name)
{uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static uint32_t terrain_function(const char* name)
{for(unsigned i=0;i<bank.declarations.function_count;++i)if(!bank.declarations.functions[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[i].name,name))return i;CHECK(false);return 0;}
static NativeScriptValue terrain_field(HSD_GObj* object,const char* name,const NativeScriptValue* input)
{
 bool fighter=object->classifier==HSD_GOBJ_CLASS_FIGHTER;NativeScriptReceiverView view;
 uint32_t generation=fighter?((Fighter*)object->user_data)->x8_spawnNum:((Item*)object->user_data)->x1C;
 CHECK(fighter?nativeScriptFighter_View(object,generation,&view):nativeScriptItem_View(object,generation,&view));
 NativeScriptDiagnostic d={0};
 for(unsigned i=0;i<bank.declarations.value_count;++i)if(bank.declarations.values[i].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[i].name,name)){
  NativeScriptValue value;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&d));
  CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&d));return value;
 }CHECK(false);return(NativeScriptValue){0};
}
static void terrain_outputs(HSD_GObj* object,int line)
{
 const char* names[]={"tx","ty","tz","ux","uy","uz"};
 for(unsigned i=0;i<6;++i){NativeScriptValue v={.type=NativeScriptType_F32,.as.f32=101.0f+i};terrain_field(object,names[i],&v);}
 NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=line};terrain_field(object,"terrain_line",&v);
 if(object->classifier==HSD_GOBJ_CLASS_FIGHTER){v=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=10};terrain_field(object,"floor_y",&v);}
}
static void terrain_args(NativeScriptValue* args,int line,unsigned count)
{args[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=line};for(unsigned i=1;i<count;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=100.0f+i};}
static void terrain_article_cases(HSD_GObj* object)
{
 NativeScriptValue out;const char* names[]={"tx","ty","tz","ux","uy","uz"};
 for(unsigned mode=0;mode<4;++mode){terrain_seed();if(mode==1)terrain_lines[0].flags=0;if(mode==2)mpIsland_80458E88.next=NULL;
  terrain_outputs(object,mode==3?-1:0);CHECK(nativeScriptItem_RunSignal(object,terrain_key("Terrain"),0,&out)==NativeScriptVM_Ok);++vm_calls;
  bool found=mode==0;CHECK(terrain_field(object,"terrain_found",NULL).as.u64==found);float values[]={-7,4,-0.0f,13,6,8};
  for(unsigned i=0;i<6;++i){NativeScriptValue actual=terrain_field(object,names[i],NULL);float expected=found?values[i]:101.0f+i;CHECK(!memcmp(&actual.as.f32,&expected,4));}++cases;++pairs;
 }
 for(unsigned found=0;found<2;++found){terrain_seed();for(unsigned i=0;i<4;++i)terrain_intersection[i]=found;terrain_outputs(object,17);
  Vec3 expected={101,102,103},normal;int line=17;u32 flags;bool native_found=mpCheckFloor(0,10,0,-10,0,&expected,&line,&flags,&normal,-1,-1,-1,NULL,NULL);CHECK(native_found==found);
  CHECK(nativeScriptItem_RunSignal(object,terrain_key("Floor"),0,&out)==NativeScriptVM_Ok);++vm_calls;
  CHECK(terrain_field(object,"terrain_found",NULL).as.u64==found);CHECK(terrain_field(object,"tx",NULL).as.f32==expected.x&&terrain_field(object,"ty",NULL).as.f32==expected.y&&terrain_field(object,"terrain_line",NULL).as.i64==line);++cases;++pairs;
 }
 terrain_seed();
}
static void terrain_cases(HSD_GObj* object)
{
 NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};NativeScriptValue a[7],out;
 for(unsigned mask=0;mask<16;++mask)for(unsigned reverse=0;reverse<2;++reverse)for(unsigned checked=0;checked<2;++checked){
  terrain_seed();terrain_checked=checked;for(unsigned i=0;i<4;++i)terrain_intersection[i]=(mask>>i)&1;
  const float values[]={0,reverse?-10:10,0,reverse?10:-10};for(unsigned i=0;i<4;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=values[i]};
  a[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=101};a[5]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=102};a[6]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=17};NativeScriptValue before[7];memcpy(before,a,sizeof(a));
  Vec3 point={101,102,103},normal;int line=17;u32 flags;terrain_floor_calls=0;
  bool found=mpCheckFloor(values[0],values[1],values[2],values[3],0,&point,&line,&flags,&normal,-1,-1,-1,NULL,NULL);unsigned calls=terrain_floor_calls;float expected_bounds[4];memcpy(expected_bounds,bounds,sizeof(bounds));terrain_floor_calls=0;
  static Fighter snapshot;snapshot=*(Fighter*)object->user_data;
  CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageReadFloor,a,7,&out,&d)&&out.type==NativeScriptType_Bool&&out.as.u64==found);
  CHECK(terrain_floor_calls==calls&&!memcmp(bounds,expected_bounds,sizeof(bounds)));CHECK(!memcmp(object->user_data,&snapshot,sizeof(snapshot)));
  if(found)CHECK(!memcmp(&a[4].as.f32,&point.x,4)&&!memcmp(&a[5].as.f32,&point.y,4)&&a[6].as.i64==line);else CHECK(!memcmp(a,before,sizeof(a)));++cases;++pairs;
 }
 for(unsigned mode=0;mode<7;++mode){terrain_seed();terrain_outputs(object,17);bool found=true;
  if(mode==1){jointListStart=NULL;found=false;}if(mode==2){terrain_joints[0].flags=CollJoint_TooFar;found=false;}
  if(mode==3){for(unsigned i=0;i<4;++i)terrain_lines[i].flags=0;found=false;}if(mode==4){mpLib_804D64B4=NULL;found=false;}
  if(mode==5){for(unsigned i=0;i<4;++i)terrain_lines[i].flags|=LINE_FLAG_EMPTY;found=false;}
  if(mode==6){terrain_joint_description.floor_count=0;terrain_joint_description.dynamic_count=1;}
  Vec3 expected={101,102,103},normal;int line=17;u32 flags;bool native_found=mpLib_804D64B4?mpCheckFloor(0,10,0,-10,0,&expected,&line,&flags,&normal,-1,-1,-1,NULL,NULL):false;CHECK(native_found==found);
  CHECK(helper(object,terrain_function("Floor"),&out)&&out.type==NativeScriptType_Bool&&out.as.u64==found);
  CHECK(terrain_field(object,"tx",NULL).as.f32==expected.x&&terrain_field(object,"ty",NULL).as.f32==expected.y);CHECK(terrain_field(object,"terrain_line",NULL).as.i64==line);++cases;
 }
 terrain_seed();terrain_outputs(object,17);NativeScriptValue nonfinite={.type=NativeScriptType_F32,.as.f32=NAN};terrain_field(object,"floor_y",&nonfinite);CHECK(helper(object,terrain_function("Floor"),&out)&&!out.as.u64);CHECK(terrain_field(object,"tx",NULL).as.f32==101&&terrain_field(object,"ty",NULL).as.f32==102&&terrain_field(object,"terrain_line",NULL).as.i64==17);++cases;
 terrain_seed();for(unsigned i=0;i<4;++i){NativeScriptValue arguments[7];for(unsigned j=0;j<4;++j)arguments[j]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};arguments[i].as.f32=i&1?INFINITY:NAN;arguments[4]=arguments[5]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=100};arguments[6]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=12};NativeScriptValue before[7];memcpy(before,arguments,sizeof(before));unsigned calls=terrain_floor_calls;
  CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageReadFloor,arguments,7,&out,&d)&&!out.as.u64&&!memcmp(arguments,before,sizeof(before))&&terrain_floor_calls==calls);++cases;
 }
 for(int line=-2;line<6;++line)for(unsigned state=0;state<4;++state)for(unsigned dimensions=2;dimensions<=3;++dimensions){
  terrain_seed();if(state==1)mpLib_804D64B4=NULL;if(state==2)terrain_lines[0].flags=0;if(state==3)mpIsland_80458E88.next=NULL;
  unsigned count=dimensions==2?5:7;terrain_args(a,line,count);NativeScriptValue before[7];memcpy(before,a,sizeof(before));
  mp_UnkStruct0* island=mpLib_804D64B4&&line>=0&&line<4?mpIsland_8005AB54(line):NULL;
  CHECK(nativeScriptBindingStage_Invoke(&c,dimensions==2?NativeScriptBinding_StageReadLineIslandBounds:NativeScriptBinding_StageReadLineIslandBounds3D,a,count,&out,&d)&&out.as.u64==(island!=NULL));
  if(island){const float expected[]={island->x8.x,island->x8.y,island->x8.z,island->x14.x,island->x14.y,island->x14.z};for(unsigned i=1;i<count;++i){unsigned component=dimensions==3?i-1:i<3?i-1:i;CHECK(!memcmp(&a[i].as.f32,&expected[component],4));}}
  else CHECK(!memcmp(a,before,count*sizeof(*a)));++pairs;++cases;
  terrain_outputs(object,line);CHECK(helper(object,terrain_function(dimensions==2?"Bounds":"Bounds3D"),&out)&&out.as.u64==(island!=NULL));
  CHECK(terrain_field(object,"tx",NULL).as.f32==(island?island->x8.x:101)&&terrain_field(object,"ux",NULL).as.f32==(island?island->x14.x:104));++cases;
 }
 for(int first=-2;first<6;++first)for(int second=-2;second<6;++second){terrain_seed();a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=first};a[1]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=second};bool found=first>=0&&first<3&&second>=0&&second<3&&(first<2)==(second<2);CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageSameLineIsland,NULL,0,a,2,&out,&d)&&out.as.u64==found);++pairs;++cases;}
 const int stages[]={Gr_Kind_Battle,Gr_Kind_BigBlue,Gr_Kind_Inishie1,Gr_Kind_Corneria,Gr_Kind_Venom};
 for(unsigned stage=0;stage<5;++stage)for(int joint=0;joint<68;++joint){terrain_seed();stage_info.grkind=stages[stage];terrain_joint_id=joint;terrain_args(a,0,1);bool expected=grBigBlue_801EF844(0)||grInishie1_801FCAAC(0)||grCorneria_801E2D90(0)||grVenom_80206D10(0);CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageCpuFloorExcluded,NULL,0,a,1,&out,&d)&&out.as.u64==expected);++pairs;++cases;}
 HSD_GObj* item=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);static Item hazard;hazard=(Item){.entity=item,.x1C=700};GObj_InitUserData(item,6,terrain_hazard_remove,&hazard);
 const int hazard_stages[]={Gr_Kind_Battle,Gr_Kind_Story,Gr_Kind_Zebes,Gr_Kind_Onett};
 for(unsigned kind=0;kind<4;++kind)for(unsigned active=0;active<2;++active)for(unsigned stage=0;stage<4;++stage)for(unsigned flags=0;flags<4;++flags){
  terrain_seed();hazard.kind=kind==0?It_Kind_BombHei:kind==1?It_Kind_G_Shell:kind==2?It_Kind_R_Shell:It_Kind_Heart;hazard.x378_itemColl.floor.index=flags&1?2:0;for(unsigned i=0;i<4;++i)hazard.x5D4_hitboxes[i].hit.state=active?HitCapsule_Enabled:HitCapsule_Disabled;
  stage_info.grkind=hazard_stages[stage];terrain_joints[0].flags=flags&1?CollJoint_B8:0;terrain_traffic=flags&2;terrain_slope=0;terrain_intercept=flags&1?-1:1;
  terrain_args(a,0,1);bool expected=ftCo_CpuIslandBlocked(mpIsland_8005AB54(0));CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageCpuIslandBlocked,NULL,0,a,1,&out,&d)&&out.as.u64==expected);++pairs;++cases;
 }
 terrain_seed();hazard.kind=It_Kind_BombHei;hazard.x378_itemColl.floor.index=100;hazard.x5D4_hitboxes[0].hit.state=HitCapsule_Enabled;terrain_args(a,0,1);out=sentinel();CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageCpuIslandBlocked,a,1,&out,&d)&&out.type==NativeScriptType_U64);HSD_GObjFree(item);++negative_cases;
 const uint32_t predicates[]={NativeScriptBinding_StageSameLineIsland,NativeScriptBinding_StageCpuFloorExcluded,NativeScriptBinding_StageCpuIslandBlocked};
 for(unsigned q=0;q<3;++q)for(unsigned bad=0;bad<4;++bad){terrain_seed();terrain_args(a,-1,q?1:2);a[1]=a[0];if(bad==0)a[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};if(bad==1)a[0]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=INT64_MAX};if(bad==2)a[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};out=sentinel();bool accepted=nativeScriptBindingStage_Invoke(&c,predicates[q],a,q?1:2,&out,&d);CHECK(bad==3?accepted&&out.type==NativeScriptType_Bool&&!out.as.u64:!accepted&&out.type==NativeScriptType_U64);++negative_cases;++cases;}
 for(unsigned action=1;action<=4;++action){HSD_GObj* source=create();NativeScriptRuntimeContext live=context(source);terrain_seed();terrain_source=source;terrain_mutation=action;uint64_t identity=bank.storage_layout.identity;terrain_args(a,0,7);for(unsigned i=0;i<4;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=i==1?10:i==3?-10:0};a[6]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=17};NativeScriptValue before[7];memcpy(before,a,sizeof(before));out=sentinel();CHECK(!nativeScriptBindingStage_Invoke(&live,NativeScriptBinding_StageReadFloor,a,7,&out,&d)&&out.type==NativeScriptType_U64&&!memcmp(a,before,sizeof(before)));bank.storage_layout.identity=identity;HSD_GObjFree(source);++negative_cases;++cases;}
 terrain_seed();
}
