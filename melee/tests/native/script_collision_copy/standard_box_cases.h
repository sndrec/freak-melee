static bool box_contact;static unsigned box_probes,box_falls;
static HSD_GObj* box_caller;static bool box_retire;
static ftCollisionBox observed_box;
bool mpColl_8004B3F0(CollData* coll,ftCollisionBox* box)
{
 ++box_probes;observed_box=*box;
 coll->cur_pos=(Vec3){9,8,7};
 return box_contact;
}
void ftCo_Fall_Enter(Fighter_GObj* object)
{
 CHECK(object==box_caller);++box_falls;
 if(box_retire)nativeScriptFighter_Detach(object);
}
static void standard_box_cases(HSD_GObj* caller,HSD_GObj* stock)
{
 Fighter* f=caller->user_data;box_caller=caller;
 NativeScriptValue args[6];
 for(unsigned i=0;i<6;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_F32};
 for(unsigned sample=0;sample<64;++sample)for(unsigned direction=0;direction<2;++direction) {
  float values[]={10+(float)sample,0.25f,-4-(float)sample,2,6+(float)sample,3};
  for(unsigned i=0;i<6;++i)args[i].as.f32=values[i];
  ftCollisionBox box={values[0],values[1],{values[2],values[3]},{values[4],values[5]}};
  f->facing_dir=direction?-1:1;f->cur_pos=(Vec3){1,2,3};f->coll_data.cur_pos=(Vec3){4,5,6};
  Fighter initial=*f;box_contact=(sample&1)!=0;box_retire=false;box_probes=box_falls=0;
  ft_800847D0(caller,&box);Fighter expected=*f;ftCollisionBox expected_box=observed_box;
  unsigned falls=box_falls;CHECK(box_probes==1);*f=initial;box_probes=box_falls=0;
  NativeScriptRuntimeContext c=context(caller);NativeScriptValue out=sentinel();
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionGroundStandardBox,NULL,0,args,6,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&box_probes==1&&box_falls==falls);
  CHECK(!memcmp(f,&expected,sizeof(expected))&&!memcmp(&observed_box,&expected_box,sizeof(expected_box)));++pairs;++cases;
  *f=initial;box_probes=box_falls=0;out=sentinel();
  CHECK(helper(caller,"StandardBox",args,6,&out)==NativeScriptVM_Ok);
  CHECK(out.type==NativeScriptType_Void&&box_probes==1&&box_falls==falls);
  CHECK(!memcmp(f,&expected,sizeof(expected))&&!memcmp(&observed_box,&expected_box,sizeof(expected_box)));++pairs;++cases;
 }
 NativeScriptRuntimeContext c=context(caller);
 for(unsigned invalid=0;invalid<10;++invalid) {
  NativeScriptRuntimeContext bad=c;NativeScriptValue values[6];memcpy(values,args,sizeof(values));
  if(invalid==0)bad=context(stock);
  if(invalid==1)++bad.generation;
  if(invalid==2)++bad.scene_epoch;
  if(invalid==3)bad.source_class=NativeCallbackSource_Item;
  if(invalid==4)bad.source=NULL;
  if(invalid==5)values[2].as.f32=NAN;
  if(invalid==6)values[4].as.f32=INFINITY;
  if(invalid==7)values[3].type=NativeScriptType_I32;
  NativeScriptValue out=sentinel(),old=out;Fighter before=*f;box_probes=box_falls=0;
  CHECK(!nativeScriptBindingCollision_Invoke(&bad,NativeScriptBinding_CollisionGroundStandardBox,
   invalid==8?NULL:values,invalid==9?5:6,&out,&diagnostic));
  CHECK(!memcmp(&out,&old,sizeof(out))&&!memcmp(f,&before,sizeof(before))&&!box_probes&&!box_falls);++cases;
 }
 // Fall entry may retire the authored source. Do not publish a successful
 // result after that callback, even though native position writes took place.
 box_contact=false;box_retire=true;box_probes=box_falls=0;
 NativeScriptValue out=sentinel(),old=out;
 CHECK(!nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionGroundStandardBox,args,6,&out,&diagnostic));
 CHECK(!memcmp(&out,&old,sizeof(out))&&box_probes==1&&box_falls==1);++cases;
 CHECK(nativeScriptFighter_Attach(caller));box_retire=false;
}
