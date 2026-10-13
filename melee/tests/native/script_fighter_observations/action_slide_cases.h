#include "native_action_metadata.inc"
static void action_metadata_cases(HSD_GObj* owned,HSD_GObj* raw)
{
 const uint32_t id=NativeScriptBinding_ActionCurrentMetadata;
 for(unsigned actor_index=0;actor_index<2;++actor_index) {
  HSD_GObj* object=actor_index?raw:owned;Fighter* f=object->user_data;
  NativeScriptRuntimeContext c=context(object);
  for(unsigned sample=0;sample<512;++sample) {
   uint32_t bits=sample==0?0:sample==1?UINT32_MAX:sample*UINT32_C(2654435761);
   f->x2070=decode_action_metadata(bits);Fighter before=*f;
   int32_t expected;memcpy(&expected,&bits,sizeof(expected));
   NativeScriptValue out;
   CHECK(nativeScriptBindings_Read(&c,id,NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_I32&&out.as.i64==expected);
   CHECK(!nativeScriptBindings_Write(&c,id,NULL,0,&out,&diagnostic));
   CHECK(!memcmp(f,&before,sizeof(before)));++cases;++pairs;
   if(!actor_index) {
    CHECK(helper(object,"ReadCurrentMetadata"));out=field(object,"current_metadata");
    CHECK(out.type==NativeScriptType_I32&&out.as.i64==expected);
    out=field(object,"current_move");CHECK(out.type==NativeScriptType_I32&&out.as.i64==(bits&255));
   }
  }
 }
 NativeScriptRuntimeContext c=context(owned);
 for(unsigned mode=0;mode<5;++mode) {
  NativeScriptRuntimeContext bad=c;
  if(mode==0)++bad.generation;
  if(mode==1)++bad.scene_epoch;
  if(mode==2)bad.source_class=NativeCallbackSource_Item;
  if(mode==3)bad.source=NULL;
  NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owned,c.generation,&view));
  bool live=view.storage->live;if(mode==4)view.storage->live=false;
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123};
  CHECK(!nativeScriptBindings_Read(&bad,id,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==123);
  view.storage->live=live;++cases;
 }
}

static void slide_velocity_cases(HSD_GObj* caller,HSD_GObj* fighter)
{
 const uint32_t ids[]={NativeScriptBinding_ItemSlideVelocityX,NativeScriptBinding_ItemSlideVelocityY,
  NativeScriptBinding_ItemSlideVelocityZ};
 const char* fields[]={"slide_x","slide_y","slide_z"};
 Item* item=caller->user_data;NativeScriptRuntimeContext c=context(caller);
 for(unsigned sample=0;sample<64;++sample) {
  float values[]={(float)sample*0.125f-4,(float)sample*-0.5f,1+(float)sample*0.25f};
  item->x7C=(Vec3){values[0],values[1],values[2]};
  item->x40_vel=(Vec3){100,200,300};Item before=*item;
  for(unsigned axis=0;axis<3;++axis) {
   NativeScriptValue out;CHECK(nativeScriptBindings_Read(&c,ids[axis],NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,&values[axis],sizeof(float)));++pairs;
  }
  CHECK(!memcmp(item,&before,sizeof(before)));
  NativeScriptValue result;
  CHECK(nativeScriptItem_RunSignal(caller,name_key("ReadSlide"),0,&result)==NativeScriptVM_Ok);++vm_calls;
  for(unsigned axis=0;axis<3;++axis) {
   NativeScriptValue out=field(caller,fields[axis]);
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,&values[axis],sizeof(float)));
  }
  CHECK(nativeScriptItem_RunSignal(caller,name_key("WriteSlide"),0,&result)==NativeScriptVM_Ok);++vm_calls;
  Item expected=before;expected.x7C=(Vec3){values[0]+1,values[1]-2,values[2]*0.5f};
  CHECK(!memcmp(item,&expected,sizeof(expected)));++cases;++pairs;
 }
 NativeScriptRuntimeContext fighter_context=context(fighter);
 for(unsigned axis=0;axis<3;++axis) {
  for(unsigned mode=0;mode<5;++mode) {
   NativeScriptRuntimeContext bad=c;
   if(mode==0)bad=fighter_context;
   if(mode==1)++bad.generation;
   if(mode==2)++bad.scene_epoch;
   if(mode==3)bad.source=NULL;
   NativeScriptReceiverView view;CHECK(nativeScriptItem_View(caller,c.generation,&view));
   bool live=view.storage->live;if(mode==4)view.storage->live=false;
   NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123},in={.type=NativeScriptType_F32,.as.f32=42};
   Item before=*item;
   CHECK(!nativeScriptBindings_Read(&bad,ids[axis],NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_U64&&out.as.u64==123);
   CHECK(!nativeScriptBindings_Write(&bad,ids[axis],NULL,0,&in,&diagnostic));
   CHECK(!memcmp(item,&before,sizeof(before)));view.storage->live=live;++cases;
  }
 }
}
