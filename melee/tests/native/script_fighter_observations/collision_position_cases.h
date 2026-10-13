static void collision_position_cases(HSD_GObj* own,HSD_GObj* foreign,HSD_GObj* stock,HSD_GObj* caller)
{
 const uint32_t words[]={0,0x80000000,0x3f800000,0xc0200000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,1,0x80000001,0x7f7fffff};
 const char* fields[]={"collision_x","collision_y","collision_z"};HSD_GObj* targets[]={own,foreign,stock};
 for(unsigned target=0;target<3;++target)for(unsigned sample=0;sample<sizeof(words)/sizeof(*words);++sample) {
  Fighter* f=targets[target]->user_data;NativeScriptRuntimeContext c=context(targets[target]);
  memcpy(&f->coll_data.cur_pos.x,&words[sample],4);memcpy(&f->coll_data.cur_pos.y,&words[(sample+1)%12],4);
  memcpy(&f->coll_data.cur_pos.z,&words[(sample+2)%12],4);f->cur_pos=(Vec3){11,12,13};
  static Fighter before;before=*f;const Vec3* vector=&f->coll_data.cur_pos;
  for(unsigned axis=0;axis<3;++axis) {
   const float* native=axis==0?&vector->x:axis==1?&vector->y:&vector->z;NativeScriptValue out;
   CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_CollisionPositionX+axis,NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,native,4));++pairs;
  }
  if(!target) {
   CHECK(helper(own,"ObserveCollision"));
   for(unsigned axis=0;axis<3;++axis){const float* native=axis==0?&vector->x:axis==1?&vector->y:&vector->z;
    NativeScriptValue out=field(own,fields[axis]);CHECK(!memcmp(&out.as.f32,native,4));++pairs;}
  }
  CHECK(nativeScriptItem_SetAttachmentOwner(caller,targets[target]));surface_signal(caller,"ObserveOwnerCollision");++receiver_calls;
  for(unsigned axis=0;axis<3;++axis){const float* native=axis==0?&vector->x:axis==1?&vector->y:&vector->z;
   NativeScriptValue out=field(caller,fields[axis]);CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,native,4));++pairs;}
  CHECK(!memcmp(f,&before,sizeof(*f)));++cases;
 }
 Item* item=caller->user_data;NativeScriptRuntimeContext c=context(caller);
 for(unsigned sample=0;sample<sizeof(words)/sizeof(*words);++sample) {
  memcpy(&item->x378_itemColl.cur_pos.x,&words[sample],4);memcpy(&item->x378_itemColl.cur_pos.y,&words[(sample+1)%12],4);
  memcpy(&item->x378_itemColl.cur_pos.z,&words[(sample+2)%12],4);item->pos=(Vec3){21,22,23};
  static Item before;before=*item;const Vec3* vector=&item->x378_itemColl.cur_pos;
  for(unsigned axis=0;axis<3;++axis){NativeScriptValue out;const float* native=axis==0?&vector->x:axis==1?&vector->y:&vector->z;
   CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_CollisionPositionX+axis,NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,native,4));++pairs;}
  surface_signal(caller,"ObserveCollision");
  for(unsigned axis=0;axis<3;++axis){NativeScriptValue out=field(caller,fields[axis]);const float* native=axis==0?&vector->x:axis==1?&vector->y:&vector->z;
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,native,4));++pairs;}
  CHECK(!memcmp(item,&before,sizeof(*item)));++cases;
 }
 // Item wall/ceiling observations expose the actual cached collision normals,
 // including absent-contact caches and all raw Float payloads.
 const uint32_t normal_semantics[]={NativeScriptBinding_WallLeftNormalX,NativeScriptBinding_WallLeftNormalY,
  NativeScriptBinding_WallRightNormalX,NativeScriptBinding_WallRightNormalY,NativeScriptBinding_CeilingNormalX,NativeScriptBinding_CeilingNormalY};
 const char* normal_fields[]={"left_x","left_y","right_x","right_y","ceiling_x","ceiling_y"};
 float* normal_components[]={&item->x378_itemColl.left_facing_wall.normal.x,&item->x378_itemColl.left_facing_wall.normal.y,
  &item->x378_itemColl.right_facing_wall.normal.x,&item->x378_itemColl.right_facing_wall.normal.y,
  &item->x378_itemColl.ceiling.normal.x,&item->x378_itemColl.ceiling.normal.y};
 for(unsigned sample=0;sample<sizeof(words)/sizeof(*words);++sample) {
  for(unsigned component=0;component<6;++component)memcpy(normal_components[component],&words[(sample+component)%12],4);
  item->x378_itemColl.env_flags=sample%2?0:Collide_LeftWallMask|Collide_RightWallMask|Collide_CeilingMask;
  static Item before;before=*item;
  for(unsigned component=0;component<6;++component){NativeScriptValue out;
   CHECK(nativeScriptBindings_Read(&c,normal_semantics[component],NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,normal_components[component],4));++pairs;}
  surface_signal(caller,"ObserveNormals");
  for(unsigned component=0;component<6;++component){NativeScriptValue out=field(caller,normal_fields[component]);
   CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,normal_components[component],4));++pairs;}
  CHECK(!memcmp(item,&before,sizeof(*item)));++cases;
 }
 for(unsigned mode=0;mode<3;++mode)for(unsigned component=0;component<6;++component){
  NativeScriptRuntimeContext invalid=c;if(mode==0)++invalid.generation;if(mode==1)++invalid.scene_epoch;
  NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel;
  bool ok=mode==2?nativeScriptBindings_Write(&invalid,normal_semantics[component],NULL,0,&sentinel,&diagnostic):
   nativeScriptBindings_Read(&invalid,normal_semantics[component],NULL,0,&out,&diagnostic);
  CHECK(!ok&&out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 }
 // Physical receiver capture keeps the original target after attachment changes.
 NativeScriptReceiverScope scope={.pool=nativeScriptRuntime_Receivers()};NativeScriptValue token,out;
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,foreign));
 CHECK(nativeScriptReceiver_Push(scope.pool,&c,c.scene_epoch,&primary_bank.declarations,Ft_Kind_CustomFirst,
  nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&diagnostic));
 CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,true,false,&token,&diagnostic));
 ((Fighter*)foreign->user_data)->coll_data.cur_pos=(Vec3){31,32,33};((Fighter*)stock->user_data)->coll_data.cur_pos=(Vec3){41,42,43};
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,stock));
 for(unsigned axis=0;axis<3;++axis){
  CHECK(nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_CollisionPositionX+axis,&token,1,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_F32&&out.as.f32==31+(float)axis);++pairs;
 }
 CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&diagnostic));++cases;
 HSD_GObj* objects[]={own,foreign,stock,caller};
 for(unsigned actor_index=0;actor_index<4;++actor_index)for(unsigned mode=0;mode<3;++mode)for(unsigned axis=0;axis<3;++axis) {
  NativeScriptRuntimeContext invalid=context(objects[actor_index]);if(mode==0)++invalid.generation;if(mode==1)++invalid.scene_epoch;
  NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99};out=sentinel;
  bool ok=mode==2?nativeScriptBindings_Write(&invalid,NativeScriptBinding_CollisionPositionX+axis,NULL,0,&sentinel,&diagnostic):
   nativeScriptBindings_Read(&invalid,NativeScriptBinding_CollisionPositionX+axis,NULL,0,&out,&diagnostic);
  CHECK(!ok&&out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 }
}
