// Read the physical actor's common attributes through production routing and
// the VM; no stock-kind lookup or imported special-attribute offsets.
static void locomotion_attribute_cases(HSD_GObj* owned,HSD_GObj* raw)
{
 const uint32_t ids[]={NativeScriptBinding_GroundWalkAccelerationMultiplier,
  NativeScriptBinding_GroundWalkMaxSpeed,NativeScriptBinding_AirGroundJumpMomentumMultiplier,
  NativeScriptBinding_AirJumpHorizontalMax};
 const char* fields[]={"walk_acceleration","walk_maximum","jump_momentum","jump_maximum"};
 Fighter* f=owned->user_data;NativeScriptRuntimeContext c=context(owned);
 for(unsigned sample=0;sample<32;++sample){
  float values[]={(float)sample*0.03125f,(float)sample*0.125f-1,
   (float)sample*0.0625f,(float)sample*0.25f};
  f->co_attrs.walk_accel_mul=values[0];f->co_attrs.walk_max_vel=values[1];
  f->co_attrs.ground_to_air_jump_momentum_multiplier=values[2];f->co_attrs.jump_h_max_velocity=values[3];
  Fighter before=*f;
  for(unsigned slot=0;slot<4;++slot){
   NativeScriptValue out;
   CHECK(nativeScriptBindings_Read(&c,ids[slot],NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_F32 && !memcmp(&out.as.f32,&values[slot],sizeof(float)));
   CHECK(!nativeScriptBindings_Write(&c,ids[slot],NULL,0,&out,&diagnostic));
   ++pairs;++cases;
  }
  CHECK(helper(owned,"ReadLocomotion"));
  for(unsigned slot=0;slot<4;++slot){
   NativeScriptValue out=field(owned,fields[slot]);
   CHECK(out.type==NativeScriptType_F32 && !memcmp(&out.as.f32,&values[slot],sizeof(float)));
  }
  CHECK(!memcmp(f,&before,sizeof(before)));
 }
 // Shared physical observations also support a live stock receiver. These
 // reads do not require script-owned state; stale owned actors still reject.
 Fighter* stock=raw->user_data;NativeScriptRuntimeContext stock_context=context(raw);
 stock->co_attrs.walk_accel_mul=0.125f;stock->co_attrs.walk_max_vel=1.75f;
 stock->co_attrs.ground_to_air_jump_momentum_multiplier=0.625f;stock->co_attrs.jump_h_max_velocity=2.5f;
 const float stock_values[]={0.125f,1.75f,0.625f,2.5f};
 for(unsigned slot=0;slot<4;++slot){
  NativeScriptValue out;
  CHECK(nativeScriptBindings_Read(&stock_context,ids[slot],NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_F32 && out.as.f32==stock_values[slot]);++cases;
 }
 for(unsigned invalid=1;invalid<6;++invalid)for(unsigned slot=0;slot<4;++slot){
  NativeScriptRuntimeContext bad=c;
  NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owned,c.generation,&view));
  bool live=view.storage->live;
  if(invalid==1)++bad.generation;
  if(invalid==2)++bad.scene_epoch;
  if(invalid==3)bad.source_class=NativeCallbackSource_Item;
  if(invalid==4)view.storage->live=false;
  if(invalid==5)bad.source=NULL;
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123};
  Fighter before=*f;
  CHECK(!nativeScriptBindings_Read(&bad,ids[slot],NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64 && out.as.u64==123);
  CHECK(!memcmp(f,&before,sizeof(before)));
  view.storage->live=live;++cases;
 }
}
