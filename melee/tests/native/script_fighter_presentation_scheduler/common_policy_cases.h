// Compare actual shared decisions with the pre-integration native Mewtwo
// policies. Animation/physics/RNG leaves remain controlled by boundary.c.
static unsigned common_policy_pairs,common_policy_retirements;
void ftCo_800D74A4(Fighter_GObj*);
static void common_policies(HSD_GObj* object)
{
 Fighter* f=object->user_data;
 for(int motion=-2;motion<376;++motion){
  reset_trace(object);bool scripted=ft_80089914(object,motion);
  CHECK(trace(object)==8&&presentation_state(object,"selected_motion",NULL).as.i64==motion);
  f->kind=Ft_Kind_Mewtwo;CHECK(scripted==reference_ft_80089914(object,motion));f->kind=Ft_Kind_Mario;
  ++common_policy_pairs;
 }
 static struct Fighter_WaitAnimData animations[2];static u8 blends[2][2];
 WaitStruct choices[]={{1,100},{-1,0}};HSD_GObj held={0};
 f->x24=animations;f->x28=blends;f->x590=NULL;
 for(unsigned frames=0;frames<2;++frames)for(unsigned available=0;available<2;++available)for(unsigned allowed=0;allowed<2;++allowed){
  reset_trace(object);policy_frames=frames;policy_restarts=policy_wait_changes=policy_rng_calls=0;
  f->anim_id=0;f->item_gobj=&held;NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=allowed};
  presentation_state(object,"idle_enabled",&value);
  ftCo_8008A7A8(object,available?choices:NULL);
  unsigned restarted=policy_restarts,changed=policy_wait_changes,rng=policy_rng_calls;int animation=f->anim_id;
  CHECK(trace(object)==(!frames&&available?7u:0u));
  policy_restarts=policy_wait_changes=policy_rng_calls=0;f->anim_id=0;
  f->kind=allowed?Ft_Kind_Mewtwo:Ft_Kind_Mario;
  reference_ftCo_8008A7A8(object,available?choices:NULL);
  CHECK(restarted==policy_restarts&&changed==policy_wait_changes&&rng==policy_rng_calls&&animation==f->anim_id);
  f->kind=Ft_Kind_Mario;++common_policy_pairs;
 }
 f->item_gobj=NULL;policy_frames=false;
 static struct Fighter_x2D0_t jump;
 jump.x0=5;jump.x2C=ftCo_MS_JumpAerialF;jump.x14[0]=2;
 for(unsigned sound=0;sound<2;++sound){
  reset_trace(object);NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=sound};
  presentation_state(object,"jump_sound_enabled",&value);policy_jumps=0;Vec3 speed={1,2,3};
  ft_800D2E7C(object,&speed);CHECK(trace(object)==9&&policy_jumps==1&&policy_jump_sound==sound);
  f->kind=sound?Ft_Kind_Mario:Ft_Kind_Mewtwo;Vec3 reference_speed={1,2,3};policy_jumps=0;
  reference_ft_800D2E7C(object,&reference_speed);
  CHECK(policy_jumps==1&&policy_jump_sound==sound&&!memcmp(&speed,&reference_speed,sizeof(speed)));
  f->kind=Ft_Kind_Mario;++common_policy_pairs;
 }
 policy_multi=true;f->x2D0=&jump;f->x1968_jumpsUsed=1;
 for(unsigned sound=0;sound<2;++sound){
  reset_trace(object);NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=sound};
  presentation_state(object,"jump_sound_enabled",&value);policy_jumps=policy_jump_tail=0;
  ftCo_800D74A4(object);CHECK(trace(object)==9&&policy_jumps==1&&policy_jump_sound==sound&&policy_jump_tail==1);
 }
 policy_multi=false;
 // Each scheduler must stop after the actual interpreter's native write leaf
 // faults, retires/replaces its actor, or changes program/storage dependencies.
 for(unsigned hook=0;hook<4;++hook)for(unsigned mode=1;mode<=8;++mode){
  HSD_GObj* g=presentation_create(Ft_Kind_Mario,true);NativeMatchPoolReference reference;
  CHECK(nativeMatchPool_Reference(&nativeGObjPool,g,&reference));
  Fighter* actor=g->user_data;actor->anim_id=0;actor->x24=animations;actor->x28=blends;
  reset_trace(g);presentation_retire_mode=mode;presentation_retire_at=1;
  policy_restarts=policy_wait_changes=policy_rng_calls=policy_jumps=policy_jump_tail=0;
  if(hook==0)ftCo_8008A7A8(g,choices);
  else if(hook==1)CHECK(!ft_80089914(g,51));
  else if(hook==2){Vec3 speed={1,2,3};ft_800D2E7C(g,&speed);}
  else{policy_multi=true;actor->x2D0=&jump;actor->x1968_jumpsUsed=1;ftCo_800D74A4(g);policy_multi=false;}
  CHECK(!policy_restarts&&!policy_wait_changes&&!policy_rng_calls&&!policy_jumps&&!policy_jump_tail);
  CHECK(presentation_callback_writes==1);
  if(mode==7)CHECK(nativeScriptFighter_Status(g)==NativeScriptVM_Error);
  presentation_retire_mode=0;presentation_restore();
  if(nativeMatchPool_Resolve(reference))HSD_GObjFree(g);
  else CHECK(mode==8);
  ++common_policy_retirements;
 }
}
