// The old native GetRotZ name actually read Y. Compare against that pinned
// implementation, not a stub which repeats the misleading name's assumption.
static void root_y_cases(HSD_GObj* object,bool quaternion)
{
 Fighter* fp=object->user_data;HSD_JObj* root=object->hsd_obj;
 HSD_JObj original=*root,copy=copies[0];
 CHECK(helper(object,"ReadRootY"));
 float expected=FrozenRootRotationY(fp,0);
 CHECK(presentation_state(object,"root_y",NULL).as.f32==expected);
 CHECK(expected!=(quaternion?copies[0].rotate.z:root->rotate.z));
 CHECK(helper(object,"TurnRootY"));
 CHECK(ftPartGetRotY(fp,0)==expected-0.25f);
 CHECK(root->rotate.z==original.rotate.z&&copies[0].rotate.z==copy.rotate.z);
 CHECK(copies[1].rotate.y==8);++comparisons;
 const uint32_t bits[]={0,0x80000000,0x3fc00000,0xbf800000,0x7f800000,0xff800000,0x7fc12345};
 for(unsigned i=0;i<sizeof(bits)/sizeof(*bits);++i){
  HSD_JObj* selected=quaternion?&copies[0]:root;
  memcpy(&selected->rotate.y,&bits[i],4);
  CHECK(helper(object,"ReadRootY"));
  float read=presentation_state(object,"root_y",NULL).as.f32;
  float native=FrozenRootRotationY(fp,0);
  CHECK(!memcmp(&read,&bits[i],4)&&!memcmp(&read,&native,4));
  CHECK(!memcmp(&read,&selected->rotate.y,4));++comparisons;
 }
 *root=original;copies[0]=copy;
 NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Fighter,
  .generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptDiagnostic diagnostic;
 for(unsigned fault=0;fault<5;++fault){
  NativeScriptRuntimeContext bad=context;HSD_JObj* saved=fp->parts[0].joint;
  uint32_t flags=copies[0].flags;
  if(fault==0)++bad.generation;
  if(fault==1)bad.source_class=NativeCallbackSource_Item;
  if(fault==2)bad.source=NULL;
  if(fault==3)fp->parts[0].joint=NULL;
  if(fault==4){root->flags|=JOBJ_USE_QUATERNION;copies[0].flags|=JOBJ_USE_QUATERNION;}
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingPresentation_Read(&bad,NativeScriptBinding_BonesRootRotationY,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
  fp->parts[0].joint=saved;copies[0].flags=flags;root->flags=original.flags;
 }
}
