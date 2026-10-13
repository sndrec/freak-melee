static void island_seed(bool found,unsigned second,bool dynamic)
{
 terrain_seed();island_compare_testing=true;island_first_exists=found;
 island_second_mode=second;island_bounds_dynamic=dynamic;
 terrain_islands[0].x4=0;terrain_islands[0].x6=1;
 terrain_descriptions[0].v0_idx=0;terrain_descriptions[0].v1_idx=2;
 terrain_descriptions[1].v0_idx=2;terrain_descriptions[1].v1_idx=1;
 terrain_descriptions[2].v0_idx=4;terrain_descriptions[2].v1_idx=5;
}
static void island_arguments(NativeScriptValue* a,bool compare)
{
 const float input[]={0,10,0,20,10,0,-20};
 for(unsigned i=0;i<7;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=input[i]};
 a[7]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=compare};
 a[8]=a[9]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
 a[10]=a[11]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=101};
}
static void island_compare_cases(HSD_GObj* object)
{
 NativeScriptRuntimeContext c=context(object);NativeScriptValue a[12],out;NativeScriptDiagnostic d;
 for(unsigned found=0;found<2;++found)for(unsigned compare=0;compare<2;++compare)
  for(unsigned second=0;second<3;++second)for(unsigned dynamic=0;dynamic<2;++dynamic){
   island_seed(found,second,dynamic);Vec3 point={0,10,0},other={20,10,0},left={0},right={0};
   mp_UnkStruct0* first=reference_mpIsland_8005AC14(&point,-20),*next=NULL;
   if(first&&compare)next=reference_mpIsland_8005AC14(&other,-20);
   if(first)reference_mpIsland_8005ACE8(first,&left,&right);
   unsigned probes=island_probes,left_calls=island_left_calls,right_calls=island_right_calls;
   bool same=first&&first==next;
   island_seed(found,second,dynamic);island_arguments(a,compare);
   CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageCompareFloorIslands,NULL,0,a,12,&out,&d));
   CHECK(out.type==NativeScriptType_Void&&a[8].as.u64==found&&a[9].as.u64==same);
   CHECK(!memcmp(&a[10].as.f32,&left.x,4)&&!memcmp(&a[11].as.f32,&right.x,4));
   CHECK(island_probes==probes&&island_left_calls==left_calls&&island_right_calls==right_calls);
   ++pairs;++cases;
  }
 // Real compiled source evaluates the first expression exactly once and stores
 // all four outputs even when the first island is absent.
 for(unsigned found=0;found<2;++found)for(unsigned second=0;second<3;++second){
  island_seed(found,second,true);
  NativeScriptValue zero={.type=NativeScriptType_I32,.as.i64=0};terrain_field(object,"evaluations",&zero);
  CHECK(helper(object,terrain_function("Compare"),&out));
  CHECK(terrain_field(object,"evaluations",NULL).as.i64==1);
  CHECK(terrain_field(object,"terrain_found",NULL).as.u64==found);
  CHECK(terrain_field(object,"terrain_same",NULL).as.u64==(found&&second==0));
  CHECK(terrain_field(object,"tx",NULL).as.f32==(found?-19:0));
  CHECK(terrain_field(object,"ux",NULL).as.f32==(found?31:0));++cases;
 }
 NativeScriptValue zero={.type=NativeScriptType_I32,.as.i64=0};terrain_field(object,"evaluations",&zero);
 // Invalid shapes, types, scalars and rig ranges preserve every argument/output.
 for(unsigned bad=0;bad<9;++bad){
  island_seed(true,0,true);island_arguments(a,true);
  if(bad==0)a[0].type=NativeScriptType_I32;
  if(bad==1)a[7].as.u64=2;if(bad==2)a[8].type=NativeScriptType_I32;
  if(bad==3)a[10].type=NativeScriptType_F64;if(bad==4)a[6].as.f32=NAN;
  if(bad==5){a[1].as.f32=FLT_MAX;a[6].as.f32=FLT_MAX;}
  if(bad==6)terrain_islands[0].x28=2;
  if(bad==7)a[3].as.f32=INFINITY;
  if(bad==8)a[9].type=NativeScriptType_I32;
  NativeScriptValue before[12];memcpy(before,a,sizeof(before));out=sentinel();
  bool rejected=!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageCompareFloorIslands,a,12,&out,&d);
  if(!rejected)fprintf(stderr,"island comparison accepted invalid case %u\n",bad);
  CHECK(rejected);
  CHECK(!memcmp(a,before,sizeof(before))&&out.type==NativeScriptType_U64);++negative_cases;++cases;
 }
 // Native geometry queries are read-only in-game; explicit observer mutations
 // exercise the surrounding source/schema/geometry publication guards.
 for(unsigned action=1;action<=4;++action){
  HSD_GObj* source=create();NativeScriptRuntimeContext live=context(source);
  island_seed(true,0,true);island_arguments(a,true);terrain_source=source;terrain_mutation=action;
  uint64_t identity=bank.storage_layout.identity;NativeScriptValue before[12];memcpy(before,a,sizeof(before));out=sentinel();
  CHECK(!nativeScriptBindingStage_Invoke(&live,NativeScriptBinding_StageCompareFloorIslands,a,12,&out,&d));
  CHECK(island_probes==1&&!island_left_calls&&!island_right_calls);
  CHECK(!memcmp(a,before,sizeof(before))&&out.type==NativeScriptType_U64);
  bank.storage_layout.identity=identity;HSD_GObjFree(source);++negative_cases;++cases;
 }
 for(unsigned action=1;action<=2;++action){
  island_seed(true,0,true);island_arguments(a,true);island_bounds_mutation=action;
  NativeScriptValue before[12];memcpy(before,a,sizeof(before));out=sentinel();
  CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageCompareFloorIslands,a,12,&out,&d));
  CHECK(!memcmp(a,before,sizeof(before))&&out.type==NativeScriptType_U64);++negative_cases;++cases;
 }
 terrain_seed();
}
