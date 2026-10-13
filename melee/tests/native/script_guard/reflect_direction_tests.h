static void reflect_direction_tests(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);
 const uint32_t bits[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc12345};
 for(unsigned i=0;i<sizeof(bits)/sizeof(*bits);++i){
  memcpy(&f->ReflectAttr.x1A2C_reflectHitDirection,&bits[i],4);Fighter before=*f;
  CHECK(helper(o,"ReflectDirection",NULL,0));
  float seen=state(o,"reflected_direction").as.f32;
  CHECK(!memcmp(&seen,&bits[i],4)&&!memcmp(&before,f,sizeof(before)));
  NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=0};
  CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_ReflectorHitDirection,&value,&diagnostic));
  CHECK(!memcmp(&before,f,sizeof(before)));++vm_calls;++pairs;++cases;
 }
 for(unsigned fault=0;fault<4;++fault){
  NativeScriptRuntimeContext bad=c;
  if(fault==0)++bad.generation;
  if(fault==1)bad.source=NULL;
  if(fault==2)++bad.scene_epoch;
  if(fault==3)bad.source_class=NativeCallbackSource_Item;
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingGuard_Read(&bad,NativeScriptBinding_ReflectorHitDirection,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 }
}
