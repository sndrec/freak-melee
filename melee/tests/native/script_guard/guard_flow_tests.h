static bool flow_reference_input(unsigned op,Fighter_GObj* object)
{
 switch(op){
 case 0:return flow_reference_ftCo_80093694(object);
 case 1:return flow_reference_ftCo_8009515C(object);
 case 2:return flow_reference_ftCo_80099794(object);
 case 3:return flow_reference_ftCo_8009980C(object);
 case 4:return flow_reference_ftCo_8009917C(object);
 default:return flow_reference_ftCo_8009A080(object);
 }
}
static void guard_flow_tests(HSD_GObj* o)
{
 Fighter* f=o->user_data;Fighter base=*f;
 *state_address(o,"transition_fault")=0;*state_address(o,"entry_fault")=0;
 flow_testing=true;flow_mutation_step=0;
 common.powershield_input_window=3;common.x314=-0.7f;common.x318=4;
 common.x31C=0.7f;common.x320=4;common.x324=true;common.x464=0.7f;common.x468=4;
 for(unsigned op=0;op<6;++op)for(unsigned variant=0;variant<128;++variant){
  *f=base;f->cur_pos.x=0;f->mv.co.guard.x0=(variant&1)?3.0f:2.99f;
  f->mv.co.itemthrow.x20=(variant&2)?2:0;
  f->input.pressed_buttons=(variant&4)?(HSD_PAD_A|HSD_PAD_L):0;
  f->input.held_buttons[0]=(variant&8)?HSD_PAD_LR:0;
  f->trigger_analog_timer=(variant&16)?3:2;
  f->input.lstick[0].x=(variant&32)?-0.7f:0.69f;
  f->input.lstick[0].y=(variant&32)?-0.7f:-0.69f;
  f->input.cstick[0].x=(variant&64)?0.7f:0;
  f->input.cstick[0].y=(variant&64)?-0.7f:0;
  f->x670_timer_lstick_tilt_x=f->x671_timer_lstick_tilt_y=(variant&16)?4:3;
  f->facing_dir=(variant&1)?-1:1;f->item_gobj=(variant&2)?o:NULL;
  flow_platform=(variant&64)!=0;
  Fighter before=*f;flow_count=0;
  NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=op};
  CHECK(helper(o,"GuardFlow",&argument,1));
  bool accepted=state(o,"input_accepted").as.u64!=0;
  unsigned events=flow_count;char trace[64];memcpy(trace,flow_trace,events);
  Fighter after=*f;*f=before;flow_count=0;
  // Stock predicate/entry recipes execute on an unowned physical stack object.
  // This keeps its native-kind path independent of the attached actor's hook.
  Fighter expected=before;HSD_GObj ref={.user_data=&expected};expected.gobj=&ref;
  CHECK(flow_reference_input(op,&ref)==accepted);
  expected.gobj=o;
  if(memcmp(&after,&expected,sizeof(after))){
   const unsigned char* a=(const unsigned char*)&after;const unsigned char* b=(const unsigned char*)&expected;
   for(unsigned i=0;i<sizeof(after);++i)if(a[i]!=b[i]){fprintf(stderr,"flow input op=%u variant=%u offset=%u actual=%u reference=%u\n",op,variant,i,a[i],b[i]);break;}
  }
  CHECK(!memcmp(&after,&expected,sizeof(after)));
  CHECK(events==flow_count&&!memcmp(trace,flow_trace,events));
  *f=after;CHECK(nativeScriptRuntime_Idle());++pairs;++cases;
 }
 for(unsigned op=7;op<=10;++op)for(unsigned variant=0;variant<32;++variant){
  *f=base;f->allow_sdi=(variant&1)!=0;
  f->gr_vel=(variant&2)?-4.0f:0.1f;
  f->co_attrs.walk_max_vel=1;f->co_attrs.ground_friction=0.4f;
  common.friction_when_above_walk_speed=1.5f;flow_friction=(variant&4)?0.5f:1.0f;
  f->coll_data.floor.normal=(Vec3){(variant&8)?0.6f:0,(variant&8)?0.8f:1,0};
  f->reflect_hit.skip_update_pos=(variant&16)!=0;
  Fighter before=*f;flow_count=0;NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=op};
  CHECK(helper(o,"GuardFlow",&arg,1));Fighter after=*f;unsigned events=flow_count;
  char trace[64];memcpy(trace,flow_trace,events);flow_count=0;
  Fighter expected=before;HSD_GObj ref={.user_data=&expected};expected.gobj=&ref;
  if(op==7)flow_reference_ftCo_Guard_Phys(&ref);
  else if(op==8)flow_reference_ftCo_GuardSetOff_Phys(&ref);
  else if(op==9)flow_reference_ftCo_Guard_Coll(&ref);
  else flow_reference_ftCo_GuardSetOff_Coll(&ref);
  expected.gobj=o;CHECK(!memcmp(&after,&expected,sizeof(after)));
  CHECK(events==flow_count&&!memcmp(trace,flow_trace,events));++pairs;++cases;
 }
 // Verify ordered powershield interrupt traversal and stop-on-first-success.
 for(unsigned branch=0;branch<5;++branch){
  *f=base;f->cur_pos.x=0;f->x221B_b0=true;f->reflecting=true;
  f->mv.co.guard.xC=false;f->mv.co.guard.x10=1;f->mv.co.guard.x1C=4;
  f->mv.co.itemthrow.x20=2;f->input.held_buttons[0]=HSD_PAD_LR;
  f->input.pressed_buttons=0;f->input.lstick[0]=(Vec2){0,0};f->input.cstick[0]=(Vec2){0,0};
  flow_platform=false;flow_choices[0]=branch==0;flow_choices[1]=branch==1;flow_choices[2]=branch==2;
  if(branch==3){f->input.lstick[0].y=-0.8f;f->x671_timer_lstick_tilt_y=0;}
  Fighter before=*f;flow_count=0;NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=6};
  CHECK(helper(o,"GuardFlow",&arg,1));Fighter after=*f;unsigned events=flow_count;
  char trace[64];memcpy(trace,flow_trace,events);flow_count=0;
  Fighter expected=before;HSD_GObj ref={.user_data=&expected};expected.gobj=&ref;
  flow_reference_ftCo_GuardReflect_IASA(&ref);expected.gobj=o;
  CHECK(!memcmp(&after,&expected,sizeof(after)));
  CHECK(events==flow_count&&!memcmp(trace,flow_trace,events));++pairs;++cases;
 }
 for(unsigned semantic=NativeScriptBinding_ControlsTryPowershield;
     semantic<=NativeScriptBinding_CollisionGuardHit;++semantic){
  NativeScriptRuntimeContext c=context(o);NativeScriptValue arg={.type=NativeScriptType_I32},out={.type=NativeScriptType_U64,.as.u64=99};
  Fighter before=*f;CHECK(!nativeScriptBindingGuard_Invoke(&c,semantic,&arg,1,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(f,&before,sizeof(before)));++cases;
 }
 *f=base;flow_testing=false;
}
static void guard_flow_retirement(void)
{
 // Motion initialization -> animation -> authored entry -> statistics must
 // stop at the original boundary for every source retirement/substitution.
 for(unsigned op=2;op<=4;++op)for(unsigned m=0;m<=8;++m){
  HSD_GObj* o=actor();Fighter* f=o->user_data;flow_testing=true;
  f->input.held_buttons[0]=HSD_PAD_LR;f->input.lstick[0]=(Vec2){0.8f,-0.8f};
  f->facing_dir=1;f->x670_timer_lstick_tilt_x=f->x671_timer_lstick_tilt_y=0;
  observed=o;replacement=NULL;mutation=m;flow_count=0;flow_mutation_step=0;
  uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;
  if(!m)*state_address(o,"entry_fault")=1;
  NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=op};
  CHECK(!helper(o,"GuardFlow",&arg,1));CHECK(!mutation&&nativeScriptRuntime_Idle());
  // Spotdodge hooks precede initialization; roll hooks follow it.
  CHECK(flow_count==(op==4?2u:0u));
  bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;
  if(m==3){CHECK(replacement==o);HSD_GObjFree(replacement);}
  else if(m!=2){if(m==5)o->user_data=f;if(m==6)o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}
  observed=NULL;flow_testing=false;++cases;
 }
 // A false decision leaf can retire an actor: no later input test may run.
 for(unsigned m=1;m<=8;++m){
  HSD_GObj* o=actor();Fighter* f=o->user_data;flow_testing=true;
  f->x221B_b0=true;f->reflecting=true;f->mv.co.guard.x10=1;
  f->input.held_buttons[0]=HSD_PAD_LR;observed=o;mutation=m;replacement=NULL;
  flow_choices[0]=flow_choices[1]=flow_choices[2]=false;flow_count=0;flow_mutation_step=1;
  uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;
  NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=6};
  CHECK(!helper(o,"GuardFlow",&arg,1));CHECK(flow_count==1&&flow_trace[0]=='s'&&!mutation&&nativeScriptRuntime_Idle());
  bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;
  if(m==3){CHECK(replacement==o);HSD_GObjFree(replacement);}
  else if(m!=2){if(m==5)o->user_data=f;if(m==6)o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}
  observed=NULL;flow_testing=false;++cases;
 }
}
