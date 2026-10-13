// Native common flags govern capture eligibility, hip-root pose compensation
// and the top-blast decision. Actor/storage admission and VM delivery are real;
// these cases compare exact native assignments, not full pose/KO algorithms.
static void engine_policy_cases(HSD_GObj* owned,HSD_GObj* raw)
{
 const uint32_t ids[]={NativeScriptBinding_CaptureArmed,
  NativeScriptBinding_AnimationHipRootCompensation,NativeScriptBinding_DamageForceTopBlastKO};
 Fighter* f=owned->user_data;NativeScriptRuntimeContext c=context(owned);
 for(unsigned initial=0;initial<8;++initial)for(unsigned slot=0;slot<3;++slot)for(unsigned set=0;set<2;++set){
  f->x221E_b6=(initial&1)!=0;f->x2221_b2=(initial&2)!=0;f->x2222_b3=(initial&4)!=0;
  Fighter expected=*f;
  if(slot==0)expected.x221E_b6=set!=0;
  else if(slot==1)expected.x2221_b2=set!=0;
  else expected.x2222_b3=set!=0;
  NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=set},out;
  CHECK(nativeScriptBindings_Write(&c,ids[slot],NULL,0,&value,&diagnostic));
  CHECK(!memcmp(f,&expected,sizeof(expected)));
  CHECK(nativeScriptBindings_Read(&c,ids[slot],NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Bool&&out.as.u64==set);++pairs;++cases;
 }
 for(unsigned set=0;set<2;++set){
  CHECK(helper(owned,set?"PolicyEnable":"PolicyDisable"));
  CHECK(f->x221E_b6==set&&f->x2221_b2==set&&f->x2222_b3==set);
  CHECK(field(owned,"policy_arm").as.u64==set&&field(owned,"policy_hip").as.u64==set&&field(owned,"policy_ko").as.u64==set);++cases;
 }
 for(unsigned invalid=0;invalid<6;++invalid)for(unsigned slot=0;slot<3;++slot){
  NativeScriptRuntimeContext bad=invalid==0?context(raw):c;
  NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owned,c.generation,&view));
  bool live=view.storage->live;uint64_t identity=observation_packages[0].program.identity;
  if(invalid==1)++bad.generation;
  if(invalid==2)++bad.scene_epoch;
  if(invalid==3)bad.source_class=NativeCallbackSource_Item;
  if(invalid==4)view.storage->live=false;
  if(invalid==5)observation_packages[0].program.identity=0;
  Fighter before=*f;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123};
  NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=1};
  CHECK(!nativeScriptBindingCommonProperties_Read(&bad,ids[slot],&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==123);
  CHECK(!nativeScriptBindingCommonProperties_Write(&bad,ids[slot],&value,&diagnostic));
  CHECK(!memcmp(f,&before,sizeof(before)));
  view.storage->live=live;observation_packages[0].program.identity=identity;++cases;
 }
 for(unsigned invalid=0;invalid<2;++invalid)for(unsigned slot=0;slot<3;++slot){
  NativeScriptValue value={.type=invalid?NativeScriptType_I32:NativeScriptType_Bool,.as.u64=2};Fighter before=*f;
  CHECK(!nativeScriptBindingCommonProperties_Write(&c,ids[slot],&value,&diagnostic));
  CHECK(!memcmp(f,&before,sizeof(before)));++cases;
 }
}
