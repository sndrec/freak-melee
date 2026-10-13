static void bounds_arguments(NativeScriptValue* a)
{
 const float coordinates[]={0,10,0,-20};
 for(unsigned i=0;i<4;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=coordinates[i]};
 a[4]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
 for(unsigned i=5;i<9;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=100+i};
}
static void bounds_check(const NativeScriptValue* a,bool found,const Vec3* left,const Vec3* right)
{
 CHECK(a[4].type==NativeScriptType_Bool&&a[4].as.u64==found);
 const float coordinates[]={left->x,left->y,right->x,right->y};
 for(unsigned i=0;i<4;++i)CHECK(a[i+5].type==NativeScriptType_F32&&!memcmp(&a[i+5].as.f32,&coordinates[i],4));
}
static void island_bounds_cases(HSD_GObj* object)
{
 NativeScriptRuntimeContext c=context(object);NativeScriptValue a[9],out;NativeScriptDiagnostic d={0};
 for(unsigned found=0;found<2;++found)for(unsigned dynamic=0;dynamic<2;++dynamic)
  for(unsigned missing=0;missing<2;++missing){
   island_seed(found,0,dynamic);if(missing)mpIsland_80458E88.next=NULL;
   Vec3 origin={0,10,0},left=origin,right=origin;
   mp_UnkStruct0* island=reference_mpIsland_8005AC14(&origin,-20);
   if(island)reference_mpIsland_8005ACE8(island,&left,&right);
   unsigned probes=island_probes,left_calls=island_left_calls,right_calls=island_right_calls;
   island_seed(found,0,dynamic);if(missing)mpIsland_80458E88.next=NULL;bounds_arguments(a);
   static Fighter before;before=*(Fighter*)object->user_data;
   CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageIslandBounds,NULL,0,a,9,&out,&d));
   CHECK(out.type==NativeScriptType_Void);bounds_check(a,island!=NULL,&left,&right);
   CHECK(!memcmp(&before,object->user_data,sizeof(before)));
   CHECK(island_probes==probes&&island_left_calls==left_calls&&island_right_calls==right_calls);++cases;++pairs;
   // Actual compiled helper also proves expression evaluation once and writes
   // all five destinations on success or missing-island failure.
   island_seed(found,0,dynamic);if(missing)mpIsland_80458E88.next=NULL;
   NativeScriptValue zero={.type=NativeScriptType_I32,.as.i64=0};terrain_field(object,"evaluations",&zero);
   CHECK(helper(object,terrain_function("Island"),&out));
   CHECK(terrain_field(object,"evaluations",NULL).as.i64==1);
   CHECK(terrain_field(object,"terrain_found",NULL).as.u64==(island!=NULL));
   CHECK(terrain_field(object,"tx",NULL).as.f32==left.x&&terrain_field(object,"ty",NULL).as.f32==left.y);
   CHECK(terrain_field(object,"ux",NULL).as.f32==right.x&&terrain_field(object,"uy",NULL).as.f32==right.y);++cases;
  }
 island_seed(true,0,true);mpLib_804D64B4=NULL;bounds_arguments(a);
 a[0].as.f32=-0.0f;Vec3 origin={-0.0f,10,0};
 CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageIslandBounds,a,9,&out,&d));
 bounds_check(a,false,&origin,&origin);++cases;
 for(unsigned bad=0;bad<13;++bad){
  island_seed(true,0,true);bounds_arguments(a);NativeScriptRuntimeContext invalid=c;
  if(bad==0)a[0].type=NativeScriptType_F64;
  if(bad==1)a[3].as.f32=NAN;if(bad==2)a[2].as.f32=INFINITY;
  if(bad==3)a[4].type=NativeScriptType_I32;if(bad==4)a[5].type=NativeScriptType_I32;
  if(bad==5){a[1].as.f32=FLT_MAX;a[3].as.f32=FLT_MAX;}
  if(bad==6)terrain_islands[0].x28=1;
  if(bad==7)terrain_islands[0].x28=-1;
  if(bad==8)a[6].type=NativeScriptType_F64;
  if(bad==9)a[7].type=NativeScriptType_I32;
  if(bad==10)invalid.generation++;if(bad==11)invalid.scene_epoch++;
  if(bad==12)a[8].type=NativeScriptType_F64;
  NativeScriptValue before[9];memcpy(before,a,sizeof(before));out=sentinel();
  CHECK(!nativeScriptBindingStage_Invoke(&invalid,NativeScriptBinding_StageIslandBounds,a,9,&out,&d));
  CHECK(out.type==NativeScriptType_U64&&!memcmp(before,a,sizeof(before)));++negative_cases;++cases;
 }
 for(unsigned action=1;action<=4;++action){
  HSD_GObj* source=create();NativeScriptRuntimeContext live=context(source);
  island_seed(true,0,true);bounds_arguments(a);terrain_source=source;terrain_mutation=action;
  uint64_t identity=bank.storage_layout.identity;NativeScriptValue before[9];memcpy(before,a,sizeof(before));out=sentinel();
  CHECK(!nativeScriptBindingStage_Invoke(&live,NativeScriptBinding_StageIslandBounds,a,9,&out,&d));
  CHECK(island_probes==1&&!island_left_calls&&!island_right_calls);
  CHECK(!memcmp(a,before,sizeof(before))&&out.type==NativeScriptType_U64);
  bank.storage_layout.identity=identity;HSD_GObjFree(source);++negative_cases;++cases;
 }
 for(unsigned action=1;action<=2;++action){
  island_seed(true,0,true);bounds_arguments(a);island_bounds_mutation=action;
  NativeScriptValue before[9];memcpy(before,a,sizeof(before));out=sentinel();
  CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageIslandBounds,a,9,&out,&d));
  CHECK(!memcmp(a,before,sizeof(before))&&out.type==NativeScriptType_U64);++negative_cases;++cases;
 }
 NativeScriptValue zero={.type=NativeScriptType_I32,.as.i64=0};terrain_field(object,"evaluations",&zero);
 terrain_seed();
}
static void island_bounds_article_cases(HSD_GObj* object)
{
 for(unsigned found=0;found<2;++found)for(unsigned dynamic=0;dynamic<2;++dynamic){
  island_seed(found,0,dynamic);Vec3 origin={0,10,0},left=origin,right=origin;
  mp_UnkStruct0* island=reference_mpIsland_8005AC14(&origin,-20);
  if(island)reference_mpIsland_8005ACE8(island,&left,&right);
  island_seed(found,0,dynamic);NativeScriptValue out;
  CHECK(nativeScriptItem_RunSignal(object,terrain_key("Island"),0,&out)==NativeScriptVM_Ok);++vm_calls;
  CHECK(terrain_field(object,"terrain_found",NULL).as.u64==found);
  CHECK(terrain_field(object,"tx",NULL).as.f32==left.x&&terrain_field(object,"ty",NULL).as.f32==left.y);
  CHECK(terrain_field(object,"ux",NULL).as.f32==right.x&&terrain_field(object,"uy",NULL).as.f32==right.y);++cases;++pairs;
 }
 terrain_seed();
}
