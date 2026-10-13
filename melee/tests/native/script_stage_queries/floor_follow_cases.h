// Native projection/traversal is extracted from mplib.c, not approximated by
// this fixture. Compare the binding's output recipe with a direct native call.
// Stage topology is controlled and valid; this is not a malformed-map audit.
static void follow_seed(unsigned shape)
{
 terrain_seed();
 for(unsigned i=0;i<4;++i){
  terrain_descriptions[i]=(MapLine){.v0_idx=i*2,.v1_idx=i*2+1,
   .next_id0=i==3?-1:(int)i+1,.next_id1=-1,
   .prev_id0=i?(int)i-1:-1,.prev_id1=-1};
  terrain_vertices[i*2].pos=(Vec2){i*10,(shape==1?i*3:0)};
  terrain_vertices[i*2+1].pos=(Vec2){(i+1)*10,(shape==1?(i+1)*3:0)};
 }
 if(shape==2)terrain_lines[1].flags=LINE_FLAG_ENABLED|CollLine_LeftWall;
 if(shape==3)terrain_lines[1].flags=LINE_FLAG_ENABLED|CollLine_Ceiling;
}
static void follow_arguments(NativeScriptValue* a,int line,float x,float distance,float gap)
{
 a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=line};
 const float v[]={x,7,-0.0F,distance,gap};
 for(unsigned i=0;i<5;++i)a[i+1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=v[i]};
 a[6]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
 for(unsigned i=7;i<10;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=100+i};
}
static void follow_expected(NativeScriptValue* a,bool* found,Vec3* expected)
{
 Vec3 origin={a[1].as.f32,a[2].as.f32,a[3].as.f32};*expected=origin;
 *found=mpLib_80056C54((int)a[0].as.i64,&origin,NULL,expected,NULL,NULL,a[4].as.f32,a[5].as.f32);
 if(!*found)*expected=origin;
}
static void follow_check(const NativeScriptValue* a,bool found,const Vec3* point)
{
 CHECK(a[6].type==NativeScriptType_Bool&&a[6].as.u64==found);
 CHECK(a[7].type==NativeScriptType_F32&&a[8].type==NativeScriptType_F32&&a[9].type==NativeScriptType_F32);
 CHECK(!memcmp(&a[7].as.f32,&point->x,4)&&!memcmp(&a[8].as.f32,&point->y,4)&&!memcmp(&a[9].as.f32,&point->z,4));
}
static void floor_follow_cases(HSD_GObj* object)
{
 NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};NativeScriptValue a[10],out;
 const float distances[]={-45,-25,-10,-1,-0.0F,0,1,10,25,45};
 for(unsigned shape=0;shape<4;++shape)for(unsigned line=0;line<4;++line)
  for(unsigned i=0;i<10;++i)for(unsigned gap=0;gap<2;++gap){
   follow_seed(shape);follow_arguments(a,line,line*10+5,distances[i],gap?20:0);
   bool found;Vec3 point;follow_expected(a,&found,&point);
   static Fighter snapshot;snapshot=*(Fighter*)object->user_data;
   CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageFollowFloor,NULL,0,a,10,&out,&d));
   CHECK(out.type==NativeScriptType_Void);follow_check(a,found,&point);
   CHECK(!memcmp(&snapshot,object->user_data,sizeof(snapshot)));++cases;++pairs;
  }
 // Primary/secondary link selection, hidden/disabled secondary links, origin
 // projection outside the initial segment, and an exhausted floor chain.
 for(unsigned mode=0;mode<6;++mode){
  follow_seed(0);terrain_descriptions[0].next_id1=2;
  if(mode==0){terrain_vertices[4].pos.x=10;terrain_vertices[5].pos.x=20;}
  if(mode==1)terrain_lines[2].flags|=LINE_FLAG_HIDDEN;
  if(mode==2)terrain_lines[2].flags=0;
  follow_arguments(a,0,mode==3?15:mode==4?-5:5,mode==5?100:12,10);
  bool found;Vec3 point;follow_expected(a,&found,&point);
  CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageFollowFloor,a,10,&out,&d));
  follow_check(a,found,&point);++cases;++pairs;
 }
 // Full compiled frontend, output stores to persistent state, source admission.
 for(unsigned found=0;found<2;++found){
  follow_seed(0);if(!found)terrain_lines[0].flags=0;
  terrain_outputs(object,0);
  NativeScriptValue x={.type=NativeScriptType_F32,.as.f32=5};terrain_field(object,"tx",&x);
  follow_arguments(a,0,5,12,10);a[2].as.f32=102;a[3].as.f32=103;
  bool expected_found;Vec3 point;follow_expected(a,&expected_found,&point);
  CHECK(expected_found==found);CHECK(helper(object,terrain_function("Follow"),&out));
  CHECK(terrain_field(object,"terrain_found",NULL).as.u64==found);
  CHECK(terrain_field(object,"ux",NULL).as.f32==point.x&&terrain_field(object,"uy",NULL).as.f32==point.y&&terrain_field(object,"uz",NULL).as.f32==point.z);++cases;++pairs;
 }
 for(unsigned mode=0;mode<4;++mode){
  follow_seed(0);follow_arguments(a,mode==0?-1:mode==1?4:0,5,12,10);
  if(mode==2)mpLib_804D64B4=NULL;if(mode==3)terrain_lines[0].flags=0;
  Vec3 origin={5,7,-0.0F};CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageFollowFloor,a,10,&out,&d));
  follow_check(a,false,&origin);++cases;
 }
 for(unsigned mode=0;mode<12;++mode){
  follow_seed(0);follow_arguments(a,0,5,12,10);NativeScriptRuntimeContext bad=c;
  if(mode==0)a[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
  if(mode==1)a[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};
  if(mode==2)a[1].as.f32=NAN;if(mode==3)a[4].as.f32=INFINITY;
  if(mode==4)a[5].as.f32=-1;if(mode==5)a[6].type=NativeScriptType_I32;
  if(mode==6)a[7].type=NativeScriptType_F64;if(mode==7)a[2].type=NativeScriptType_I32;
  if(mode==8)terrain_lines[0].x0=NULL;if(mode==9)terrain_descriptions[0].v1_idx=8;
  if(mode==10)bad.generation++;if(mode==11)bad.scene_epoch++;
  NativeScriptValue before[10];memcpy(before,a,sizeof(before));out=sentinel();
  CHECK(!nativeScriptBindingStage_Invoke(&bad,NativeScriptBinding_StageFollowFloor,a,10,&out,&d));
  CHECK(out.type==NativeScriptType_U64&&!memcmp(before,a,sizeof(before)));++cases;++negative_cases;
 }
 terrain_seed();
}
static void floor_follow_article_cases(HSD_GObj* object)
{
 for(unsigned found=0;found<2;++found){
  follow_seed(0);if(!found)terrain_lines[0].flags=0;terrain_outputs(object,0);
  NativeScriptValue x={.type=NativeScriptType_F32,.as.f32=5};terrain_field(object,"tx",&x);
  NativeScriptValue a[10],out;follow_arguments(a,0,5,12,10);a[2].as.f32=102;a[3].as.f32=103;
  bool expected_found;Vec3 point;follow_expected(a,&expected_found,&point);CHECK(expected_found==found);
  CHECK(nativeScriptItem_RunSignal(object,terrain_key("Follow"),0,&out)==NativeScriptVM_Ok);++vm_calls;
  CHECK(terrain_field(object,"terrain_found",NULL).as.u64==found);
  CHECK(terrain_field(object,"ux",NULL).as.f32==point.x&&terrain_field(object,"uy",NULL).as.f32==point.y&&terrain_field(object,"uz",NULL).as.f32==point.z);++cases;++pairs;
 }
 terrain_seed();
}
