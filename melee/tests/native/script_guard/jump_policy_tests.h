static void jump_reference_destroy(void* data) { (void)data; }
static void jump_policy_tests(HSD_GObj* object)
{
 Fighter* f=object->user_data;Fighter base=*f;
 Fighter reference;HSD_GObj* ref=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(ref);
 GObj_InitUserData(ref,4,jump_reference_destroy,&reference);
 FighterBone saved_part=parts[43];HSD_JObj saved_joint=joints[43],saved_blend=joints[44];
 HSD_AObj animation={0},blend_animation={0};
 flow_testing=true;flow_mutation_step=0;
 for(unsigned variant=0;variant<64;++variant){
  *f=base;f->motion_id=ftCo_MS_JumpAerialF;
  f->mv.co.fall.smid=77;f->mv.co.fallaerial.x4=12.5f;
  f->x8A4_animBlendFrames=(variant&32)?2:0;
  parts[43].flags_b1=(variant&1)!=0;parts[43].flags_b0=(variant&2)!=0;parts[43].flags_b5=(variant&4)!=0;
  parts[43].x4_jobj2=&joints[44];animation.flags=(variant&8)?AOBJ_NO_ANIM:0;
  blend_animation.flags=(variant&8)?0:AOBJ_NO_ANIM;
  joints[43].aobj=(variant&16)?NULL:&animation;joints[44].aobj=(variant&16)?NULL:&blend_animation;
  reference=*f;reference.gobj=ref;flow_count=0;
  CHECK(helper(object,"StepJump",NULL,0));Fighter after=*f;
  unsigned events=flow_count;char trace[64];memcpy(trace,flow_trace,events);flow_count=0;
  jump_reference_ftCo_JumpAerial_Anim(ref);reference.gobj=object;
  CHECK(!memcmp(&after,&reference,sizeof(after)));
  CHECK(events==flow_count&&!memcmp(trace,flow_trace,events));
  CHECK(state(object,"fall_selector").as.i64==(after.motion_id==ftCo_MS_FallAerial?after.mv.co.fall.smid:-1));
  CHECK(nativeScriptRuntime_Idle());++pairs;++cases;
 }
 const int32_t selectors[]={INT32_MIN,-1,0,1,35,INT32_MAX};
 const int motions_to_check[]={ftCo_MS_Fall,ftCo_MS_FallAerial,ftCo_MS_JumpAerialF,ftCo_MS_Wait};
 for(unsigned motion=0;motion<4;++motion)for(unsigned n=0;n<sizeof(selectors)/sizeof(*selectors);++n){
  *f=base;f->motion_id=motions_to_check[motion];f->mv.co.fall.smid=77;
  Fighter expected=*f;NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=selectors[n]};
  CHECK(helper(object,"WriteFall",&arg,1));
  if(motion<2)expected.mv.co.fall.smid=(FtMotionId)selectors[n];
  CHECK(!memcmp(f,&expected,sizeof(expected)));CHECK(helper(object,"ReadFall",NULL,0));
  CHECK(state(object,"fall_selector").as.i64==(motion<2?selectors[n]:-1));++pairs;++cases;
 }
 for(unsigned reverse=0;reverse<2;++reverse){
  *f=base;f->x2226_b1=!reverse;Fighter expected=*f;expected.x2226_b1=reverse!=0;
  NativeScriptValue arg={.type=NativeScriptType_Bool,.as.u64=reverse};
  CHECK(helper(object,"ReversePose",&arg,1));CHECK(state(object,"reverse_pose").as.u64==reverse);
  CHECK(!memcmp(f,&expected,sizeof(expected)));++pairs;++cases;
 }
 NativeScriptRuntimeContext c=context(object);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
 NativeScriptValue invalid={.type=NativeScriptType_I32,.as.i64=INT64_C(2147483648)};
 Fighter before=*f;
 CHECK(!nativeScriptBindingJump_Write(&c,NativeScriptBinding_MotionFallAnimation,&invalid,&diagnostic));
 invalid=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2};
 CHECK(!nativeScriptBindingCombat_Write(&c,NativeScriptBinding_DamageReverseKnockdownPose,&invalid,&diagnostic));
 CHECK(!nativeScriptBindingJump_Invoke(&c,NativeScriptBinding_JumpUpdateAnimation,&invalid,1,&out,&diagnostic));
 CHECK(!memcmp(f,&before,sizeof(before))&&out.type==NativeScriptType_U64&&out.as.u64==99);cases+=3;
 // Missing owned table / active normal or blended joint fails before querying.
 for(unsigned bad=0;bad<3;++bad){
  *f=base;f->x8A4_animBlendFrames=bad==2?1:0;parts[43].flags_b1=true;
  parts[43].flags_b0=parts[43].flags_b5=false;
  if(bad==0)tables[f->kind]=NULL;
  if(bad==1)parts[43].joint=NULL;
  if(bad==2)parts[43].x4_jobj2=NULL;
  before=*f;flow_count=0;
  CHECK(!nativeScriptBindingJump_Invoke(&c,NativeScriptBinding_JumpUpdateAnimation,NULL,0,&out,&diagnostic));
  CHECK(!memcmp(f,&before,sizeof(before))&&!flow_count&&out.type==NativeScriptType_U64&&out.as.u64==99);
  tables[f->kind]=&guard_packages[0].assets.parts;parts[43]=saved_part;++cases;
 }
 // Stale scene contexts cannot read/write either motion policy.
 c.scene_epoch++;
 NativeScriptValue valid={.type=NativeScriptType_Bool,.as.u64=1};
 CHECK(!nativeScriptBindingCombat_Read(&c,NativeScriptBinding_DamageReverseKnockdownPose,&out,&diagnostic));
 CHECK(!nativeScriptBindingCombat_Write(&c,NativeScriptBinding_DamageReverseKnockdownPose,&valid,&diagnostic));
 CHECK(!nativeScriptBindingJump_Read(&c,NativeScriptBinding_MotionFallAnimation,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 parts[43]=saved_part;joints[43]=saved_joint;joints[44]=saved_blend;*f=base;
 HSD_GObjFree(ref);flow_testing=false;
}
static void jump_policy_retirement(void)
{
 FighterBone saved_part=parts[43];parts[43].flags_b1=false;
 for(unsigned m=1;m<=8;++m){
  HSD_GObj* o=actor();Fighter* f=o->user_data;
  f->motion_id=ftCo_MS_JumpAerialF;f->mv.co.fall.smid=77;f->mv.co.fallaerial.x4=12.5f;
  observed=o;mutation=m;replacement=NULL;flow_testing=true;flow_count=0;flow_mutation_step=1;
  uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;
  CHECK(!helper(o,"StepJump",NULL,0));
  CHECK(flow_count==1&&!mutation&&f->mv.co.fall.smid==77&&f->mv.co.fallaerial.x4==12.5f&&nativeScriptRuntime_Idle());
  bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;
  if(m==3){CHECK(replacement==o);HSD_GObjFree(replacement);}
  else if(m!=2){if(m==5)o->user_data=f;if(m==6)o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}
  observed=NULL;flow_testing=false;++cases;
 }
 parts[43]=saved_part;
}
